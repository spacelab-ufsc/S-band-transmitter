/*
 * main.c
 *
 * Copyright (C) 2026 Hardik Singhal.
 *
 * This file is part of the CubeSat Packet Pipeline project, developed
 * during a Summer Internship at SpaceLab (UFSC).
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * \brief CubeSat packet pipeline entry point — fragmentation, queuing,
 *        NGHam encoding, and telemetry monitoring, implemented as four
 *        FreeRTOS tasks (POSIX simulation).
 *
* \author Hardik Singhal <24uec235@lnmiit.ac.in>
* \author Shivansh Gupta <24uec228@lnmiit.ac.in>
* \author Amrit Mishra <https://github.com/Amrit14feb/>
 * \version 1.0.0
 * \mentor Lucas Ryan
 * \date 2026/07/29
 *
 * \defgroup pipeline Packet Pipeline
 * \{
 */

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "app/structs/payload_chunk.h"
#include "drivers/ngham/ngham.h"
#include "app/structs/telemetry_data.h"

QueueHandle_t xQueue1_RawChunks;
QueueHandle_t xQueue2_OrderedChunks;

/* Shared telemetry struct, protected by mutex since multiple tasks write to it */
telemetry_data_t g_telemetry;
SemaphoreHandle_t xTelemetryMutex;

static void telemetry_lock(void)   { xSemaphoreTake(xTelemetryMutex, portMAX_DELAY); }
static void telemetry_unlock(void) { xSemaphoreGive(xTelemetryMutex); }

/**
 * \brief Simulated occultation event used to fake payload memory reads.
 */
typedef struct {
    uint16_t occultation_id;
    uint16_t data_size;
} FakeEvent_t;

static const FakeEvent_t fake_events[] = {
    { .occultation_id = 1000, .data_size = 100 },
    { .occultation_id = 1001, .data_size = 45  },
    { .occultation_id = 1002, .data_size = 70  },
};

#define NUM_FAKE_EVENTS (sizeof(fake_events) / sizeof(fake_events[0]))

/**
 * \brief Task 1 — Payload Reader / Fragmenter.
 *
 * Simulates reading payload data for each occultation event and splits
 * it into fixed-size chunks (CHUNK_DATA_SIZE), pushing each chunk into
 * Queue 1 for downstream ordering.
 *
 * \param pvParameters Unused.
 */
void vTask1_PayloadReader(void *pvParameters)
{
    (void) pvParameters;

    for (int e = 0; e < (int)NUM_FAKE_EVENTS; e++)
    {
        uint16_t occ_id   = fake_events[e].occultation_id;
        uint16_t data_len = fake_events[e].data_size;

        uint8_t fake_payload[256];
        for (int i = 0; i < data_len; i++)
        {
            fake_payload[i] = (uint8_t)(0x10 + (e * 0x30) + i);
        }

        /* Round up: number of chunks needed to cover data_len bytes */
        uint8_t total_chunks = (data_len + CHUNK_DATA_SIZE - 1) / CHUNK_DATA_SIZE;

        printf("[Task1-PayloadReader] occultation %d: %d bytes -> %d chunk(s)\n",
               occ_id, data_len, total_chunks);
        fflush(stdout);

        int bytes_remaining = data_len;
        int offset = 0;

        for (uint8_t seq = 1; seq <= total_chunks; seq++)
        {
            PayloadChunk_t chunk;
            memset(&chunk, 0, sizeof(chunk));

            uint8_t this_chunk_size = (bytes_remaining < CHUNK_DATA_SIZE)
                                        ? (uint8_t)bytes_remaining
                                        : CHUNK_DATA_SIZE;

            chunk.occultation_id  = occ_id;
            chunk.sequence_number = seq;
            chunk.total_chunks    = total_chunks;
            chunk.data_size       = this_chunk_size;
            memcpy(chunk.data, &fake_payload[offset], this_chunk_size);

            /* Timed send (not portMAX_DELAY) so overflow is measurable */
            if (xQueueSend(xQueue1_RawChunks, &chunk, pdMS_TO_TICKS(500)) == pdTRUE)
            {
                telemetry_lock();
                g_telemetry.chunks_generated++;
                telemetry_unlock();
            }
            else
            {
                telemetry_lock();
                g_telemetry.queue1_overflow_count++;
                telemetry_unlock();
                printf("[Task1-PayloadReader] WARNING: Queue1 full, chunk dropped!\n");
                fflush(stdout);
            }

            offset          += this_chunk_size;
            bytes_remaining -= this_chunk_size;

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    printf("[Task1-PayloadReader] all occultations fragmented. Task finished.\n");
    fflush(stdout);

    for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}

/**
 * \brief Task 2 — FIFO Queue Manager.
 *
 * Drains Queue 1 and forwards each chunk into Queue 2, preserving
 * transmission order for the NGHam encoder.
 *
 * \param pvParameters Unused.
 */
void vTask2_FifoManager(void *pvParameters)
{
    (void) pvParameters;
    PayloadChunk_t chunk;
    int total_forwarded = 0;

    printf("[Task2-FifoManager] started.\n");
    fflush(stdout);

    for (;;)
    {
        if (xQueueReceive(xQueue1_RawChunks, &chunk, pdMS_TO_TICKS(3000)) == pdTRUE)
        {
            total_forwarded++;
            printf("[Task2-FifoManager] forwarding  occ=%d  seq=%d/%d  bytes=%d\n",
                   chunk.occultation_id, chunk.sequence_number,
                   chunk.total_chunks, chunk.data_size);
            fflush(stdout);

            if (xQueueSend(xQueue2_OrderedChunks, &chunk, pdMS_TO_TICKS(500)) != pdTRUE)
            {
                telemetry_lock();
                g_telemetry.queue2_overflow_count++;
                telemetry_unlock();
                printf("[Task2-FifoManager] WARNING: Queue2 full, chunk dropped!\n");
                fflush(stdout);
            }
        }
        else
        {
            printf("[Task2-FifoManager] no more chunks arriving. Total forwarded: %d\n",
                   total_forwarded);
            fflush(stdout);
            for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

/**
 * \brief Task 3 — NGHam Encoder.
 *
 * Drains Queue 2, serializes each chunk's metadata and payload, and
 * encodes it into an NGHam frame ready for transmission.
 *
 * \param pvParameters Unused.
 */
void vTask3_NGHamEncoder(void *pvParameters)
{
    (void) pvParameters;
    PayloadChunk_t chunk;
    int total_encoded = 0;

    printf("[Task3-NGHamEncoder] started.\n");
    fflush(stdout);

    for (;;)
    {
        if (xQueueReceive(xQueue2_OrderedChunks, &chunk, pdMS_TO_TICKS(3000)) == pdTRUE)
        {
            total_encoded++;

            /* Serialize chunk metadata + data into the NGHam payload buffer */
            uint8_t ngham_payload[64];
            int p = 0;

            /* occultation_id is 16-bit; split into big-endian byte pair */
            ngham_payload[p++] = (uint8_t)(chunk.occultation_id >> 8);
            ngham_payload[p++] = (uint8_t)(chunk.occultation_id & 0xFF);

            /* Remaining metadata fields are already single bytes */
            ngham_payload[p++] = chunk.sequence_number;
            ngham_payload[p++] = chunk.total_chunks;
            ngham_payload[p++] = chunk.data_size;

            memcpy(&ngham_payload[p], chunk.data, chunk.data_size);
            p += chunk.data_size;

            uint8_t ngham_packet[300];
            int packet_len = ngham_build_packet(ngham_payload, (uint8_t)p, ngham_packet);

            if (packet_len < 0)
            {
                printf("[Task3-NGHamEncoder] ERROR: payload too large to encode!\n");
                fflush(stdout);
                continue;
            }

            printf("[Task3-NGHamEncoder] occ=%d seq=%d/%d -> NGHam packet (%d bytes): ",
                   chunk.occultation_id, chunk.sequence_number, chunk.total_chunks, packet_len);
            for (int i = 0; i < packet_len; i++) printf("%02X ", ngham_packet[i]);
            printf("\n");
            fflush(stdout);

            telemetry_lock();
            g_telemetry.packets_transmitted++;
            telemetry_unlock();
        }
        else
        {
            printf("[Task3-NGHamEncoder] no more chunks arriving. Total encoded: %d\n",
                   total_encoded);
            fflush(stdout);
            for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

/**
 * \brief Task 4 — Telemetry Monitor.
 *
 * Samples queue occupancy once per second and updates generation/transmission
 * rates and overflow counters in the shared telemetry struct.
 *
 * \param pvParameters Unused.
 */
void vTask4_TelemetryMonitor(void *pvParameters)
{
    (void) pvParameters;
    uint32_t last_chunks_generated = 0;
    uint32_t last_packets_transmitted = 0;

    printf("[Task4-TelemetryMonitor] started.\n");
    fflush(stdout);

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); /* sample once per second */

        UBaseType_t q1_count = uxQueueMessagesWaiting(xQueue1_RawChunks);
        UBaseType_t q2_count = uxQueueMessagesWaiting(xQueue2_OrderedChunks);

        telemetry_lock();
        g_telemetry.queue1_occupancy = (uint8_t) q1_count;
        g_telemetry.queue2_occupancy = (uint8_t) q2_count;

        uint32_t current_chunks  = g_telemetry.chunks_generated;
        uint32_t current_packets = g_telemetry.packets_transmitted;

        g_telemetry.generation_rate   = (uint16_t)(current_chunks  - last_chunks_generated);
        g_telemetry.transmission_rate = (uint16_t)(current_packets - last_packets_transmitted);

        telemetry_data_t snapshot = g_telemetry; /* copy out for safe printing below */
        telemetry_unlock();

        last_chunks_generated    = current_chunks;
        last_packets_transmitted = current_packets;

        printf("[Task4-TelemetryMonitor] Q1=%u  Q2=%u  gen_rate=%u/s  tx_rate=%u/s  "
               "total_chunks=%u  total_packets=%u  overflow(Q1=%u, Q2=%u)\n",
               (unsigned)q1_count, (unsigned)q2_count,
               snapshot.generation_rate, snapshot.transmission_rate,
               snapshot.chunks_generated, snapshot.packets_transmitted,
               snapshot.queue1_overflow_count, snapshot.queue2_overflow_count);
        fflush(stdout);
    }
}

int main(void)
{
    printf("=== CubeSat Packet Pipeline - Task 1+2+3+4 (Telemetry) Test ===\n\n");
    fflush(stdout);

    memset(&g_telemetry, 0, sizeof(g_telemetry));
    xTelemetryMutex = xSemaphoreCreateMutex();

    xQueue1_RawChunks     = xQueueCreate(10, sizeof(PayloadChunk_t));
    xQueue2_OrderedChunks = xQueueCreate(10, sizeof(PayloadChunk_t));

    xTaskCreate(vTask1_PayloadReader,    "Task1", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    xTaskCreate(vTask2_FifoManager,      "Task2", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    xTaskCreate(vTask3_NGHamEncoder,     "Task3", configMINIMAL_STACK_SIZE * 4, NULL, 1, NULL);
    xTaskCreate(vTask4_TelemetryMonitor, "Task4", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);

    vTaskStartScheduler();

    for (;;);
    return 0;
}

void vAssertCalled(const char *pcFile, unsigned long ulLine)
{
    printf("ASSERT FAILED: %s, line %lu\n", pcFile, ulLine);
    fflush(stdout);
    for (;;);
}

void vApplicationMallocFailedHook(void)
{
    printf("MALLOC FAILED! Out of heap memory.\n");
    fflush(stdout);
    for (;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void) xTask;
    printf("STACK OVERFLOW in task: %s\n", pcTaskName);
    fflush(stdout);
    for (;;);
}

/** \} End of pipeline group */
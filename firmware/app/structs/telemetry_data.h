/*
 * telemetry_data.h
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
 * \brief Telemetry data structure for the payload transmission pipeline.
 *        Modeled after the style used in radio_data.h (spacelab-ufsc/ttc2).
 *
* \author Hardik Singhal <24uec235@lnmiit.ac.in>
* \author Shivansh Gupta 
* \author Amrit Mishra 
 * \version 1.0.0
 * \mentor Lucas Ryan
 * \version 1.0.0
 *
 * \date 2026/07/29
 *
 * \defgroup telemetry_data Telemetry Data
 * \{
 */

#ifndef TELEMETRY_DATA_H_
#define TELEMETRY_DATA_H_

#include <stdint.h>

/**
 * \brief Payload transmission pipeline telemetry data type.
 */
typedef struct
{
    uint8_t  queue1_occupancy;      /**< Current items waiting in Queue 1 (raw chunks). */
    uint8_t  queue2_occupancy;      /**< Current items waiting in Queue 2 (ordered chunks). */
    uint32_t chunks_generated;      /**< Total chunks produced by Task 1 since startup. */
    uint32_t packets_transmitted;   /**< Total NGHam packets transmitted by Task 3 since startup. */
    uint16_t generation_rate;       /**< Chunks generated per second (updated periodically). */
    uint16_t transmission_rate;     /**< Packets transmitted per second (updated periodically). */
    uint32_t queue1_overflow_count; /**< Number of xQueueSend failures on Queue 1 (queue full). */
    uint32_t queue2_overflow_count; /**< Number of xQueueSend failures on Queue 2 (queue full). */
} telemetry_data_t;

#endif /* TELEMETRY_DATA_H_ */

/** \} End of telemetry_data group */
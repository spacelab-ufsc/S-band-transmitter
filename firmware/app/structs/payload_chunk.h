/*
 * payload_chunk.h
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
 * \brief Payload chunk structure definition, used to fragment occultation
 *        event data before NGHam encoding.
 *
 * \author Hardik Singhal <24uec235@lnmiit.ac.in>
 *
 * \mentor Lucas Ryan Carneiro
 *
 * \version 1.0.0
 *
 * \date 2026/07/29
 *
 * \defgroup payload_chunk Payload Chunk
 * \{
 */

#ifndef PAYLOAD_CHUNK_H
#define PAYLOAD_CHUNK_H

#include <stdint.h>

/* Fixed chunk size; matches NGHam's small-frame payload class */
#define CHUNK_DATA_SIZE 32

/**
 * \brief Payload chunk type.
 */
typedef struct {
    uint16_t occultation_id;        /**< Event that generated this data. */
    uint8_t  sequence_number;       /**< Chunk index within the event (1-indexed). */
    uint8_t  total_chunks;          /**< Total number of chunks for this event. */
    uint8_t  data_size;             /**< Valid byte count in 'data'. */
    uint8_t  data[CHUNK_DATA_SIZE]; /**< Payload data. */
} PayloadChunk_t;

#endif /* PAYLOAD_CHUNK_H */

/** \} End of payload_chunk group */
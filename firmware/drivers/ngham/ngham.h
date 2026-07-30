/*
 * ngham.h
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
 * \brief Minimal NGHam packet encoder (CRC16 + Reed-Solomon FEC + framing),
 *        used to convert payload chunks into transmittable NGHam frames.
 *
* \author Hardik Singhal <24uec235@lnmiit.ac.in>
* \author Shivansh Gupta <24uec228@lnmiit.ac.in>

 * \mentor Lucas Ryan
 *
 * \version 1.0.0
 *
 * \date 2026/07/29
 *
 * \defgroup ngham NGHam Encoder
 * \{
 */

#ifndef NGHAM_H
#define NGHAM_H

#include <stdint.h>
#include <string.h>

/**
 * \brief CRC16-CCITT (NGHam spec: poly 0x1021, init 0xFFFF, final XOR 0xFFFF).
 */
static uint16_t ngham_crc16(const uint8_t *data, int len)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < len; i++)
    {
        crc ^= ((uint16_t)data[i]) << 8;
        for (int b = 0; b < 8; b++)
        {
            if (crc & 0x8000) crc = (uint16_t)((crc << 1) ^ 0x1021);
            else               crc = (uint16_t)(crc << 1);
        }
    }
    return (uint16_t)(crc ^ 0xFFFF);
}

/**
 * \brief NGHam size-tag configuration (data/parity split per size class),
 *        as defined by the NGHam protocol spec.
 */
typedef struct {
    uint8_t tag[3];
    uint8_t rs_n;        /**< Total RS block size (data + parity). */
    uint8_t rs_k;        /**< Data bytes (header + payload + CRC + padding). */
    uint8_t max_payload; /**< Max payload bytes for this size class. */
} NGHamSizeConfig_t;

static const NGHamSizeConfig_t ngham_size_table[7] = {
    { {59, 73, 205},   47,  31,  28 },
    { {77, 218, 87},   79,  63,  60 },
    { {118,147,154},  111,  95,  92 },
    { {155,180,174},  159, 127, 124 },
    { {160,253,99},   191, 159, 156 },
    { {214,110,249},  223, 191, 188 },
    { {237,39,52},    255, 223, 220 },
};

/*
 * GF(256) arithmetic for Reed-Solomon encoding.
 *
 * Note: this implementation uses the common primitive polynomial 0x11D
 * for GF(256). The official NGHam spec uses field polynomial 0x187 with
 * a specific generator root; matching that exactly is required for
 * byte-level interoperability with the reference PyNGHam/C implementation.
 * As implemented, this is a structurally correct, working systematic
 * Reed-Solomon encoder, sufficient to validate the pipeline architecture.
 */

static uint8_t gf_exp[512];
static uint8_t gf_log[256];
static int gf_initialized = 0;

static void gf_init(void)
{
    int x = 1;
    for (int i = 0; i < 255; i++)
    {
        gf_exp[i] = (uint8_t)x;
        gf_log[(uint8_t)x] = (uint8_t)i;
        x <<= 1;
        if (x & 0x100) x ^= 0x11D;
    }
    for (int i = 255; i < 512; i++) gf_exp[i] = gf_exp[i - 255];
    gf_log[0] = 0;
    gf_initialized = 1;
}

static uint8_t gf_mul(uint8_t a, uint8_t b)
{
    if (a == 0 || b == 0) return 0;
    return gf_exp[gf_log[a] + gf_log[b]];
}

#define NGHAM_MAX_PARITY 32

/**
 * \brief Build the RS generator polynomial for 'nsym' parity symbols.
 *
 * \param nsym Number of parity symbols.
 * \param gen  Output buffer, ascending order, size (nsym+1); gen[nsym] is
 *             always 1 (monic).
 */
static void rs_generator_poly(int nsym, uint8_t *gen)
{
    memset(gen, 0, (size_t)(nsym + 1));
    gen[0] = 1;
    for (int i = 0; i < nsym; i++)
    {
        gen[i + 1] = 1;
        for (int j = i; j > 0; j--)
        {
            gen[j] = (uint8_t)(gen[j - 1] ^ gf_mul(gen[j], gf_exp[i]));
        }
        gen[0] = gf_mul(gen[0], gf_exp[i]);
    }
}

/**
 * \brief Systematic RS encode: given 'k' data bytes, produce 'nsym' parity bytes.
 */
static void rs_encode(const uint8_t *data, int k, int nsym, uint8_t *parity)
{
    if (!gf_initialized) gf_init();

    uint8_t gen[NGHAM_MAX_PARITY + 1];
    rs_generator_poly(nsym, gen);

    memset(parity, 0, (size_t)nsym);

    for (int i = 0; i < k; i++)
    {
        uint8_t feedback = (uint8_t)(data[i] ^ parity[0]);
        if (feedback != 0)
        {
            for (int j = 1; j < nsym; j++)
            {
                parity[j - 1] = (uint8_t)(parity[j] ^ gf_mul(gen[nsym - j], feedback));
            }
            parity[nsym - 1] = gf_mul(gen[0], feedback);
        }
        else
        {
            for (int j = 1; j < nsym; j++) parity[j - 1] = parity[j];
            parity[nsym - 1] = 0;
        }
    }
}

/**
 * \brief Build a complete NGHam packet from a payload buffer.
 *
 * \param payload     Input payload bytes.
 * \param payload_len Length of payload in bytes.
 * \param out_packet  Output buffer for the encoded packet.
 *
 * \return Total packet length written into out_packet, or -1 on error
 *         (payload too large for any NGHam size class).
 */
static int ngham_build_packet(const uint8_t *payload, uint8_t payload_len, uint8_t *out_packet)
{
    int idx = -1;
    for (int i = 0; i < 7; i++)
    {
        if (payload_len <= ngham_size_table[i].max_payload) { idx = i; break; }
    }
    if (idx < 0) return -1;

    const NGHamSizeConfig_t *cfg = &ngham_size_table[idx];
    uint8_t k     = cfg->rs_k;
    uint8_t n     = cfg->rs_n;
    uint8_t nsym  = (uint8_t)(n - k);

    /* k = header(1) + payload + crc(2) + padding */
    uint8_t padding_size = (uint8_t)(k - 3 - payload_len);
    uint8_t header = (uint8_t)(padding_size & 0x1F);

    uint8_t data_block[256];
    int pos = 0;
    data_block[pos++] = header;
    memcpy(&data_block[pos], payload, payload_len);
    pos += payload_len;

    /* CRC computed over header + payload only */
    uint16_t crc = ngham_crc16(data_block, pos);
    data_block[pos++] = (uint8_t)(crc >> 8);
    data_block[pos++] = (uint8_t)(crc & 0xFF);

    for (int i = 0; i < padding_size; i++) data_block[pos++] = 0x00;
    /* pos should now equal k */

    uint8_t parity[NGHAM_MAX_PARITY];
    rs_encode(data_block, k, nsym, parity);

    int out_pos = 0;
    out_packet[out_pos++] = 0xAA; out_packet[out_pos++] = 0xAA;
    out_packet[out_pos++] = 0xAA; out_packet[out_pos++] = 0xAA;
    out_packet[out_pos++] = 0x5D; out_packet[out_pos++] = 0xE6;
    out_packet[out_pos++] = 0x2A; out_packet[out_pos++] = 0x7E;
    out_packet[out_pos++] = cfg->tag[0];
    out_packet[out_pos++] = cfg->tag[1];
    out_packet[out_pos++] = cfg->tag[2];
    memcpy(&out_packet[out_pos], data_block, k); out_pos += k;
    memcpy(&out_packet[out_pos], parity, nsym);  out_pos += nsym;

    return out_pos;
}

#endif /* NGHAM_H */

/** \} End of ngham group */
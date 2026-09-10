// MIT License
//
// Copyright (c) 2026 Kevin Thomas
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Author:  Kevin Thomas
// Email:   kevin@mytechnotalent.com
// GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
// File:    mesh_transport.c
// Desc:    Implements MeshCore transport-scope packet routing.
// Created: 2026

#include "mesh_transport.h"

#include "mbedtls/md.h"

#include <string.h>

/**
 * @brief Calculate a two-byte transport code for a packet.
 *
 * Computes HMAC-SHA256 over packet type and payload using the 16-byte key,
 * mapping 0x0000 to 0x0001 and 0xFFFF to 0xFFFE.
 *
 * @param packet Pointer to MeshPacket structure.
 * @param key 16-byte scope secret key.
 * @param code Destination pointer for the calculated 16-bit code.
 * @return bool true on success, false on crypto error or NULL pointers.
 */
static bool _mesh_transport_code(const MeshPacket *packet, const uint8_t key[16], uint16_t *code) {
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_context_t context;
    uint8_t digest[32U];
    uint8_t type;
    if (packet == NULL || key == NULL || code == NULL || info == NULL) {
        return false;
    }
    type = (uint8_t)packet->type;
    mbedtls_md_init(&context);
    if (mbedtls_md_setup(&context, info, 1) != 0 ||
        mbedtls_md_hmac_starts(&context, key, 16U) != 0 ||
        mbedtls_md_hmac_update(&context, &type, 1U) != 0 ||
        mbedtls_md_hmac_update(&context, packet->payload, packet->payload_length) != 0 ||
        mbedtls_md_hmac_finish(&context, digest) != 0) {
        mbedtls_md_free(&context);
        return false;
    }
    mbedtls_md_free(&context);
    memcpy(code, digest, sizeof(*code));
    if (*code == 0U) {
        *code = 1U;
    } else if (*code == 0xFFFFU) {
        *code = 0xFFFEU;
    }
    return true;
}

/**
 * @brief Apply a nonzero MeshCore transport scope to a packet.
 *
 * If the key is all zeros, routes the packet as MESH_ROUTE_FLOOD with zero codes;
 * otherwise calculates transport code and sets route to MESH_ROUTE_TRANSPORT_FLOOD.
 *
 * @param packet Pointer to MeshPacket structure.
 * @param key 16-byte scope secret key.
 * @return bool true on success, false on failure or NULL pointers.
 */
bool mesh_transport_apply_scope(MeshPacket *packet, const uint8_t key[16]) {
    uint16_t code;
    bool null_key = true;
    if (packet == NULL || key == NULL) {
        return false;
    }
    for (size_t index = 0U; index < 16U; index++) {
        null_key = null_key && key[index] == 0U;
    }
    if (null_key) {
        packet->route = MESH_ROUTE_FLOOD;
        packet->transport_codes[0] = 0U;
        packet->transport_codes[1] = 0U;
        return true;
    }
    if (!_mesh_transport_code(packet, key, &code)) {
        return false;
    }
    packet->route = MESH_ROUTE_TRANSPORT_FLOOD;
    packet->transport_codes[0] = code;
    packet->transport_codes[1] = 0U;
    return true;
}

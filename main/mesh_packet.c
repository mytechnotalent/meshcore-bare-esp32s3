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
// File:    mesh_packet.c
// Desc:    Implements the MeshCore line packet envelope without dynamic allocation.
// Created: 2026

#include "mesh_packet.h"

#include <string.h>

/**
 * @brief Return the encoded path byte length.
 *
 * Computes number of hops times bytes per hop from the path_length field.
 *
 * @param path_length Encoded MeshCore path metadata byte.
 * @return size_t Path byte length in bytes.
 */
static size_t _mesh_packet_path_bytes(uint8_t path_length) {
    return (size_t)(path_length & 63U) * (size_t)((path_length >> 6U) + 1U);
}

/**
 * @brief Validate MeshCore path metadata and its byte length.
 *
 * Checks that the hash size does not exceed bounds and total path size
 * fits within MESH_PACKET_MAX_PATH.
 *
 * @param path_length Encoded MeshCore path metadata byte.
 * @return bool true if path length descriptor is valid, false otherwise.
 */
static bool _mesh_packet_valid_path(uint8_t path_length) {
    uint8_t hash_size = (uint8_t)((path_length >> 6U) + 1U);
    return hash_size < 4U && _mesh_packet_path_bytes(path_length) <= MESH_PACKET_MAX_PATH;
}

/**
 * @brief Encode a packet into the MeshCore header, path, and payload layout.
 *
 * Packs routing flags, optional transport codes, path bytes, and payload bytes
 * into the destination buffer.
 *
 * @param packet Pointer to MeshPacket structure to encode.
 * @param out Destination output buffer.
 * @param capacity Output buffer capacity in bytes.
 * @param out_length Pointer to store total encoded byte count.
 * @return bool true on success, false if invalid or buffer too small.
 */
bool mesh_packet_encode(const MeshPacket *packet,
                        uint8_t *out,
                        size_t capacity,
                        size_t *out_length) {
    size_t position = 0U;
    bool transports = false;
    size_t path_bytes;
    if (packet == NULL || out == NULL || out_length == NULL ||
        !_mesh_packet_valid_path(packet->path_length) ||
        packet->payload_length > MESH_PACKET_MAX_PAYLOAD) {
        return false;
    }
    path_bytes = _mesh_packet_path_bytes(packet->path_length);
    transports =
        packet->route == MESH_ROUTE_TRANSPORT_FLOOD || packet->route == MESH_ROUTE_TRANSPORT_DIRECT;
    if (capacity < 2U + (transports ? 4U : 0U) + path_bytes + packet->payload_length) {
        return false;
    }
    out[position++] =
        (uint8_t)(((uint8_t)packet->route & 3U) | (((uint8_t)packet->type & 15U) << 2U) |
                  ((packet->version & 3U) << 6U));
    if (transports) {
        out[position++] = (uint8_t)(packet->transport_codes[0] & 255U);
        out[position++] = (uint8_t)(packet->transport_codes[0] >> 8U);
        out[position++] = (uint8_t)(packet->transport_codes[1] & 255U);
        out[position++] = (uint8_t)(packet->transport_codes[1] >> 8U);
    }
    out[position++] = packet->path_length;
    memcpy(&out[position], packet->path, path_bytes);
    position += path_bytes;
    memcpy(&out[position], packet->payload, packet->payload_length);
    position += packet->payload_length;
    *out_length = position;
    return true;
}

/**
 * @brief Decode a MeshCore header, path, and payload with bounds checks.
 *
 * Unpacks the raw wire frame into the target MeshPacket structure,
 * validating route, type, version, path bytes, and payload bounds.
 *
 * @param data Pointer to raw received wire frame bytes.
 * @param length Total length of received frame in bytes.
 * @param packet Destination MeshPacket pointer.
 * @return bool true on success, false if malformed or truncated.
 */
bool mesh_packet_decode(const uint8_t *data, size_t length, MeshPacket *packet) {
    size_t position = 0U;
    bool transports;
    if (data == NULL || packet == NULL || length < 2U || length > MESH_PACKET_MAX_SIZE) {
        return false;
    }
    memset(packet, 0, sizeof(*packet));
    packet->route = (MeshRouteType)(data[0] & 3U);
    packet->type = (MeshPayloadType)((data[0] >> 2U) & 15U);
    packet->version = (uint8_t)(data[0] >> 6U);
    position = 1U;
    transports =
        packet->route == MESH_ROUTE_TRANSPORT_FLOOD || packet->route == MESH_ROUTE_TRANSPORT_DIRECT;
    if (transports) {
        if (length < 6U) {
            return false;
        }
        packet->transport_codes[0] =
            (uint16_t)data[position] | ((uint16_t)data[position + 1U] << 8U);
        packet->transport_codes[1] =
            (uint16_t)data[position + 2U] | ((uint16_t)data[position + 3U] << 8U);
        position += 4U;
    }
    packet->path_length = data[position++];
    if (!_mesh_packet_valid_path(packet->path_length) ||
        position + _mesh_packet_path_bytes(packet->path_length) > length) {
        return false;
    }
    memcpy(packet->path, &data[position], _mesh_packet_path_bytes(packet->path_length));
    position += _mesh_packet_path_bytes(packet->path_length);
    if (length - position > MESH_PACKET_MAX_PAYLOAD) {
        return false;
    }
    packet->payload_length = (uint8_t)(length - position);
    memcpy(packet->payload, &data[position], packet->payload_length);
    return true;
}
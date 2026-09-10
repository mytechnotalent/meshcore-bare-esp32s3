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
// File:    mesh_packet.h
// Desc:    Defines the compact MeshCore over-the-air packet envelope.
// Created: 2026

#ifndef MESH_PACKET_H
#define MESH_PACKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Maximum MeshCore packet size on the LoRa line in bytes.
 */
#define MESH_PACKET_MAX_SIZE 255U

/**
 * @brief Maximum MeshCore path size in bytes.
 */
#define MESH_PACKET_MAX_PATH 64U

/**
 * @brief Maximum MeshCore payload size in bytes.
 */
#define MESH_PACKET_MAX_PAYLOAD 184U

/**
 * @brief MeshCore payload type enumeration.
 */
typedef enum {
    /**
     * @brief Request payload.
     */
    MESH_PAYLOAD_REQUEST = 0,
    /**
     * @brief Response payload.
     */
    MESH_PAYLOAD_RESPONSE = 1,
    /**
     * @brief Direct text payload.
     */
    MESH_PAYLOAD_TEXT = 2,
    /**
     * @brief Acknowledgement payload.
     */
    MESH_PAYLOAD_ACK = 3,
    /**
     * @brief Signed advertisement payload.
     */
    MESH_PAYLOAD_ADVERT = 4,
    /**
     * @brief Group text payload.
     */
    MESH_PAYLOAD_GROUP_TEXT = 5,
    /**
     * @brief Group data payload.
     */
    MESH_PAYLOAD_GROUP_DATA = 6,
    /**
     * @brief Anonymous request payload.
     */
    MESH_PAYLOAD_ANON_REQ = 7,
    /**
     * @brief Stream payload.
     */
    MESH_PAYLOAD_STREAM = 8,
    /**
     * @brief Trace and ping diagnostic payload.
     */
    MESH_PAYLOAD_TRACE = 9,
    /**
     * @brief Raw custom payload.
     */
    MESH_PAYLOAD_RAW = 15
} MeshPayloadType;

/**
 * @brief MeshCore route type enumeration.
 */
typedef enum {
    /**
     * @brief Flood route with transport codes.
     */
    MESH_ROUTE_TRANSPORT_FLOOD = 0,
    /**
     * @brief Flood route.
     */
    MESH_ROUTE_FLOOD = 1,
    /**
     * @brief Direct route.
     */
    MESH_ROUTE_DIRECT = 2,
    /**
     * @brief Direct route with transport codes.
     */
    MESH_ROUTE_TRANSPORT_DIRECT = 3
} MeshRouteType;

/**
 * @brief MeshCore packet envelope structure.
 */
typedef struct {
    /**
     * @brief Payload type.
     */
    MeshPayloadType type;
    /**
     * @brief Route type.
     */
    MeshRouteType route;
    /**
     * @brief Protocol version.
     */
    uint8_t version;
    /**
     * @brief Optional transport codes.
     */
    uint16_t transport_codes[2];
    /**
     * @brief Path byte count.
     */
    uint8_t path_length;
    /**
     * @brief Route path bytes.
     */
    uint8_t path[MESH_PACKET_MAX_PATH];
    /**
     * @brief Payload byte count.
     */
    uint8_t payload_length;
    /**
     * @brief Payload bytes.
     */
    uint8_t payload[MESH_PACKET_MAX_PAYLOAD];
} MeshPacket;

/**
 * @brief Serialize a packet into a radio frame.
 *
 * Encodes packet header, path, optional transport codes, and payload bytes
 * into caller-supplied buffer.
 *
 * @param packet Pointer to MeshPacket structure to encode.
 * @param out Destination frame buffer.
 * @param capacity Capacity of destination buffer in bytes.
 * @param out_length Pointer to store encoded frame length in bytes.
 * @return bool true on success, false when bounds are invalid or capacity exceeded.
 */
bool mesh_packet_encode(const MeshPacket *packet,
                        uint8_t *out,
                        size_t capacity,
                        size_t *out_length);

/**
 * @brief Decode a radio frame into a packet.
 *
 * Validates header format, path length, transport codes, and payload bounds,
 * unpacking bytes into target MeshPacket structure.
 *
 * @param data Source frame bytes.
 * @param length Source frame length in bytes.
 * @param packet Destination MeshPacket pointer.
 * @return bool true on success, false when the frame is malformed.
 */
bool mesh_packet_decode(const uint8_t *data, size_t length, MeshPacket *packet);

#endif // MESH_PACKET_H
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
// File:    mesh_transport.h
// Desc:    Defines MeshCore transport-scope packet routing helpers.
// Created: 2026

#ifndef MESH_TRANSPORT_H
#define MESH_TRANSPORT_H

#include "mesh_packet.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Apply a MeshCore transport scope to a packet.
 *
 * Derives transport codes from the 16-byte scope key and packet payload,
 * setting the packet's route type to MESH_ROUTE_TRANSPORT_FLOOD.
 *
 * @param packet Pointer to MeshPacket structure to update.
 * @param key 16-byte scope secret key.
 * @return bool true on success, false if parameters are invalid.
 */
bool mesh_transport_apply_scope(MeshPacket *packet, const uint8_t key[16]);

#endif // MESH_TRANSPORT_H

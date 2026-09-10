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
// File:    ble_companion.h
// Desc:    Declares the MeshCore BLE companion transport.
// Created: 2026

#ifndef BLE_COMPANION_H
#define BLE_COMPANION_H

/**
 * @brief Initialize MeshCore BLE services and security configuration.
 *
 * Registers custom GATT service, RX/TX characteristics, GAP appearance,
 * and security parameters (fixed PIN pairing).
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_init(void);

/**
 * @brief Start MeshCore BLE advertising and host processing.
 *
 * Spawns NimBLE host task and command dispatch task, and begins BLE advertising.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_start(void);

/**
 * @brief Flush one queued companion response to the connected BLE client.
 *
 * Sends a GATT notification if connected, subscribers exist, and congestion permits.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_flush_pending(void);

/**
 * @brief Update the advertised and GAP BLE device name.
 *
 * Updates GAP device name and active scan response payload with current name.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_update_device_name(void);

#endif // BLE_COMPANION_H

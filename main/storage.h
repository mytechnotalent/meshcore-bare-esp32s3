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
// File:    storage.h
// Desc:    Declares the ESP32 persistence boundary for portable node state.
// Created: 2026

#ifndef STORAGE_H
#define STORAGE_H

#include "identity.h"

#include <stdbool.h>

/**
 * @brief Initialize storage and load saved node state.
 *
 * Mounts NVS flash, initializes persistent identity, imports node state blob,
 * and restores dedicated device name and default channel scope.
 *
 * @param void No parameters.
 * @return bool true when NVS is ready and loaded, false on initialization failure.
 */
bool storage_init(void);

/**
 * @brief Save node state to persistent flash when it has changed.
 *
 * Commits the dedicated node name, flood scope, and serialized state blob to NVS.
 *
 * @param void No parameters.
 * @return bool true on success or if state was not dirty, false on write error.
 */
bool storage_save_if_dirty(void);

/**
 * @brief Save the dedicated node name directly to persistent flash.
 *
 * Sets and commits the node name key immediately in NVS so persistence does
 * not depend on secondary blob operations.
 *
 * @param name Pointer to null-terminated name string.
 * @return bool true if successfully saved and committed, false otherwise.
 */
bool storage_save_node_name(const char *name);

/**
 * @brief Claim the one-time BLE bond reset for the current firmware.
 *
 * Checks if the bond reset marker has been claimed, and sets it to prevent repeated
 * bond resets across reboots.
 *
 * @param void No parameters.
 * @return bool true if reset was successfully claimed, false if already claimed or storage
 * unavailable.
 */
bool storage_claim_ble_bond_reset(void);

/**
 * @brief Return the persistent local identity.
 *
 * @param void No parameters.
 * @return const MeshIdentity* Pointer to the loaded local identity.
 */
const MeshIdentity *storage_identity(void);

#endif // STORAGE_H
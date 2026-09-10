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
// File:    main.c
// Desc:    Initializes the bare MeshCore application entry point.
// Created: 2026

#include "ble_companion.h"
#include "config.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "node_state.h"
#include "runtime.h"
#include "storage.h"
#include "sx1262.h"

/**
 * @brief Application log tag.
 */
static const char *_main_log_tag = "main";

/**
 * @brief Start the bare MeshCore application.
 *
 * Initializes node state, persistent storage, NimBLE host, companion
 * BLE service, SX1262 LoRa transceiver, runtime task scheduler, and
 * companion advertising.
 *
 * @param void No parameters.
 * @return void
 */
void app_main(void) {
    node_state_init();
    storage_init();
    nimble_port_init();
    ble_companion_init();
    if (!sx1262_init(MESHCORE_DEFAULT_FREQUENCY,
                     MESHCORE_DEFAULT_BANDWIDTH,
                     MESHCORE_DEFAULT_SPREADING_FACTOR,
                     MESHCORE_DEFAULT_CODING_RATE)) {
        ESP_LOGE(_main_log_tag, "SX1262 initialization failed");
    }
    runtime_start();
    ble_companion_start();
}

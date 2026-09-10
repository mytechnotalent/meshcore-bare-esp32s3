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
// File:    sx1262.h
// Desc:    Declares the small SPI command driver used by the MeshCore radio.
// Created: 2026

#ifndef SX1262_H
#define SX1262_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize the SX1262 bus and radio.
 *
 * Configures SPI bus, resets device, configures regulator/DIO/LoRa packet engine,
 * and enters continuous RX mode.
 *
 * @param frequency RF carrier frequency in hertz.
 * @param bandwidth LoRa bandwidth in hertz.
 * @param spreading_factor LoRa spreading factor (5 to 12).
 * @param coding_rate LoRa coding-rate denominator (5 to 8).
 * @return bool true if radio initialized and ready, false otherwise.
 */
bool sx1262_init(uint32_t frequency,
                 uint32_t bandwidth,
                 uint8_t spreading_factor,
                 uint8_t coding_rate);

/**
 * @brief Apply new LoRa modulation parameters to the active SX1262.
 *
 * Sets RF frequency, bandwidth, spreading factor, and coding rate, and re-enters RX.
 *
 * @param frequency RF carrier frequency in hertz.
 * @param bandwidth LoRa bandwidth in hertz.
 * @param spreading_factor LoRa spreading factor (5 to 12).
 * @param coding_rate LoRa coding-rate denominator (5 to 8).
 * @return bool true on success, false on invalid parameters or bus error.
 */
bool sx1262_set_params(uint32_t frequency,
                       uint32_t bandwidth,
                       uint8_t spreading_factor,
                       uint8_t coding_rate);

/**
 * @brief Transmit one raw LoRa frame.
 *
 * Loads payload into TX buffer, starts transmission, waits for completion,
 * and restores RX mode.
 *
 * @param data Pointer to raw frame payload bytes to transmit.
 * @param length Number of bytes to transmit.
 * @return bool true if transmission completed successfully, false on error.
 */
bool sx1262_transmit(const uint8_t *data, size_t length);

/**
 * @brief Poll for one received raw LoRa frame.
 *
 * Reads RX buffer if packet is ready, copies bytes to data, and updates SNR/RSSI.
 *
 * @param data Destination buffer for received packet payload.
 * @param capacity Capacity of destination buffer in bytes.
 * @return size_t Number of bytes received, or 0 if no packet or error.
 */
size_t sx1262_receive(uint8_t *data, size_t capacity);

/**
 * @brief Return the SNR times four of the last received LoRa frame.
 *
 * @param void No parameters.
 * @return int8_t SNR value multiplied by four.
 */
int8_t sx1262_last_snr_x4(void);

/**
 * @brief Return the RSSI in dBm of the last received LoRa frame.
 *
 * @param void No parameters.
 * @return int8_t RSSI value in dBm.
 */
int8_t sx1262_last_rssi(void);

#endif // SX1262_H
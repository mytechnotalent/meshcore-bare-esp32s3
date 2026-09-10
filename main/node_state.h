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
// File:    node_state.h
// Desc:    Defines allocation-free node state for the MeshCore companion protocol.
// Created: 2026

#ifndef NODE_STATE_H
#define NODE_STATE_H

#include "config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Maximum stored MeshCore contacts.
 */
#define NODE_STATE_MAX_CONTACTS 40U

/**
 * @brief Maximum MeshCore group channels.
 */
#define NODE_STATE_MAX_CHANNELS MESHCORE_MAX_CHANNELS

/**
 * @brief MeshCore public-key length in bytes.
 */
#define NODE_STATE_PUBLIC_KEY_SIZE 32U

/**
 * @brief MeshCore contact name length in bytes.
 */
#define NODE_STATE_NAME_SIZE 32U

/**
 * @brief MeshCore channel secret length in bytes.
 */
#define NODE_STATE_CHANNEL_SECRET_SIZE 16U

/**
 * @brief Stored MeshCore contact structure.
 */
typedef struct {
    /**
     * @brief Contact public key.
     */
    uint8_t public_key[NODE_STATE_PUBLIC_KEY_SIZE];
    /**
     * @brief Contact type.
     */
    uint8_t type;
    /**
     * @brief Contact flags.
     */
    uint8_t flags;
    /**
     * @brief Contact outgoing path length.
     */
    int8_t out_path_length;
    /**
     * @brief Contact outgoing path bytes.
     */
    uint8_t out_path[64U];
    /**
     * @brief Contact display name.
     */
    char name[NODE_STATE_NAME_SIZE];
    /**
     * @brief Last contact advertisement timestamp.
     */
    uint32_t last_advert_timestamp;
    /**
     * @brief Contact latitude in millionths of a degree.
     */
    int32_t latitude;
    /**
     * @brief Contact longitude in millionths of a degree.
     */
    int32_t longitude;
    /**
     * @brief Contact modification timestamp.
     */
    uint32_t last_modified;
} NodeContact;

/**
 * @brief Stored MeshCore group channel structure.
 */
typedef struct {
    /**
     * @brief Whether this channel slot is configured.
     */
    bool configured;
    /**
     * @brief Channel display name.
     */
    char name[NODE_STATE_NAME_SIZE];
    /**
     * @brief Channel secret key.
     */
    uint8_t secret[NODE_STATE_CHANNEL_SECRET_SIZE];
} NodeChannel;

/**
 * @brief MeshCore radio configuration settings.
 */
typedef struct {
    /**
     * @brief Radio frequency in hertz.
     */
    uint32_t frequency;
    /**
     * @brief LoRa bandwidth in hertz.
     */
    uint32_t bandwidth;
    /**
     * @brief LoRa spreading factor.
     */
    uint8_t spreading_factor;
    /**
     * @brief LoRa coding-rate denominator.
     */
    uint8_t coding_rate;
    /**
     * @brief Radio transmit power in dBm.
     */
    int8_t transmit_power;
} NodeRadioSettings;

/**
 * @brief Initialize bounded node state.
 *
 * Clears contacts, channels, and sets default name and radio parameters.
 *
 * @param void No parameters.
 * @return void
 */
void node_state_init(void);

/**
 * @brief Set the advertised node name.
 *
 * Strips whitespace/newlines, bounds length to 31 bytes, and sets dirty flag.
 *
 * @param name Pointer to raw name bytes.
 * @param length Length of name in bytes.
 * @return bool true if name set successfully, false if empty or NULL.
 */
bool node_state_set_name(const uint8_t *name, size_t length);

/**
 * @brief Get the advertised node name.
 *
 * @param void No parameters.
 * @return const char* Pointer to null-terminated advertised node name string.
 */
const char *node_state_get_name(void);

/**
 * @brief Set the device Unix epoch time.
 *
 * Updates baseline epoch and tracks uptime offset.
 *
 * @param timestamp Unix epoch timestamp in seconds.
 * @return void
 */
void node_state_set_time(uint32_t timestamp);

/**
 * @brief Get the device Unix epoch time.
 *
 * Computes current timestamp from baseline epoch and elapsed uptime.
 *
 * @param void No parameters.
 * @return uint32_t Current Unix epoch timestamp in seconds.
 */
uint32_t node_state_get_time(void);

/**
 * @brief Return the number of stored contacts.
 *
 * @param void No parameters.
 * @return size_t Count of active contacts.
 */
size_t node_state_contact_count(void);

/**
 * @brief Find a contact by a public-key prefix.
 *
 * Matches up to key_length bytes against stored contacts.
 *
 * @param key Pointer to public key prefix bytes.
 * @param key_length Number of bytes to match.
 * @return NodeContact* Pointer to matching contact, or NULL if not found.
 */
NodeContact *node_state_find_contact(const uint8_t *key, size_t key_length);

/**
 * @brief Add or replace a contact in the table.
 *
 * If contact with matching public key exists, updates it; otherwise inserts
 * in an empty slot or replaces the oldest contact.
 *
 * @param contact Pointer to contact data to insert/update.
 * @return bool true on success, false if contact pointer is NULL.
 */
bool node_state_upsert_contact(const NodeContact *contact);

/**
 * @brief Remove a contact by public key prefix.
 *
 * @param key Pointer to public key bytes.
 * @param key_length Number of prefix bytes to match.
 * @return bool true if contact was found and removed, false otherwise.
 */
bool node_state_remove_contact(const uint8_t *key, size_t key_length);

/**
 * @brief Return a contact by linear index.
 *
 * @param index Zero-based slot index.
 * @return const NodeContact* Pointer to contact, or NULL if empty or out of bounds.
 */
const NodeContact *node_state_contact_at(size_t index);

/**
 * @brief Read a configured channel slot.
 *
 * @param index Channel index (0 to NODE_STATE_MAX_CHANNELS - 1).
 * @param channel Destination pointer for copied channel data.
 * @return bool true if channel slot is configured, false otherwise.
 */
bool node_state_get_channel(uint8_t index, NodeChannel *channel);

/**
 * @brief Store a configured channel into a slot.
 *
 * @param index Channel index (0 to NODE_STATE_MAX_CHANNELS - 1).
 * @param channel Pointer to channel data to store.
 * @return bool true on success, false if index out of bounds.
 */
bool node_state_set_channel(uint8_t index, const NodeChannel *channel);

/**
 * @brief Find a configured channel by its one-byte MeshCore hash.
 *
 * Compares hash with SHA-256(secret)[0] for each configured channel.
 *
 * @param hash 1-byte channel hash.
 * @return NodeChannel* Pointer to matching channel, or NULL if not found.
 */
NodeChannel *node_state_find_channel_hash(uint8_t hash);

/**
 * @brief Return the configured channel slot index.
 *
 * @param channel Pointer to channel within internal channel table.
 * @return uint8_t Slot index (0-7), or 255 if not found.
 */
uint8_t node_state_channel_index(const NodeChannel *channel);

/**
 * @brief Serialize all node state into a caller-provided buffer.
 *
 * Packs magic, settings, name, channels, and contacts into binary format.
 *
 * @param data Destination buffer for serialized state.
 * @param capacity Capacity of destination buffer in bytes.
 * @param length Pointer to store output byte count.
 * @return bool true on success, false if capacity is insufficient.
 */
bool node_state_export(uint8_t *data, size_t capacity, size_t *length);

/**
 * @brief Restore node state from a serialized buffer.
 *
 * Validates magic and version, and restores contacts, channels, and settings.
 *
 * @param data Pointer to serialized state bytes.
 * @param length Total length of serialized state in bytes.
 * @return bool true on success, false if corrupted or version mismatch.
 */
bool node_state_import(const uint8_t *data, size_t length);

/**
 * @brief Return whether node state changed since last persistence save.
 *
 * @param void No parameters.
 * @return bool true if uncommitted modifications exist, false otherwise.
 */
bool node_state_is_dirty(void);

/**
 * @brief Clear the node-state dirty marker.
 *
 * @param void No parameters.
 * @return void
 */
void node_state_clear_dirty(void);

/**
 * @brief Set node advertisement coordinates.
 *
 * @param latitude Latitude in millionths of a degree.
 * @param longitude Longitude in millionths of a degree.
 * @return bool true if location changed, false if identical.
 */
bool node_state_set_location(int32_t latitude, int32_t longitude);

/**
 * @brief Get node advertisement coordinates.
 *
 * @param latitude Destination pointer for latitude.
 * @param longitude Destination pointer for longitude.
 * @return void
 */
void node_state_get_location(int32_t *latitude, int32_t *longitude);

/**
 * @brief Set radio configuration parameters.
 *
 * @param settings Pointer to new NodeRadioSettings structure.
 * @return bool true if settings were updated, false if NULL.
 */
bool node_state_set_radio(const NodeRadioSettings *settings);

/**
 * @brief Get radio configuration parameters.
 *
 * @param settings Destination pointer for NodeRadioSettings structure.
 * @return void
 */
void node_state_get_radio(NodeRadioSettings *settings);

/**
 * @brief Set the MeshCore path-hash mode.
 *
 * @param mode Path hash mode byte.
 * @return bool true on success.
 */
bool node_state_set_path_hash_mode(uint8_t mode);

/**
 * @brief Get the MeshCore path-hash mode.
 *
 * @param void No parameters.
 * @return uint8_t Current path hash mode byte.
 */
uint8_t node_state_get_path_hash_mode(void);

/**
 * @brief Store the default flood-scope name and key.
 *
 * @param name Pointer to scope name string bytes.
 * @param name_length Length of name string in bytes.
 * @param key Pointer to 16-byte scope secret key.
 * @return bool true on success, false if parameters invalid.
 */
bool node_state_set_default_scope(const uint8_t *name, size_t name_length, const uint8_t *key);

/**
 * @brief Clear the default flood-scope name and key.
 *
 * @param void No parameters.
 * @return void
 */
void node_state_clear_default_scope(void);

/**
 * @brief Read the default flood-scope name and key.
 *
 * @param name Destination buffer for 31-byte null-terminated scope name (can be NULL).
 * @param key Destination buffer for 16-byte scope key (can be NULL).
 * @return void
 */
void node_state_get_default_scope(uint8_t *name, uint8_t *key);

#endif // NODE_STATE_H
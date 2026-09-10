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
// File:    node_state.c
// Desc:    Implements bounded in-memory MeshCore node state without heap allocation.
// Created: 2026

#include "node_state.h"

#include "config.h"
#include "mesh_crypto.h"

#include <string.h>

/**
 * @brief Stored contact table.
 */
static NodeContact _contacts[NODE_STATE_MAX_CONTACTS];

/**
 * @brief Contact table occupancy flags.
 */
static bool _contact_used[NODE_STATE_MAX_CONTACTS];

/**
 * @brief Stored channel table.
 */
static NodeChannel _channels[NODE_STATE_MAX_CHANNELS];

/**
 * @brief Advertised node name buffer.
 */
static char _node_name[NODE_STATE_NAME_SIZE];

/**
 * @brief Device Unix timestamp in seconds.
 */
static uint32_t _device_time;

/**
 * @brief State mutation dirty marker.
 */
static bool _state_dirty;

/**
 * @brief Node latitude in millionths of a degree.
 */
static int32_t _latitude;

/**
 * @brief Node longitude in millionths of a degree.
 */
static int32_t _longitude;

/**
 * @brief Active radio configuration parameters.
 */
static NodeRadioSettings _radio = {910525000U, 62500U, 7U, 5U, 17};

/**
 * @brief MeshCore path-hash mode.
 */
static uint8_t _path_hash_mode = 1U;

/**
 * @brief Default flood-scope name buffer.
 */
static uint8_t _scope_name[31U];

/**
 * @brief Default flood-scope secret key buffer.
 */
static uint8_t _scope_key[16U];

/**
 * @brief Public default MeshCore channel secret.
 */
static const uint8_t _public_channel_secret[NODE_STATE_CHANNEL_SECRET_SIZE] = {0x8B,
                                                                               0x33,
                                                                               0x87,
                                                                               0xE9,
                                                                               0xC5,
                                                                               0xCD,
                                                                               0xEA,
                                                                               0x6A,
                                                                               0xC9,
                                                                               0xE5,
                                                                               0xED,
                                                                               0xBA,
                                                                               0xA1,
                                                                               0x15,
                                                                               0xCD,
                                                                               0x72};

/**
 * @brief Compare public-key prefixes.
 *
 * Checks if the first length bytes of left and right public keys match.
 *
 * @param left Pointer to first key buffer.
 * @param right Pointer to second key buffer.
 * @param length Number of prefix bytes to compare.
 * @return bool true when prefixes match, false otherwise.
 */
static bool _node_state_key_matches(const uint8_t *left, const uint8_t *right, size_t length) {
    return left != NULL && right != NULL && length <= NODE_STATE_PUBLIC_KEY_SIZE &&
           memcmp(left, right, length) == 0;
}

/**
 * @brief Initialize all bounded state tables.
 *
 * Zeros out contacts, channels, name, and sets defaults including "Public" channel.
 *
 * @param void No parameters.
 * @return void
 */
void node_state_init(void) {
    memset(_contacts, 0, sizeof(_contacts));
    memset(_contact_used, 0, sizeof(_contact_used));
    memset(_channels, 0, sizeof(_channels));
    memset(_node_name, 0, sizeof(_node_name));
    strncpy(_node_name, MESHCORE_DEVICE_NAME, sizeof(_node_name) - 1U);
    _node_name[sizeof(_node_name) - 1U] = '\0';
    _channels[0].configured = true;
    memcpy(_channels[0].name, "Public", 6U);
    memcpy(_channels[0].secret, _public_channel_secret, sizeof(_public_channel_secret));
    memset(_scope_name, 0, sizeof(_scope_name));
    memset(_scope_key, 0, sizeof(_scope_key));
    _device_time = 0U;
    _state_dirty = false;
    _latitude = 0;
    _longitude = 0;
}

/**
 * @brief Copy a bounded advertised node name.
 *
 * Trims trailing newlines/nulls, copies up to 31 bytes, and sets dirty flag.
 *
 * @param name Pointer to raw name string bytes.
 * @param length Number of bytes in name string.
 * @return bool true if name set successfully, false if empty or NULL.
 */
bool node_state_set_name(const uint8_t *name, size_t length) {
    if (name == NULL || length == 0U) {
        return false;
    }
    size_t actual_length = strnlen((const char *)name, length);
    if (actual_length > 0U && actual_length < length) {
        length = actual_length;
    }
    while (length > 0U && (name[length - 1U] == '\0' || name[length - 1U] == '\r' ||
                           name[length - 1U] == '\n' || name[length - 1U] == ' ')) {
        length--;
    }
    if (length == 0U) {
        return false;
    }
    if (length >= sizeof(_node_name)) {
        length = sizeof(_node_name) - 1U;
    }
    memset(_node_name, 0, sizeof(_node_name));
    memcpy(_node_name, name, length);
    _node_name[length] = '\0';
    _state_dirty = true;
    return true;
}

/**
 * @brief Return the current advertised node name.
 *
 * @param void No parameters.
 * @return const char* Pointer to null-terminated advertised node name.
 */
const char *node_state_get_name(void) {
    return _node_name;
}

/**
 * @brief Set the device Unix time.
 *
 * @param timestamp Unix epoch timestamp in seconds.
 * @return void
 */
void node_state_set_time(uint32_t timestamp) {
    _device_time = timestamp;
    _state_dirty = true;
}

/**
 * @brief Return the device Unix time.
 *
 * @param void No parameters.
 * @return uint32_t Stored epoch timestamp in seconds.
 */
uint32_t node_state_get_time(void) {
    return _device_time;
}

/**
 * @brief Count occupied contact slots.
 *
 * @param void No parameters.
 * @return size_t Count of active contact records.
 */
size_t node_state_contact_count(void) {
    size_t count = 0U;
    for (size_t index = 0U; index < NODE_STATE_MAX_CONTACTS; index++) {
        if (_contact_used[index]) {
            count++;
        }
    }
    return count;
}

/**
 * @brief Find a contact by a full key or a supplied prefix.
 *
 * @param key Pointer to public key prefix bytes.
 * @param key_length Number of bytes in key prefix.
 * @return NodeContact* Pointer to matching contact, or NULL if not found.
 */
NodeContact *node_state_find_contact(const uint8_t *key, size_t key_length) {
    if (key == NULL || key_length == 0U || key_length > NODE_STATE_PUBLIC_KEY_SIZE) {
        return NULL;
    }
    for (size_t index = 0U; index < NODE_STATE_MAX_CONTACTS; index++) {
        if (_contact_used[index] &&
            _node_state_key_matches(_contacts[index].public_key, key, key_length)) {
            return &_contacts[index];
        }
    }
    return NULL;
}

/**
 * @brief Add a new contact or replace an existing contact.
 *
 * @param contact Pointer to NodeContact data to insert or update.
 * @return bool true when stored, false when table is full or input is invalid.
 */
bool node_state_upsert_contact(const NodeContact *contact) {
    NodeContact *existing;
    size_t oldest_index = 0U;
    uint32_t oldest_time = UINT32_MAX;
    if (contact == NULL) {
        return false;
    }
    existing = node_state_find_contact(contact->public_key, NODE_STATE_PUBLIC_KEY_SIZE);
    if (existing != NULL) {
        *existing = *contact;
        existing->name[sizeof(existing->name) - 1U] = '\0';
        _state_dirty = true;
        return true;
    }
    for (size_t index = 0U; index < NODE_STATE_MAX_CONTACTS; index++) {
        if (!_contact_used[index]) {
            _contacts[index] = *contact;
            _contacts[index].name[sizeof(_contacts[index].name) - 1U] = '\0';
            _contact_used[index] = true;
            _state_dirty = true;
            return true;
        }
        if (_contacts[index].last_modified < oldest_time) {
            oldest_time = _contacts[index].last_modified;
            oldest_index = index;
        }
    }
    _contacts[oldest_index] = *contact;
    _contacts[oldest_index].name[sizeof(_contacts[oldest_index].name) - 1U] = '\0';
    _contact_used[oldest_index] = true;
    _state_dirty = true;
    return true;
}

/**
 * @brief Remove a contact by a full key or prefix.
 *
 * @param key Pointer to public key prefix bytes.
 * @param key_length Length of key prefix in bytes.
 * @return bool true if contact was found and removed, false otherwise.
 */
bool node_state_remove_contact(const uint8_t *key, size_t key_length) {
    NodeContact *contact = node_state_find_contact(key, key_length);
    if (contact == NULL) {
        return false;
    }
    memset(contact, 0, sizeof(*contact));
    for (size_t index = 0U; index < NODE_STATE_MAX_CONTACTS; index++) {
        if (&_contacts[index] == contact) {
            _contact_used[index] = false;
            _state_dirty = true;
            return true;
        }
    }
    return false;
}

/**
 * @brief Return an occupied contact by ordinal index.
 *
 * @param index Ordinal index among occupied slots.
 * @return const NodeContact* Pointer to contact, or NULL if out of bounds.
 */
const NodeContact *node_state_contact_at(size_t index) {
    size_t found = 0U;
    for (size_t slot = 0U; slot < NODE_STATE_MAX_CONTACTS; slot++) {
        if (_contact_used[slot]) {
            if (found == index) {
                return &_contacts[slot];
            }
            found++;
        }
    }
    return NULL;
}

/**
 * @brief Validate whether a channel name is printable ASCII and non-empty.
 *
 * @param name Null-terminated channel name string.
 * @return bool true if valid and printable, false otherwise.
 */
static bool _is_valid_channel_name(const char *name) {
    size_t len;
    if (name == NULL || name[0] == '\0') {
        return false;
    }
    len = strnlen(name, NODE_STATE_NAME_SIZE);
    if (len == 0U || len >= NODE_STATE_NAME_SIZE) {
        return false;
    }
    for (size_t i = 0U; i < len; i++) {
        unsigned char c = (unsigned char)name[i];
        if (c < 0x20 || c > 0x7E) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Copy a configured channel from a channel slot.
 *
 * @param index Channel slot index (0 to NODE_STATE_MAX_CHANNELS - 1).
 * @param channel Destination pointer for copied channel data.
 * @return bool true when configured, false otherwise.
 */
bool node_state_get_channel(uint8_t index, NodeChannel *channel) {
    if (channel == NULL || index >= NODE_STATE_MAX_CHANNELS || !_channels[index].configured) {
        return false;
    }
    if (!_is_valid_channel_name(_channels[index].name)) {
        _channels[index].configured = false;
        return false;
    }
    *channel = _channels[index];
    return true;
}

/**
 * @brief Store a configured channel in a channel slot.
 *
 * @param index Channel slot index (0 to NODE_STATE_MAX_CHANNELS - 1).
 * @param channel Pointer to channel data to store.
 * @return bool true on success, false if index out of bounds or pointer NULL.
 */
bool node_state_set_channel(uint8_t index, const NodeChannel *channel) {
    if (channel == NULL || index >= NODE_STATE_MAX_CHANNELS) {
        return false;
    }
    if (channel->name[0] == '\0') {
        if (index == 0U) {
            return false;
        }
        memset(&_channels[index], 0, sizeof(NodeChannel));
        _channels[index].configured = false;
        _state_dirty = true;
        return true;
    }
    if (!_is_valid_channel_name(channel->name)) {
        return false;
    }
    _channels[index] = *channel;
    _channels[index].name[sizeof(_channels[index].name) - 1U] = '\0';
    _channels[index].configured = true;
    _state_dirty = true;
    return true;
}

/**
 * @brief Serialize bounded state with a versioned binary format.
 *
 * @param data Destination buffer for serialized binary state.
 * @param capacity Capacity of destination buffer in bytes.
 * @param length Pointer to store output byte count.
 * @return bool true on success, false if buffer is too small or NULL pointers.
 */
bool node_state_export(uint8_t *data, size_t capacity, size_t *length) {
    size_t position = 0U;
    uint32_t magic = 0x3343534DU;
    size_t active_channels = 0U;
    size_t active_contacts = 0U;
    size_t base_required = 4U + 4U + sizeof(_node_name) + sizeof(_latitude) + sizeof(_longitude) +
                           sizeof(_radio) + sizeof(_path_hash_mode) + sizeof(_scope_name) +
                           sizeof(_scope_key) + 1U + 1U;
    for (size_t index = 0U; index < NODE_STATE_MAX_CHANNELS; index++) {
        if (_channels[index].configured && _is_valid_channel_name(_channels[index].name)) {
            active_channels++;
        }
    }
    for (size_t index = 0U; index < NODE_STATE_MAX_CONTACTS; index++) {
        if (_contact_used[index]) {
            active_contacts++;
        }
    }
    size_t required = base_required + (active_channels * (1U + sizeof(NodeChannel))) +
                      (active_contacts * (1U + sizeof(NodeContact)));
    if (data == NULL || length == NULL || capacity < required) {
        return false;
    }
    memcpy(&data[position], &magic, 4U);
    position += 4U;
    memcpy(&data[position], &_device_time, 4U);
    position += 4U;
    memcpy(&data[position], _node_name, sizeof(_node_name));
    position += sizeof(_node_name);
    memcpy(&data[position], &_latitude, sizeof(_latitude));
    position += sizeof(_latitude);
    memcpy(&data[position], &_longitude, sizeof(_longitude));
    position += sizeof(_longitude);
    memcpy(&data[position], &_radio, sizeof(_radio));
    position += sizeof(_radio);
    memcpy(&data[position], &_path_hash_mode, sizeof(_path_hash_mode));
    position += sizeof(_path_hash_mode);
    memcpy(&data[position], _scope_name, sizeof(_scope_name));
    position += sizeof(_scope_name);
    memcpy(&data[position], _scope_key, sizeof(_scope_key));
    position += sizeof(_scope_key);
    data[position++] = (uint8_t)active_channels;
    for (size_t index = 0U; index < NODE_STATE_MAX_CHANNELS; index++) {
        if (_channels[index].configured && _is_valid_channel_name(_channels[index].name)) {
            data[position++] = (uint8_t)index;
            memcpy(&data[position], &_channels[index], sizeof(NodeChannel));
            position += sizeof(NodeChannel);
        }
    }
    data[position++] = (uint8_t)active_contacts;
    for (size_t index = 0U; index < NODE_STATE_MAX_CONTACTS; index++) {
        if (_contact_used[index]) {
            data[position++] = (uint8_t)index;
            memcpy(&data[position], &_contacts[index], sizeof(NodeContact));
            position += sizeof(NodeContact);
        }
    }
    *length = position;
    return true;
}

/**
 * @brief Restore bounded state after validating its version marker.
 *
 * Supports compact V3 ('MSC3'), compact V2 ('MSC2'), and legacy monolithic V1 ('MSC1') formats.
 *
 * @param data Pointer to serialized state bytes.
 * @param length Length of serialized state in bytes.
 * @return bool true on success, false if corrupted, bad magic, or invalid length.
 */
bool node_state_import(const uint8_t *data, size_t length) {
    size_t position = 0U;
    uint32_t magic;
    if (data == NULL || length < 8U) {
        return false;
    }
    memcpy(&magic, &data[position], 4U);
    position += 4U;
    if (magic == 0x3343534DU) {
        size_t base_required = 4U + 4U + sizeof(_node_name) + sizeof(_latitude) +
                               sizeof(_longitude) + sizeof(_radio) + sizeof(_path_hash_mode) +
                               sizeof(_scope_name) + sizeof(_scope_key) + 1U + 1U;
        if (length < base_required) {
            return false;
        }
        memcpy(&_device_time, &data[position], 4U);
        position += 4U;
        memcpy(_node_name, &data[position], sizeof(_node_name));
        position += sizeof(_node_name);
        if (_node_name[0] == '\0' || strcmp(_node_name, "NONAME") == 0) {
            memset(_node_name, 0, sizeof(_node_name));
            strncpy(_node_name, MESHCORE_DEVICE_NAME, sizeof(_node_name) - 1U);
            _node_name[sizeof(_node_name) - 1U] = '\0';
            _state_dirty = true;
        }
        memcpy(&_latitude, &data[position], sizeof(_latitude));
        position += sizeof(_latitude);
        memcpy(&_longitude, &data[position], sizeof(_longitude));
        position += sizeof(_longitude);
        memcpy(&_radio, &data[position], sizeof(_radio));
        position += sizeof(_radio);
        memcpy(&_path_hash_mode, &data[position], sizeof(_path_hash_mode));
        position += sizeof(_path_hash_mode);
        memcpy(_scope_name, &data[position], sizeof(_scope_name));
        position += sizeof(_scope_name);
        memcpy(_scope_key, &data[position], sizeof(_scope_key));
        position += sizeof(_scope_key);
        uint8_t active_channels = data[position++];
        if (length < base_required + (active_channels * (1U + sizeof(NodeChannel)))) {
            return false;
        }
        memset(_channels, 0, sizeof(_channels));
        for (uint8_t count = 0U; count < active_channels; count++) {
            uint8_t slot = data[position++];
            if (slot < NODE_STATE_MAX_CHANNELS) {
                memcpy(&_channels[slot], &data[position], sizeof(NodeChannel));
            }
            position += sizeof(NodeChannel);
        }
        uint8_t active_contacts = data[position++];
        if (length < base_required + (active_channels * (1U + sizeof(NodeChannel))) +
                         (active_contacts * (1U + sizeof(NodeContact)))) {
            return false;
        }
        memset(_contacts, 0, sizeof(_contacts));
        memset(_contact_used, 0, sizeof(_contact_used));
        for (uint8_t count = 0U; count < active_contacts; count++) {
            uint8_t slot = data[position++];
            if (slot < NODE_STATE_MAX_CONTACTS) {
                memcpy(&_contacts[slot], &data[position], sizeof(NodeContact));
                _contact_used[slot] = true;
            }
            position += sizeof(NodeContact);
        }
    } else if (magic == 0x3243534DU) {
        size_t header_len = 4U + 4U + sizeof(_node_name) + sizeof(_latitude) + sizeof(_longitude) +
                            sizeof(_radio) + sizeof(_path_hash_mode) + sizeof(_scope_name) +
                            sizeof(_scope_key);
        size_t legacy_channels_bytes = 8U * sizeof(NodeChannel);
        size_t current_channels_bytes = sizeof(_channels);
        size_t channels_to_read_bytes = 0U;
        if (length >= header_len + current_channels_bytes + 1U) {
            channels_to_read_bytes = current_channels_bytes;
        } else if (length >= header_len + legacy_channels_bytes + 1U) {
            channels_to_read_bytes = legacy_channels_bytes;
        } else {
            return false;
        }
        size_t base_required = header_len + channels_to_read_bytes + 1U;
        memcpy(&_device_time, &data[position], 4U);
        position += 4U;
        memcpy(_node_name, &data[position], sizeof(_node_name));
        position += sizeof(_node_name);
        if (_node_name[0] == '\0' || strcmp(_node_name, "NONAME") == 0) {
            memset(_node_name, 0, sizeof(_node_name));
            strncpy(_node_name, MESHCORE_DEVICE_NAME, sizeof(_node_name) - 1U);
            _node_name[sizeof(_node_name) - 1U] = '\0';
            _state_dirty = true;
        }
        memcpy(&_latitude, &data[position], sizeof(_latitude));
        position += sizeof(_latitude);
        memcpy(&_longitude, &data[position], sizeof(_longitude));
        position += sizeof(_longitude);
        memcpy(&_radio, &data[position], sizeof(_radio));
        position += sizeof(_radio);
        memcpy(&_path_hash_mode, &data[position], sizeof(_path_hash_mode));
        position += sizeof(_path_hash_mode);
        memcpy(_scope_name, &data[position], sizeof(_scope_name));
        position += sizeof(_scope_name);
        memcpy(_scope_key, &data[position], sizeof(_scope_key));
        position += sizeof(_scope_key);
        memset(_channels, 0, sizeof(_channels));
        memcpy(_channels, &data[position], channels_to_read_bytes);
        position += channels_to_read_bytes;
        uint8_t active_contacts = data[position++];
        if (length < base_required + (active_contacts * (1U + sizeof(NodeContact)))) {
            return false;
        }
        memset(_contacts, 0, sizeof(_contacts));
        memset(_contact_used, 0, sizeof(_contact_used));
        for (uint8_t count = 0U; count < active_contacts; count++) {
            uint8_t slot = data[position++];
            if (slot < NODE_STATE_MAX_CONTACTS) {
                memcpy(&_contacts[slot], &data[position], sizeof(NodeContact));
                _contact_used[slot] = true;
            }
            position += sizeof(NodeContact);
        }
    } else if (magic == 0x3143534DU) {
        size_t legacy_channels_bytes = 8U * sizeof(NodeChannel);
        size_t old_required = 4U + 4U + sizeof(_node_name) + sizeof(_contact_used) +
                              sizeof(_contacts) + legacy_channels_bytes;
        size_t legacy_required = old_required + sizeof(_latitude) + sizeof(_longitude) +
                                 sizeof(_radio) + sizeof(_scope_name) + sizeof(_scope_key);
        size_t required = old_required + sizeof(_latitude) + sizeof(_longitude) + sizeof(_radio) +
                          sizeof(_path_hash_mode) + sizeof(_scope_name) + sizeof(_scope_key);
        if (length != old_required && length != legacy_required && length != required) {
            return false;
        }
        memcpy(&_device_time, &data[position], 4U);
        position += 4U;
        memcpy(_node_name, &data[position], sizeof(_node_name));
        position += sizeof(_node_name);
        if (_node_name[0] == '\0' || strcmp(_node_name, "NONAME") == 0) {
            memset(_node_name, 0, sizeof(_node_name));
            strncpy(_node_name, MESHCORE_DEVICE_NAME, sizeof(_node_name) - 1U);
            _node_name[sizeof(_node_name) - 1U] = '\0';
            _state_dirty = true;
        }
        memcpy(_contact_used, &data[position], sizeof(_contact_used));
        position += sizeof(_contact_used);
        memcpy(_contacts, &data[position], sizeof(_contacts));
        position += sizeof(_contacts);
        memset(_channels, 0, sizeof(_channels));
        memcpy(_channels, &data[position], legacy_channels_bytes);
        position += legacy_channels_bytes;
        if (length == legacy_required || length == required) {
            memcpy(&_latitude, &data[position], sizeof(_latitude));
            position += sizeof(_latitude);
            memcpy(&_longitude, &data[position], sizeof(_longitude));
            position += sizeof(_longitude);
            memcpy(&_radio, &data[position], sizeof(_radio));
            position += sizeof(_radio);
            if (length == required) {
                memcpy(&_path_hash_mode, &data[position], sizeof(_path_hash_mode));
                position += sizeof(_path_hash_mode);
            }
            memcpy(_scope_name, &data[position], sizeof(_scope_name));
            position += sizeof(_scope_name);
            memcpy(_scope_key, &data[position], sizeof(_scope_key));
        }
    } else {
        return false;
    }
    _scope_name[sizeof(_scope_name) - 1U] = '\0';
    if (!_channels[0].configured || !_is_valid_channel_name(_channels[0].name)) {
        _channels[0].configured = true;
        memset(_channels[0].name, 0, sizeof(_channels[0].name));
        memcpy(_channels[0].name, "Public", 6U);
        memcpy(_channels[0].secret, _public_channel_secret, sizeof(_public_channel_secret));
    }
    for (size_t index = 1U; index < NODE_STATE_MAX_CHANNELS; index++) {
        if (_channels[index].configured && !_is_valid_channel_name(_channels[index].name)) {
            memset(&_channels[index], 0, sizeof(NodeChannel));
            _channels[index].configured = false;
        }
    }
    _node_name[sizeof(_node_name) - 1U] = '\0';
    _state_dirty = false;
    return true;
}

/**
 * @brief Return whether node state changed since persistence.
 *
 * @param void No parameters.
 * @return bool true if state has uncommitted changes, false otherwise.
 */
bool node_state_is_dirty(void) {
    return _state_dirty;
}

/**
 * @brief Clear the node-state dirty marker.
 *
 * @param void No parameters.
 * @return void
 */
void node_state_clear_dirty(void) {
    _state_dirty = false;
}

/**
 * @brief Set valid node advertisement coordinates.
 *
 * @param latitude Latitude in millionths of a degree.
 * @param longitude Longitude in millionths of a degree.
 * @return bool true when valid and stored, false otherwise.
 */
bool node_state_set_location(int32_t latitude, int32_t longitude) {
    if (latitude < -90000000 || latitude > 90000000 || longitude < -180000000 ||
        longitude > 180000000) {
        return false;
    }
    _latitude = latitude;
    _longitude = longitude;
    _state_dirty = true;
    return true;
}

/**
 * @brief Return node advertisement coordinates.
 *
 * @param latitude Destination pointer for latitude in millionths of a degree.
 * @param longitude Destination pointer for longitude in millionths of a degree.
 * @return void
 */
void node_state_get_location(int32_t *latitude, int32_t *longitude) {
    if (latitude != NULL) {
        *latitude = _latitude;
    }
    if (longitude != NULL) {
        *longitude = _longitude;
    }
}

/**
 * @brief Store validated radio parameters.
 *
 * @param settings Pointer to NodeRadioSettings structure to validate and store.
 * @return bool true when valid and stored, false otherwise.
 */
bool node_state_set_radio(const NodeRadioSettings *settings) {
    if (settings == NULL || settings->frequency < 150000U || settings->frequency > 2500000000U ||
        settings->bandwidth < 7000U || settings->bandwidth > 500000U ||
        settings->spreading_factor < 5U || settings->spreading_factor > 12U ||
        settings->coding_rate < 5U || settings->coding_rate > 8U) {
        return false;
    }
    _radio = *settings;
    _state_dirty = true;
    return true;
}

/**
 * @brief Return active radio parameters.
 *
 * @param settings Destination pointer for radio settings.
 * @return void
 */
void node_state_get_radio(NodeRadioSettings *settings) {
    if (settings != NULL) {
        *settings = _radio;
    }
}

/**
 * @brief Set the MeshCore path-hash mode.
 *
 * @param mode Path-hash mode from zero through two.
 * @return bool true when accepted and stored, false otherwise.
 */
bool node_state_set_path_hash_mode(uint8_t mode) {
    if (mode >= 3U) {
        return false;
    }
    _path_hash_mode = mode;
    _state_dirty = true;
    return true;
}

/**
 * @brief Get the MeshCore path-hash mode.
 *
 * @param void No parameters.
 * @return uint8_t Current path-hash mode byte.
 */
uint8_t node_state_get_path_hash_mode(void) {
    return _path_hash_mode;
}

/**
 * @brief Store a bounded default flood-scope name and key.
 *
 * @param name Pointer to scope name bytes.
 * @param name_length Number of name bytes.
 * @param key Pointer to 16-byte transport secret key.
 * @return bool true when valid and stored, false otherwise.
 */
bool node_state_set_default_scope(const uint8_t *name, size_t name_length, const uint8_t *key) {
    if (name == NULL || key == NULL || name_length == 0U || name_length >= sizeof(_scope_name)) {
        return false;
    }
    memset(_scope_name, 0, sizeof(_scope_name));
    memcpy(_scope_name, name, name_length);
    memcpy(_scope_key, key, sizeof(_scope_key));
    _state_dirty = true;
    return true;
}

/**
 * @brief Clear the default flood-scope name and key.
 *
 * @param void No parameters.
 * @return void
 */
void node_state_clear_default_scope(void) {
    memset(_scope_name, 0, sizeof(_scope_name));
    memset(_scope_key, 0, sizeof(_scope_key));
    _state_dirty = true;
}

/**
 * @brief Read the default flood-scope name and key.
 *
 * @param name Destination buffer for scope name.
 * @param key Destination buffer for 16-byte transport key.
 * @return void
 */
void node_state_get_default_scope(uint8_t *name, uint8_t *key) {
    if (name != NULL) {
        memcpy(name, _scope_name, sizeof(_scope_name));
    }
    if (key != NULL) {
        memcpy(key, _scope_key, sizeof(_scope_key));
    }
}

/**
 * @brief Find a configured channel by its SHA-256 hash prefix.
 *
 * @param hash One-byte channel hash.
 * @return NodeChannel* Pointer to matching channel, or NULL when absent.
 */
NodeChannel *node_state_find_channel_hash(uint8_t hash) {
    uint8_t channel_hash;
    for (size_t index = 0U; index < NODE_STATE_MAX_CHANNELS; index++) {
        if (_channels[index].configured && _is_valid_channel_name(_channels[index].name) &&
            mesh_crypto_sha256(
                _channels[index].secret, sizeof(_channels[index].secret), &channel_hash, 1U) &&
            channel_hash == hash) {
            return &_channels[index];
        }
    }
    return NULL;
}

/**
 * @brief Return a configured channel's slot index or 255.
 *
 * @param channel Pointer to configured channel.
 * @return uint8_t Slot index, or 255 when invalid or absent.
 */
uint8_t node_state_channel_index(const NodeChannel *channel) {
    if (channel == NULL) {
        return 255U;
    }
    for (uint8_t index = 0U; index < NODE_STATE_MAX_CHANNELS; index++) {
        if (&_channels[index] == channel) {
            return index;
        }
    }
    return 255U;
}
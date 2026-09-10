/**
 * MIT License
 *
 * Copyright (c) 2026 Kevin Thomas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * sx1262.c
 *
 * SX1262 radio driver implementation.
 *
 * Author: Kevin Thomas
 * Date: 2026
 * Email: kevin@mytechnotalent.com
 * GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
 */

#include "sx1262.h"

#include "config.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mesh_packet.h"

#include <string.h>

/**
 * @brief SX1262 SPI device handle.
 */
static spi_device_handle_t _sx1262_device;

/**
 * @brief Mutex protecting SX1262 hardware and state.
 */
static SemaphoreHandle_t _sx1262_mutex = NULL;

/**
 * @brief SNR times four of last received packet.
 */
static int8_t _last_snr_x4;

/**
 * @brief RSSI in dBm of last received packet.
 */
static int8_t _last_rssi = -60;

/**
 * @brief SX1262 log tag.
 */
static const char *_sx1262_log_tag = "sx1262";

/**
 * @brief Set the external RF switch to receive mode.
 *
 * @param void No parameters.
 * @return void
 */
static void _sx1262_receive_path(void) {
    gpio_set_level(MESHCORE_PIN_TXEN, 0);
    gpio_set_level(MESHCORE_PIN_RXEN, 1);
}

/**
 * @brief Set the external RF switch to transmit mode.
 *
 * @param void No parameters.
 * @return void
 */
static void _sx1262_transmit_path(void) {
    gpio_set_level(MESHCORE_PIN_RXEN, 0);
    gpio_set_level(MESHCORE_PIN_TXEN, 1);
}

/**
 * @brief Wait until the SX1262 releases its busy line.
 *
 * @param void No parameters.
 * @return bool True when ready, false on timeout.
 */
static bool _sx1262_wait(void) {
    uint32_t elapsed = 0U;
    esp_rom_delay_us(25U);
    while (gpio_get_level(MESHCORE_PIN_BUSY) != 0) {
        esp_rom_delay_us(10U);
        elapsed += 10U;
        if (elapsed > 100000U) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Send a short command to SX1262.
 *
 * @param command Pointer to the command buffer.
 * @param length Length of the command in bytes.
 * @return bool True if successful, false otherwise.
 */
static bool _sx1262_command(const uint8_t *command, size_t length) {
    spi_transaction_t transaction = {0};
    if (!_sx1262_wait()) {
        return false;
    }
    transaction.length = length * 8U;
    transaction.tx_buffer = command;
    if (spi_device_transmit(_sx1262_device, &transaction) != ESP_OK) {
        return false;
    }
    return _sx1262_wait();
}

/**
 * @brief Transfer an SX1262 command while collecting returned bytes.
 *
 * @param buffer Pointer to the command/response buffer.
 * @param length Length of the transfer in bytes.
 * @return bool True if successful, false otherwise.
 */
static bool _sx1262_transfer(uint8_t *buffer, size_t length) {
    spi_transaction_t transaction = {0};
    uint8_t tx_buffer[MESH_PACKET_MAX_SIZE + 3U] = {0};
    uint8_t rx_buffer[MESH_PACKET_MAX_SIZE + 3U] = {0};
    if (length > sizeof(tx_buffer)) {
        return false;
    }
    if (!_sx1262_wait()) {
        return false;
    }
    memcpy(tx_buffer, buffer, length);
    transaction.length = length * 8U;
    transaction.tx_buffer = tx_buffer;
    transaction.rx_buffer = rx_buffer;
    if (spi_device_transmit(_sx1262_device, &transaction) != ESP_OK) {
        return false;
    }
    memcpy(buffer, rx_buffer, length);
    return _sx1262_wait();
}

/**
 * @brief Query SX1262 device error flags.
 *
 * @param void No parameters.
 * @return uint16_t 16-bit error flags.
 */
static uint16_t _sx1262_device_errors(void) {
    uint8_t command[4U] = {0x17U, 0x00U, 0x00U, 0x00U};
    if (!_sx1262_transfer(command, sizeof(command))) {
        return 0xFFFFU;
    }
    return ((uint16_t)command[2] << 8U) | command[3];
}

/**
 * @brief Query SX1262 status byte.
 *
 * @param void No parameters.
 * @return uint8_t 8-bit status.
 */
static uint8_t _sx1262_status(void) {
    uint8_t command[2U] = {0xC0U, 0x00U};
    if (!_sx1262_transfer(command, sizeof(command))) {
        return 0xFFU;
    }
    return command[1];
}

/**
 * @brief Wait until SX1262 finishes transmission or times out.
 *
 * @param timeout_ms Maximum time to wait in milliseconds.
 * @return bool True when TX completed, false on timeout.
 */
static bool _sx1262_wait_tx_done(uint32_t timeout_ms) {
    uint32_t elapsed = 0U;
    uint8_t irq_status[4U];
    uint16_t irq;
    while (elapsed < timeout_ms) {
        irq_status[0] = 0x12U;
        irq_status[1] = 0x00U;
        irq_status[2] = 0x00U;
        irq_status[3] = 0x00U;
        if (_sx1262_transfer(irq_status, sizeof(irq_status))) {
            irq = ((uint16_t)irq_status[2] << 8U) | irq_status[3];
            if ((irq & 0x0001U) != 0U) {
                ESP_LOGI(_sx1262_log_tag,
                         "tx_done at %lu ms dio1=%d irq=0x%04X",
                         (unsigned long)elapsed,
                         gpio_get_level(MESHCORE_PIN_DIO1),
                         (unsigned)irq);
                return true;
            }
            if ((irq & 0x0200U) != 0U) {
                ESP_LOGE(_sx1262_log_tag,
                         "tx_timeout at %lu ms dio1=%d irq=0x%04X",
                         (unsigned long)elapsed,
                         gpio_get_level(MESHCORE_PIN_DIO1),
                         (unsigned)irq);
                return false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10U) > 0 ? pdMS_TO_TICKS(10U) : 1);
        elapsed += 10U;
    }
    ESP_LOGE(_sx1262_log_tag,
             "tx host timeout after %lu ms dio1=%d errors=0x%04X",
             (unsigned long)elapsed,
             gpio_get_level(MESHCORE_PIN_DIO1),
             (unsigned)_sx1262_device_errors());
    return false;
}

/**
 * @brief Configure TCXO or XTAL oscillator and calibrate internal blocks.
 *
 * @param void No parameters.
 * @return bool True if oscillator started without error, false otherwise.
 */
static bool _sx1262_start_oscillator(void) {
    uint8_t set_rc[] = {0x80U, 0x00U};
    uint8_t set_xosc[] = {0x80U, 0x01U};
    uint8_t clear_err[] = {0x07U, 0x00U, 0x00U};
    uint8_t calibrate_all[] = {0x89U, 0x7FU};
    uint8_t calibrate_img[] = {0x98U, 0xE1U, 0xE9U};
    uint8_t tcxo_voltages[] = {0x00U, 0x02U, 0x07U};
    uint8_t tcxo_cmd[] = {0x97U, 0x00U, 0x00U, 0x01U, 0x40U};
    uint16_t errors;
    size_t i;
    for (i = 0U; i < sizeof(tcxo_voltages); i++) {
        tcxo_cmd[1] = tcxo_voltages[i];
        _sx1262_command(set_rc, sizeof(set_rc));
        _sx1262_command(clear_err, sizeof(clear_err));
        _sx1262_command(tcxo_cmd, sizeof(tcxo_cmd));
        vTaskDelay(pdMS_TO_TICKS(10U) > 0 ? pdMS_TO_TICKS(10U) : 1);
        _sx1262_wait();
        _sx1262_command(calibrate_all, sizeof(calibrate_all));
        vTaskDelay(pdMS_TO_TICKS(10U) > 0 ? pdMS_TO_TICKS(10U) : 1);
        _sx1262_wait();
        _sx1262_command(calibrate_img, sizeof(calibrate_img));
        _sx1262_wait();
        _sx1262_command(clear_err, sizeof(clear_err));
        _sx1262_command(set_xosc, sizeof(set_xosc));
        _sx1262_wait();
        errors = _sx1262_device_errors();
        if ((errors & 0x0020U) == 0U) {
            ESP_LOGI(_sx1262_log_tag,
                     "tcxo locked volt_code=0x%02X errors=0x%04X status=0x%02X",
                     (unsigned)tcxo_voltages[i],
                     (unsigned)errors,
                     (unsigned)_sx1262_status());
            return true;
        }
        ESP_LOGW(_sx1262_log_tag,
                 "tcxo attempt %u (code 0x%02X) errors=0x%04X",
                 (unsigned)i,
                 (unsigned)tcxo_voltages[i],
                 (unsigned)errors);
    }
    gpio_set_level(MESHCORE_PIN_RESET, 0);
    esp_rom_delay_us(10000U);
    gpio_set_level(MESHCORE_PIN_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(10U) > 0 ? pdMS_TO_TICKS(10U) : 1);
    _sx1262_wait();
    _sx1262_command(set_rc, sizeof(set_rc));
    _sx1262_command(clear_err, sizeof(clear_err));
    _sx1262_command(calibrate_all, sizeof(calibrate_all));
    vTaskDelay(pdMS_TO_TICKS(10U) > 0 ? pdMS_TO_TICKS(10U) : 1);
    _sx1262_wait();
    _sx1262_command(calibrate_img, sizeof(calibrate_img));
    _sx1262_wait();
    _sx1262_command(clear_err, sizeof(clear_err));
    _sx1262_command(set_xosc, sizeof(set_xosc));
    _sx1262_wait();
    errors = _sx1262_device_errors();
    ESP_LOGI(_sx1262_log_tag,
             "xtal fallback errors=0x%04X status=0x%02X",
             (unsigned)errors,
             (unsigned)_sx1262_status());
    return (errors & 0x0020U) == 0U;
}

/**
 * @brief Configure the SX1262 for a basic explicit-header LoRa packet.
 *
 * @param frequency RF frequency in Hertz.
 * @param bandwidth Signal bandwidth in Hertz.
 * @param spreading_factor LoRa spreading factor (SF7-SF12).
 * @param coding_rate LoRa coding rate (5-8).
 * @return bool True if configuration succeeded, false otherwise.
 */
static bool _sx1262_configure(uint32_t frequency,
                              uint32_t bandwidth,
                              uint8_t spreading_factor,
                              uint8_t coding_rate) {
    uint32_t rf_frequency = (uint32_t)(((uint64_t)frequency << 25U) / 32000000U);
    uint8_t packet_type[] = {0x8AU, 0x01U};
    uint8_t frequency_command[] = {0x86U,
                                   (uint8_t)(rf_frequency >> 24U),
                                   (uint8_t)(rf_frequency >> 16U),
                                   (uint8_t)(rf_frequency >> 8U),
                                   (uint8_t)rf_frequency};
    uint8_t pa_config[] = {0x95U, 0x04U, 0x07U, 0x00U, 0x01U};
    uint8_t modulation[] = {
        0x8BU, (uint8_t)spreading_factor, 0x06U, (uint8_t)(coding_rate - 4U), 0x00U};
    uint8_t packet[] = {0x8CU, 0x00U, 0x20U, 0x00U, 0xFFU, 0x01U, 0x00U};
    uint8_t power[] = {0x8EU, 0x16U, 0x04U};
    uint8_t current_limit[] = {0x0DU, 0x08U, 0xE7U, 0x38U};
    uint8_t rx_gain[] = {0x0DU, 0x08U, 0xACU, 0x96U};
    uint8_t buffer[] = {0x8FU, 0x00U, 0x00U};
    uint8_t irq[] = {0x08U, 0x02U, 0x63U, 0x00U, 0x03U, 0x00U, 0x00U, 0x00U, 0x00U};
    uint8_t regulator[] = {0x96U, 0x00U};
    uint8_t dio2_switch[] = {0x9DU, 0x01U};
    uint8_t sync_word[] = {0x0DU, 0x07U, 0x40U, 0x14U, 0x24U};
    uint8_t clear_errors[] = {0x07U, 0x00U, 0x00U};
    if (bandwidth == 7800U)
        modulation[2] = 0x00U;
    else if (bandwidth == 10400U)
        modulation[2] = 0x08U;
    else if (bandwidth == 15600U)
        modulation[2] = 0x01U;
    else if (bandwidth == 20800U)
        modulation[2] = 0x09U;
    else if (bandwidth == 31250U)
        modulation[2] = 0x02U;
    else if (bandwidth == 41700U)
        modulation[2] = 0x0AU;
    else if (bandwidth == 62500U)
        modulation[2] = 0x03U;
    else if (bandwidth == 125000U)
        modulation[2] = 0x04U;
    else if (bandwidth == 250000U)
        modulation[2] = 0x05U;
    else if (bandwidth == 500000U)
        modulation[2] = 0x06U;
    else
        return false;
    return _sx1262_command(regulator, sizeof(regulator)) &&
           _sx1262_command(dio2_switch, sizeof(dio2_switch)) &&
           _sx1262_command(packet_type, sizeof(packet_type)) &&
           _sx1262_command(frequency_command, sizeof(frequency_command)) &&
           _sx1262_command(pa_config, sizeof(pa_config)) &&
           _sx1262_command(modulation, sizeof(modulation)) &&
           _sx1262_command(packet, sizeof(packet)) && _sx1262_command(power, sizeof(power)) &&
           _sx1262_command(current_limit, sizeof(current_limit)) &&
           _sx1262_command(rx_gain, sizeof(rx_gain)) && _sx1262_command(buffer, sizeof(buffer)) &&
           _sx1262_command(sync_word, sizeof(sync_word)) && _sx1262_command(irq, sizeof(irq)) &&
           _sx1262_command(clear_errors, sizeof(clear_errors));
}

/**
 * @brief Initialize SPI, reset SX1262, and configure standby.
 *
 * @param frequency RF frequency in Hertz.
 * @param bandwidth Signal bandwidth in Hertz.
 * @param spreading_factor LoRa spreading factor (SF7-SF12).
 * @param coding_rate LoRa coding rate (5-8).
 * @return bool True if initialization succeeded, false otherwise.
 */
bool sx1262_init(uint32_t frequency,
                 uint32_t bandwidth,
                 uint8_t spreading_factor,
                 uint8_t coding_rate) {
    spi_bus_config_t bus = {.mosi_io_num = MESHCORE_PIN_MOSI,
                            .miso_io_num = MESHCORE_PIN_MISO,
                            .sclk_io_num = MESHCORE_PIN_SCK,
                            .quadwp_io_num = -1,
                            .quadhd_io_num = -1,
                            .max_transfer_sz = MESH_PACKET_MAX_SIZE + 3U};
    spi_device_interface_config_t device = {
        .clock_speed_hz = 8000000, .mode = 0, .spics_io_num = MESHCORE_PIN_NSS, .queue_size = 1};
    uint8_t set_standby[] = {0x80U, 0x00U};
    uint8_t status;
    uint16_t errors;
    bool initialized;
    if (_sx1262_mutex == NULL) {
        _sx1262_mutex = xSemaphoreCreateMutex();
    }
    gpio_set_direction(MESHCORE_PIN_RESET, GPIO_MODE_OUTPUT);
    gpio_set_direction(MESHCORE_PIN_BUSY, GPIO_MODE_INPUT);
    gpio_set_direction(MESHCORE_PIN_DIO1, GPIO_MODE_INPUT);
    gpio_set_direction(MESHCORE_PIN_RXEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(MESHCORE_PIN_TXEN, GPIO_MODE_OUTPUT);
    gpio_set_level(MESHCORE_PIN_RESET, 0);
    esp_rom_delay_us(10000U);
    gpio_set_level(MESHCORE_PIN_RESET, 1);
    _sx1262_receive_path();
    if (spi_bus_initialize(MESHCORE_SPI_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK ||
        spi_bus_add_device(MESHCORE_SPI_HOST, &device, &_sx1262_device) != ESP_OK) {
        return false;
    }
    if (_sx1262_mutex != NULL) {
        xSemaphoreTake(_sx1262_mutex, portMAX_DELAY);
    }
    initialized = _sx1262_command(set_standby, sizeof(set_standby)) &&
                  _sx1262_command((const uint8_t[]){0x96U, 0x00U}, 2U) &&
                  _sx1262_start_oscillator() &&
                  _sx1262_configure(frequency, bandwidth, spreading_factor, coding_rate) &&
                  _sx1262_command((const uint8_t[]){0x82U, 0xFFU, 0xFFU, 0xFFU}, 4U);
    _sx1262_wait();
    status = _sx1262_status();
    errors = _sx1262_device_errors();
    if (_sx1262_mutex != NULL) {
        xSemaphoreGive(_sx1262_mutex);
    }
    ESP_LOGI(_sx1262_log_tag,
             "init=%s frequency=%lu bandwidth=%lu sf=%u cr=%u dio1=%d busy=%d status=0x%02X "
             "errors=0x%04X",
             initialized ? "ok" : "failed",
             (unsigned long)frequency,
             (unsigned long)bandwidth,
             (unsigned)spreading_factor,
             (unsigned)coding_rate,
             gpio_get_level(MESHCORE_PIN_DIO1),
             gpio_get_level(MESHCORE_PIN_BUSY),
             (unsigned)status,
             (unsigned)errors);
    return initialized;
}

/**
 * @brief Apply new LoRa modulation parameters to the active SX1262.
 *
 * @param frequency RF frequency in Hertz.
 * @param bandwidth Signal bandwidth in Hertz.
 * @param spreading_factor LoRa spreading factor (SF7-SF12).
 * @param coding_rate LoRa coding rate (5-8).
 * @return bool True if parameters were applied, false otherwise.
 */
bool sx1262_set_params(uint32_t frequency,
                       uint32_t bandwidth,
                       uint8_t spreading_factor,
                       uint8_t coding_rate) {
    bool result;
    if (_sx1262_mutex != NULL && xSemaphoreTake(_sx1262_mutex, pdMS_TO_TICKS(1000U)) != pdTRUE) {
        return false;
    }
    result = _sx1262_configure(frequency, bandwidth, spreading_factor, coding_rate) &&
             _sx1262_command((const uint8_t[]){0x82U, 0xFFU, 0xFFU, 0xFFU}, 4U);
    if (_sx1262_mutex != NULL) {
        xSemaphoreGive(_sx1262_mutex);
    }
    return result;
}

/**
 * @brief Transmit one raw frame using the SX1262 buffer command.
 *
 * @param data Pointer to the buffer containing data to transmit.
 * @param length Length of data in bytes.
 * @return bool True if transmission was successful, false otherwise.
 */
bool sx1262_transmit(const uint8_t *data, size_t length) {
    uint8_t command[2U + MESH_PACKET_MAX_SIZE] = {0};
    uint8_t packet_params[] = {0x8CU, 0x00U, 0x20U, 0x00U, (uint8_t)length, 0x01U, 0x00U};
    uint8_t rx_packet[] = {0x8CU, 0x00U, 0x20U, 0x00U, 0xFFU, 0x01U, 0x00U};
    uint8_t transmit[] = {0x83U, 0x00U, 0x00U, 0x00U};
    uint8_t clear_irq[] = {0x02U, 0xFFU, 0xFFU};
    uint8_t receive[] = {0x82U, 0xFFU, 0xFFU, 0xFFU};
    uint8_t status;
    uint16_t errors;
    bool sent;
    if (data == NULL || length == 0U || length > MESH_PACKET_MAX_SIZE) {
        return false;
    }
    if (_sx1262_mutex != NULL && xSemaphoreTake(_sx1262_mutex, pdMS_TO_TICKS(3000U)) != pdTRUE) {
        return false;
    }
    command[0] = 0x0EU;
    command[1] = 0x00U;
    memcpy(&command[2], data, length);
    _sx1262_transmit_path();
    _sx1262_command(packet_params, sizeof(packet_params));
    _sx1262_command(command, length + 2U);
    _sx1262_command(clear_irq, sizeof(clear_irq));
    sent = _sx1262_command(transmit, sizeof(transmit));
    if (sent) {
        sent = _sx1262_wait_tx_done(2000U);
    }
    _sx1262_command(clear_irq, sizeof(clear_irq));
    _sx1262_receive_path();
    _sx1262_command(rx_packet, sizeof(rx_packet));
    _sx1262_command(receive, sizeof(receive));
    status = _sx1262_status();
    errors = _sx1262_device_errors();
    if (_sx1262_mutex != NULL) {
        xSemaphoreGive(_sx1262_mutex);
    }
    ESP_LOGI(_sx1262_log_tag,
             "tx=%s length=%u status=0x%02X errors=0x%04X",
             sent ? "ok" : "failed",
             (unsigned)length,
             (unsigned)status,
             (unsigned)errors);
    return sent;
}

/**
 * @brief Read one completed SX1262 receive frame and restart reception.
 *
 * @param data Destination buffer to copy the received payload.
 * @param capacity Maximum capacity of the destination buffer.
 * @return size_t Number of bytes received, or 0 on error/no packet.
 */
size_t sx1262_receive(uint8_t *data, size_t capacity) {
    uint8_t irq_status[4U] = {0x12U, 0x00U, 0x00U, 0x00U};
    uint8_t status[4U] = {0x13U, 0x00U, 0x00U, 0x00U};
    uint8_t buffer[3U + MESH_PACKET_MAX_SIZE] = {0x1EU, 0x00U, 0x00U};
    uint8_t clear_irq[] = {0x02U, 0xFFU, 0xFFU};
    uint8_t receive[] = {0x82U, 0xFFU, 0xFFU, 0xFFU};
    uint8_t packet_status[5U] = {0x14U, 0x00U, 0x00U, 0x00U, 0x00U};
    size_t length;
    uint16_t irq;
    if (data == NULL || capacity == 0U) {
        return 0U;
    }
    if (_sx1262_mutex != NULL && xSemaphoreTake(_sx1262_mutex, 0) != pdTRUE) {
        return 0U;
    }
    _sx1262_receive_path();
    if (!_sx1262_transfer(irq_status, sizeof(irq_status))) {
        if (_sx1262_mutex != NULL) {
            xSemaphoreGive(_sx1262_mutex);
        }
        return 0U;
    }
    irq = ((uint16_t)irq_status[2] << 8U) | irq_status[3];
    if (irq != 0U) {
        _sx1262_command(clear_irq, sizeof(clear_irq));
    }
    if ((irq & 0x0002U) == 0U) {
        if ((irq & 0x0260U) != 0U) {
            _sx1262_command(receive, sizeof(receive));
        }
        if (_sx1262_mutex != NULL) {
            xSemaphoreGive(_sx1262_mutex);
        }
        return 0U;
    }
    if ((irq & 0x0040U) != 0U) {
        _sx1262_command(receive, sizeof(receive));
        if (_sx1262_mutex != NULL) {
            xSemaphoreGive(_sx1262_mutex);
        }
        return 0U;
    }
    if (!_sx1262_transfer(status, sizeof(status))) {
        _sx1262_command(receive, sizeof(receive));
        if (_sx1262_mutex != NULL) {
            xSemaphoreGive(_sx1262_mutex);
        }
        return 0U;
    }
    length = status[2];
    if (length == 0U || length > MESH_PACKET_MAX_SIZE || length > capacity) {
        _sx1262_command(receive, sizeof(receive));
        if (_sx1262_mutex != NULL) {
            xSemaphoreGive(_sx1262_mutex);
        }
        return 0U;
    }
    buffer[1] = status[3];
    if (!_sx1262_transfer(buffer, length + 3U)) {
        _sx1262_command(receive, sizeof(receive));
        if (_sx1262_mutex != NULL) {
            xSemaphoreGive(_sx1262_mutex);
        }
        return 0U;
    }
    memcpy(data, &buffer[3], length);
    if (_sx1262_transfer(packet_status, sizeof(packet_status))) {
        _last_rssi = packet_status[2] > 0U ? (int8_t)(-(int16_t)packet_status[2] / 2) : (int8_t)-60;
        _last_snr_x4 = (int8_t)packet_status[3];
    }
    _sx1262_command(receive, sizeof(receive));
    if (_sx1262_mutex != NULL) {
        xSemaphoreGive(_sx1262_mutex);
    }
    ESP_LOGI(_sx1262_log_tag,
             "rx=ok length=%u rssi=%d snr=%d",
             (unsigned)length,
             (int)_last_rssi,
             (int)(_last_snr_x4 / 4));
    return length;
}

/**
 * @brief Return the SNR times four of the last received LoRa frame.
 *
 * @param void No parameters.
 * @return int8_t SNR times four as a signed 8-bit integer.
 */
int8_t sx1262_last_snr_x4(void) {
    return _last_snr_x4;
}

/**
 * @brief Return the RSSI in dBm of the last received LoRa frame.
 *
 * @param void No parameters.
 * @return int8_t RSSI in dBm as a signed 8-bit integer.
 */
int8_t sx1262_last_rssi(void) {
    return _last_rssi;
}
![encryption-c-rp2350](https://raw.githubusercontent.com/mytechnotalent/meshcore-bare-esp32s3/main/meshcore-bare-esp32s3.png)

## FREE Reverse Engineering Self-Study Course [HERE](https://github.com/mytechnotalent/reverse-engineering)

<br>

# meshcore-bare-esp32s3

`meshcore-bare-esp32s3` is a bare-metal ESP32-S3 companion firmware for MeshCore: pure-C SX1262 LoRa radio driver, Ed25519 identity, AES-128 CTR / HMAC-SHA256 authenticated datagram encryption, multi-channel management, and NimBLE BLE companion for iPhone and Android.

GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3

The project uses PlatformIO only as the build and flashing tool. The firmware
source is pure C and uses ESP-IDF directly. It does not use Arduino, RadioLib,
NimBLE-Arduino, or other external firmware libraries.

## Architecture Highlights

- **Pure-C Architecture**: Direct ESP-IDF and FreeRTOS integration with zero C++ runtime dependencies.
- **Headless Design**: Streamlined for autonomous mesh operation, minimal RAM/ROM footprint, and maximum reliability.
- **SX1262 LoRa Driver**: Native SPI command driver with DIO1 event handling, fast FIFO transfers, and RF switch management (RXEN/TXEN).
- **MeshCore Protocol Engine**: Complete packet envelope encoding and decoding, path metadata tracking, flood routing, and payload dispatch.
- **Scalable Multi-Channel Support**: Supports up to 40 channels (`MESHCORE_MAX_CHANNELS 40`) with compact V3 NVS serialization (`'MSC3'`) to preserve flash endurance while maintaining backward compatibility with legacy formats.
- **Companion BLE Interface**: Native ESP-IDF NimBLE service providing standard MeshCore RX/TX characteristics, push notifications, and companion framing.
- **Enforced BLE Pairing**: Hardware-enforced Passkey Entry PIN `123456` with MITM protection, Display-Only I/O capabilities, and encrypted/authenticated GATT characteristics.
- **Persistent BLE Discoverability**: Active primary advertising and scan response packets with general discoverable flags and full local name, ensuring continuous visibility and effortless reconnects.
- **Cryptographic Security**: Persistent Ed25519 identity generation (pure-C implementation), signed adverts, AES-128 ECB encrypted datagrams, and truncated HMAC-SHA256 authentication.

## Requirements

- macOS, Linux, or Windows.
- Python 3.
- PlatformIO Core.
- An ESP32-S3 development board.
- An SX1262 LoRa radio module.
- A USB-C cable for flashing and serial monitoring.

PlatformIO is expected at:

```text
$HOME/.platformio/penv/bin/platformio
```

If PlatformIO is installed elsewhere or in your system `PATH`, replace that path
with `platformio`.

## Hardware Configuration

The default pin configuration for the ESP32-S3 and SX1262 LoRa radio:

| Signal | ESP32-S3 GPIO | Description |
| --- | ---: | --- |
| SX1262 NSS / CS | 7 | SPI Chip Select |
| SX1262 SCK | 5 | SPI Clock |
| SX1262 MISO | 3 | SPI Master In Slave Out |
| SX1262 MOSI | 6 | SPI Master Out Slave In |
| SX1262 DIO1 | 16 | Interrupt / RX Done / TX Done |
| SX1262 BUSY | 15 | Radio Busy Status Line |
| SX1262 RESET | 8 | Active-low Hardware Reset |
| SX1262 RXEN | 4 | RF Switch RX Enable |
| SX1262 TXEN | 9 | RF Switch TX Enable |

### Default Radio Profile

- Frequency: 910.525 MHz
- Bandwidth: 62.5 kHz
- Spreading Factor: 7
- Coding Rate: 4/5
- Transmit Power: 17 dBm

Pin and radio defaults are defined in [main/config.h](main/config.h). Board
and toolchain settings are located in [platformio.ini](platformio.ini).

## Build and Flash

### Build

From the repository root:

```bash
$HOME/.platformio/penv/bin/platformio run -e bare_meshcore
```

Clean and rebuild:

```bash
$HOME/.platformio/penv/bin/platformio run -e bare_meshcore -t clean
$HOME/.platformio/penv/bin/platformio run -e bare_meshcore
```

Firmware binaries are generated under:

```text
.pio/build/bare_meshcore/firmware.bin
```

### Flash

Connect the ESP32-S3 via USB and locate its serial port:

```bash
$HOME/.platformio/penv/bin/platformio device list
```

Upload the firmware:

```bash
$HOME/.platformio/penv/bin/platformio run -e bare_meshcore -t upload --upload-port /dev/cu.usbmodemXXXX
```

*(Replace `/dev/cu.usbmodemXXXX` with your actual device port).*

### Monitor

Open the serial monitor:

```bash
$HOME/.platformio/penv/bin/platformio device monitor -e bare_meshcore -b 115200 --port /dev/cu.usbmodemXXXX
```

## BLE Companion Interface & Security

The firmware implements the MeshCore companion service via ESP-IDF NimBLE:

- **Service UUID**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- **App to Device (RX)**: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` (Write with response / Write without response)
- **Device to App (TX)**: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` (Notify / Read)

### Secure Pairing

- **Passkey PIN**: `123456`
- **Security Flags**: Configured with `BLE_HS_IO_DISPLAY_ONLY`, MITM protection, and bonding enabled.
- **PIN Prompt**: Standard passkey pairing forces mobile operating systems (iOS and Android) to prompt the user for the static PIN `123456`.
- **Authenticated Characteristics**: GATT characteristics mandate link encryption and authentication (`BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_READ_AUTHEN | BLE_GATT_CHR_F_WRITE_ENC | BLE_GATT_CHR_F_WRITE_AUTHEN`).
- **Persistent Listing**: Fast advertising intervals combined with general discovery flags and device name in both primary advertising and scan response packets guarantee that the node remains discoverable in Bluetooth device scans after disconnection.

## Multi-Channel Scalability & Compact Storage

The node supports up to 40 distinct LoRa channels (`MESHCORE_MAX_CHANNELS 40`).

To avoid exhausting the ESP32-S3 NVS flash partition (20 KB total capacity):
- **Compact V3 Serialization (`'MSC3'`)**: Only configured active channels are written to NVS, shrinking storage requirements from ~2 KB down to under 260 bytes.
- **Backward Compatibility**: The state deserializer transparently imports legacy V1 and V2 state blobs, automatically converting them to the compact V3 format on the subsequent save.

## Source Layout

- [main/main.c](main/main.c): ESP-IDF application entry point, BLE service orchestration, and FreeRTOS tasks.
- [main/ble_companion.c](main/ble_companion.c): NimBLE GATT service, secure pairing callback, and advertising logic.
- [main/companion.c](main/companion.c): Companion frame parser, command handling, and serialization.
- [main/mesh_packet.c](main/mesh_packet.c): MeshCore packet envelope encoding and decoding.
- [main/sx1262.c](main/sx1262.c): SX1262 SPI command driver and LoRa TX/RX processing.
- [main/identity.c](main/identity.c): Portable Ed25519 identity generation and management.
- [main/advert.c](main/advert.c): Signed advert construction and verification.
- [main/mesh_crypto.c](main/mesh_crypto.c): AES-128 and HMAC-SHA256 cryptographic datagram processing.
- [main/datagram.c](main/datagram.c): Encrypted text datagram construction and parsing.
- [main/node_state.c](main/node_state.c): Bounded contacts, 40-channel state management, and compact V3 serialization.
- [main/storage.c](main/storage.c): ESP-IDF NVS persistence layer.
- [main/config.h](main/config.h): Hardware pin definitions, channel limits, and radio parameters.
- [crypto/ed25519](crypto/ed25519): Vendored pure-C Ed25519 implementation.
- [platformio.ini](platformio.ini): PlatformIO build environment configuration.
- [scripts/audit_blank_lines.py](scripts/audit_blank_lines.py): C code-standard audit tool.
- [scripts/audit_python_standard.py](scripts/audit_python_standard.py): Python tooling audit tool.

## Code Standards & Auditing

The repository enforces strict code quality standards:

1. **C Code Standard**:
   - All C source and header files must include comprehensive Doxygen documentation.
   - Function bodies must not contain blank lines.
   - Run audit:
     ```bash
     python3 scripts/audit_blank_lines.py
     ```

2. **Python Tooling Standard**:
   - Owned Python scripts in `scripts/` must follow PEP8, 79-column line limits, eight-line executable function limits, and NumPy docstring conventions.
   - Run audit:
     ```bash
     python3 scripts/audit_python_standard.py
     ```

<br>

## License

MIT — see [LICENSE](LICENSE).

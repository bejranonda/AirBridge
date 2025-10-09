# AirBridge v2.0 - Open Source WiFi-to-IR Bridge for Air Conditioners

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/version-2.0.0-blue.svg)](https://github.com/yourusername/AirBridge)
[![ESP8266](https://img.shields.io/badge/platform-ESP8266-green.svg)](https://www.esp8266.com/)

**AirBridge v2.0** is an open-source ESP8266-based WiFi-to-IR bridge that transforms any air conditioner into a network-controlled device.

Supporting 60+ IR protocols across 12 major brands (Daikin, Mitsubishi, LG, Samsung, Panasonic, and more), this DIY IoT hardware project provides universal remote control functionality with temperature monitoring and web-based configuration.

Originally developed as [NextAir](https://wenext.net/%e0%b8%95%e0%b8%b1%e0%b8%a7%e0%b8%ad%e0%b8%a2%e0%b9%88%e0%b8%b2%e0%b8%87%e0%b8%aa%e0%b8%b4%e0%b8%99%e0%b8%84%e0%b9%89%e0%b8%b2) in 2016-2019, this simplified open-source release focuses on reliable IR signal relay without proprietary energy-saving features.


<img width="864" height="566" alt="image" src="https://github.com/user-attachments/assets/a1cafae8-69be-475c-84c0-b61e5b12146e" />

*NextAir / AirBridge prototype during real-world operation (2017)*

---

## ⚠️ Important Notice

**AirBridge** is an ESP8266-based WiFi-to-IR bridge that allows you to control your air conditioner remotely. This is the **open-source edition** that provides universal IR signal relay functionality for 60+ AC protocols.

This is **AirBridge v2.0** - the open-source edition.

**What's Included:**
- ✅ IR signal detection and relay
- ✅ WiFi connectivity and web configuration
- ✅ Temperature sensor display (optional)
- ✅ Support for 12+ AC brands (60+ protocols)

**What's NOT Included (Available in Commercial Version):**
- ❌ Smart energy-saving algorithms (certified 20-50% savings)
- ❌ Baseline learning and adaptation
- ❌ Automatic power cutting
- ❌ Remote monitoring (MQTT)
- ❌ Predictive maintenance
- ❌ Professional support

**Commercial Version:** For the full-featured version with patented energy-saving technology, visit **[https://wenext.net/](https://wenext.net/)**

<img width="778" height="543" alt="image" src="https://github.com/user-attachments/assets/a569046a-1932-41be-be0c-db4368434759" />

*3D rendering of the third-generation design for NextAir / AirBridge (2019)*


---

## What It Does

AirBridge v2.0 acts as a WiFi-to-IR bridge for your air conditioner:

1. **Receives IR signals** from your existing AC remote
2. **Relays them** to your AC unit via WiFi-controlled IR transmitter
3. **Displays temperature** from optional DS18B20 sensors
4. **Web interface** for configuration and status

**Simple. No intelligence. No automatic control.**

---

## Features

### Core Functionality

- **Universal IR Protocol Support** - Works with 12+ major AC brands:
  - Carrier, Daikin, Haier, Hitachi, LG, Mitsubishi
  - Panasonic, Saijo Denki, Samsung, Sharp, Toshiba, and more
  - 60+ remote control protocols supported

- **WiFi Configuration** - Easy setup via web portal
  - Connect to "NextAir_XXXX" network
  - Configure your home WiFi
  - Access device status via web browser

- **Temperature Monitoring** (Optional)
  - Supports up to 2x DS18B20 sensors
  - Display only - no automatic control
  - Shows room and coil temperatures

- **Status LEDs**
  - Green: IR signal activity
  - Yellow: System status/warnings


<img width="563" height="376" alt="image" src="https://github.com/user-attachments/assets/df2faae3-e536-4b59-8d6b-dee98623ace6" />

*The hardware (2019) features two temperature sensors, an infrared receiver, and a transmitter : [NextAir](https://wenext.net/%e0%b8%95%e0%b8%b1%e0%b8%a7%e0%b8%ad%e0%b8%a2%e0%b9%88%e0%b8%b2%e0%b8%87%e0%b8%aa%e0%b8%b4%e0%b8%99%e0%b8%84%e0%b9%89%e0%b8%b2)*

---

## Hardware Requirements

### Required Components

- **ESP8266 Microcontroller** (NodeMCU, Wemos D1 Mini, or similar)
- **IR Receiver Module** (38kHz, e.g., TSOP4838)
- **IR Transmitter LED** (38kHz, e.g., TSAL6200)
- **2x Status LEDs** (Green and Yellow)
- **Power Supply** (5V USB or AC adapter)

### Optional Components

- **2x DS18B20 Temperature Sensors** (for temperature display)
- **4.7kΩ Resistor** (pull-up for DS18B20)
- **Resistors** for LEDs (220Ω recommended)

### Pin Configuration

```
GPIO2  → DS18B20 Sensors (1-Wire, optional)
GPIO4  → IR Receiver
GPIO5  → IR Transmitter LED
GPIO12 → Yellow Status LED
GPIO14 → Green Status LED
```


<img width="537" height="463" alt="image" src="https://github.com/user-attachments/assets/4c128127-ade0-424e-b9c8-7d0a6bc717f2" />

*Flashing new firmware on the second model of NextAir / AirBridge*

---

## Installation

### 1. Hardware Assembly

1. Connect IR receiver to GPIO4
2. Connect IR transmitter LED to GPIO5 (with current-limiting resistor ~100-220Ω)
3. Connect status LEDs to GPIO12 (yellow) and GPIO14 (green)
4. *Optional:* Connect DS18B20 sensors to GPIO2 with 4.7kΩ pull-up resistor
5. Power via USB (5V)


<img width="342" height="490" alt="image" src="https://github.com/user-attachments/assets/c9e606c2-fd80-436e-8658-8c017e94b52c" />

*Assessment of the hardware and saving algorithm during operation in 2017*

### 2. Software Upload

Using **Arduino IDE**:

```bash
1. Install ESP8266 board support:
   - File → Preferences → Additional Board URLs
   - Add: http://arduino.esp8266.com/stable/package_esp8266com_index.json

2. Install required libraries:
   - IRremoteESP8266
   - DallasTemperature
   - OneWire
   - ESP8266WiFi (included)
   - ESP8266WebServer (included)

3. Open airbridge/airbridge.ino

4. Select Board: Tools → Board → ESP8266 → NodeMCU 1.0 (or your board)
   Set CPU Frequency: 160MHz

5. Upload to ESP8266
```

### 3. Initial Configuration

```
1. Power on device
   - Yellow & Green LEDs will flash for 3 seconds

2. Connect to WiFi Access Point:
   - SSID: AirBridge_XXXXXX (where XXXXXX is device ID)
   - Default IP: 192.168.4.1

3. Open browser to: http://192.168.4.1

4. Configure:
   - Your home WiFi SSID and password
   - Save settings

5. Device will restart and connect to your WiFi
```

<img width="1197" height="773" alt="image" src="https://github.com/user-attachments/assets/b67cfa8c-5417-431e-af0d-8133772f6cf1" />

*The first NextAir prototype used a different microprocessor — a Raspberry Pi.*


---

## Usage

### Basic Operation

1. **Point your AC remote at AirBridge's IR receiver**
2. Press any button on your remote
3. AirBridge receives the signal and forwards it to your AC
4. Green LED blinks to confirm signal received
5. **That's it!** No automatic control or intelligence

### Web Interface

Access the web interface at `http://airbridge.local` or the device's IP address:

- View current temperature (if sensors connected)
- Check system status
- View IR protocol detected
- Reset configuration

### Temperature Display

If DS18B20 sensors are connected:
- Room temperature displayed
- Coil temperature displayed
- **Display only - no automatic control**

---

## Supported Air Conditioner Brands

NextAir includes IR protocol definitions for 60+ remote controls:

| Brand | Protocols | Notes |
|-------|-----------|-------|
| **Carrier** | 3 variants | 96, 144, 168-bit |
| **Daikin** | 3 variants | 64, 216, 280-bit with checksum |
| **Haier** | 1 variant | 112-bit |
| **Hitachi** | 2 variants | 104, 264-bit |
| **LG** | 1 variant | 28-bit compact |
| **Mitsubishi** | 5 variants | 48, 88, 112, 136, 288-bit |
| **Panasonic** | 2 variants | 128, 216-bit |
| **Saijo Denki** | 2 variants | 64, 128-bit |
| **Samsung** | 3 variants | 112, 168-bit |
| **Sharp** | 2 variants | 96, 104-bit |
| **Toshiba** | 1 variant | 112-bit |

Each protocol includes:
- Power on/off control
- Mode selection (Cool, Dry, Fan, Auto)
- Temperature range (16-32°C typically)
- Fan speed control
- Vane/swing position
- Automatic checksum calculation

---

## Configuration

### Serial Monitor

Connect via serial monitor at **115200 baud** for debug output.

Enable verbose logging in `config.h`:
```cpp
const bool show_data = true;  // Enable detailed serial logs
```

### Temperature Sensor Setup

Place sensors carefully for accurate readings:

**Room Sensor:**
- Representative location in room
- Avoid direct sunlight or drafts
- Same height as AC air intake

**Coil Sensor (Optional):**
- Attach to AC evaporator coil
- Good thermal contact (use thermal paste)
- Insulate from ambient air

---

## Troubleshooting

### Common Issues

**LED Patterns:**
- Green blinks 2x = IR signal received
- Yellow blinks = System warning
- Both flash 3x = WiFi connected
- No LEDs = Power issue

**IR Not Working:**
1. Check IR LED has current-limiting resistor (100-220Ω)
2. Verify IR LED is positioned with line-of-sight to AC
3. Check detected protocol in serial monitor
4. Try different IR LED (some ACs need specific wavelength)

**WiFi Won't Connect:**
1. Verify SSID and password are correct
2. Check WiFi signal strength
3. Reset device and reconfigure
4. Access point auto-closes after 20 loops (~2 minutes)

**Temperature Sensors Not Reading:**
1. Check 4.7kΩ pull-up resistor is connected
2. Verify DS18B20 sensors are genuine (many fakes)
3. Test sensors with multimeter (~10kΩ at 25°C)
4. Check connections to GPIO2

---

## Technical Specifications

### Software
- **Platform:** ESP8266 Arduino
- **Language:** C++
- **IDE:** Arduino IDE 1.8.x+
- **Framework:** Arduino Core for ESP8266

### Memory
- **Flash:** ~400KB program size
- **RAM:** ~20KB used (plenty of free space)
- **EEPROM:** WiFi credentials only

### Performance
- **Loop Time:** ~5 seconds
- **IR Detection:** < 100ms
- **WiFi Reconnect:** ~5-10 seconds

---

## Development

### Building from Source

```bash
git clone https://github.com/yourusername/AirBridge.git
cd AirBridge
# Open airbridge/airbridge.ino in Arduino IDE
# Select board and upload
```

### Project Structure

```
AirBridge/
├── airbridge/
│   ├── airbridge.ino            # Main entry point
│   ├── operating_states.ino     # Simplified state machine
│   ├── web_interface.ino        # WiFi and web interface
│   ├── system_maintenance.ino   # System maintenance
│   ├── datetime_utils.ino       # Time utilities
│   ├── test_config.ino          # Test configuration
│   ├── config.h                 # Configuration constants
│   ├── memory_manager.h/.cpp    # EEPROM management
│   ├── ir_remote.h/.cpp         # IR handling
│   ├── ir_protocols.h           # IR protocol definitions
│   ├── led_controller.h         # LED control
│   └── timer.h                  # Non-blocking timers
├── libraries/                    # Required libraries
├── examples/                     # Example code
├── README.md                     # This file
├── CLAUDE.md                     # Claude Code instructions
├── LICENSE                       # MIT License
└── CHANGELOG.md                  # Version history
```

---

## Contributing

Contributions are welcome! Please follow these guidelines:

1. **Fork the repository**
2. **Create a feature branch** (`git checkout -b feature/amazing-feature`)
3. **Commit your changes** (`git commit -m 'Add amazing feature'`)
4. **Push to the branch** (`git push origin feature/amazing-feature`)
5. **Open a Pull Request**

### Code Style
- Use Arduino-style naming conventions
- Comment complex logic
- Test on real hardware before submitting
- Update documentation if needed

---

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2016-2019 Werapol Bejranonda and Pathawee Somsak
Copyright (c) 2025 Werapol Bejranonda (Open-Source Release)

Permission is hereby granted, free of charge...
```

---

## Contributors & Acknowledgments

### Original Development (2016-2019)
**Co-developers:**
- **Werapol Bejranonda** - Lead Developer
- **Pathawee Somsak** - Co-developer

### Open-Source Release (2025)
**Maintainer:**
- **Werapol Bejranonda** - Reorganized for open-source release

### Funding
- Wenext.net (Smart Energy Technology Co., Ltd.)
- Dr.Siwanand Misara

### Third-Party Libraries & Credits

This project builds upon excellent open-source libraries. We are grateful to:

**1. IRremoteESP8266** (LGPL v2.1)
- **Original Authors:** Ken Shirriff (Arduino-IRremote)
- **ESP8266 Port:** Mark Szabo, Sebastien Warin
- **Current Maintainer:** David Conran (crankyoldgit)
- **Repository:** [github.com/crankyoldgit/IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)
- **Description:** Infrared remote library supporting 60+ AC protocols

**2. DallasTemperature** (LGPL v2.1)
- **Authors:** Miles Burton, Tim Newsome, Guil Barros, Rob Tillaart
- **Repository:** [github.com/milesburton/Arduino-Temperature-Control-Library](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- **Description:** Temperature sensor library for DS18B20 and compatible sensors

**3. OneWire** (MIT-like)
- **Authors:** Jim Studt, Tom Pollard, Robin James, Paul Stoffregen
- **Repository:** [github.com/PaulStoffregen/OneWire](https://github.com/PaulStoffregen/OneWire)
- **Description:** 1-Wire communication protocol implementation

**4. PubSubClient** (MIT)
- **Author:** Nicholas O'Leary
- **Repository:** [github.com/knolleary/pubsubclient](https://github.com/knolleary/pubsubclient)
- **Description:** MQTT client library for Arduino

**5. Adafruit MQTT Library** (MIT)
- **Author:** Adafruit Industries
- **Repository:** [github.com/adafruit/Adafruit_MQTT_Library](https://github.com/adafruit/Adafruit_MQTT_Library)
- **Description:** MQTT client with Adafruit IO support

**6. ESP8266Ping** (LGPL v2.1)
- **Author:** Daniele Colanardi
- **Repository:** [github.com/dancol90/ESP8266Ping](https://github.com/dancol90/ESP8266Ping)
- **Description:** ICMP ping library for ESP8266

**7. ESP8266 Arduino Core**
- **Authors:** ESP8266 Community
- **Repository:** [github.com/esp8266/Arduino](https://github.com/esp8266/Arduino)
- **Description:** Arduino core for ESP8266 WiFi chip

### Special Thanks
- ESP8266 community for continuous support
- Arduino community for the development ecosystem
- All open-source contributors who made this project possible
- All future contributors to AirBridge

See [LICENSE](LICENSE) file for complete third-party license information.

---

## Disclaimer

- This device modifies AC operation behavior
- Ensure compatibility with your AC unit
- Improper use may void AC warranty
- The developers assume no liability for equipment damage
- **No energy savings guaranteed in this open-source version**

---

## Commercial Version

Want automatic energy savings? Check out the **Commercial Edition** (formerly NextAir v1.0):

🌐 **[https://wenext.net/](https://wenext.net/)**

**Features in Commercial v1.0:**
- ✅ Patented smart energy-saving algorithms
- ✅ 20-50% energy reduction
- ✅ Baseline learning and adaptation
- ✅ Remote monitoring via MQTT
- ✅ Email reports and analytics
- ✅ Professional support
- ✅ Continuous updates

**Open-source v2.0 vs Commercial v1.0:**

| Feature | v2.0 (Free) | v1.0 (Commercial) |
|---------|-------------|-------------------|
| IR Relay | ✅ | ✅ |
| Temperature Display | ✅ | ✅ |
| WiFi Control | ✅ | ✅ |
| **Energy Saving** | ❌ | ✅ **20-50%** |
| **Smart Algorithms** | ❌ | ✅ Patented |
| **Remote Monitoring** | ❌ | ✅ MQTT |
| **Support** | Community | ✅ Professional |

---

## Contact

- **Issues:** [GitHub Issues](https://github.com/yourusername/AirBridge/issues)
- **Discussions:** [GitHub Discussions](https://github.com/yourusername/AirBridge/discussions)
- **Commercial Version:** [https://wenext.net/](https://wenext.net/)

---

## Version History

- **v2.0.0** (2025-01-XX) - Open-source release as "AirBridge"
  - Reorganized for open-source by Werapol Bejranonda
  - Removed proprietary energy-saving features
  - Simplified to basic IR relay functionality
  - MIT License

- **v1.0.0** (2016-2019) - Original development as "NextAir"
  - Co-developed by Werapol Bejranonda and Pathawee Somsak
  - Commercial release with smart energy-saving features
  - Available at wenext.net with professional support

---

**Made with ❤️ by the AirBridge community**

**For energy savings, visit the commercial version at [wenext.net](https://wenext.net/)**

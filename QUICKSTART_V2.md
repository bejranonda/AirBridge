# AirBridge v2.0 - Quick Start Guide

## What is AirBridge v2.0?

**AirBridge v2.0** is an open-source WiFi-enabled IR relay for air conditioners.

**Simple Function:** Your AC remote → AirBridge → Your AC unit

**What It Does:**
- ✅ Receives IR signals from your remote
- ✅ Forwards them to your AC via WiFi
- ✅ Displays temperature (optional sensors)
- ✅ Web configuration interface

**What It DOESN'T Do:**
- ❌ NO energy savings (that's v1.0 commercial)
- ❌ NO automatic control
- ❌ NO intelligence or learning

---

## 5-Minute Hardware Setup

### You Need:
1. ESP8266 board (NodeMCU or Wemos D1 Mini)
2. IR receiver module (TSOP4838)
3. IR transmitter LED
4. 2x LEDs (green, yellow) + resistors
5. USB cable

### Connections:
```
ESP8266          Component
-------          ---------
GPIO4     →      IR Receiver
GPIO5     →      IR Transmitter (+ resistor 100-220Ω)
GPIO12    →      Yellow LED (+ resistor)
GPIO14    →      Green LED (+ resistor)
GPIO2     →      DS18B20 sensors (optional, + 4.7kΩ pullup)
USB       →      5V Power
```

---

## 10-Minute Software Setup

### 1. Install Arduino IDE
Download from: https://www.arduino.cc/en/software

### 2. Add ESP8266 Support
```
Arduino IDE → File → Preferences
Additional Board URLs:
http://arduino.esp8266.com/stable/package_esp8266com_index.json

Then: Tools → Board → Boards Manager → Search "ESP8266" → Install
```

### 3. Install Libraries
```
Arduino IDE → Sketch → Include Library → Manage Libraries
Install:
- IRremoteESP8266
- DallasTemperature
- OneWire
```

### 4. Upload Code
```
1. Open: s_air_2019/s_air_2019.ino
2. Tools → Board → ESP8266 Boards → NodeMCU 1.0
3. Tools → Port → (select your USB port)
4. Click Upload button
5. Wait for "Done uploading"
```

---

## First Use

### 1. Power On
- Plug in USB
- Yellow + Green LEDs flash for 3 seconds

### 2. Connect to WiFi
- Look for WiFi network: **AirBridge_XXXXXX**
- Connect (no password needed)
- Open browser to: **http://192.168.4.1**

### 3. Configure
- Enter your home WiFi name
- Enter WiFi password
- Click "Connect"
- Device restarts and connects to your WiFi

### 4. Test
- Point your AC remote at AirBridge
- Press any button
- Green LED blinks = signal received!
- AC should respond

---

## Troubleshooting

### LEDs Don't Flash
- Check USB power connection
- Verify ESP8266 is powered
- Try different USB cable/port

### Can't See WiFi Network
- Wait 30 seconds after power on
- Network auto-closes after 2 minutes
- Reset device (press reset button)

### IR Not Working
- Check IR LED resistor (100-220Ω)
- Verify IR LED positioning (line-of-sight to AC)
- Try different AC remote button
- Check serial monitor (115200 baud) for IR detection

### WiFi Won't Connect
- Double-check SSID/password spelling
- Ensure 2.4GHz WiFi (ESP8266 doesn't support 5GHz)
- Check WiFi signal strength
- Try simpler password (no special characters)

---

## What's Next?

### Optional: Add Temperature Sensors
- Connect 2x DS18B20 to GPIO2
- Add 4.7kΩ pull-up resistor
- View temperatures in web interface

### Optional: Serial Monitor
```
Arduino IDE → Tools → Serial Monitor
Baud rate: 115200
See debug output and IR detection
```

### Learn More
- Full documentation: README.md
- Hardware details: README.md → Hardware Requirements
- Supported AC brands: README.md → Supported Brands

---

## Want Energy Savings?

v2.0 is a basic IR relay only.

For **automatic energy savings (20-50%)**, check out the commercial version:

**🌐 https://wenext.net/**

**Commercial v1.0 includes:**
- Patented smart algorithms
- Baseline learning
- Automatic power cutting
- Remote monitoring
- Professional support

---

## Support

- **Documentation:** README.md
- **Issues:** GitHub Issues
- **Questions:** GitHub Discussions
- **Commercial:** https://wenext.net/

---

**License:** MIT (Open Source)
**Version:** 2.0.0
**Hardware:** ESP8266
**Supported ACs:** 60+ protocols (12 brands)

**Get Started in 15 Minutes! 🚀**

# Migration from NextAir v1.0 (Commercial) to AirBridge v2.0 (Open Source)

## Overview

This document details the conversion of AirBridge from the commercial version (NextAir v1.0) to the open-source non-commercial edition (AirBridge v2.0).

---

## Changes Summary

### Removed Proprietary Features

The following patented and commercial features have been removed:

1. **Smart Cutting Algorithm** (States 5-7)
   - Intelligent power cutting based on room temperature
   - Dynamic duty cycle calculation
   - Adaptive percentage-based cutting table

2. **Baseline Learning** (State 3)
   - Automatic AC behavior learning
   - Compressor cycle analysis
   - Duty cycle recording and optimization

3. **Adaptive Algorithms**
   - Self-adjusting compressor detection thresholds
   - Heavy load condition detection
   - Environmental factor adaptation
   - Dirty coil detection

4. **Remote Monitoring** (MQTT Integration)
   - Cloud-based performance tracking
   - Remote temperature monitoring
   - Baseline data transmission
   - Energy saving reports

5. **Fail-Safe Intelligence**
   - State 9 emergency mode with smart fallback
   - Adaptive sensor error recovery
   - Intelligent state transitions

### Removed Confidential Information

1. **MQTT Credentials**
   - Server: dev.wenext.net
   - Username: wetest1
   - Password: Ne.xt2t

2. **Email Credentials**
   - Gmail account credentials (Base64 encoded)
   - eval@easy-green.de contact

3. **Encryption Keys**
   - s_air_key: "Child$Sing8Pae+X"
   - XXTEA encryption keys

4. **Company Information**
   - WeNext.net branding
   - Easy-Green references

---

## v2.0 Architecture

### Simplified State Machine

**v1.0 States (10 states):**
- State 0: Web Install
- State 1: Standby
- State 2: Init Coil (compressor detection)
- State 3: Baseline Learning ❌ REMOVED
- State 4: Pre-Cut ❌ REMOVED
- State 5: Smart Cut ❌ REMOVED
- State 6: Cool ❌ REMOVED
- State 7: Confirm Cool ❌ REMOVED
- State 8: Power Off Detection
- State 9: Emergency Mode ❌ SIMPLIFIED

**v2.0 States (3 states):**
- State 0: Web Configuration
- State 1: Standby (AC Off)
- State 2: IR Passthrough (AC On)

### Core Functionality (v2.0)

The open-source version provides **basic IR relay functionality**:

1. **IR Signal Detection**
   - Receives IR signals from your AC remote
   - Decodes 12+ AC brands (60+ protocols)
   - Tracks AC state (power, mode, temperature, fan, vane)

2. **IR Signal Relay**
   - Forwards commands to AC unit
   - No modification or intelligence applied
   - Simple passthrough operation

3. **WiFi Configuration**
   - Web-based setup portal
   - SSID and password configuration
   - Device status display

4. **Temperature Monitoring** (Optional)
   - Reads DS18B20 sensors (if connected)
   - Displays current temperatures
   - No analysis or decision-making

5. **Status LEDs**
   - Green: IR signal received
   - Yellow: System status/warnings

### What's Removed

❌ **No energy saving features**
❌ **No automatic power cutting**
❌ **No baseline learning**
❌ **No remote monitoring (MQTT)**
❌ **No adaptive algorithms**
❌ **No email reports**
❌ **No compressor detection logic**
❌ **No intelligent decision making**

---

## File Changes

### Files to Remove Entirely

```
s_air_2019/Gsender.h              # Email sending
s_air_2019/Gsender.cpp            # Email implementation
s_air_2019/S_air_log_data_email.ino  # Energy logging and email
s_air_2019/S_air_mqtt.ino         # MQTT integration
examples/refactored_mqtt_example.ino  # MQTT refactoring examples
```

### Files to Heavily Modify

```
s_air_2019/S_air_operating.ino    # Remove states 3-7, simplify state 2 & 9
s_air_2019/SAIRconfig.h           # Remove all proprietary constants
s_air_2019/SAIRmemory.h           # Remove baseline and logging structures
s_air_2019/SAIRmemory.cpp         # Remove baseline and logging code
s_air_2019/S_air_web_AP.ino       # Remove email fields, encryption keys
s_air_2019/S_air_sys_mainten.ino  # Simplify system maintenance
s_air_2019/s_air_2019.ino         # Remove MQTT, email, logging references
```

### Files to Update

```
README.md                         # Complete rewrite for v2.0
CLAUDE.md                         # Update for simplified architecture
SAIRconfig.h                      # Remove proprietary constants
```

### New Files to Create

```
LICENSE                           # Open source license (MIT recommended)
CHANGELOG.md                      # v1.0 → v2.0 changes
CONTRIBUTING.md                   # Contribution guidelines
.gitignore                        # Git ignore patterns
```

---

## Configuration Changes

### SAIRconfig.h - Constants to Remove

```cpp
// REMOVE: Smart cutting configuration
const uint8_t mem_percent_to_cut_table_size
const float mem_percent_to_cut_table[]
const float diff_btw_tr_and_trset_to_start_cut
const float tr_less_than_trset_to_cut
const byte min_comp_on_loop_to_start_cut

// REMOVE: Baseline recording
const uint16_t baseline_finish_state_cnt_normal
const uint16_t baseline_comp_on_loop
const byte baseline_cycle_compressor

// REMOVE: Compressor detection intelligence
const float default_min_tc_to_detect_compressor_on
const float default_min_tc_slope_considering_compressor_on
const float default_tdiff_tr_tc_compressor

// REMOVE: Adaptive algorithms
const uint16_t considering_change_detect_compressor_on_after_loop
const float minimum_gap_max_min_tc_considering_revision

// REMOVE: Heavy duty detection
const uint8_t num_of_heavy_to_change_Trset
const float heavy_duty_tr_slope_boundary

// REMOVE: MQTT credentials
const char* mqtt_Server
const char* mqtt_Username
const char* mqtt_Password

// REMOVE: Email and encryption
const char s_air_key[]
const char ev_email[]
const char* EMAILBASE64_LOGIN
const char* EMAILBASE64_PASSWORD
```

### SAIRmemory.h - Structures to Remove

```cpp
// REMOVE: Baseline learning data
struct bl_t {
  bool finish;
  uint8_t trset[mem_bl_length];
  uint16_t compressor_off_cnt, compressor_on_cnt, total_cycle;
  float trstart, duty[mem_bl_length], av_tr, lowest_tc, best_tc_slope;
};

// REMOVE: Energy logging
struct raw_data_t
struct hr_log_full_t
struct mo_log_full_t
struct mo_log_lite_t

// REMOVE: Baseline and logging variables
bl_t bl_log;
bool sent_baseline_already;
uint8_t compressor_log, coff_log;
float trstart, best_tc_slope;
raw_data_t raw_data_log[];
// ... all logging arrays
```

---

## Code Migration Strategy

### Phase 1: Remove External Dependencies
1. Delete MQTT files completely
2. Delete email/Gsender files completely
3. Delete logging file completely
4. Remove MQTT includes from main file

### Phase 2: Simplify State Machine
1. Keep only States 0, 1, 2
2. Remove state_3() through state_7() functions
3. Simplify state_8() and state_9()
4. Update state_function() switch statement

### Phase 3: Remove Proprietary Constants
1. Edit SAIRconfig.h - remove all smart cutting constants
2. Edit SAIRconfig.h - remove baseline constants
3. Edit SAIRconfig.h - remove adaptive algorithm constants
4. Edit SAIRconfig.h - remove all credentials

### Phase 4: Simplify Memory Management
1. Remove baseline structures from SAIRmemory.h
2. Remove logging structures from SAIRmemory.h
3. Remove email storage from EEPROM layout
4. Keep only WiFi credentials in EEPROM

### Phase 5: Update Web Interface
1. Remove email input field
2. Remove encryption key references
3. Simplify configuration page
4. Update status display (no energy savings)

### Phase 6: Clean Main Operating File
1. Remove compressor detection intelligence
2. Remove temperature slope analysis
3. Keep only basic temperature reading
4. Simplify IR signal handling

---

## Testing Checklist

After migration to v2.0:

### Basic Functionality
- [ ] Device boots without errors
- [ ] WiFi configuration works
- [ ] Web interface loads correctly
- [ ] IR signals detected from remote
- [ ] IR signals forwarded to AC
- [ ] AC responds to remote commands
- [ ] Temperature sensors read (if connected)
- [ ] LEDs function correctly

### Removed Features Verified
- [ ] No MQTT connection attempts
- [ ] No email sending attempts
- [ ] No baseline learning active
- [ ] No smart cutting behavior
- [ ] No compressor detection logic
- [ ] No adaptive algorithms running
- [ ] No energy saving calculations

### Code Quality
- [ ] Compiles without warnings
- [ ] No hardcoded credentials remain
- [ ] No references to wenext.net
- [ ] No references to easy-green.de
- [ ] No encryption keys present
- [ ] No proprietary algorithms present

---

## License Recommendation

**MIT License** is recommended for maximum compatibility and adoption:

```
MIT License

Copyright (c) 2025 AirBridge Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

**Note:** Original commercial version (v1.0) remains proprietary and is available at https://wenext.net/

---

## Version Comparison

| Feature | v1.0 (Commercial) | v2.0 (Open Source) |
|---------|-------------------|---------------------|
| **IR Signal Detection** | ✅ Yes | ✅ Yes |
| **IR Signal Transmission** | ✅ Yes | ✅ Yes |
| **Temperature Sensors** | ✅ Yes | ✅ Yes (display only) |
| **WiFi Configuration** | ✅ Yes | ✅ Yes |
| **Smart Energy Saving** | ✅ Yes (20-50%) | ❌ No |
| **Baseline Learning** | ✅ Yes | ❌ No |
| **Adaptive Algorithms** | ✅ Yes | ❌ No |
| **Remote Monitoring (MQTT)** | ✅ Yes | ❌ No |
| **Email Reports** | ✅ Yes | ❌ No |
| **Compressor Detection** | ✅ Yes | ❌ No |
| **Smart Cutting** | ✅ Yes | ❌ No |
| **Fail-Safe Intelligence** | ✅ Yes | ❌ Simplified |
| **State Machine** | 10 states | 3 states |
| **Lines of Code** | ~8000 | ~2000 (estimated) |
| **RAM Usage** | ~50KB | ~20KB (estimated) |
| **Complexity** | High | Low |

---

## Commercial Version Notice

The original commercial version (v1.0) with full smart energy-saving features remains available at:

**https://wenext.net/**

Features exclusive to v1.0 commercial version:
- Patented smart cutting algorithm
- Baseline learning and adaptation
- 20-50% energy savings
- Remote monitoring and analytics
- Professional support
- Continuous updates

---

**Document Version:** 1.0
**Date:** 2025-10-07
**Purpose:** Guide for converting NextAir v1.0 → AirBridge v2.0 non-commercial edition

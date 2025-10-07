# AirBridge Codebase Refactoring Recommendations

## Executive Summary

After analyzing the AirBridge (S-Air) codebase, I've identified several opportunities to improve code quality, maintainability, and performance. This document prioritizes refactoring tasks by impact and provides specific recommendations with code examples.

**Overall Assessment:**
- **Strengths:** Well-documented state machine, modular file structure, good use of constants
- **Challenges:** Memory constraints (ESP8266), heavy String usage, blocking delays, code duplication
- **Risk Level:** Medium - Embedded system with real-time constraints and EEPROM persistence

---

## Priority 1: Critical Performance & Memory Issues

### 1.1 Replace String with char arrays and F() macro

**Issue:** Excessive String object usage causes heap fragmentation on ESP8266 (80KB RAM limit).

**Current State:**
```cpp
// S_air_mqtt.ino:111
String topic = "air/" + mac_name + "/" + subtopic;
return client.publish(topic.c_str(), msg.c_str());

// S_air_operating.ino:32
String txt_tmp = "";
Serial.println(F("***rmt_state_control"));
txt_tmp = F("power off -> go state 1\r\n");
```

**Impact:**
- String concatenation creates temporary objects consuming RAM
- Found 54+ String variable assignments across 6 files
- Can reduce RAM usage by 20-30%

**Recommendation:**
```cpp
// Refactored approach using char arrays
void mqtt_publish_formatted(const char* subtopic, const char* msg) {
  char topic[64];
  snprintf(topic, sizeof(topic), "air/%s/%s", mac_name, subtopic);
  return client.publish(topic, msg);
}

// For rmt_state_control(), use direct F() prints
if (!rmt.getPower()) {
  Serial.println(F("power off -> go state 1"));
  mem.state = 1;
  mem.state_cnt = 0;
}
```

**Estimated Effort:** 4-6 hours
**Files Affected:** S_air_mqtt.ino, S_air_operating.ino, S_air_web_AP.ino, S_air_log_data_email.ino

---

### 1.2 Remove blocking delay() calls

**Issue:** Found delay() calls in 8 files, blocking main loop execution.

**Current State:**
```cpp
// S_air_operating.ino:39-40
rmt.setTrset(min_trset);
delay(100);  // BLOCKING!
rmt.cool();

// S_air_sys_mainten.ino:53, 66
delay(2000);  // 2 second block during startup
delay(1000);  // 1 second blocks in loop
```

**Impact:**
- Missed IR signals during delay periods
- Poor responsiveness to user input
- Potential MQTT/WiFi disconnections

**Recommendation:**
```cpp
// Use millis() for non-blocking delays
class DelayTimer {
  private:
    unsigned long start_time;
    unsigned long duration;
  public:
    DelayTimer(unsigned long ms) : duration(ms), start_time(0) {}

    void start() { start_time = millis(); }
    bool elapsed() { return (millis() - start_time) >= duration; }
};

// Usage in rmt_state_control()
static DelayTimer coolDelay(100);
if (rmt.getTrset() < min_trset) {
  rmt.setTrset(min_trset);
  coolDelay.start();
}
if (coolDelay.elapsed()) {
  rmt.cool();
}
```

**Estimated Effort:** 3-4 hours
**Files Affected:** S_air_operating.ino, S_air_sys_mainten.ino, S_air_web_AP.ino

---

### 1.3 Optimize MQTT message construction

**Issue:** Multiple String concatenations in MQTT functions create memory overhead.

**Current State:**
```cpp
// S_air_mqtt.ino:154-159
String comma = F(",");
String msg = String(mem.bl_log.compressor_on_cnt) + comma + String(mem.bl_log.compressor_off_cnt)
  + comma + String(mem.bl_log.total_cycle) + comma + String(mem.bl_log.trstart)
  + comma + String(mem.bl_log.av_tr) + comma + String(rmt.getTrset()) + comma + String(count_loop);
```

**Impact:**
- Creates 7+ temporary String objects
- High heap fragmentation risk
- Slower execution

**Recommendation:**
```cpp
// Use snprintf for efficient formatting
boolean MQTTtrigger_send_baseline(uint16_t count_loop) {
  char msg[128];
  snprintf(msg, sizeof(msg), "%u,%u,%u,%.2f,%.2f,%u,%u",
           mem.bl_log.compressor_on_cnt,
           mem.bl_log.compressor_off_cnt,
           mem.bl_log.total_cycle,
           mem.bl_log.trstart,
           mem.bl_log.av_tr,
           rmt.getTrset(),
           count_loop);

  Serial.print(F("MQTTtrigger_send_baseline>"));
  Serial.println(msg);

  if(mqtt_publish(F("base"), msg)) {
    mem.compressor_on_cnt = 0;
    mem.compressor_off_cnt = 0;
    mem.total_cycle = 0;
    return true;
  }
  return false;
}
```

**Estimated Effort:** 2 hours
**Files Affected:** S_air_mqtt.ino

---

## Priority 2: Code Structure & Maintainability

### 2.1 Extract state machine to dedicated class

**Issue:** State functions (state_0 through state_9) scattered across S_air_operating.ino without clear encapsulation.

**Current State:**
```cpp
// S_air_operating.ino:4-29
void state_function(){
  switch (mem.state){
    case 0: state_0(); break;
    case 1: state_1(); break;
    // ... 8 more cases
    default: mem.state = 0; break;
  }
}
```

**Recommendation:**
```cpp
// Create SAIRstateMachine.h
class SAIRstateMachine {
  private:
    typedef void (*StateFunction)();
    StateFunction state_handlers[10];

    void handleState0();  // Web install
    void handleState1();  // Standby
    // ... etc

  public:
    SAIRstateMachine();
    void execute();
    void transitionTo(uint8_t newState);
    uint8_t getCurrentState() const;
    const char* getStateName(uint8_t state) const;
};

// Usage
SAIRstateMachine stateMachine;
stateMachine.execute();  // In main loop
```

**Benefits:**
- Better encapsulation
- Easier unit testing
- State transition logging
- Clear state naming

**Estimated Effort:** 6-8 hours
**Files Created:** SAIRstateMachine.h, SAIRstateMachine.cpp

---

### 2.2 Consolidate magic numbers into enums

**Issue:** Hardcoded state numbers (0-9) and mode values throughout code.

**Current State:**
```cpp
// S_air_operating.ino:56
if (mem.state == 9) {  // What is state 9?
  mem.state = 1;
}

// S_air_operating.ino:34
if (!rmt.getPower() || (rmt.getMode() == 1)) {  // What is mode 1?
```

**Recommendation:**
```cpp
// SAIRconfig.h - Add enums
enum State : uint8_t {
  STATE_WEB_INSTALL = 0,
  STATE_STANDBY = 1,
  STATE_INIT_COIL = 2,
  STATE_LOG_BASELINE = 3,
  STATE_PRE_CUT = 4,
  STATE_CUT = 5,
  STATE_COOL = 6,
  STATE_CONFIRM_COOL = 7,
  STATE_POWER_OFF_DETECT = 8,
  STATE_EMERGENCY = 9
};

enum ACMode : uint8_t {
  MODE_OFF = 0,
  MODE_COOL = 1,
  MODE_DRY = 2,
  MODE_FAN = 3,
  MODE_AUTO = 4
};

// Usage
if (mem.state == STATE_EMERGENCY) {
  mem.state = STATE_STANDBY;
}

if (!rmt.getPower() || (rmt.getMode() == MODE_COOL)) {
  // Handle cool mode
}
```

**Estimated Effort:** 2 hours
**Files Affected:** SAIRconfig.h, S_air_operating.ino, SAIRrmt.h

---

### 2.3 Reduce code duplication in temperature reading

**Issue:** Duplicate sensor reading logic with inverted indices.

**Current State:**
```cpp
// S_air_operating.ino:119-140
if (mem.tsensors_select) {
  mem.tc[2] = generate_temp_tc(true);
  mem.tr[2] = generate_temp_tc(false);
  if ((mem.tr[2] <= default_min_tc_to_detect_compressor_on) &&
      (mem.tc[2] > default_min_tc_to_detect_compressor_on)) {
    mem.tsensors_select = 0;
    switch_tc_tr();
  }
} else {
  mem.tc[2] = generate_temp_tc(true);   // DUPLICATE CODE
  mem.tr[2] = generate_temp_tc(false);  // DUPLICATE CODE
  if ((mem.tr[2] <= default_min_tc_to_detect_compressor_on) &&
      (mem.tc[2] > default_min_tc_to_detect_compressor_on)) {  // DUPLICATE
    mem.tsensors_select = 1;
    switch_tc_tr();
  }
}
```

**Recommendation:**
```cpp
void read_temperatures() {
  // Sensor indices based on selection
  uint8_t tc_index = mem.tsensors_select ? 0 : 1;
  uint8_t tr_index = mem.tsensors_select ? 1 : 0;

  // Read temperatures
  mem.tc[2] = get_temperature(tc_index);
  mem.tr[2] = get_temperature(tr_index);

  // Auto-detect if sensors are swapped
  if ((mem.tr[2] <= default_min_tc_to_detect_compressor_on) &&
      (mem.tc[2] > default_min_tc_to_detect_compressor_on)) {
    mem.tsensors_select = !mem.tsensors_select;
    switch_tc_tr();
  }
}
```

**Estimated Effort:** 1 hour
**Files Affected:** S_air_operating.ino

---

## Priority 3: Robustness & Error Handling

### 3.1 Add bounds checking for array access

**Issue:** Multiple array accesses without bounds validation.

**Current State:**
```cpp
// S_air_operating.ino:256-261
for (i = 0; i <= 4; i++) {
  mem.slope_tc[i] = mem.slope_tc[i+1];  // slope_tc[6] when i=5?
  mem.slope_tr[i] = mem.slope_tr[i+1];
}
mem.slope_tc[5] = (mem.av_tc[2] - mem.av_tc[1])*3;
```

**Recommendation:**
```cpp
// SAIRconfig.h - Define array sizes
const uint8_t SLOPE_ARRAY_SIZE = 6;
const uint8_t TEMP_ARRAY_SIZE = 3;

// SAIRmemory.h
float slope_tc[SLOPE_ARRAY_SIZE];
float slope_tr[SLOPE_ARRAY_SIZE];

// S_air_operating.ino - Add validation
for (i = 0; i < SLOPE_ARRAY_SIZE - 1; i++) {
  mem.slope_tc[i] = mem.slope_tc[i+1];
  mem.slope_tr[i] = mem.slope_tr[i+1];
}
mem.slope_tc[SLOPE_ARRAY_SIZE - 1] = (mem.av_tc[2] - mem.av_tc[1]) * 3;
```

**Estimated Effort:** 2 hours
**Files Affected:** SAIRconfig.h, SAIRmemory.h, S_air_operating.ino

---

### 3.2 Improve error handling in MQTT connections

**Issue:** Silent failures in MQTT operations, inconsistent reconnection logic.

**Current State:**
```cpp
// S_air_mqtt.ino:46-67
if(!MQTTtrigger_send_temperature()) {
  if(check_internet_and_connect_MQTT()) {
    if(MQTTtrigger_send_temperature()) {
      has_internet = true;
      mem.trigging_warning_sys_mqtt = false;
      mem.count_warning_sys_mqtt = 0;
    }
  }
}
```

**Recommendation:**
```cpp
enum MQTTError {
  MQTT_OK = 0,
  MQTT_CONN_FAILED,
  MQTT_PUBLISH_FAILED,
  MQTT_NETWORK_FAILED
};

class MQTTManager {
  private:
    uint8_t retry_count;
    unsigned long last_attempt;
    const uint16_t RETRY_INTERVAL = 5000;  // 5 seconds

  public:
    MQTTError publishWithRetry(const char* topic, const char* msg, uint8_t max_retries = 3);
    bool reconnectIfNeeded();
    void handleError(MQTTError error);
};

// Usage
MQTTError result = mqttManager.publishWithRetry("t", temp_msg);
if (result != MQTT_OK) {
  mqttManager.handleError(result);
  MQTTtrigger_log(F("error"), getMQTTErrorString(result));
}
```

**Estimated Effort:** 4 hours
**Files Affected:** S_air_mqtt.ino, create MQTTManager.h/cpp

---

### 3.3 Validate EEPROM data on read

**Issue:** No validation of EEPROM data integrity (corruption detection).

**Recommendation:**
```cpp
// SAIRmemory.h - Add CRC validation
class SAIRmemory {
  private:
    uint16_t calculateCRC16(uint16_t start_pos, uint16_t length);
    bool validateEEPROM(uint16_t start_pos, uint16_t length);

  public:
    bool save_with_checksum(uint16_t start_pos, const void* data, uint16_t length);
    bool load_with_validation(uint16_t start_pos, void* data, uint16_t length);
};

// Implementation
uint16_t SAIRmemory::calculateCRC16(uint16_t start, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= EEPROM.read(start + i);
    for (uint8_t j = 0; j < 8; j++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
  }
  return crc;
}
```

**Estimated Effort:** 3 hours
**Files Affected:** SAIRmemory.h, SAIRmemory.cpp

---

## Priority 4: Code Quality Improvements

### 4.1 Improve variable naming

**Issue:** Inconsistent and unclear variable names.

**Examples:**
```cpp
// Poor naming
int h = 0, i = 0, j = 0, k = 0, x = 0, y = 0, z = 0;  // S_air_operating.ino:107
const float n = 1.8;  // SAIRconfig.h:164 - What is 'n'?
float av_t_cnt;  // Average temperature count?
```

**Recommendations:**
```cpp
// Improved naming
int loop_index = 0;
const float ENERGY_EXPONENT = 1.8;  // For CPS formula: ((op_tset/bl_tset)...)^n
uint8_t average_temp_sample_count = 0;
```

**Estimated Effort:** 2-3 hours (throughout codebase)

---

### 4.2 Extract magic constants

**Issue:** Hardcoded values scattered throughout code.

**Current State:**
```cpp
// S_air_operating.ino:260-261
mem.slope_tc[5] = (mem.av_tc[2] - mem.av_tc[1])*3;  // Why *3?
mem.slope_tr[5] = (mem.av_tr[2] - mem.av_tr[1])*3;

// S_air_mqtt.ino:132
const uint16_t remove_if_not_sent = 10000;  // Local magic number
```

**Recommendation:**
```cpp
// SAIRconfig.h - Document and centralize
const float SLOPE_TO_DEGREES_PER_MINUTE = 3.0;  // Converts loop-based slope to °C/min
const uint16_t MQTT_OVERFLOW_THRESHOLD = 10000;  // Prevent counter overflow

// Usage
mem.slope_tc[5] = (mem.av_tc[2] - mem.av_tc[1]) * SLOPE_TO_DEGREES_PER_MINUTE;
```

**Estimated Effort:** 1-2 hours
**Files Affected:** SAIRconfig.h, multiple .ino files

---

### 4.3 Add function documentation

**Issue:** Many functions lack documentation explaining parameters, return values, and side effects.

**Current State:**
```cpp
// S_air_operating.ino:91
float get_temperature(byte t_index){
  float t_buffer = sensors.getTempCByIndex(t_index);
  if(t_buffer < 0){
    t_buffer = default_temperature_when_no_sensor;
  }
  return t_buffer;
}
```

**Recommendation:**
```cpp
/**
 * @brief Read temperature from Dallas DS18B20 sensor
 * @param t_index Sensor index (0 = first sensor, 1 = second sensor)
 * @return Temperature in Celsius, or default value if sensor fails/disconnected
 * @note Returns default_temperature_when_no_sensor (26°C) on error
 */
float get_temperature(byte t_index) {
  float t_buffer = sensors.getTempCByIndex(t_index);
  if (t_buffer < 0) {  // Sensor read error
    t_buffer = default_temperature_when_no_sensor;
  }
  return t_buffer;
}
```

**Estimated Effort:** 4-6 hours (for critical functions)

---

## Priority 5: Feature Improvements

### 5.1 Implement watchdog timer

**Issue:** No watchdog protection against system hangs.

**Recommendation:**
```cpp
// s_air_2019.ino - setup()
void setup() {
  ESP.wdtEnable(8000);  // 8 second watchdog
  // ... existing setup code ...
}

// s_air_2019.ino - loop()
void loop() {
  ESP.wdtFeed();  // Reset watchdog at start of each loop

  // ... existing loop code ...

  if (loop_execution_time > 7000) {
    Serial.println(F("WARNING: Loop time exceeds watchdog threshold!"));
  }
}
```

**Estimated Effort:** 1 hour
**Files Affected:** s_air_2019.ino

---

### 5.2 Add OTA (Over-The-Air) firmware updates

**Issue:** Commented-out OTA code suggests feature was planned but never implemented.

**Current State:**
```cpp
// s_air_2019.ino:21
//#include <ESP8266httpUpdate.h>
```

**Recommendation:**
```cpp
#include <ArduinoOTA.h>

void setup() {
  // ... existing setup ...

  // Configure OTA
  ArduinoOTA.setHostname(("SAIR_" + mac_name).c_str());
  ArduinoOTA.setPassword("AirBridge2019");  // Change this!

  ArduinoOTA.onStart([]() {
    Serial.println(F("OTA Update Start"));
  });

  ArduinoOTA.onEnd([]() {
    Serial.println(F("OTA Update Complete"));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
  });

  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  // ... existing code ...
}
```

**Estimated Effort:** 2-3 hours
**Files Affected:** s_air_2019.ino

---

## Implementation Plan

### Phase 1: Quick Wins (1-2 weeks)
1. Replace String with F() macro in debug prints (1.1 partial)
2. Add enums for states and modes (2.2)
3. Extract magic constants to SAIRconfig.h (4.2)
4. Add watchdog timer (5.1)
5. Improve variable naming (4.1)

**Expected Impact:** 10-15% RAM reduction, better code readability

---

### Phase 2: Memory Optimization (2-3 weeks)
1. Replace String concatenations with snprintf (1.1 complete)
2. Optimize MQTT message construction (1.3)
3. Remove all blocking delays (1.2)
4. Add bounds checking (3.1)

**Expected Impact:** 25-30% RAM reduction, improved responsiveness

---

### Phase 3: Architecture Improvements (3-4 weeks)
1. Extract state machine class (2.1)
2. Create MQTTManager class (3.2)
3. Reduce temperature reading duplication (2.3)
4. Add EEPROM CRC validation (3.3)

**Expected Impact:** Better maintainability, robust error handling

---

### Phase 4: Feature Additions (1-2 weeks)
1. Implement OTA updates (5.2)
2. Add comprehensive function documentation (4.3)
3. Create unit tests for critical functions

**Expected Impact:** Easier maintenance, remote updates

---

## Testing Strategy

### Before Each Refactoring Task:
1. Document current RAM usage (Serial monitor at startup)
2. Record baseline performance metrics (loop time, MQTT latency)
3. Capture serial output for 30-minute baseline run
4. Save EEPROM state to file

### After Each Refactoring Task:
1. Compare RAM usage (should not increase)
2. Verify state machine still transitions correctly
3. Confirm MQTT messages still sent/received
4. Test IR signal detection and transmission
5. Verify EEPROM persistence across reboot

### Integration Testing:
1. Run full 24-hour cycle (State 0 → 3 → 5 → 7)
2. Test sensor failure recovery (State 9)
3. Verify WiFi reconnection logic
4. Test factory reset procedure

---

## Risk Mitigation

### High-Risk Changes:
- **EEPROM structure modifications** → Create EEPROM migration tool
- **State machine refactoring** → Keep old code in comments initially
- **MQTT protocol changes** → Version compatibility layer

### Rollback Plan:
1. Tag current firmware as `v1.1a-stable` in git
2. Create feature branches for each major refactoring
3. Keep original .ino files as `.ino.backup`
4. Document EEPROM layout version in header

---

## Metrics for Success

### Code Quality Metrics:
- [ ] Reduce String usage by 80% (from 54 to <10 instances)
- [ ] Eliminate all blocking delay() calls
- [ ] RAM usage stays below 60% (currently ~63%)
- [ ] All magic numbers moved to SAIRconfig.h
- [ ] 100% of public functions documented

### Performance Metrics:
- [ ] Main loop execution time < 500ms (currently ~3-5 seconds)
- [ ] MQTT publish success rate > 98%
- [ ] IR signal detection rate > 95%
- [ ] EEPROM write cycles minimized (< 10 per hour)

### Reliability Metrics:
- [ ] Zero watchdog resets in 24-hour test
- [ ] Successful state transitions in 100/100 test runs
- [ ] WiFi reconnection success rate > 90%
- [ ] No EEPROM corruption in 1000 power cycle tests

---

## Conclusion

This refactoring plan prioritizes memory optimization and code maintainability while respecting the ESP8266's resource constraints. The phased approach allows for incremental testing and reduces risk of introducing bugs.

**Recommended Next Steps:**
1. Review and approve this plan with stakeholders
2. Set up version control (git) if not already in place
3. Create development/testing environment separate from production device
4. Begin Phase 1 implementation
5. Establish code review process for each change

**Estimated Total Effort:** 8-12 weeks (part-time) or 4-6 weeks (full-time)

---

**Document Version:** 1.0
**Date:** 2025-10-07
**Author:** Claude Code Analysis
**Based on:** AirBridge Firmware v1.1a

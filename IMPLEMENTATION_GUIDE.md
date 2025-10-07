# AirBridge Refactoring Implementation Guide

## Quick Start

This guide provides step-by-step instructions for implementing the refactoring recommendations in the AirBridge codebase. Follow these steps to improve code quality while maintaining system stability.

---

## Prerequisites

### Required Tools
- Arduino IDE 1.8.x or newer
- ESP8266 board package installed
- Serial monitor (115200 baud)
- Git for version control (recommended)

### Before You Start
1. **Backup current firmware**
   ```bash
   # Create backup directory
   mkdir -p backups/v1.1a-$(date +%Y%m%d)

   # Copy all source files
   cp -r s_air_2019/* backups/v1.1a-$(date +%Y%m%d)/
   ```

2. **Document current state**
   - Record RAM usage from serial monitor at startup
   - Note baseline loop execution time
   - Save EEPROM state (if possible)
   - Document current behavior for testing

3. **Set up test environment**
   - Use development ESP8266 (not production device)
   - Keep backup firmware ready to flash
   - Have physical reset button accessible

---

## Phase 1: Foundation Improvements (Day 1-2)

### Step 1.1: Add Enums for States and Modes

**Status:** ✅ COMPLETED

The following has been added to `SAIRconfig.h`:

```cpp
enum SystemState : uint8_t {
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
  MODE_POWER_OFF = 0,
  MODE_COOL = 1,
  MODE_DRY = 2,
  MODE_FAN = 3,
  MODE_AUTO = 4
};
```

**Next Actions:**
1. Gradually replace hardcoded state numbers in code
2. Example migration:
   ```cpp
   // OLD
   if (mem.state == 9) {
     mem.state = 1;
   }

   // NEW
   if (mem.state == STATE_EMERGENCY) {
     mem.state = STATE_STANDBY;
   }
   ```

3. Search and replace candidates:
   ```bash
   # Find all hardcoded state comparisons
   grep -n "mem.state == [0-9]" s_air_2019/*.ino
   ```

**Testing:**
- [ ] Verify compilation succeeds
- [ ] Check RAM usage (should be identical)
- [ ] Confirm state transitions work correctly

---

### Step 1.2: Add Non-Blocking Timer Utility

**Status:** ✅ COMPLETED

New file created: `s_air_2019/SAIRtimer.h`

**Integration Steps:**

1. **Add include to main file**
   ```cpp
   // In s_air_2019.ino, after other includes
   #include "SAIRtimer.h"
   ```

2. **Replace first delay() - Simple Example**

   Find in `S_air_operating.ino` line 39-40:
   ```cpp
   // OLD CODE
   rmt.setTrset(min_trset);
   delay(100);  // BLOCKING!
   rmt.cool();
   ```

   Replace with:
   ```cpp
   // NEW CODE
   static SAIRTimer coolDelayTimer(100);
   static bool waiting_for_cool = false;

   if (rmt.getTrset() < min_trset) {
     rmt.setTrset(min_trset);
     coolDelayTimer.start();
     waiting_for_cool = true;
   }

   if (waiting_for_cool && coolDelayTimer.elapsed()) {
     rmt.cool();
     waiting_for_cool = false;
   }
   ```

3. **Replace startup delays in S_air_sys_mainten.ino**

   Find line 53, 62-66:
   ```cpp
   // OLD CODE
   delay(2000);
   // ... code ...
   for (i = 3; i > 0; i--) {
     Serial.println(i);
     delay(1000);
   }
   ```

   Replace with:
   ```cpp
   // NEW CODE
   static SAIRTimer startupTimer(1000);
   static uint8_t countdown = 3;
   static bool countdown_active = false;

   if (!countdown_active) {
     startupTimer.start();
     countdown_active = true;
     countdown = 3;
   }

   if (countdown_active && startupTimer.elapsedAndRestart()) {
     if (countdown > 0) {
       Serial.println(countdown);
       countdown--;
     } else {
       countdown_active = false;
     }
   }
   ```

**Testing:**
- [ ] System still boots correctly
- [ ] LED patterns unchanged
- [ ] IR signals not missed during "delays"
- [ ] Serial output timing looks correct

---

### Step 1.3: Add Array Size Constants

**Status:** ✅ COMPLETED

Added to `SAIRconfig.h`:
```cpp
const uint8_t TEMP_ARRAY_SIZE = 3;
const uint8_t SLOPE_ARRAY_SIZE = 6;
const uint8_t MIN_SLOPE_ARRAY_SIZE = 3;
const float SLOPE_TO_DEGREES_PER_MINUTE = 3.0;
```

**Migration Steps:**

1. **Update array declarations in SAIRmemory.h**
   ```cpp
   // OLD
   float slope_tc[6], slope_tr[6];
   float tc[3], tr[3], av_tc[3], av_tr[3];

   // NEW
   float slope_tc[SLOPE_ARRAY_SIZE], slope_tr[SLOPE_ARRAY_SIZE];
   float tc[TEMP_ARRAY_SIZE], tr[TEMP_ARRAY_SIZE];
   float av_tc[TEMP_ARRAY_SIZE], av_tr[TEMP_ARRAY_SIZE];
   ```

2. **Update loop bounds in S_air_operating.ino**
   ```cpp
   // OLD (line 256)
   for (i = 0; i <= 4; i++) {
     mem.slope_tc[i] = mem.slope_tc[i+1];
   }

   // NEW
   for (i = 0; i < SLOPE_ARRAY_SIZE - 1; i++) {
     mem.slope_tc[i] = mem.slope_tc[i+1];
   }
   ```

**Testing:**
- [ ] No out-of-bounds array access
- [ ] Slope calculations produce same results
- [ ] No compilation warnings

---

## Phase 2: Memory Optimization (Day 3-5)

### Step 2.1: Refactor MQTT String Usage

**Reference:** See `examples/refactored_mqtt_example.ino` for complete examples.

**Priority Order:**
1. MQTTtrigger_send_temperature() - Called every 5 minutes
2. MQTTtrigger_send_baseline() - Large message
3. mqtt_publish() - Core function
4. MQTTtrigger_send_performance()
5. MQTTtrigger_get_ir()
6. MQTTtrigger_log()

**Implementation Pattern:**

```cpp
// BEFORE
boolean MQTTtrigger_send_temperature(){
  String msg = String(mem.tc[2]) + "," + String(mem.tr[2]);
  return mqtt_publish(F("t"), msg);
}

// AFTER
boolean MQTTtrigger_send_temperature(){
  char msg[32];
  snprintf(msg, sizeof(msg), "%.2f,%.2f", mem.tc[2], mem.tr[2]);
  return mqtt_publish(F("t"), msg);
}
```

**Step-by-Step Migration:**

1. **Backup S_air_mqtt.ino**
   ```bash
   cp s_air_2019/S_air_mqtt.ino s_air_2019/S_air_mqtt.ino.backup
   ```

2. **Refactor one function at a time**
   - Start with MQTTtrigger_send_temperature()
   - Compile and test
   - Move to next function

3. **Update mqtt_publish() signature**
   ```cpp
   // Add overload for const char*
   boolean mqtt_publish(const __FlashStringHelper* subtopic, const char* msg) {
     char topic[64];
     char subtopic_buf[32];

     strncpy_P(subtopic_buf, (PGM_P)subtopic, sizeof(subtopic_buf) - 1);
     subtopic_buf[sizeof(subtopic_buf) - 1] = '\0';

     snprintf(topic, sizeof(topic), "air/%s/%s", mac_name, subtopic_buf);
     return client.publish(topic, msg);
   }
   ```

**Testing After Each Change:**
- [ ] Function compiles without errors
- [ ] MQTT message format identical to original
- [ ] Message arrives at MQTT server correctly
- [ ] Check serial output for correct formatting
- [ ] Run for 1 hour, verify no crashes

**Memory Verification:**
```cpp
// Add to setup() for monitoring
Serial.print(F("Free heap: "));
Serial.println(ESP.getFreeHeap());
```

Expected improvement: 2-4KB more free heap after all changes.

---

### Step 2.2: Refactor Temperature Reading

**Reference:** See `examples/refactored_temperature_reading.ino`

**Key Changes:**

1. **Create helper function** (add to S_air_operating.ino):
   ```cpp
   void read_and_classify_temperatures() {
     uint8_t tc_index = mem.tsensors_select ? 0 : 1;
     uint8_t tr_index = mem.tsensors_select ? 1 : 0;

     #ifdef USE_SIMULATED_TEMPS
       mem.tc[2] = generate_temp_tc(true);
       mem.tr[2] = generate_temp_tc(false);
     #else
       mem.tc[2] = get_temperature(tc_index);
       mem.tr[2] = get_temperature(tr_index);
     #endif

     if ((mem.tr[2] <= default_min_tc_to_detect_compressor_on) &&
         (mem.tc[2] > default_min_tc_to_detect_compressor_on)) {
       mem.tsensors_select = !mem.tsensors_select;
       switch_tc_tr();
     }
   }
   ```

2. **Replace duplicate code blocks** in read_temp_slope_compressor():
   ```cpp
   // OLD: Lines 119-140 (22 lines of duplicate code)
   if (mem.tsensors_select) {
     // ... duplicate code ...
   } else {
     // ... duplicate code ...
   }

   // NEW: Single function call
   read_and_classify_temperatures();
   ```

3. **Add bounds checking to loops**:
   ```cpp
   // OLD
   for (i = 0; i <= 4; i++) {
     mem.slope_tc[i] = mem.slope_tc[i+1];
   }

   // NEW
   for (i = 0; i < SLOPE_ARRAY_SIZE - 1; i++) {
     mem.slope_tc[i] = mem.slope_tc[i+1];
   }
   ```

**Testing:**
- [ ] Temperature readings match original (±0.01°C)
- [ ] Sensor swap detection triggers correctly
- [ ] No array out-of-bounds errors
- [ ] Run 24-hour test with sensor disconnection test

---

## Phase 3: Testing & Validation

### Comprehensive Test Suite

#### Test 1: Boot and Initialization
```
[ ] Device boots without errors
[ ] LEDs flash correctly (yellow + green for 3 seconds)
[ ] WiFi connects successfully
[ ] MQTT connection established
[ ] Web server starts on 192.168.4.1
[ ] Serial output shows correct firmware version
```

#### Test 2: State Machine Transitions
```
[ ] State 0 → State 1 (web install → standby)
[ ] State 1 → State 2 (standby → init coil on AC power on)
[ ] State 2 → State 3 (init → baseline after compressor detected)
[ ] State 3 → State 4 (baseline → pre-cut after 25+ min)
[ ] State 4 → State 5 (pre-cut → cut when conditions met)
[ ] State 5 → State 6 → State 7 (cut → cool → confirm cool)
[ ] State X → State 9 (any → emergency on sensor failure)
```

#### Test 3: MQTT Communication
```
[ ] Temperature messages sent every ~5 minutes
[ ] Performance messages sent every ~60 minutes
[ ] Baseline message sent when complete
[ ] IR state changes published immediately
[ ] Messages received by MQTT broker correctly
[ ] Message format matches original specification
```

#### Test 4: Memory Stability
```
[ ] Free heap at boot: _________ bytes
[ ] Free heap after 1 hour: _________ bytes
[ ] Free heap after 24 hours: _________ bytes
[ ] No memory leaks detected
[ ] No heap fragmentation warnings
```

#### Test 5: IR Signal Detection
```
[ ] AC remote signals detected correctly
[ ] Remote protocol number identified
[ ] Temperature setpoint changes tracked
[ ] Power on/off commands processed
[ ] Mode changes (cool/dry/fan) recognized
[ ] Smart cutting IR commands sent successfully
```

#### Test 6: Error Recovery
```
[ ] Sensor disconnect → State 9 → Fallback values used
[ ] WiFi disconnect → Auto-reconnect successful
[ ] MQTT disconnect → Reconnection logic works
[ ] Power cycle → EEPROM state restored correctly
[ ] Watchdog reset → System recovers (if implemented)
```

---

## Rollback Procedures

### If Issues Occur

1. **Immediate Rollback**
   ```bash
   # Flash backup firmware
   cd backups/v1.1a-YYYYMMDD/
   # Upload via Arduino IDE
   ```

2. **Partial Rollback**
   ```bash
   # Restore specific file
   cp backups/v1.1a-YYYYMMDD/S_air_mqtt.ino s_air_2019/
   ```

3. **EEPROM Reset** (if state corrupted)
   - Press and hold reset button during 3-second LED flash
   - Reconfigure via web interface

---

## Performance Benchmarks

### Before Refactoring (Baseline)
```
Free Heap at Boot:     _____ bytes
Loop Execution Time:   _____ ms
String Objects:        54 instances
MQTT Publish Time:     _____ ms
```

### After Phase 1 (Expected)
```
Free Heap at Boot:     +100 bytes (minimal change)
Loop Execution Time:   _____ ms (same)
String Objects:        54 instances (no change yet)
MQTT Publish Time:     _____ ms (same)
```

### After Phase 2 (Target)
```
Free Heap at Boot:     +2000-4000 bytes
Loop Execution Time:   -50-100 ms (faster)
String Objects:        <10 instances
MQTT Publish Time:     -50-100 ms (faster)
```

---

## Troubleshooting

### Common Issues

**Issue:** Compilation errors about enum types
```
Solution: Make sure SAIRconfig.h is included before first use
Check: #include order in s_air_2019.ino
```

**Issue:** MQTT messages empty or corrupted
```
Solution: Check buffer sizes in snprintf calls
Debug: Add Serial.println(msg) before mqtt_publish()
```

**Issue:** Timer doesn't work, still blocking
```
Solution: Verify static keyword used for timer objects
Check: Timer.start() called before checking elapsed()
```

**Issue:** Increased RAM usage instead of decreased
```
Solution: Ensure old String variables removed completely
Check: No String objects left in refactored functions
```

**Issue:** State machine stuck in one state
```
Solution: Verify enum values match original numbers (0-9)
Check: mem.state assignment uses correct enum value
```

---

## Code Review Checklist

Before committing each change:

### Correctness
- [ ] Functionality identical to original
- [ ] No new bugs introduced
- [ ] Edge cases handled
- [ ] Error conditions managed

### Performance
- [ ] RAM usage same or lower
- [ ] Execution time same or faster
- [ ] No blocking operations added
- [ ] MQTT timing unchanged

### Code Quality
- [ ] Comments added where needed
- [ ] Variable names clear and descriptive
- [ ] Magic numbers replaced with constants
- [ ] No compiler warnings

### Testing
- [ ] Unit tests pass (if applicable)
- [ ] Integration tests pass
- [ ] 24-hour stability test completed
- [ ] MQTT messages verified correct

---

## Next Steps After Phase 2

Once Phase 1 and 2 are complete and stable:

1. **Document improvements**
   - Update CLAUDE.md with new patterns
   - Record memory savings achieved
   - Note any unexpected issues

2. **Plan Phase 3** (Architecture improvements)
   - State machine class extraction
   - MQTT manager class
   - EEPROM validation

3. **Consider features**
   - OTA firmware updates
   - Watchdog timer
   - Enhanced error logging

---

## Support Resources

- **Original Documentation:** README.md
- **Refactoring Plan:** REFACTORING_RECOMMENDATIONS.md
- **Code Examples:** examples/ directory
- **Configuration Reference:** SAIRconfig.h comments

---

## Change Log

| Date | Phase | Changes | Status |
|------|-------|---------|--------|
| 2025-10-07 | Phase 1.1 | Added enums to SAIRconfig.h | ✅ Complete |
| 2025-10-07 | Phase 1.2 | Created SAIRtimer.h utility | ✅ Complete |
| 2025-10-07 | Phase 1.3 | Added array size constants | ✅ Complete |
| 2025-10-07 | Phase 2.0 | Created MQTT example code | ✅ Complete |
| 2025-10-07 | Phase 2.0 | Created temp reading example | ✅ Complete |

---

**Document Version:** 1.0
**Last Updated:** 2025-10-07
**Maintained By:** AirBridge Development Team

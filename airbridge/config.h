#ifndef CONFIG_H
#define CONFIG_H
/**********************************************************************************
 * AirBridge v2.0 - Open Source WiFi-to-IR Bridge for Air Conditioners
 *
 * Universal IR relay supporting 60+ AC protocols across 12 major brands.
 * This is the open-source edition.
 *
 * ORIGINAL DEVELOPMENT (2016-2019):
 * Co-developed by Werapol Bejranonda and Pathawee Somsak
 *
 * OPEN-SOURCE RELEASE (2025):
 * Reorganized for open-source by Werapol Bejranonda
 *
 * REMOVED FEATURES (Available in commercial version at https://wenext.net/):
 * - Smart energy-saving algorithms (20-50% savings)
 * - Baseline learning and adaptation
 * - Adaptive compressor detection
 * - Remote monitoring (MQTT)
 * - Email reporting and analytics
 *
 * This version provides basic IR signal relay functionality only.
 * ********************************************************************************
 */

/*
 * Firmware info / Version
 */
const uint8_t firmware_version = 2;
const uint8_t firmware_build = 1;
const char firmware_release = 'a';

/*
 * State Machine States - Simplified for v2.0
 * Only 3 states in open-source version
 */
enum SystemState : uint8_t {
  STATE_WEB_CONFIG = 0,      // WiFi configuration via web portal
  STATE_STANDBY = 1,          // AC is off, system idle
  STATE_IR_RELAY = 2          // AC on, relaying IR signals (no intelligence)
};

/*
 * Air Conditioner Modes
 */
enum ACMode : uint8_t {
  MODE_POWER_OFF = 0,
  MODE_COOL = 1,
  MODE_DRY = 2,
  MODE_FAN = 3,
  MODE_AUTO = 4
};

/*
 * Array Size Constants
 */
const uint8_t TEMP_ARRAY_SIZE = 3;        // Size of temperature arrays (for display only)

/*
 * Debug / Testing
 */
const bool show_data = true;              // Enable serial debug output
const bool test_rmt = 0;                  // IR testing mode
const bool show_hardware_warning = 0;     // Display sensor warnings

/*
 * Time control
 */
const byte finish_webserver_loop = 20;    // Close webserver after N loops (~20 loops)
const uint32_t sleep_period = 617330;     // Main loop period (~5 seconds)

/*
 * Sensor boundary validation
 */
const float physical_sensor_boundary_tmin = -5.0;   // Minimum valid temperature (°C)
const float physical_sensor_boundary_tmax = 50.0;   // Maximum valid temperature (°C)

/*
 * Hardware pin configuration
 */
const uint8_t ONE_WIRE_BUS = 2;           // DS18B20 temperature sensors pin
const uint8_t frequency = 38;             // IR frequency (38kHz)
const uint8_t tolerance = 150;            // IR receiver tolerance
const uint8_t RX_PIN = 4;                 // IR receiver pin
const uint8_t TX_PIN = 5;                 // IR transmitter pin

/*
 * Temperature defaults
 */
const uint8_t default_trset = 25;                    // Default temperature setpoint
const uint8_t default_temperature_when_no_sensor = 26;  // Fallback if sensor fails

/*
 * Temperature setpoint limits (safety bounds)
 */
const uint8_t min_trset = 18;             // Minimum allowed setpoint
const uint8_t max_trset = 30;             // Maximum allowed setpoint

/*
 * Webserver configuration
 */
const int8_t web_install_duration = 5;    // Web config duration (minutes)

/*
 * Utility constants
 */
const String rn = "\r\n";                 // Carriage return + newline
const int default_val_ini_zero = 0;       // Default integer zero
const float default_val_intial_high = 99; // Default high value for initialization

/*
 * Timing Constants (for loop-based calculations)
 * v2.1: Added missing constants from v1.0 for compatibility
 */
const uint8_t one_mi = 3;                 // Loops per minute (~20s * 3 = 1 min)
const uint8_t one_hr = 180;               // Loops per hour (one_mi * 60)

/*
 * Compressor Detection Constants (legacy from v1.0, minimal in v2.x)
 * v2.1: Required for temperature monitoring logic
 */
const float default_min_tc_to_detect_compressor_on = 12.0;  // °C
const uint16_t considering_change_detect_compressor_on_after_loop = 90;  // ~30 minutes
const float default_min_tc_slope_considering_compressor_on = -0.15;  // °C/min
const float defaultmin_min_tc_slope_considering_compressor_on = -0.15;  // °C/min

/*
 * State 9 (emergency mode) - v2.0 simplified
 */
const uint8_t state_9_default_trset = 25;  // Default setpoint °C
const float state_9_av_bl_log_duty_default = 0.5;  // 50% (stub)
const float state_9_percent_of_compressor_from_av_bl_log_duty = 0.7;  // 70% (stub)

/*
 * Power Detection (legacy stubs)
 */
const uint16_t power_off_detect_after_compressor_off_loop = 30;  // ~10 minutes

/*
 * Cutting Algorithm Constants (legacy from v1.0 - stubs for v2.x)
 * v2.1: Not used in open-source version, kept for code compatibility only
 */
const uint8_t mem_percent_to_cut_table_size = 21;
const float mem_percent_to_cut_table[21] = {
  0.99, 0.95, 0.90, 0.85, 0.80, 0.75, 0.70, 0.65, 0.60, 0.55,
  0.50, 0.45, 0.40, 0.35, 0.30, 0.25, 0.20, 0.15, 0.10, 0.05, 0.01
};

/*
 * Other Legacy Constants (stubs for compatibility)
 * These are NOT used in v2.x but prevent compilation errors
 */
const uint8_t initial_first_loop_to_considering_cut = 60;  // ~20 minutes
const float diff_btw_tr_and_trset_to_start_cut = 1.0;      // °C
const float tr_less_than_trset_to_cut = 1.5;               // °C
const uint8_t min_comp_on_loop_to_start_cut = 9;           // 3 minutes
const uint8_t considering_heavy_cut_after_loop = 120;      // ~40 minutes
const float heavy_duty_tr_slope_boundary = 0.02;           // °C/min
const uint8_t heavy_duty_count_loop_max = 6;               // 2 minutes
const uint8_t total_heavyduty_cut_to_adjust_trset = 3;     // cycles
const uint8_t max_trset_to_adjust_by_heavy_duty = 28;      // °C
const uint8_t heavy_warning_after_min = 30;                // minutes
const uint8_t dirty_min_slope_not_pass_cnt = 5;            // cycles
const float dirty_max_tc_from_min_tc = 3.0;                // °C
const float dirty_slope_tc_potential = 0.7;                // ratio
const float max_tc_slope_compressor_off = -0.05;           // °C/min
const float minimum_gap_max_min_tc_considering_revision = 2.0;  // °C
const float min_gap_tcmin_and_tdetect_considering_new_detect = 1.0;  // °C
const float considering_change_slope_compressor_on_by_gap_tc_max_min_beyond = 0.1;  // °C/min
const uint8_t state_2_duration = 30;                       // minutes
const uint16_t baseline_finish_state_cnt_normal = 60;      // ~20 minutes
const uint16_t baseline_finish_state_cnt_max = 180;        // ~60 minutes
const uint16_t baseline_comp_on_loop = 30;                 // ~10 minutes
const uint8_t baseline_cycle_compressor = 2;               // cycles
const uint8_t broke_ir_warning_after_min[2] = {5, 5};      // minutes
const float default_tdiff_tr_tc_compressor = 3.0;          // °C
const uint8_t count_warning_sys_mqtt_to_reconnect_wifi = 5;  // attempts

/*
 * Encryption Key (CHANGE THIS FOR YOUR DEPLOYMENT!)
 * v2.1: Used for WiFi password encryption in EEPROM
 */
const char s_air_key[] = "AirBridge2025Key";  // 16-byte key for xxtea

#endif

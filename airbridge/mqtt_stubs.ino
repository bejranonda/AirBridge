/****************************************************************************************
 * mqtt_stubs.ino
 *
 * MQTT Function Stubs for AirBridge v2.1
 *
 * These functions are stubs for MQTT functionality that was removed in v2.0.
 * They prevent compilation errors but do not perform actual MQTT operations.
 *
 * MQTT support was removed because:
 * - Requires cloud infrastructure
 * - Adds complexity for simple IR relay use case
 * - Available in commercial version at https://wenext.net/
 *
 * To enable MQTT in the future, implement these functions properly.
 *
 * v2.1: Added by Claude Code refactoring (2025-01-07)
 * **************************************************************************************
 */

/**
 * Initialize MQTT connection and settings
 * @return false - MQTT not implemented in v2.x
 */
boolean mqtt_set_all() {
  if (show_data) {
    Serial.println(F("[MQTT] mqtt_set_all() - not implemented in v2.x"));
  }
  return false;
}

/**
 * Trigger MQTT message when IR signal received
 * @return false - MQTT not implemented in v2.x
 */
boolean MQTTtrigger_get_ir() {
  if (show_data) {
    Serial.println(F("[MQTT] IR received (MQTT disabled in v2.x)"));
  }
  return false;
}

/**
 * Send log message via MQTT
 * @param category Log category (e.g., "system", "error", "warning")
 * @param message Log message text
 * @return false - MQTT not implemented in v2.x
 */
boolean MQTTtrigger_log(String category, String message) {
  // Print to serial instead of MQTT
  Serial.print(F("[LOG:"));
  Serial.print(category);
  Serial.print(F("] "));
  Serial.println(message);
  return false;
}

/**
 * Send temperature readings via MQTT
 * @return false - MQTT not implemented in v2.x
 */
boolean MQTTtrigger_send_temperature() {
  if (show_data) {
    Serial.print(F("[TEMP] Room: "));
    Serial.print(mem.tr[2]);
    Serial.print(F("°C, Coil: "));
    Serial.print(mem.tc[2]);
    Serial.println(F("°C (MQTT disabled)"));
  }
  return false;
}

/**
 * Control MQTT message sending during web app loop
 * No-op in v2.x
 */
void control_MQTTsending_webapp_loop() {
  // No operation - web app doesn't use MQTT in v2.x
}

/**
 * Trigger MQTT message during WiFi setup
 * @return false - MQTT not implemented in v2.x
 */
boolean MQTTtrigger_wifi_setup() {
  if (show_data) {
    Serial.println(F("[MQTT] WiFi setup (MQTT disabled in v2.x)"));
  }
  return false;
}

/**
 * Check if MQTT is connected
 * @return false - Always disconnected in v2.x
 */
boolean mqtt_is_connected() {
  return false;
}

/**
 * Reconnect to MQTT broker
 * No-op in v2.x
 */
void mqtt_reconnect() {
  // No operation in v2.x
}

/**
 * Process MQTT loop (check messages, maintain connection)
 * No-op in v2.x
 */
void mqtt_loop() {
  // No operation in v2.x
}

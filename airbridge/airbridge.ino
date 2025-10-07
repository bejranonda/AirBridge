
/* Note
 * 
 */

/****************************************************************************************
 * AirBridge v2.0 - Open Source WiFi-to-IR Bridge for Air Conditioners
 *
 * Universal IR relay supporting 60+ AC protocols across 12 major brands.
 *
 * ORIGINAL DEVELOPMENT (2016-2019):
 * Co-developed by Werapol Bejranonda and Pathawee Somsak
 *
 * OPEN-SOURCE RELEASE (2025):
 * Reorganized for open-source by Werapol Bejranonda
 *
 * Coding Guidelines:
 * 1. use global variable to save
 * 2. use local variable to calculate
 * 3. use Serial.print only one time at the end of function
 * 4. use F() every time you can (saves RAM)
 * 5. try to improve code performance
 *
 * GitHub: https://github.com/yourusername/AirBridge
 * License: MIT
 * Commercial Version: https://wenext.net/
 * **************************************************************************************
 */
#include <pgmspace.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
//#include <ESP8266httpUpdate.h>
//#include <ESP8266mDNS.h>
#include <time.h>
#include <ESP8266Ping.h>
#include <xxtea-lib.h>
#include <PubSubClient.h>

extern "C"{
  #include "gpio.h"
  #include "user_interface.h"
}

//all class and config
#include "config.h"
#include "memory_manager.h"
MemoryManager mem;

//MQTT
WiFiClient espClient;
PubSubClient client(espClient);

//LED
const byte ledYellowPin = 12;
const byte ledGreenPin = 14;
#include "led_controller.h"
LEDController led_yellow(ledYellowPin);
LEDController led_green(ledGreenPin);

#include "ir_remote.h"
IRRemote rmt(RX_PIN, TX_PIN, tolerance, frequency);
//#include "Gsender.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~ S_air_web_AP file ~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
//-- function 
boolean wifi_connect();
void set_hotspot_name(); 
void web_install_sort_ssid(int32_t a[], int b[], int size);
void web_install();
void web_home_main();
void home_setting();
//void home_full();
void home_lite();
void wifi_setting();
void reset_all_setting();
void web_show_status();
void web_page_not_found();
void reset_web_every_one_hr();
void start_web();
void close_web();
//-- vars
ESP8266WebServer server(80);
//IPAddress local_IP(192, 168, 4, 1);
//IPAddress subnet(255, 255, 255, 0);
char hotspot_name[15]; // SSID name of this module
String mac_name; // SSID name of this module
//char mac_name[13]; // SSID name of this module


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~ S-air_log_data_email-file ~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
/*
 * ~~~~~~~~~~ esaving main ~~~~~~~~
 */
void log_esaving(); 
void set_trstart();
float get_cps_fkt(uint8_t *bl_tr_set, uint8_t *op_tr_set, float *op_tr_start, float *bl_tr_start, const float *op_tout, const float *bl_tout);

/*
 * ~~~~~~~~~~ esaving full ~~~~~~
 */
//void log_raw_data();
//void log_raw_data_algo();
//void log_hr_no_t_full();
//void check_last_hr();
//void stamp_hr_log_t_full();
//void stamp_hr_log_new_t_full();   
//void stamp_hr_log_old_t_full(double t_from_internet);   
//void log_dy_full();
//void log_mo_full();
void stamp_hr_log_t_full_log_dy_full_log_mo_full_send_email();
//float get_av_tr_from_log();
void show_log_full();

/* 
 *  ~~~~~~~~~~ esaving lite ~~~~~~
 */
int get_av_esaving_lite(String str1, uint8_t index);
void log_hr_lite();
//void show_log_lite();

/*
 * sending email
 */
//void send_email_to_user(String *email);
//void send_email_to_operator();
//void send_email();
void stamp_t_send_email();


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~ S_air_date_time-file ~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
//int get_yr(time_t *unix_t);
//int get_mo(time_t *unix_t);
int get_dy(time_t *unix_t);
int get_hr(time_t *unix_t);
int get_mi(time_t *unix_t);


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~ S-air-operating-file ~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */ 
boolean check_ir(); // in SAIRrmt.h 
float get_min_float(float* array, int size);
float get_max_float(float* array, int size);
void sleep_long();
void sleep_20s();
void show_op_status();
void test_config();
//sensors
void read_temp_slope();
void switch_tc_tr();
void check_compressor();
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);// Pass our oneWire reference to Dallas Temperature.
//state_function
void state_function(); 
void state_0();
void state_1();
void state_2();
void state_3();
void state_4();
void state_5();
void state_6();
void state_7();
void state_8();
void state_9();


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~ S-air-sys-mainten-file ~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
void clearing_eeprom_at_first_start(); 
void saving_vars_to_eeprom();
void restart_s_air_1time();
void reset_or_not();
void returning_vars_from_eeprom();   
void check_power_off();
void check_trset();
void check_percent_to_cut();
void check_dirty();
void check_broke_temp();
//void check_batt();
void check_state_9();

void led_blink_together(byte pin_yellow, byte pin_green, byte repeat_loop){
  // Both off
  for (int i = 0; i < repeat_loop; i++) {
    digitalWrite(pin_yellow, LOW); 
    digitalWrite(pin_green, LOW);
    delay(300);
    digitalWrite(pin_yellow, HIGH); 
    digitalWrite(pin_green, HIGH);
    delay(500);
  }
  digitalWrite(pin_yellow, LOW); 
  digitalWrite(pin_green, LOW);
}

/*
 * Generate tc or tr for test simulation
 */
float generate_temp_tc(boolean send_back_tc){
  // For Tc
  // send_back_tc, otherwise it will send tr
  const float max_tc_output = 21;
  const float min_tc_output = 12;

  const float percent_decreasing_slope = 8; // % of difference between current and min (decreasing) or max (increasing) degree C / per loop
  const float percent_increasing_slope = 40; // degree C / per loop
  static boolean increasing_slope = true;// wheter increasing> true, or decreasing> false

  // For Tr
  const float max_tr_output = 26;
  const float min_tr_output = 24.5;

  float current_tc = mem.tc[2];
  float randon_slope = random(0, 210) - 10;  // random with middle value of 110, will small amout of possible minus
  
  if(send_back_tc){
    
  
    if(current_tc < min_tc_output){
      current_tc = (max_tc_output + min_tc_output)/2;
    }
    
    if(increasing_slope){
      // keep randon_slope positive
      randon_slope = randon_slope/10000*percent_increasing_slope*abs(max_tc_output - current_tc); //  random with middle value at 100, and % of difference between current and t_min --> degree C / per loop, higher slope at begining, and lower when current_tc is reaching max_tc_output
      if(current_tc >= max_tc_output*0.95){
        increasing_slope = false;
      }
    }else{
      // turn randon_slope negative
      randon_slope = randon_slope/10000*percent_decreasing_slope*abs(current_tc - min_tc_output)*-1; //  random with middle value at 100, and % of difference between current and t_min --> degree C / per loop , higher slope at begining, and lower when current_tc is reaching min_tc_output
        if(current_tc <= min_tc_output*1.05){
          increasing_slope = true;
        }
    }
    //Serial.print("generate_temp_tc:");Serial.println(current_tc + randon_slope);
    return current_tc + randon_slope;
    
  }else{
    float current_tr = (max_tr_output - min_tr_output)/(max_tc_output - min_tc_output)*(current_tc - min_tc_output)*randon_slope/100 + min_tr_output;
    Serial.print("current_tr1:");Serial.println(current_tr);
    if(current_tr > max_tr_output){
      current_tr = max_tr_output;
    }else if(current_tr < min_tr_output){
      current_tr = min_tr_output;
    }
    Serial.print("current_tr2:");Serial.println(current_tr);
    return current_tr; // return simulate room temperature
  }
  
}




/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~ S-air_mqtt ~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
boolean mqtt_set_all();
boolean MQTTtrigger_get_ir();
boolean check_internet_and_connect_MQTT();

 

void setup(){
  /*
   * ~~~~~~~~ set cpu_speed, wdt_time_out, EEPROM ~~~~~~~~
   */
  system_update_cpu_freq(160);// set cpu frequency to 160MHz
  ESP.wdtDisable();
  ESP.wdtEnable(15000);// set wdt every 15s, call ESP.wdtFeed() to keep running
  Serial.begin(115200);// start serial
  
  //clearing eeprom at first start to confirm all vars-value = default-value and email = ""
  clearing_eeprom_at_first_start(); 

  restart_s_air_1time();

  //setup sensor
  led_yellow.setup();
  led_green.setup();
  sensors.begin();
  sensors.setResolution(11); // 9 bit  0.5 degrees C 93.75 mSec, 10 bit  0.25 degrees C  187.5 mSec, 11 bit  0.125 degrees C 375 mSec, 12 bit  0.0625 degrees C  750 mSec
  rmt.setup();
  set_hotspot_name();
  server.on("/", web_home_main); // setting up http-page
  server.on("/wifi_setting", wifi_setting);
  server.on("/reset_all_setting", reset_all_setting);
  //server.on("/show_status", web_show_status);
  server.onNotFound(web_page_not_found);

  // Show configuration
  Serial.print("hotspot_name:");Serial.println(hotspot_name);
  Serial.print("mac_name:");Serial.println(mac_name);

  
  //warning user for restarting by turning all leds on for 3s
  reset_or_not();

  //all led off
  led_yellow.lo();
  led_green.lo();

  //starting http-server
  //close_web(); 
  //delay(1000);
  start_web();
  //led_blink_together(ledYellowPin, ledGreenPin, 1); // Blink both LED once
  delay(10);

  //return last update parameter from EEPROM, traing to return vars every time after restart
  returning_vars_from_eeprom();

  //set use-usb every time, cause this version use direct AC220V-power source
  //mem.use_usb = 1;
  //set test
  test_config();

//  // Check WiFi connection
//  if(WiFi.status() != WL_CONNECTED){
//    // WiFi connecting
//    // input SSID and password for connecting wifi
//    Serial.println(F("Connecting to WiFi"));
//    if(wifi_connect()){
//      led_blink_together(ledYellowPin, ledGreenPin, 3); // Blink both LED once
//    }
//  }
//  ESP.wdtFeed();
//  
//  // Check internet connection
//  if(Ping.ping("www.google.com")){
//    Serial.println(F("Internet connected already!"));
//    ESP.wdtFeed();
    
  // Check WiFi connection and Connect to MQTT
  if(check_internet_and_connect_MQTT()){
    led_blink_together(ledYellowPin, ledGreenPin, 3); // Blink both LED once
    String comma = F(",");
    String send_at_start = String(rmt.getNum()) + comma + String(mem.bl_log.finish) + comma + String(firmware_version) + comma + String(firmware_build)+ comma + String(firmware_release);
    //MQTTtrigger_log("start," + String(rmt.rmt_detected) + "," + String(mem.bl_log.finish));  // Sending info to server
    MQTTtrigger_log(F("start"), send_at_start);  // Sending info to server
  }


  //running http-server
  server.handleClient();

}

/*******************************************************************************
 * void loop
 * there are 2 importent parts 
 * 1. operating 
 * 2. check IR
 * ******************************************************************************
 */
void loop(){//20190121
  // v2.1: Feed watchdog at start of loop to prevent random reboots
  ESP.wdtFeed();

  static uint16_t count_main_loop= 0;// marker for loop_cycle

  //if not intitial (state != 0), then calling state-function and another function every 20 seconds
  //if (((mem.next_op_tstamp - system_get_rtc_time() > 3294967295) || (system_get_rtc_time() > mem.next_op_tstamp)) && (mem.state != 0)){
  if (((mem.next_op_tstamp - system_get_rtc_time() > 3294967295) || (system_get_rtc_time() > mem.next_op_tstamp)) && (mem.state != 0)){ // every 3 seconds
    
    
    //calling state-function
    state_function();

    //setting time for next loop
    mem.next_op_tstamp = system_get_rtc_time() + sleep_period;     

    //logging esaving-data
    //data_logger(); 

   //saving vars in eeprom: Temperature, current remote status, will be restored after next restart
   //saving_vars_to_eeprom();

    //reseting http-server
//    if (mem.state != 0){
//      if (mem.user_need_email){
//        //reset_web_every_one_hr();
//        Serial.println("Next hour"); // only for #testing
//      }
//    }
    Serial.print("count_main_loop: "); Serial.println(count_main_loop); // only for #testing
    control_MQTTsending_webapp_loop();     // Check loop_cnt and run trigger once every one_period for sending MQTT

    show_op_status(); //showing op_status via serial-monitor
    // Test sending IR
    //rmt.cut(); // only for #testing
    MQTTtrigger_log(F("testing-main_loop"),String(count_main_loop));  // testing only

    count_main_loop++;
    //Serial.print(get_t_from_internet());
    //send_email();  
  }

  //if state = 0, then calling state-function only and suddenly
  else if (mem.state == 0){
    state_function();     //calling state-function
  }

  if (mem.access_point_on){   //running http-server
    server.handleClient();
  }

  //If found IR signal then process in check_ir and report over MQTT in MQTTtrigger_get_ir
  if(rmt.check_ir()){
    // v2.1: Throttle EEPROM writes to prevent hardware degradation
    // Only write once per hour instead of every IR signal
    static uint32_t last_eeprom_write = 0;
    const uint32_t EEPROM_WRITE_INTERVAL = 3600000;  // 1 hour in milliseconds

    if (millis() - last_eeprom_write > EEPROM_WRITE_INTERVAL) {
      saving_vars_to_eeprom(); //saving vars in eeprom: Temperature, current remote status
      last_eeprom_write = millis();
      if (show_data) Serial.println(F("[EEPROM] State saved"));
    }
    MQTTtrigger_get_ir();

  }
  led_yellow.run(); // Run yellow LED
  led_green.run(); // Run green LED

  //if remote was detected and state != 0, then calling state-control and increase next loop time for 20s
  if (rmt.rmt_detected && (mem.state != 0)){
    rmt.rmt_detected = 0;
    rmt_state_control();
    mem.next_op_tstamp = system_get_rtc_time() + sleep_period;
  }
 
  delay(1);   //running delay(1) hier to prevent wdt error
}

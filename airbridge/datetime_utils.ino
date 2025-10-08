/*****************************************************************************************************************************************************
 * all function doing about time
 * 1. get_time_from_internet() will use ESP8266 to connect internet and load unix-time, if it' success and log_stack >= bl_duration do not disconnect
 * ***************************************************************************************************************************************************
 */

//int get_yr(time_t *unix_t){
//  struct tm* p_tm = localtime(unix_t);
//  return(p_tm->tm_year) + 1900;
//}

//int get_mo(time_t *unix_t){
//  struct tm* p_tm = localtime(unix_t);
//  return(p_tm->tm_mon) + 1;
//}

int get_dy(time_t *unix_t){
  struct tm* p_tm = localtime(unix_t);
  return(p_tm->tm_mday);
}

int get_hr(time_t *unix_t){
  struct tm* p_tm = localtime(unix_t);
  return(p_tm->tm_hour);
}

int get_mi(time_t *unix_t){
  struct tm* p_tm = localtime(unix_t);
  return(p_tm->tm_min);
}

//double get_t_from_internet(){//change date 2018.09.21, 15:45, Germany
//  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
//  Serial.print(F("getting date&time from internet by using"));
//  /*
//   * set station mode
//   */
//  
////  for (i = 0; i < 3; i++){
////    if (wifi_set_opmode(0x01)){
////      break;
////    }
////    if (i == 2){
////      //Serial.println(F("connot set STA-Mode! -> return"));
////      //WiFi.disconnect();
////      while (WiFi.status() == WL_CONNECTED){
////        delay(100);
////      }
////      delay(100);
////      //WiFi.forceSleepBegin();
////      delay(1);
////      return 0;
////    }
////    delay(2000);
////  }
//
//  /*
//   * load ssid from EEPROM
//   */
//  //String ssid = mem.read_eeprom_string(mem_ssid_log_pos[0], mem_ssid_log_pos[1]);
//  //String password = mem.read_eeprom_string(mem_pass_log_pos[0], mem_pass_log_pos[1]);
//  //String time_zone = mem.read_eeprom_string(mem_time_zone_log_pos[0], mem_time_zone_log_pos[1]);
//  String time_zone = default_time_zone;
////  mem.eeprom_clear(mem_time_zone_log_pos[0], mem_time_zone_log_pos[1]);
////  mem.eeprom_confirm();
////  mem.write_eeprom_string(mem_time_zone_log_pos[0], mem_time_zone_log_pos[1], time_zone.c_str());
////  mem.eeprom_confirm();
//  
//  // Example: ssid = "YourSSID", password = "YourPassword", e_mail = "your.email@example.com", time_zone = "7.0";
////  if (show_data){
////    Serial.println(F("get date&time from internet by using"));
////    Serial.print(F("ssid = ")); Serial.println(ssid);
////    Serial.print(F("password = ")); Serial.println(password);
////    Serial.print(F("time_zone = ")); Serial.println(time_zone);
////  }
//
//  /*
//   * connecting WiFi
//   */
////   wifi_connect(); // input SSID and password for connecting wifi
////  if (password.substring(0,1) != '\0'){
////    WiFi.begin(ssid.c_str(), password.c_str());
////  }
////  else{
////    WiFi.begin(ssid.c_str());
////  }
////  //Serial.println(F("Connecting"));
////  i = 0;
////  while (WiFi.status() != WL_CONNECTED){
////    delay(500);
////    Serial.print(F("."));
////    if ((i%20 == 0) && (i > 0)){
////      if (password.substring(0,1) != '\0'){
////        WiFi.begin(ssid.c_str(), password.c_str());
////      }
////      else{
////        WiFi.begin(ssid.c_str());
////      }
////      Serial.println(F("Connecting"));
////    }
////
////    if (i > 70){
////      Serial.println(F("cannot connect WiFi"));
////      WiFi.disconnect();
////      while (WiFi.status() == WL_CONNECTED){
////        delay(100);
////      }
////      delay(200);
////      WiFi.forceSleepBegin();
////      delay(1);
////      return 0;
////    }
////    ESP.wdtFeed();
////    i++;
////  }
////  Serial.print(F("Connected, IP address: "));Serial.println(WiFi.localIP());
//  
//  /*
//   * ping to check internet, very important
//   */
//  //Serial.println(F("Pinging host \"www.google.com\""));
////  if(Ping.ping("www.google.com")){
////    Serial.println(F("Success!!"));
////    //led_yellow.run();
////  } 
////  else{
////    //Serial.println(F("Error :("));
////    //WiFi.disconnect();
////    while (WiFi.status() == WL_CONNECTED){
////      delay(100);
////    }
////    delay(200);
////    //WiFi.forceSleepBegin();
////    delay(1);
////    return 0;
////  }
//
//  /*
//   * set time
//   */
//  configTime(time_zone.toFloat()*3600, 0, "time.nist.gov", "pool.ntp.org");
//  //Serial.println(F("Waiting for time"));
//  for (i = 0; i < 2; i++){
//    if (time(nullptr)){
//      break;
//    }
//    //Serial.print(".");
//    delay(200);
//    ESP.wdtFeed();
//  }
//  time_t now = time(nullptr);
//  for (i = 0; i < 40; i++){
//    now = time(nullptr);
//    if (show_data){
//      Serial.println(ctime(&now));
//      Serial.print(F("unix_time = "));Serial.println(now);
//    }
//    if (double(now) > 1526774400){
//      break;
//    }
//    delay(500);
//    if (i == 10){
//      configTime(time_zone.toFloat()*3600, 0, "pool.ntp.org", "time.nist.gov");
//    }
//    ESP.wdtFeed();
//  }
//  //digitalWrite(14, LOW);
//  if (double(now) > 1526774400){
//    return double(now);
//  }
//  else{
//    if (show_data){
//      Serial.println(F("connot get time, need to try again next time"));
//    }
//    return 0;
//  }
//}

/*
 * Using t_from_internet
 */
//double t_from_internet = 0; 
//t_from_internet = get_t_from_internet();
//Serial.print(F("t_from_internet"));Serial.println(t_from_internet);
//if (t_from_internet > 1526774400){
//mem.last_tstamp = time_t(t_from_internet);
//if (get_mi(&mem.last_tstamp) > 30){
//  mem.last_tstamp = mem.last_tstamp + time_t(3600);
//}
//t_from_internet = t_from_internet - time_t(3600*mem_bl_length);

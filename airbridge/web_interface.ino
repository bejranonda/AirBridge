//const String html_header= "<html><head><title>AirBridge: Wi-Fi setting</title></head> <center><h2>AirBridge<br></h2></center>";

void web_home_main(){
//  if (mem.state == 0){
    home_setting();
//  }
//  else{
//    home_full();
//  } 
//  else if (mem.user_need_email){
//    home_full();
//  }  
//  else{
//    home_lite();
//  }
}

void start_web(){
  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  
  /*
   * set WiFi to dual mode
   */
  WiFi.mode(WIFI_OFF);
  for (i = 0; i < 3; i++){
    if (wifi_set_opmode(0x03)){
      break;
    }
    if (i == 2){
      if (show_data){
        Serial.println(F("connot start Webserver! --> return \r\n"));
      }
      mem.state = 1;
      return;
    }
    delay(2000);
  }
  xxtea.setKey(s_air_key);
  String result = xxtea.encrypt(String(hotspot_name).substring(6));
  result.toLowerCase();
  if (show_data){
    Serial.println("AP started // password> "+result);
  }
//  while (!MDNS.begin(s_air_domain)) { //set domain
//  }
  //WiFi.softAPConfig(local_IP, local_IP, subnet);
  WiFi.softAP(hotspot_name, result.c_str());
  server.begin();
  mem.access_point_on = 1;
}

void close_web(){
  if (show_data){
    Serial.println("Stop AP");
  }
  server.stop(); // Stop server
  WiFi.softAPdisconnect (true); // Stop access point
//  WiFi.disconnect();
//  while (WiFi.status() == WL_CONNECTED){
//    delay(1000);
//  }
//  WiFi.forceSleepBegin();
  delay(1);
  mem.access_point_on = 0;
}

//void reset_web_every_one_hr(){
//  static uint8_t loop_cnt = 0;// marker for loop_cycle
//  if (!mem.access_point_on || (loop_cnt%one_hr == 0)){// reseting http-server every 1 hour
//    close_web();
//    start_web();
//    loop_cnt = 0;
//  }
//  loop_cnt++; //increase loop cnt
//}

//void control_MQTTsending_webapp_loop(){
//  static byte loop_cnt = 0;// marker for loop_cycle
//  Serial.print("loop_cnt:");Serial.println(loop_cnt);
//  if (loop_cnt%one_period == 0){// reseting http-server every 1 hour
//    MQTTtrigger_send_temperature();
//    loop_cnt = 0;
//  }
//  loop_cnt++; //increase loop cnt
//}
//
//
//void MQTTtrigger_send_temperature(){
//  // Publish temperature
//  String msg = String(mem.tc[2]) + "," + String(mem.tr[2]);
//  mqtt_publish("t",msg);
//}

// Connecting WiFi with existing SSID and password
boolean wifi_connect(){
  String ssid = mem.read_eeprom_string(mem_ssid_log_pos[0], mem_ssid_log_pos[1]);
  String password = mem.read_eeprom_string(mem_pass_log_pos[0], mem_pass_log_pos[1]);
  WiFi.hostname(hotspot_name);
  // v2.1: Fixed string comparison (was comparing to char instead of string)
  if (password.length() > 0 && password.substring(0,1) != ""){
    WiFi.begin(ssid.c_str(), password.c_str());
  }
  else{
    WiFi.begin(ssid.c_str());
  }
  if(ssid != ""){
    Serial.print(F("ssid:"));  Serial.println(ssid);
    Serial.print(F("password:"));  Serial.println(password);
    Serial.print(F("Email:"));  Serial.println(mem.read_eeprom_string(mem_email_log_pos[0], mem_email_log_pos[1]));
    int i = 0;
    while (WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(F("."));
      if ((i%20 == 0) && (i > 0)){
        // v2.1: Fixed string comparison
        if (password.length() > 0 && password.substring(0,1) != ""){
          WiFi.begin(ssid.c_str(), password.c_str());
        }
        else{
          WiFi.begin(ssid.c_str());
        }
        Serial.println(F("Connecting"));
      }
  
      if (i > 70){
        Serial.println(F("cannot connect WiFi"));
        //WiFi.disconnect();
  //        while (WiFi.status() == WL_CONNECTED){
  //        delay(100);
  //      }
  //      delay(200);
        //WiFi.forceSleepBegin();
  //      delay(1);
        return false;
      }
      ESP.wdtFeed();
      i++;
    }
    Serial.print(F("Connected, IP address: "));Serial.println(WiFi.localIP());
    return true;
  }else{
    Serial.println(F("No ssid"));
    return false;
  }
}

boolean check_ssid_available(String ssid_tocheck){
  int n = WiFi.scanNetworks();
  //ssid_tocheck += "\n";
  //ssid_tocheck = String(ssid_tocheck. // convert the original array of char with fix length 32, to string with exact String, then give as output;
  //Serial.print(F("Checking:"));Serial.println(ssid_tocheck);
  //Serial.print(F("ssid_tocheck Length:"));Serial.println(ssid_tocheck.length());
  if (n == 0){
    Serial.println(F("no wifi around"));
    return false;
  }else{
    //Serial.print(n); Serial.println(F(" networks found"));
    for (int i = 0; i < n; ++i)
    {
      String ssid_found = String(WiFi.SSID(i));
      ssid_found.trim();
      //Serial.print(ssid_found);
      //Serial.print(F(" Length:"));Serial.println(ssid_found.length());
      if(ssid_tocheck == ssid_found){ //enter the ssid which you want to search
        Serial.println(F("SSID found"));
        return true;
      }
    }
  }
  // v2.1: Added missing return statement
  return false;
}

// Check internet and connecting WiFi with existing SSID and password
boolean check_internet_and_connect_MQTT(){
  boolean connected_already = true;
  //Serial.println(F("check_internet_and_connect_MQTT"));
  // Check WiFi connection, or tried MQTT connection with WiFi for xx already, but not succesful then connnect WiFi again
  if(((WiFi.status() != WL_CONNECTED) || (mem.count_warning_sys_mqtt >= count_warning_sys_mqtt_to_reconnect_wifi))){
    // check wheter SSID available
    if(check_ssid_available(mem.read_eeprom_string(mem_ssid_log_pos[0], mem_ssid_log_pos[1]))){

      if(mem.count_warning_sys_mqtt >= count_warning_sys_mqtt_to_reconnect_wifi){
        mem.count_warning_sys_mqtt = 0; // reset counting error from MQTT
      }
      
      Serial.println(F("Connecting to WiFi"));        
      // WiFi connecting
      // input SSID and password for connecting wifi
      if(!wifi_connect()){
        connected_already = false;
        mem.trigging_warning_sys_ssid = true;
      }else{
        //connected_already = true; // already set at begining
        mem.trigging_warning_sys_ssid = false;      
      }
    }else{
      Serial.println(F("your WiFi-SSID not found"));
      mem.trigging_warning_sys_ssid = true;
      return false;
    }
  }

  
  // Check internet connection
//  if(Ping.ping("www.google.com")){
//    Serial.println(F("Internet connected already!"));
//    ESP.wdtFeed();
//  }

  ESP.wdtFeed();
  // Connect to MQTT
  if(connected_already){
    if(mqtt_set_all()){
      if(MQTTtrigger_log(F("wifi"),F("connected"))){
        // If fail to send info to server
        Serial.println(F("MQTT OK"));
        mem.trigging_warning_sys_mqtt = false;
        mem.count_warning_sys_mqtt = 0;
        return true;
      }else{
        //Serial.println(F("Connected to MQTT"));
        mem.trigging_warning_sys_mqtt = true;
        mem.count_warning_sys_mqtt++;
        Serial.println(F("MQTT error"));
        return false;
      }
    }
    Serial.println(F("MQTT error"));
    return false;
  }
  Serial.println(F("WiFi error"));
  return false;
}


//void web_show_status(){
//  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
//  String txt_tmp = "";
//  txt_tmp = F("<html><head><title>S-Air-show status</title></head><p>--- All Status ---<p><meta http-equiv=\"refresh\" content=\"20; url=/show_status\"<p>state = ");
//  txt_tmp += String(mem.state);
//  switch (mem.state){
//    case 0:
//      txt_tmp += F("-web install<p>"); break;
//    case 1:
//      txt_tmp += F("-do notthing<p>"); break;
//    case 2:
//      txt_tmp += F("-init coil<p>"); break;
//    case 3:
//      txt_tmp += F("-pre cut<p>"); break;
//    case 4:
//      txt_tmp += F("-cut<p>"); break;
//    case 5:
//      txt_tmp += F("-confirm cut<p>"); break;
//    case 6:
//      txt_tmp += F("-cool<p>"); break;
//    case 7:
//      txt_tmp += F("-confirm cool<p>"); break;
//    case 8:
//      txt_tmp += F("-log base line<p>"); break;
//    case 9:
//      txt_tmp += F("-operating with out tempsensors<p>"); break;        
//    default:
//      break;  
//  }    
//  if ((mem.state == 3) || ((mem.state == 9) && (mem.loop_cnt_considering_cut < pre_smart_cut_period*one_mi))){
//    txt_tmp += F("<p>pre cut time left ");txt_tmp += String((pre_smart_cut_period*one_mi - mem.loop_cnt_considering_cut)/one_hr) + "h" + String(((pre_smart_cut_period*one_mi - mem.loop_cnt_considering_cut)%one_hr)/one_mi) + "m" + String(((pre_smart_cut_period*one_mi - mem.loop_cnt_considering_cut)%one_mi)*20) + "s<p>";
//  }
//  else if (mem.state == 8){
//    txt_tmp += F("<p>log base line time left ");txt_tmp += String((mem_bl_length*one_hr - mem.state_cnt)/one_hr) + "h" + String(((mem_bl_length*one_hr - mem.state_cnt)%one_hr)/one_mi) + "m" + String(((mem_bl_length*one_hr - mem.state_cnt)%one_mi)*20) + "s<p>";
//    txt_tmp += F("<p>bl_log.duty[] = |");
//    for (i = 0; i < mem_bl_length; i++){
//      txt_tmp += String(mem.bl_log.duty[i]) + "|";
//    }
//    txt_tmp += F("<p><p>bl_log.tr_set[] = |");
//    for (i = 0; i < mem_bl_length; i++){
//      txt_tmp += String(mem.bl_log.trset[i]) + "|";
//    }
//    txt_tmp += F("<p>");
//  }
//  txt_tmp += F("<p>state_time = ");txt_tmp += String(mem.state_cnt/one_hr) + "h" + String((mem.state_cnt%one_hr)/one_mi) + "m" + String((mem.state_cnt%one_mi)*20) + "s<p>";
//  txt_tmp += F("<p>loop_cnt_considering_cutime = ");txt_tmp += String(mem.loop_cnt_considering_cut/one_hr) + "h" + String((mem.loop_cnt_considering_cut%one_hr)/one_mi) + "m" + String((mem.loop_cnt_considering_cut%one_mi)*20) + "s<p>";
//  txt_tmp += F("<p>remote = ");
//  switch (rmt.getNum()){
//    case 101:
//      txt_tmp += F("carrier_toshiba_96"); break;
//    case 102:
//      txt_tmp += F("carrier_toshiba_144"); break;
//    case 103:
//      txt_tmp += F("carrier_toshiba_168"); break;
//    case 201:
//      txt_tmp += F("daikin_64"); break;
//    case 202:
//      txt_tmp += F("daikin_216"); break;
//    case 203:
//      txt_tmp += F("daikin_280"); break;
//    case 301:
//      txt_tmp += F("haier_112"); break;    
//    case 401:
//      txt_tmp += F("hitashi_104"); break; 
//    case 402:
//      txt_tmp += F("hitashi_264"); break;       
//    case 501:
//      txt_tmp += F("lg_28"); break;    
//    case 601:
//      txt_tmp += F("mitsubishi_48"); break;
//    case 602:
//      txt_tmp += F("mitsubishi_88"); break;    
//    case 603:
//      txt_tmp += F("mitsubishi_112"); break;
//    case 604:
//      txt_tmp += F("mitsubishi_136"); break;       
//    case 605:
//      txt_tmp += F("mitsubishi_288"); break;
//    case 701:
//      txt_tmp += F("panasonic_128"); break;    
//    case 702:
//      txt_tmp += F("panasonic_216"); break;
//    case 801:
//      txt_tmp += F("saijo_64"); break;
//    case 802:
//      txt_tmp += F("saijo_128"); break;        
//    case 901:
//      txt_tmp += F("samsung_56"); break;
//    case 902:
//      txt_tmp += F("samsung_112"); break;    
//    case 903:
//      txt_tmp += F("samsung_168"); break;
//    case 1001:
//      txt_tmp += F("sharp_96"); break;
//    case 1002:
//      txt_tmp += F("sharp_104"); break;        
//    case 1101:
//      txt_tmp += F("singer_"); break;
//    case 1201:
//      txt_tmp += F("turbocool_112"); break;
//    case 1401:
//      txt_tmp += F("universal_A"); break;
//    case 1402:
//      txt_tmp += F("universal_B"); break;
//    case 1403:
//      txt_tmp += F("universal_C"); break;  
//    default:
//      break;
//  }
//  txt_tmp += F("<p>");
//  txt_tmp += F("<p>remote_number = ");txt_tmp += String(rmt.getNum()) + "<p>"; 
//  txt_tmp += F("<p>power = ");
//  if (rmt.getPower()){
//    txt_tmp += F("on");
//  }
//  else{
//    txt_tmp += F("off");
//  }
//  txt_tmp += F("<p>");
//  txt_tmp += F("<p>mode = ");
//  if (rmt.getMode() == 0){
//    txt_tmp += F("auto");
//  }
//  else if (rmt.getMode() == 1){
//    txt_tmp += F("cool");
//  }
//  else if (rmt.getMode() == 2){
//    txt_tmp += F("dry");
//  }
//  else if (rmt.getMode() == 3){
//    txt_tmp += F("fan");
//  }
//  txt_tmp += F("<p>");
//  txt_tmp += F("<p>trset = ");txt_tmp += String(rmt.getTrset()) + "<p>";
//  txt_tmp += F("<p>fan_speed = ");
//  if (rmt.getFan() == 0){
//    txt_tmp += F("fan_auto");
//  }
//  else if (rmt.getFan() == 1){
//    txt_tmp += F("fan_1");
//  }
//  else if (rmt.getFan() == 2){
//    txt_tmp += F("fan_2");
//  }
//  else if (rmt.getFan() == 3){
//    txt_tmp += F("fan_3");
//  }
//  else if (rmt.getFan() == 4){
//    txt_tmp += F("fan_4");
//  }
//  else if (rmt.getFan() == 5){
//    txt_tmp += F("fan_5");
//  }
//  txt_tmp += F("<p>");
//  txt_tmp += F("<p>direction = ");
//  if (rmt.getVane() == 0){
//    txt_tmp += F("direction_auto");
//  }
//  else if (rmt.getVane() == 1){
//    txt_tmp += F("direction_1");
//  }
//  else if (rmt.getVane() == 2){
//    txt_tmp += F("direction_2");
//  }
//  else if (rmt.getVane() == 3){
//    txt_tmp += F("direction_3");
//  }
//  else if (rmt.getVane() == 4){
//    txt_tmp += F("direction_4");
//  }
//  else if (rmt.getVane() == 5){
//    txt_tmp += F("direction_5");
//  }
//  txt_tmp += F("<p>");
//  txt_tmp += F("<p>con = ");
//  if (mem.compressor_state[2]){
//    txt_tmp += "on " + String(mem.compressor_on_cnt/one_hr) + "h" + String((mem.compressor_on_cnt%one_hr)/one_mi) + "m" + String((mem.compressor_on_cnt%one_mi)*20) + "s<p>";
//  }
//  else{
//    txt_tmp += "off " + String(mem.compressor_off_cnt/one_hr) + "h" + String((mem.compressor_off_cnt%one_hr)/one_mi) + "m" + String((mem.compressor_off_cnt%one_mi)*20) + "s<p>";  
//  }
//  txt_tmp += F("<p>min_tc_to_detect_compressor_on = ");txt_tmp += String(mem.min_tc_to_detect_compressor_on) + "°C<p>";
//  txt_tmp += F("<p>total_cycle = ");txt_tmp += String(mem.total_cycle) + "<p>";
//  txt_tmp += F("<p>tsensors_select = ");txt_tmp += String(mem.tsensors_select) + "<p>";
//  txt_tmp += F("<p>free_memory = |");txt_tmp += String(system_get_free_heap_size()) + "Bytes|" + String((float(system_get_free_heap_size())/81920.0)*100) + "%|<p>";
//  txt_tmp += F("<p>system_adc_read() = |");txt_tmp += String(system_adc_read()) + "|" + String(float(system_adc_read()*4.3)/1024.0) + "V|<p>";
//  txt_tmp += F("<p>user_need_email = ");txt_tmp += String(mem.user_need_email) + "<p>";
//  //txt_tmp += "<p>use_usb_cnt = " + String(use_usb_cnt) + "<p>";
//  txt_tmp += F("<p>use_usb&access_point_on = |");txt_tmp += String(mem.use_usb) + "|" + String(mem.access_point_on) + "|<p>";
//  txt_tmp += F("<p>heavy_cnt = ");txt_tmp += String(mem.heavy_cnt) + "<p>";
//  txt_tmp += F("<p>dirty_cnt = ");txt_tmp += String(mem.dirty_cnt) + "<p>";
//  txt_tmp += F("<p>percent_to_cut = ");txt_tmp += String(mem.percent_to_cut*100.0) + String("%<p>");
//  txt_tmp += F("<p>tc[3] = |");txt_tmp += String(mem.tc[0]) + "|" + String(mem.tc[1]) + "|" + String(mem.tc[2]) + "|<p>";  
//  txt_tmp += F("<p>tr[3] = |");txt_tmp += String(mem.tr[0]) + "|" + String(mem.tr[1]) + "|" + String(mem.tr[2]) + "|<p>";
//  txt_tmp += F("<p>av_tc[3] = |");txt_tmp += String(mem.av_tc[0]) + "|" + String(mem.av_tc[1]) + "|" + String(mem.av_tc[2]) + "|<p>";  
//  txt_tmp += F("<p>av_tr[3] = |");txt_tmp += String(mem.av_tr[0]) + "|" + String(mem.av_tr[1]) + "|" + String(mem.av_tr[2]) + "|<p>";
//  txt_tmp += F("<p>min_tc[2] = |");txt_tmp += String(mem.min_tc[0]) + "|" + String(mem.min_tc[1]) + "|<p>";
//  txt_tmp += F("<p>min_tr[2] = |");txt_tmp += String(mem.min_tr[0]) + "|" + String(mem.min_tr[1]) + "|<p>";
//  txt_tmp += F("<p>slope_tc[6] = |");txt_tmp += String(mem.slope_tc[0]) + "|" + String(mem.slope_tc[1]) + "|" + String(mem.slope_tc[2]) + "|" + String(mem.slope_tc[3]) + "|" + String(mem.slope_tc[4]) + "|" + String(mem.slope_tc[5]) + "|<p>";  
//  txt_tmp += F("<p>min_slope_tc[3] = |");txt_tmp += String(mem.min_slope_tc[0]) + "|" + String(mem.min_slope_tc[1]) + "|" + String(mem.min_slope_tc[2]) + "|<p>";
//  txt_tmp += F("<p>&nbsp;</p><p>***<p><p>slope in (°C/minute)*100<p><p>if heavy_cnt >= 3, then incease trset<p><p>if dirty_cnt >= 30, then dirty air warning<p></html>");
//  server.send(200, "text/html", txt_tmp);
//}

/*
 * webserver that is like an email
 */
//void home_full(){
//  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
//  String txt_tmp = "";
//  txt_tmp += F("<html><head><title>AirBridge-Energy Saving Report</title></head>");
//  txt_tmp += F("<h1 style=\"text-align: center;\"><strong>Energy Saving Report ");
//  txt_tmp += String(hotspot_name);
//  txt_tmp += F("&nbsp;</strong></h1><p><strong>Date: ");
//  txt_tmp += ctime(&mem.last_tstamp);
//  txt_tmp += F("</strong></p><p><strong>Remote: ");
//  switch (rmt.getNum()){
//    case 101:
//      txt_tmp += F("carrier_toshiba_96"); break;
//    case 102:
//      txt_tmp += F("carrier_toshiba_144"); break;
//    case 103:
//      txt_tmp += F("carrier_toshiba_168"); break;
//    case 201:
//      txt_tmp += F("daikin_64"); break;
//    case 202:
//      txt_tmp += F("daikin_216"); break;
//    case 203:
//      txt_tmp += F("daikin_280"); break;
//    case 301:
//      txt_tmp += F("haier_112"); break;    
//    case 401:
//      txt_tmp += F("hitashi_104"); break; 
//    case 402:
//      txt_tmp += F("hitashi_264"); break;       
//    case 501:
//      txt_tmp += F("lg_28"); break;    
//    case 601:
//      txt_tmp += F("mitsubishi_48"); break;
//    case 602:
//      txt_tmp += F("mitsubishi_88"); break;    
//    case 603:
//      txt_tmp += F("mitsubishi_112"); break;
//    case 604:
//      txt_tmp += F("mitsubishi_136"); break;       
//    case 605:
//      txt_tmp += F("mitsubishi_288"); break;
//    case 701:
//      txt_tmp += F("panasonic_128"); break;    
//    case 702:
//      txt_tmp += F("panasonic_216"); break;
//    case 801:
//      txt_tmp += F("saijo_64"); break;
//    case 802:
//      txt_tmp += F("saijo_128"); break;        
//    case 901:
//      txt_tmp += F("samsung_56"); break;
//    case 902:
//      txt_tmp += F("samsung_112"); break;    
//    case 903:
//      txt_tmp += F("samsung_168"); break;
//    case 1001:
//      txt_tmp += F("sharp_96"); break;
//    case 1002:
//      txt_tmp += F("sharp_104"); break;        
//    case 1101:
//      txt_tmp += F("singer_"); break;
//    case 1201:
//      txt_tmp += F("turbocool_112"); break;
//    case 1401:
//      txt_tmp += F("universal_A"); break;
//    case 1402:
//      txt_tmp += F("universal_B"); break;
//    case 1403:
//      txt_tmp += F("universal_C"); break;  
//    default:
//      break;
//  } 
//  if (mem.esaving_log_full_stack > 0){
//    txt_tmp += F("<p><strong>last operating hour</strong></p>");
//    txt_tmp += F("<p>average room temperature = ");txt_tmp += String(get_av_tr_from_log()) + "&deg;C</p>";
//    txt_tmp += F("<table style=\"height: 72px;\" border=\"black\" width=\"515\"><tbody><tr>");
//    txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>year</strong></td>");
//    txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>mon</strong></td>");
//    txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>day</strong></td>");
//    txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>hour</strong></td>");
//    txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>op_duty</strong></td>");
//    txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>bl_duty</strong></td>");
//    txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>saving [%]</strong></td>");
//    txt_tmp += F("</tr>");
//    for (i = (24 - mem.esaving_log_full_stack); i < 24; i++){
//      txt_tmp += F("<tr>");
//      txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.hr_log_full[i].yr) + "</td>";
//      txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.hr_log_full[i].mo) + "</td>";
//      txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.hr_log_full[i].dy) + "</td>";
//      txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.hr_log_full[i].hr) + "</td>";
//      txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.hr_log_full[i].op_duty) + "</td>";
//      txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.hr_log_full[i].bl_duty) + "</td>";
//      txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.hr_log_full[i].esaving) + "</td>";
//      txt_tmp += F("</tr>");
//    }
//    txt_tmp += F("</tbody></table>");
//  }
//  txt_tmp += F("<p><strong>last 24h</strong></p><table style=\"height: 39px;\" border=\"black\" width=\"442\"><tbody><tr>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>year</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>mon</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>day</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>op_duty</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>bl_duty</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>saving [%]</strong></td></tr>");
//  time_t last_24hr_tstamp = mem.last_tstamp - time_t(86400);
//  if ((mem.dy_log_full[mem_dy_log_full_length - 1].yr == get_yr(&last_24hr_tstamp)) && (mem.dy_log_full[mem_dy_log_full_length - 1].mo == get_mo(&last_24hr_tstamp)) && (mem.dy_log_full[mem_dy_log_full_length - 1].dy == get_dy(&last_24hr_tstamp))){
//    txt_tmp += F("<tr>");
//    txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[mem_dy_log_full_length - 1].yr) + "</td>";
//    txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[mem_dy_log_full_length - 1].mo) + "</td>";
//    txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[mem_dy_log_full_length - 1].dy) + "</td>";
//    txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[mem_dy_log_full_length - 1].op_duty) + "</td>";
//    txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[mem_dy_log_full_length - 1].bl_duty) + "</td>";
//    txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[mem_dy_log_full_length - 1].esaving) + "</td>";
//    txt_tmp += F("</tr>");
//  }
//  else{
//    txt_tmp += F("<tr>");
//    txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(get_yr(&last_24hr_tstamp)) + "</td>";
//    txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(get_mo(&last_24hr_tstamp)) + "</td>";
//    txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(get_dy(&last_24hr_tstamp)) + "</td>";
//    txt_tmp += F("<td style=\"width: 67px;\">-</td><td style=\"width: 67px;\">-</td><td style=\"width: 67px;\">-</td></tr>");
//  }
//  txt_tmp += F("</tbody></table><p><strong>last 7 day</strong></p><table style=\"height: 47px;\" border=\"black\" width=\"442\"><tbody><tr>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>year</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>mon</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>day</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>op_duty</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>bl_duty</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>saving [%]</strong></td></tr>");
//  time_t last_7dy_tstamp = mem.last_tstamp - time_t(86400*7);
//  for (i = 0; i < 7; i++){
//    for (j = mem_dy_log_full_length - 1; j >= mem_dy_log_full_length - 8; j--){
//      if ((mem.dy_log_full[j].yr == get_yr(&last_7dy_tstamp)) && (mem.dy_log_full[j].mo == get_mo(&last_7dy_tstamp)) && (mem.dy_log_full[j].dy == get_dy(&last_7dy_tstamp))){
//        txt_tmp += F("<tr>");
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[j].yr) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[j].mo) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[j].dy) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[j].op_duty) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[j].bl_duty) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.dy_log_full[j].esaving) + "</td>";
//        txt_tmp += F("</tr>");
//        break;
//      }
//      if (j == mem_dy_log_full_length - 8){
//        txt_tmp += F("<tr>");
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(get_yr(&last_7dy_tstamp)) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(get_mo(&last_7dy_tstamp)) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(get_dy(&last_7dy_tstamp)) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">-</td><td style=\"width: 67px;\">-</td><td style=\"width: 67px;\">-</td></tr>");
//      }
//    }
//    last_7dy_tstamp = last_7dy_tstamp + time_t(86400);
//  }
//  txt_tmp += F("</tbody></table><p><strong>last 6 month</strong></p><table style=\"height: 41px;\" border=\"black\" width=\"369\"><tbody><tr>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>year</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>mon</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>op_duty</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>bl_duty</strong></td>");
//  txt_tmp += F("<td style=\"width: 67px; text-align: center;\"><strong>saving [%]</strong></td></tr>");
//  int8_t last_6mo = get_mo(&mem.last_tstamp) - 6;
//  int16_t yr_temp = 0;
//  if (last_6mo <= 0){
//    last_6mo = 12 + last_6mo;
//    yr_temp = get_yr(&mem.last_tstamp) - 1;
//  }
//  else{
//    yr_temp = get_yr(&mem.last_tstamp);
//  }
//  for (i = 0; i < 6; i++){
//    for (j = mem_mo_log_full_length - 1; j >= mem_mo_log_full_length - 6; j--){
//      if ((mem.mo_log_full[j].mo == last_6mo) && (mem.mo_log_full[j].yr == yr_temp)){
//        txt_tmp += F("<tr>");
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.mo_log_full[j].yr) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.mo_log_full[j].mo) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.mo_log_full[j].op_duty) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.mo_log_full[j].bl_duty) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(mem.mo_log_full[j].esaving) + "</td>";
//        txt_tmp += F("</tr>");
//        break;
//      }
//      if ((mem.mo_log_full[mem_mo_log_full_length - 6].mo != last_6mo) && (j == mem_mo_log_full_length - 6)){
//        txt_tmp += F("<tr>");
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(yr_temp) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">");txt_tmp += String(last_6mo) + "</td>";
//        txt_tmp += F("<td style=\"width: 67px;\">-</td><td style=\"width: 67px;\">-</td><td style=\"width: 67px;\">-</td></tr>");
//      }
//    }
//    last_6mo++;
//    if (last_6mo > 12){
//      yr_temp++;
//      last_6mo = last_6mo%12;
//    }
//  }
//  txt_tmp += F("</tbody></table></html>");  
//  server.send(200, "text/html", txt_tmp);
//}

/*
 * webserver only saving
 */
/* Root page for the webserver */
//void home_lite(){
//  String txt_tmp = "";
//  txt_tmp += F("<html><head><title>AirBridge-Energy Saving Report</title></head><p style=\"text-align: center;\"><strong>AirBridge-Energy Saving Report</strong></p>");
//  txt_tmp += F("<p>&nbsp;</p><p>booking rate 100%</p><table style=\"height: 64px;\" border=\"black\" width=\"412\"><tbody><tr>");
//  txt_tmp += F("<td style=\"width: 95px; text-align: center;\">last 7 days</td><td style=\"width: 97px; text-align: center;\">last month</td>");
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">last 6 months</td></tr><tr>");
//  txt_tmp += F("<td style=\"width: 95px; text-align: center;\">");txt_tmp += String(get_av_esaving_lite("7d", 0)) + "%</td>";
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">");txt_tmp += String(mem.mo_log_lite[0].esaving[mem_mo_log_lite_length - 1]) + "%</td>";
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">");txt_tmp += String(get_av_esaving_lite("6m", 0)) + "%</td>";
//  txt_tmp += F("</tr></tbody></table><p>&nbsp;</p><p>booking rate 80%</p><table style=\"height: 64px;\" border=\"black\" width=\"412\">");
//  txt_tmp += F("<tbody><tr><td style=\"width: 95px; text-align: center;\">last 7 days</td><td style=\"width: 97px; text-align: center;\">last month</td>");
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">last 6 months</td></tr><tr>");
//  txt_tmp += F("<td style=\"width: 95px; text-align: center;\">");txt_tmp += String(get_av_esaving_lite("7d", 1)) + "%</td>";
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">");txt_tmp += String(mem.mo_log_lite[1].esaving[mem_mo_log_lite_length - 1]) + "%</td>";
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">");txt_tmp += String(get_av_esaving_lite("6m", 1)) + "%</td>";
//  txt_tmp += F("</tr></tbody></table><p>&nbsp;</p><p>booking rate 60%</p><table style=\"height: 64px;\" border=\"black\" width=\"412\"><tbody>");
//  txt_tmp += F("<tr><td style=\"width: 95px; text-align: center;\">last 7 days</td><td style=\"width: 97px; text-align: center;\">last month</td>");
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">last 6 months</td></tr><tr>");
//  txt_tmp += F("<td style=\"width: 95px; text-align: center;\">");txt_tmp += String(get_av_esaving_lite("7d", 2)) + "%</td>";
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">");txt_tmp += String(mem.mo_log_lite[2].esaving[mem_mo_log_lite_length - 1]) + "%</td>";
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">");txt_tmp += String(get_av_esaving_lite("6m", 2)) + "%</td>";
//  txt_tmp += F("</tr></tbody></table><p>&nbsp;</p><p>booking rate 40%</p><table style=\"height: 64px;\" border=\"black\" width=\"412\">");
//  txt_tmp += F("<tbody><tr><td style=\"width: 95px; text-align: center;\">last 7 days</td><td style=\"width: 97px; text-align: center;\">last month</td>");
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">last 6 months</td></tr><tr>");
//  txt_tmp += F("<td style=\"width: 95px; text-align: center;\">");txt_tmp += String(get_av_esaving_lite("7d", 3)) + "%</td>";
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">");txt_tmp += String(mem.mo_log_lite[3].esaving[mem_mo_log_lite_length - 1]) + "%</td>";
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">");txt_tmp += String(get_av_esaving_lite("6m", 3)) + "%</td>";
//  txt_tmp += F("</tr></tbody></table><p>&nbsp;</p><p>booking rate 20%</p><table style=\"height: 64px;\" border=\"black\" width=\"412\">");
//  txt_tmp += F("<tbody><tr><td style=\"width: 95px; text-align: center;\">last 7 days</td><td style=\"width: 97px; text-align: center;\">last month</td>");
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">last 6 months</td></tr><tr>");
//  txt_tmp += F("<td style=\"width: 95px; text-align: center;\">");txt_tmp += String(get_av_esaving_lite("7d", 4)) + "%</td>";
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">");txt_tmp += String(mem.mo_log_lite[4].esaving[mem_mo_log_lite_length - 1]) + "%</td>";
//  txt_tmp += F("<td style=\"width: 97px; text-align: center;\">");txt_tmp += String(get_av_esaving_lite("6m", 4)) + "%</td>";
//  txt_tmp += F("</tr></tbody></table><p>&nbsp;</p><p>* the calculating is based on using of air-condition 8 hours per day.&nbsp;</p></html>");  
//  server.send(200, "text/html", txt_tmp);
//}

/*
 * this function sorting ssid for webserver
 */
void web_install_sort_ssid(int32_t a[], int b[], int size){
  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  for (i = 0; i < (size - 1); i++){
    for ( y = 0; y < (size - (i + 1)); y++){
      if (a[y] < a[y + 1]){
      // sorting _RSSI
      x = a[y];
      a[y] = a[y + 1];
      a[y + 1] = x;
      // sorting ssid
      z = b[y];
      b[y] = b[y + 1];
      b[y + 1] = z;
      }
    }
  }
}

/* Root page for the webserver */
void home_setting(){
  String txt_tmp = "";
  txt_tmp = F("<html><head><title>AirBridge: Main</title></head><body style=\"background-color:#7FDBFF;\"><h2 style=\"text-align: center;\">AirBridge<br></h2><h1 style=\"text-align: center;\">Main menu</h1><br><br><p style=\"text-align: center;font-size:36px\"><a title=\"AirBridge Setting\" href=\"wifi_setting\">Wi-Fi setting</a><br><br><br><a title=\"Reset all Setting\" href=\"reset_all_setting\">Reset all Setting</a></p></body>");
  server.send(200, "text/html", txt_tmp);
}

//void wifi_setup_result(boolean wifi_connected){
//  String txt_tmp = "";
//  txt_tmp = F("<html><head><title>AirBridge: Wi-Fi setting</title></head><body> <center><h2 style=\"text-align: center;\">AirBridge<br></h2><h1 style=\"text-align: center;\">");
//  if(wifi_connected){
//    txt_tmp += F("<font color=\"blue\">Congratulation, Wi-Fi is connnected!</font></h1></center><meta http-equiv=\"refresh\" content=\"5; url=/wifi_setting\"</body></html>");
//  }else{
//    txt_tmp += F("<font color=\"red\">SSID or Password is incorrect, please check again.</font></h1></center><meta http-equiv=\"refresh\" content=\"5; url=/wifi_setting\"</body></html>");
//  } 
//  server.send(200, "text/html", txt_tmp);
//}

/*
 * wifi_setting
 */
void wifi_setting(){
  static boolean trying_wifi = false;// marker for trying to connect wifi
  static boolean is_wifi_connect = false;// marker for result of connecting wifi
  static boolean has_email = false;// marker for email giving by wifi_setting
  
  //int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  const String html_header = F("<html><head><title>AirBridge: Wi-Fi setting</title></head><body style=\"background-color:#7FDBFF;\"><center><h2>AirBridge<br></h2></center>");
  String txt_tmp = "";

  // Check if there are any GET parameters
  
//  if (server.hasArg("DropDownTimezone")){
//    Serial.print(F("time_zone -> "));Serial.println(server.arg("DropDownTimezone"));
//    mem.eeprom_clear(mem_time_zone_log_pos[0], mem_time_zone_log_pos[1]);
//    mem.eeprom_confirm();
//    mem.write_eeprom_string(mem_time_zone_log_pos[0], mem_time_zone_log_pos[1], String(server.arg("DropDownTimezone").c_str()));
//    mem.eeprom_confirm();
//  }

  ///////////////////
  // if trying to connect wifi in last call
  if(trying_wifi){
    //txt_tmp = F("<html><head><title>AirBridge: Wi-Fi setting</title></head><body> <center><h2 style=\"text-align: center;\">AirBridge<br></h2><h1 style=\"text-align: center;\"><font color=\"blue\">Congratulation, Wi-Fi is connnected!</font></h1></center><meta http-equiv=\"refresh\" content=\"5; url=/wifi_setting\"</body></html>");
    //txt_tmp += F("<h1 style=\"text-align: center;\"><font color=\"blue\">Congratulation, Wi-Fi is connnected!</font></h1></center><meta http-equiv=\"refresh\" content=\"5; url=/wifi_setting\"</body></html>");
    //server.send(200, "text/html", txt_tmp);
    Serial.println(F("trying_wifi"));
    txt_tmp = html_header + "<h1><center>";
    if(is_wifi_connect){
      Serial.println(F("Showing: SSID ok"));     
      txt_tmp += F("<font color=\"blue\">Congratulation, Wi-Fi is connnected!</font></h1><h1><center>");
      
      if(has_email){
        txt_tmp += F("<mark>");
        txt_tmp += mem.read_eeprom_string(mem_email_log_pos[0], mem_email_log_pos[1]);
        txt_tmp += F("</mark> is recoreded");
      }else{
        txt_tmp += F("<font color=\"OrangeRed\"><i>E-mail is incorrect, no record!</i></font>");       
      }
      txt_tmp += F("</center>");
      
    }else{
      Serial.println(F("Showing: SSID error"));
      txt_tmp += F("<font color=\"red\">SSID or Password is incorrect!</font>");
    }
    txt_tmp += F("</h1></center><meta http-equiv=\"refresh\" content=\"5; url=/wifi_setting\"/></body></html>");
    Serial.println(F("Server.send: connection results"));
    server.send(200, "text/html", txt_tmp);
        
    // Reset parameter of trying connecting wifi
    trying_wifi = false;// marker for trying to connect wifi
    is_wifi_connect = false;// marker for result of connecting wifi
    has_email = false;// marker for email giving by wifi_setting
  }

  //////////////////////
  // Checking value submission from WiFi-Setup
  else if ((String(server.arg("ssid")).length() <= 32) && (String(server.arg("password")).length() <= 32)){
    Serial.println(F("value submission"));
    if (server.hasArg("ssid")){ 
      txt_tmp = html_header;
      //txt_tmp = F("<html><head><title>AirBridge: Wi-Fi setting</title></head><body> <center><h2>AirBridge<br></h2><h1>Connecting Wi-Fi, please wait..</h1></center><meta http-equiv=\"refresh\" content=\"15; url=/wifi_setting\"</body></html>");
      txt_tmp += F("<h1>Connecting Wi-Fi, please wait..</h1></center><meta http-equiv=\"refresh\" content=\"15; url=/wifi_setting\"/></body></html>");
      //txt_tmp += F("<h1>Connecting Wi-Fi, please wait..</h1></center></body></html>");
      server.send(200, "text/html", txt_tmp);
      wifi_station_disconnect();
      delay(500);
      Serial.print(F("ssid:"));Serial.println(server.arg("ssid"));
      Serial.print(F("password:"));Serial.println(server.arg("password"));
      Serial.print(F("email:"));Serial.println(server.arg("email"));
      if (server.hasArg("password")){
        WiFi.begin(server.arg("ssid").c_str(), server.arg("password").c_str());      
      }
      else{
        WiFi.begin(server.arg("ssid").c_str());
      }
      trying_wifi = true; // trying to connect wifi
      uint32_t time_3 = millis() + 13500;
      Serial.println(F("connecting to wifi"));
      while ((wifi_station_get_connect_status() != 5) && (millis() < time_3)){
        delay(100);
        Serial.print(F("."));
      } 
           
      if (wifi_station_get_connect_status() == 5){
        Serial.println(F("WiFi connected"));  
        Serial.print(F("IP address = "));Serial.println(WiFi.localIP());
        Serial.println(F("clear EEPROM")); 
        mem.eeprom_clear(mem_ssid_log_pos[0], mem_pass_log_pos[1]);
        mem.eeprom_confirm();
        is_wifi_connect = true;
        if (String(server.arg("password")).length() == 0){//--> saving ssid only
          Serial.println(F("saving ssid and email"));
          Serial.println(server.arg("ssid"));
          Serial.println(server.arg("email"));
          mem.write_eeprom_string(mem_ssid_log_pos[0], mem_ssid_log_pos[1], String(server.arg("ssid").c_str()));
          mem.eeprom_confirm();
        }
        //--> saving ssid & password
        else{
          Serial.println(F("saving ssid, password"));
          Serial.println(server.arg("ssid"));
          Serial.println(server.arg("password"));
          Serial.println(server.arg("email"));
          mem.write_eeprom_string(mem_ssid_log_pos[0], mem_ssid_log_pos[1], server.arg("ssid"));
          mem.write_eeprom_string(mem_pass_log_pos[0], mem_pass_log_pos[1], server.arg("password"));
          mem.eeprom_confirm();
        }
        is_wifi_connect = true;
        Serial.println(F("WiFi connected")); 
        
      // If WiFi SSID and Password cannot be connected
      }else{
        Serial.println(F("WiFi not connected")); 
        is_wifi_connect = false;
      }

      // Check email info, If has email, then record email
      if((String(server.arg("email")).length() <= 32) && (String(server.arg("email")).indexOf("@") != -1) && (String(server.arg("email")).indexOf(".") != -1)){
        Serial.println(F("saving email"));
        mem.eeprom_clear(mem_email_log_pos[0], mem_email_log_pos[1]);
        mem.eeprom_confirm();
        delay(50);
        mem.write_eeprom_string(mem_email_log_pos[0], mem_email_log_pos[1], String(server.arg("email").c_str()));
        mem.eeprom_confirm();
        has_email = true;
      }else{
        Serial.println(F("no email to save"));
      }

      if(is_wifi_connect){
        MQTTtrigger_wifi_setup(); // Sending info to server
      }
      
    }
    else if (server.hasArg("password")){
      Serial.println(F("Showing: SSID error"));
      txt_tmp = html_header;
      txt_tmp += F("<h1 style=\"text-align: center;\"><font color=\"red\">Please select SSID of your Wi-Fi</font></h1></center><meta http-equiv=\"refresh\" content=\"5; url=/wifi_setting\"/></body></html>");
      server.send(200, "text/html", txt_tmp);
    }
  }


  //////////////////////////////
  // normal main wifi setup page
  txt_tmp = html_header;
  //txt_tmp = F("<html><head><title>NextAir: Wi-Fi setting</title></head> <center><h2>NextAir<br></h2></center><h1 style=\"text-align: center;\">Wi-Fi setting<br></h1>");
  txt_tmp += F("<h1 style=\"text-align: center;\">Wi-Fi setting<br></h1>");

  if (wifi_station_get_connect_status() == 5){
    String ssid = mem.read_eeprom_string(mem_ssid_log_pos[0], mem_ssid_log_pos[1]);
    txt_tmp += F("<h1 style=\"text-align: center;\">WiFi connected: <font color=\"blue\">");
    txt_tmp += ssid;
    txt_tmp += F("</font></h1>");
    //txt_tmp += F("</b></h1><p style=\"text-align: center;\">Battery Option : ");
//    if (system_adc_read() > 16){
//        txt_tmp += F(" use battery "); 
//    }
//    else{
//      txt_tmp += F(" do not use battery "); 
//    }
//    txt_tmp += F("</p>");
  }
  else{
    txt_tmp += F("<h1 style=\"text-align: center;\">WiFi: <font color=\"red\">disconnected!</font></h1>");
    //txt_tmp += F("<h1 style=\"text-align: center;\">WiFi: <mark>disconnected</mark> </h1><p style=\"text-align: center;\">Battery Option : ");
//    if (system_adc_read() > 16){
//        txt_tmp += F(" use battery "); 
//    }
//    else{
//      txt_tmp += F(" do not use battery "); 
//    }
//    txt_tmp += F("</p>");
  }

  // Get number of visible access points
  Serial.println(F("scanNetworks")); 
  int AP_COUT = WiFi.scanNetworks();
  int AP_INDEX = 0, SSID_POS[AP_COUT];
  int32_t _RSSI[AP_COUT];
  
  if (AP_COUT == 0){
    txt_tmp += F("Press F5 to find Wifi.<br>");
  }
  else{
    txt_tmp += F("<div style=\"font-size:28px;\"><p><br><strong>Select your Wi-FI</strong></p>");
    txt_tmp += F("<p> refresh this page, if cannot see your WiFi<br></p><form method=\"get\">");
    for (AP_INDEX = 0; AP_INDEX < AP_COUT; AP_INDEX++){
      _RSSI[AP_INDEX] = WiFi.RSSI(AP_INDEX);
      SSID_POS[AP_INDEX] = AP_INDEX;
    }
    txt_tmp += F("<table style=\"height: 120px; border-color: black;\" border=\"black\" width=\"602\"><tbody><tr><td style=\"width: 212px; text-align: center;font-size:28px\"><strong>SSID</strong></td><td style=\"width: 274px; text-align: center;font-size:28px;\"><strong>Strength</strong></td></tr>");
    web_install_sort_ssid(_RSSI, SSID_POS, AP_COUT);
    for (AP_INDEX = 0; AP_INDEX < AP_COUT; AP_INDEX++){
      txt_tmp += F("<tr><td style=\"width: 492px; text-align: center;font-size:28px\"><input type=\"radio\" style=\"width: 2em; height: 2em;\" name=\"ssid\" value=\""); 
      //txt_tmp += F("<tr><td style=\"width: 492px; text-align: center;font-size:28px\"><input type=\"radio\" name=\"ssid\" value=\""); 
      txt_tmp += String(WiFi.SSID(SSID_POS[AP_INDEX])) + "\">" + String(WiFi.SSID(SSID_POS[AP_INDEX]));
      txt_tmp += F("</td><td style=\"width: 194px; text-align: center;font-size:28px\">");
      // RSSI max -50, min -100
      if (_RSSI[AP_INDEX] > -55){
        txt_tmp += F("*****");
      }
      else if (_RSSI[AP_INDEX] > -60){
        txt_tmp += F("****");
      }
      else if (_RSSI[AP_INDEX] > -70){
        txt_tmp += F("***");
      }
      else if (_RSSI[AP_INDEX] > -80){
        txt_tmp += F("**");
      }
      else{
        txt_tmp += F("*");
      }
      txt_tmp += F("</td></tr>");
    }
    //txt_tmp += F("</tbody></table><br>WiFi password (if required):<br><input style=\"font-size:28px;\" type=\"text\" name=\"password\"><br><br>Time Zone:<br><select style=\"font-size:28px;\" name=\"DropDownTimezone\" id=\"DropDownTimezone\"><option value=\"7.0\">(GMT +7:00) Bangkok, Hanoi, Jakarta</option><option value=\"5.0\">(GMT +5:00) Ekaterinburg, Islamabad, Karachi, Tashkent</option><option value=\"5.5\">(GMT +5:30) Bombay, Calcutta, Madras, New Delhi</option><option value=\"5.75\">(GMT +5:45) Kathmandu</option><option value=\"6.0\">(GMT +6:00) Almaty, Dhaka, Colombo</option><option value=\"7.0\">(GMT +7:00) Bangkok, Hanoi, Jakarta</option><option value=\"8.0\">(GMT +8:00) Beijing, Perth, Singapore, Hong Kong</option><option value=\"9.0\">(GMT +9:00) Tokyo, Seoul, Osaka, Sapporo, Yakutsk</option></select><br><br>E-Mail for App connection<br><input style=\"font-size:28px;\" type=\"text\" name=\"email\"><br><br><input style=\"font-size:30px;padding:8px;background-color:#4CAF50;border:2px solid green\" type=\"submit\" value=\"Connect\"></form>");
    txt_tmp += F("</tbody></table><br><strong>WiFi password</strong> (if required):<br><input style=\"font-size:28px;\" type=\"text\" name=\"password\"><br><br><strong>E-Mail</strong> (for App connection)<br><input style=\"font-size:28px;\" type=\"text\" name=\"email\"><br><br><input style=\"font-size:30px;padding:8px;background-color:#4CAF50;border:2px solid green\" type=\"submit\" value=\"Connect\"></form>");
  }
  txt_tmp += F("<br><p><a title=\"return to Main menu\" href=\"/\">return to Main menu</a></p><br></div></body></html>");

  //for(int i = 0; i < 1; i++){
    server.send(200, "text/html", txt_tmp);
    Serial.println(F("server.send WiFi main"));
  // delay(1000);   
  //}
}


/*
 * reset all setting
 */
void reset_all_setting(){
  String txt_tmp = "";
  mem.eeprom_clear(0, mem_reset_vars_eeprom_pos);
  mem.eeprom_confirm();
  txt_tmp = F("<body style=\"background-color:#7FDBFF;\"><h1 style=\"text-align: center;\"><h2 style=\"text-align: center;\">AirBridge<br></h2><h1 style=\"text-align: center;\"><font color=\"red\">settings are reset</font><br></h1><p style=\"text-align: center;font-size:32px;\">go back to Main menu to setup Wi-Fi again<br><br><a title=\"return to Main menu\" href=\"/\">return to Main menu</a></p></body>");
  wifi_station_disconnect();
  //WiFi.begin("");
  delay(500);
  server.send(200, "text/html", txt_tmp);
}

/* Called if requested page is not found */
void web_page_not_found(){
  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  String txt_tmp = "";
  txt_tmp = F("File Not Found\n\nURI: ");
  txt_tmp += server.uri();
  txt_tmp += F("\nMethod: ");
  txt_tmp += (server.method() == HTTP_GET)?"GET":"POST";
  txt_tmp += F("\nArguments: ");
  txt_tmp += server.args();
  txt_tmp += F("\n");
  
  for (i = 0; i < server.args(); i++){
    txt_tmp += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", txt_tmp);
}

void web_install(){
//  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
//  /*
//   * set WiFi to dual mode
//   */
//  WiFi.mode(WIFI_OFF); 
//  for (i = 0; i < 3; i++){
//    if (wifi_set_opmode(0x03)){
//      break;
//    }
//    if (i == 2){
//      Serial.println(F("connot start Webserver! -> return"));
//      mem.state = 1;
//      return;
//    }
//    delay(2000);
//  }
//  xxtea.setKey(s_air_key);
//  String result = xxtea.encrypt(String(hotspot_name).substring(6));
//  result.toLowerCase();
////  while (!MDNS.begin(s_air_domain)) {
////  }
//  WiFi.softAP(hotspot_name, result.c_str());
//  server.begin();
//  uint32_t time_1 = millis() + web_install_duration*60000;//--> end time
//  uint32_t time_2 = millis() + 10000;//led blinken
//  led_green.init(300, 2);
//  while (millis() <= time_1){
//    led_green.run();
//    if (millis() > time_2){
//      led_green.init(300, 2);
//      time_2 = millis() + 10000;//led blinken
//    }
//    server.handleClient();  
//    rmt.check_ir();
//    if (rmt.rmt_detected){
//      rmt.rmt_detected = 0;
//      rmt_state_control();
//      break;
//    }
//    delay(1);
//  }
//  server.stop();
//
//  /*
//   * if webserver end without using remote from user we go state 1
//   */
//  if (rmt.getNum() == 0){
//    led_green.lo();
//    mem.state = 1;
//    if (show_data){
//      Serial.println(F("state 0-web-install's finish, go state 1"));
//    }
//  }
//  else{
//    delay(500);
//    rmt.cool();
//    if (show_data){
//      Serial.println(F("state 0-web-install's finish, go state 2"));
//    }
//    mem.state = 2; mem.state_cnt = 0;     
//  }
//
//  /*
//   * reading of ssid, password, e-mail, time-zone, then check if user need report we need to do state 8 for 4 hour and recording of data
//   */
//  
//  String ssid = mem.read_eeprom_string(mem_ssid_log_pos[0], mem_ssid_log_pos[1]);
//  String password = mem.read_eeprom_string(mem_pass_log_pos[0], mem_pass_log_pos[1]);
//  String email = mem.read_eeprom_string(mem_email_log_pos[0], mem_email_log_pos[1]);
//  String time_zone = mem.read_eeprom_string(mem_time_zone_log_pos[0], mem_time_zone_log_pos[1]);
//  if ((ssid.substring(0,1) != '\0') && (password.substring(0,1) != '\0') && (email.substring(0,1) != '\0') && (time_zone.substring(0,1) != '\0')){
//    if (show_data){
//      Serial.print(F("ssid = ")); Serial.println(ssid);
//      Serial.print(F("password = ")); Serial.println(password);
//      Serial.print(F("email = ")); Serial.println(email);
//      Serial.print(F("time_zone = ")); Serial.println(time_zone);
//    }
//    mem.user_need_email = 1;
//  }
//  WiFi.disconnect();
//  while (WiFi.status() == WL_CONNECTED){
//    delay(1000);
//  }
//  WiFi.forceSleepBegin();
//  delay(1);

  //int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  static uint32_t state_0_t_left = millis() + web_install_duration*60000;//--> end time
  static uint32_t state_0_led_t_stamp = millis() + 3000;//led blinken t stamp
  if ((millis() > state_0_t_left) || rmt.rmt_detected){

    // first time to detect remote control
    if (rmt.rmt_detected){//remote was detected go changing state
      rmt.rmt_detected = 0;
      rmt_state_control();

      // send rc model to server
      if(!MQTTtrigger_log(F("rc"),String(rmt.getNum()))){ // Sending info to server
        if(check_internet_and_connect_MQTT()){ // if cannot send check internet and send again
         Serial.println(F("reconnecting for RC-Model sending"));
          if(MQTTtrigger_send_temperature()){
            Serial.println(F("reconnecting for RC-Model sending-sent"));
          }else{
            Serial.println(F("reconnecting for RC-Model sending-no sent"));
          }
          
        }
      }
      rmt.cool(); // confirm remote control by resending signal
    }
    else if (rmt.getNum() == 0){//if webserver end without using remote from user we go state 1, else go state 2
      led_green.lo();
      mem.state = 1;
      if (show_data){
        Serial.println(F("state 0-web-install's finish no remote, go state 1"));
      }
    }
    else if(rmt.getNum() != 0){
      led_green.lo();
      mem.state = 2;
      if (show_data){
        Serial.println(F("state 0-web-install's finish remote exited, go state 2"));
      }
    }
    
  
    /*
     * reading of ssid, password, e-mail, time-zone, then check if user need report we need to do state 8 for 4 hour and recording of data
     */
    
//    String ssid = mem.read_eeprom_string(mem_ssid_log_pos[0], mem_ssid_log_pos[1]);
//    String password = mem.read_eeprom_string(mem_pass_log_pos[0], mem_pass_log_pos[1]);
//    String email = mem.read_eeprom_string(mem_email_log_pos[0], mem_email_log_pos[1]);
//    //String time_zone = mem.read_eeprom_string(mem_time_zone_log_pos[0], mem_time_zone_log_pos[1]);
//    String time_zone = default_time_zone;
//    if ((ssid.substring(0,1) != '\0') && (password.substring(0,1) != '\0') && (email.substring(0,1) != '\0') && (time_zone.substring(0,1) != '\0')){
//      if (show_data){
//        Serial.print(F("ssid = ")); Serial.println(ssid);
//        Serial.print(F("password = ")); Serial.println(password);
//        Serial.print(F("email = ")); Serial.println(email);
//        Serial.print(F("time_zone = ")); Serial.println(time_zone);
//      }
//      mem.user_need_email = 1;
//    }
//    WiFi.disconnect();
//    delay(1);
    mem.next_op_tstamp = system_get_rtc_time() + sleep_period;
  }

  //led_init for state 0
  if (millis() > state_0_led_t_stamp){
    led_green.init(300, 3);
    state_0_led_t_stamp = millis() + 20000;//led blinken
  }
}


/*
 * name S-Air
 */
void set_hotspot_name(){
  //int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  int i = 0;
  char mac_name_array[13]; // SSID name of this module
  hotspot_name[0] = 'A';
  hotspot_name[1] = 'R';
  hotspot_name[2] = '-';

  uint8_t MAC_ARRAY[6] = {0,0,0,0,0,0};
  WiFi.macAddress(MAC_ARRAY);  // Mac Address
  
  // Setup hotspot name
  for (i = 0; i < 6; ++i) 
  {
    sprintf(hotspot_name, "%s%02x", hotspot_name, MAC_ARRAY[i]);
  }

  // Setup MAC name
  for (i = 0; i < 6; ++i) 
  {
    sprintf(mac_name_array, "%s%02x", mac_name_array, MAC_ARRAY[i]);
  }
  mac_name = String(mac_name_array);
  
  //Serial.print("mac_name:");Serial.println(mac_name);
  //Serial.println(WiFi.macAddress());
  //Serial.print("hotspot_name:");Serial.println(hotspot_name);
  //Serial.print("encrypt:");Serial.println(String(hotspot_name).substring(6));
  xxtea.setKey(s_air_key);
  String result = xxtea.encrypt(String(hotspot_name).substring(6));
  result.toLowerCase();
  Serial.println(rn + String(WiFi.macAddress()) + rn + String(hotspot_name) + rn + result);


}

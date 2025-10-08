#include "ir_remote.h"

void IRRemote::setup(){
  irrecv.disableIRIn();
  irrecv.enableIRIn();
  irsend.begin();
  for(int i = 0; i < 586; i++){
    ps_buffer[i] = 0;
  }
  min_bit_length = 0;
  rmt_num = 0;
  last_cut_t = 0;
  ps_length_in_mcu_cycle = 200000;
}

void IRRemote::reset(){
  irrecv.disableIRIn();
  irrecv.enableIRIn();
  irsend.begin();
}

int IRRemote::getNum(){
  return rmt_num;
}

void IRRemote::setNum(int num){
  rmt_num = num;
} 

void IRRemote::incTrset(){
  if(air_status.Trset < max_trset_to_adjust_by_heavy_duty){
    air_status.Trset++;     
  }
}

uint8_t IRRemote::getPower(){
  return air_status.power;
}

uint8_t IRRemote::getMode(){
  return air_status.mode;
}

uint8_t IRRemote::getTrset(){
  return air_status.Trset;
}

uint8_t IRRemote::getFan(){
  return air_status.fan;
}

uint8_t IRRemote::getVane(){
  return air_status.vane;
}

void IRRemote::setPower(uint8_t power){
  air_status.power = power;
}

void IRRemote::setMode(uint8_t mode){
  air_status.mode = mode;
}

void IRRemote::setTrset(uint8_t Trset){
  air_status.Trset = Trset;
}

void IRRemote::setFan(uint8_t fan){
  air_status.fan = fan;
}

void IRRemote::setVane(uint8_t vane){
  air_status.vane = vane;
}

    
/*
 * ~~~~~~~ check_ir ~~~~~~~
 * this function will call after woke up from light sleep to check ir for giving time (ps_length_in_mcu_cycle)
 * it will use to blink led too
 */
boolean IRRemote::check_ir(){
  if (irrecv.decode(&results)){
    return dump_control();
  }
  return false;
}

boolean IRRemote::dump_control(){
  boolean found_remote;
  if (rmt_num == 903){//this is for samsung-remote, it has 2 version of remote 168 and 112, if 168 is't working, then try 112
    found_remote = dump(&results);
    if (!rmt_detected){
      rmt_num = 902; min_bit_length = 56;
      found_remote = dump(&results);
      if (!rmt_detected){
        rmt_num = 903;  min_bit_length = 120;
      }
    }
  }
  else if (rmt_num == 902){//and when 112 is't working, then try 168
    found_remote = dump(&results);
    if (!rmt_detected){
      rmt_num = 903;  min_bit_length = 120;
      found_remote = dump(&results);
      if (!rmt_detected){
        rmt_num = 902; min_bit_length = 56;
      }
    }
  }
  else{
    found_remote = dump(&results);
  }
  irrecv.resume();
  return found_remote;
}

boolean IRRemote::dump(decode_results *results){
  //int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  int i = 0;
  /*
   * if time more than 1 second it means signal from user and results->rawlen are between 586 and 32
   */
  uint16_t ps_cnt = results->rawlen; 
  if ((system_get_rtc_time() - last_cut_t > 260850) && ((ps_cnt <= 586) && (ps_cnt >= 32))){
    for (i = 0; i < 586; i++){//-> clearing big_array
      ps_buffer[i] = 0;
    }
    
    for (i = 1; i < ps_cnt; i++){
      if ((i % 2 == 1) && (i != ps_cnt - 1)){
        ps_buffer[i - 1] = (results->rawbuf[i] * RAWTICK);
      }
      if ((i % 2 == 0) && (i != ps_cnt - 1)){
        ps_buffer[i - 1] = (results->rawbuf[i] * RAWTICK);
      }
      if (((i % 2) == 1) && (i == (ps_cnt - 1)))
      { //the last pulse
        ps_buffer[i - 1] = (results->rawbuf[i] * RAWTICK);
      }
      if ((i % 2 == 0) && (i == ps_cnt - 1))
      { //the last pulse
        ps_buffer[i - 1] = (results->rawbuf[i] * RAWTICK);
      }
    }

    if (show_signal){
      Serial.print(F("Recieved ir = "));
      Serial.print(String(ps_cnt));
      Serial.println(F(" bits"));
      for (i = 0; i < ps_cnt; i++){
        Serial.print(ps_buffer[i]);
        Serial.print(",");
      }
      Serial.println();
    }
    return check_rmt_num_and_change_state(ps_cnt);
  }
  // v2.1: Added missing return statement
  return false;
}    

/*
 * Reading value from driver
 */
String IRRemote::get_rmt_const(String var, String *rmt_data){
  if ((*rmt_data).indexOf(var + String("=")) != -1){
    
//    Serial.print(var);Serial.print(" : "); Serial.println((*rmt_data).substring((*rmt_data).indexOf(var + String("=")) + (var + String("=")).length(), (*rmt_data).indexOf(",",(*rmt_data).indexOf(var + String("="))))); 
    
    return (*rmt_data).substring((*rmt_data).indexOf(var + String("=")) + (var + String("=")).length(), (*rmt_data).indexOf(",",(*rmt_data).indexOf(var + String("="))));
  }
  else
  {
    return "?";
  }
}

// Return true, if signal is fit and correct to the database
boolean IRRemote::check_rmt_num_and_change_state(uint16_t ps_cnt){
  //int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  int i = 0;
  
  if (show_data){
    Serial.println(F("recieved remote!"));
  }
  uint16_t zero = GetZero(ps_cnt);
  uint16_t one = GetOne(ps_cnt);
  
  if ((one > 4000) || (zero > 1800)){
    return false;
  }
  if ((one == 0) || (zero == 0)){
    if (show_data){
      Serial.println(F("cannot detect one&zero-space -> return"));
    }
    return false;
  }
  String bit = ""; String bit_temp = "";
  uint16_t sum_of_1 = 0;
  uint16_t sum_of_0 = 0;
  for (i = 1; i < ps_cnt; i = i + 2){
    if (((one - tolerance) < ps_buffer[i]) && (ps_buffer[i] < (one + tolerance))){
      bit += "1";
      sum_of_1++;
    }
    else if (((zero - tolerance) < ps_buffer[i]) && (ps_buffer[i] < (zero + tolerance))){
      bit += "0";
      sum_of_0++;
    }
  }
  if ((sum_of_1 < 2)||(sum_of_0 < 2)){
    return false;
  }
  
  if ((bit.length() > 280)&&(rmt_num == 203)){//bit fix for daikin_285-5
    bit = bit.substring(bit.length() - 280,bit.length());
  }

  if(show_bit){
    Serial.print(F("MSB = ")); Serial.println(bit);
    Serial.print(F("bit.length() = ")); Serial.print(String(bit.length())); Serial.print(F(" bits\r\n"));
  }
  
  if (rmt_num == 0){
    String str_0 = "0";
    /* comparing bit
     *  constatn must be in MSB-format
     */
    //---------------------------------------------------- LG_28
    if ((26 <= bit.length()) &&(bit.length() <= 28)){
      bit_temp = "";
      for (i=0; i < (28-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      rmt_num = 501;
    }

    //---------------------------------------------------- universal_44
    else if ((42 <= bit.length()) &&(bit.length() <= 44)){
      bit_temp = "";
      for (i=0; i < (44-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      if ((bit.indexOf(F("10")) == 2) && (bit.indexOf(F("0000000000000000")) == 16)){//universal_44_A
        rmt_num = 1401;
      }
      else if (bit.substring(2,8) == F("101001")){//universal_44_B
        rmt_num = 1402;
      }
      else if (bit.indexOf(F("0000000000000110")) == 16){//universal_44_C
        rmt_num = 1403;
      }
    }
    
    //----------------------------------------------------- CDL = 48
    else if ((46 <= bit.length()) &&(bit.length() <= 48)){
      bit_temp = "";
      for (i=0; i < (48-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      if ((bit.substring(7,12) == F("10000")) && (bit.substring(42,44) == F("10"))){//mitsu_48 MSB-format
        rmt_num = 601;
      }
    }
    
    //----------------------------------------------------- Samsung_56
    else if ((54 <= bit.length()) &&(bit.length() <= 56)){
      bit_temp = "";
      for (i=0; i < (56-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      rmt_num = 901;
    }
  
    //----------------------------------------------------- Daikin_64
    else if ((62 <= bit.length()) &&(bit.length() <= 64)){
      bit_temp = "";
      for (i=0; i < (64-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      if (bit.indexOf(F("010110")) == 2){// daikin_64
        rmt_num = 201;
      }
      else if ((bit.indexOf(F("100110")) == 2) && (bit.indexOf(F("10110111")) == 56)){// saijo_64
        rmt_num = 801;
      } 
    }

    //----------------------------------------------------- mitsubishi_88
    else if ((86 <= bit.length()) &&(bit.length() <= 88)){
      bit_temp = "";
      for (i=0; i < (88-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      rmt_num = 602;
    }   
    
    //----------------------------------------------------- carrier_toshiba_96
    else if ((94 <= bit.length()) &&(bit.length() <= 96)){
      bit_temp = "";
      for (i=0; i < (96-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      if (bit.indexOf(F("00010000")) == 32){// sharp_96
        rmt_num = 1001;
      }
      else{
        rmt_num = 101;
      }
    }     
  
    //----------------------------------------------------- Sharp_104
    else if ((102 <= bit.length()) &&(bit.length() <= 104)){
      bit_temp = "";
      for (i=0; i < (104-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      if (bit.indexOf(F("00101010111001001101")) == 4){// hitashi_104 MSB-format
        rmt_num = 401;
      }
      else if (bit.indexOf(F("01010101011010")) == 2){// sharp_104
        rmt_num = 1002;
      }   
    }
  
    //----------------------------------------------------- Heier_112,samsung_112,Turbocool_112
    else if ((110 <= bit.length()) &&(bit.length() <= 112)){
      bit_temp = "";
      for (i=0; i < (112-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      if (bit.indexOf(F("111110000000000000000000000000000")) == 15){// samsung_112
        rmt_num = 902;
      }    
      else if (bit.indexOf(F("100101")) == 2){// heier_112
        rmt_num = 301;
      }
      else if (bit.indexOf(F("010011010011")) == 4){// mitsu_112
        rmt_num = 603;
      }
    }
  
  
    //----------------------------------------------------- Panasonic_128
    else if ((126 <= bit.length()) &&(bit.length() <= 128)){
      rmt_num = 701;
      bit_temp = "";
      for (i=0; i < (128-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      if (bit.indexOf(F("0000000010011001")) == 24){// saijo_128
        rmt_num = 802;
      }
    }
  
    //----------------------------------------------------- Mitsubishi_136
    else if ((134 <= bit.length()) &&(bit.length() <= 136)){
      bit_temp = "";
      for (i=0; i < (136-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      rmt_num = 604;
    }
    
    //----------------------------------------------------- Carrier_Toshiba_144
    else if ((142 <= bit.length()) &&(bit.length() <= 144)){
      bit_temp = "";
      for (i=0; i < (144-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      rmt_num = 102;
    }
    //----------------------------------------------------- Carrier_Toshiba_168, samsung_168
    else if ((166 <= bit.length()) &&( bit.length() <= 168)){
      bit_temp = "";
      for (i=0; i < (168-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      if (bit.indexOf(F("111100000000000000000000000000000000000010000000")) == 72){
        rmt_num = 903;
      }
      else {
        rmt_num = 103;
      }
    }     
    //----------------------------------------------------- toshiba_96 FAN QUIET
    else if ((190 <= bit.length()) &&( bit.length() <= 192)){
      bit = bit.substring(bit.length() - 96, bit.length());
      rmt_num = 101;
    } 
    //----------------------------------------------------- daikin_216, panasonic_216
    else if ((214 <= bit.length()) &&( bit.length() <= 216)){
      bit_temp = "";
      for (i=0; i < (216-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      if (bit.indexOf(F("00100001011011")) == 2){
        rmt_num = 202;//daikin
      }
      else{
        rmt_num = 702;// panasonic_216
      }
    }
    //----------------------------------------------------- hitashi_264
    else if ((262 <= bit.length()) &&( bit.length() <= 264)){
      bit_temp = "";
      for (i=0; i < (264-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      rmt_num = 402;
    }
    //----------------------------------------------------- daikin_285-5
    else if ((281 <= bit.length()) &&( bit.length() <= 285)){
      bit = bit.substring(bit.length() - 280,bit.length());
      rmt_num = 203;
    }
    
    //------------------------------------------------------ Mitsubishi_288  
    else if ((286 <= bit.length()) &&( bit.length() <= 288))
    {
      bit_temp = "";
      for (i=0; i < (288-bit.length()); i++)
      {
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
      rmt_num = 605;
    }
  
    //------------------------------------------------------ Hitashi_532
    else if ((530 <= bit.length()) &&( bit.length() <= 532)){
      rmt_num = 401;
      bit_temp = "";
      for (i=0; i < (532-bit.length()); i++){
        bit_temp += str_0;
      }
      bit = bit_temp + bit;
    }
    else{
      if (show_data){
        Serial.println(F("cannot detect rmt!"));
      }
      rmt_num == 0; return false;
    }
    if (show_data){
      Serial.print(F("rmt_num = ")); Serial.print(String(rmt_num) + F("\r\n"));
    }
  }

  else if (bit.length() >= min_bit_length){// if there is remote number
    String remote_data = get_rmt_data_from_eeprom(&rmt_num);
    uint16_t CDL = get_rmt_const("CDL", &remote_data).toInt();
    remote_data = "";
    if (bit.length() <= CDL){
      bit_temp = "";
      for (i=0; i < (CDL - bit.length()); i++){
        bit_temp += "0";
      }
      bit = bit_temp + bit;
    }
    else if ((bit.length() > CDL) && (bit.length() <= 2*CDL) && (rmt_num == 101)){//if user use fan quiet of toshiba_96, then translate last 96 bit only
      bit = bit.substring(bit.length() - CDL, bit.length());
    }
    else{
      if (show_data){
        Serial.println(F("cannot detect anything, do not change status"));
      }
      return false;
    }
  }
  else{
    if (show_data){
      Serial.println(F("cannot detect anything, do not change status"));
    }
    return false;
  }

  /*
   * translating part
   */
  int translating[5] = {air_status.power, air_status.mode, air_status.Trset, air_status.fan, air_status.vane};//first define translating = air_status now
  trans_air_status(&bit, &translating[0], &translating[1], &translating[2], &translating[3], &translating[4]);//then translate    
  if ((translating[0] == -1) || (translating[1] == -1) || (translating[3] == -1)){//if cannot read power or mode or fan, then return and use old air_status
    if (show_data){
      Serial.println(F("cannot detect power or mode or fan -> return"));
    }
    return false;                        
  }
  else{
    
  /*
   * set rmt_num
   */
    String txt_tmp;    
    txt_tmp = F("** Result **\r\n");
    switch (rmt_num){
      case 101:
        txt_tmp += F("carrier_toshiba_96"); ps_length_in_mcu_cycle = 130000; min_bit_length = 36; break;
      case 102:
        txt_tmp += F("carrier_toshiba_144"); ps_length_in_mcu_cycle = 90000; min_bit_length = 24; break;
      case 103:
        txt_tmp += F("carrier_toshiba_168"); break;
      case 201:
        txt_tmp += F("daikin_64"); ps_length_in_mcu_cycle = 60000; min_bit_length = 56; break;    
      case 202:
        txt_tmp += F("daikin_216"); ps_length_in_mcu_cycle = 120000; min_bit_length = 112; break;
      case 203:
        txt_tmp += F("daikin_280"); ps_length_in_mcu_cycle = 180000; min_bit_length = 120; break;
      case 301:
        txt_tmp += F("haier_112"); min_bit_length = 80; break;    
      case 401:
        txt_tmp += F("hitashi_104"); ps_length_in_mcu_cycle = 35000; min_bit_length = 64; break;    
      case 402:
        txt_tmp += F("hitashi_264"); ps_length_in_mcu_cycle = 160000; min_bit_length = 64; break;    
      case 501:
        txt_tmp += F("lg_28"); ps_length_in_mcu_cycle = 25000; min_bit_length = 16; break;    
      case 601:
        txt_tmp += F("mitsubishi_48"); ps_length_in_mcu_cycle = 45000; min_bit_length = 48; break;
      case 602:
        txt_tmp += F("mitsubishi_88"); ps_length_in_mcu_cycle = 70000; min_bit_length = 32; break;    
      case 603:
        txt_tmp += F("mitsubishi_112"); ps_length_in_mcu_cycle = 90000; min_bit_length = 72; break;        
      case 604:
        txt_tmp += F("mitsubishi_136"); ps_length_in_mcu_cycle = 100000; min_bit_length = 96; break;
      case 605:
        txt_tmp += F("mitsubishi_288"); ps_length_in_mcu_cycle = 160000; min_bit_length = 104; break;  
      case 701:
        txt_tmp += F("panasonic_128"); ps_length_in_mcu_cycle = 80000; min_bit_length = 96; break;
      case 702:
        txt_tmp += F("panasonic_216"); ps_length_in_mcu_cycle = 120000; min_bit_length = 112; break;
      case 801:
        txt_tmp += F("saijo_64"); ps_length_in_mcu_cycle = 80000; min_bit_length = 56; break;
      case 802:
        txt_tmp += F("saijo_128"); ps_length_in_mcu_cycle = 110000; min_bit_length = 56; break;            
      case 901:
        txt_tmp += F("samsung_56"); ps_length_in_mcu_cycle = 91000; break;
      case 902:
        txt_tmp += F("samsung_112"); ps_length_in_mcu_cycle = 91000; min_bit_length = 56; break;    
      case 903:
        txt_tmp += F("samsung_168"); ps_length_in_mcu_cycle = 91000; min_bit_length = 120; break;
      case 1001:
        txt_tmp += F("sharp_96"); ps_length_in_mcu_cycle = 90000; min_bit_length = 52; break;
      case 1002:
        txt_tmp += F("sharp_104"); ps_length_in_mcu_cycle = 40000; min_bit_length = 64; break;         
      case 1101:
        txt_tmp += F("singer_"); break;
      case 1201:
        txt_tmp += F("turbocool_112"); min_bit_length = 72; break;
      case 1401:
        txt_tmp += F("universal_44_A"); ps_length_in_mcu_cycle = 45000; min_bit_length = 40; break;
      case 1402:
        txt_tmp += F("universal_44_B"); ps_length_in_mcu_cycle = 45000; min_bit_length = 36; break;
      case 1403:
        txt_tmp += F("universal_44_C"); ps_length_in_mcu_cycle = 45000; min_bit_length = 43; break;                                                                                         
      default:
        txt_tmp += F("No Driver! -> do not change any thing"); 
        return false; break;  
    }
  
    
    air_status.power = translating[0];
    air_status.mode = translating[1];
    //if cannot translate Trset&vane, then set Trset = 25 and vane = 0
    if (translating[2] == -1){
      if (air_status.Trset == 0){
        air_status.Trset = 25;
      }
    }
    else{
      air_status.Trset = translating[2];
    }
    air_status.fan = translating[3];
    
    if (translating[4] != -1){
      air_status.vane = translating[4];
    }
    else{
      if ((air_status.vane < 0) && (air_status.vane > 6)){
        air_status.vane = 0;
      }
    }
    
    if (show_bit){
      txt_tmp += F("\r\npower -> ");txt_tmp += String(air_status.power);
      txt_tmp += F("\r\nmode -> ");txt_tmp += String(air_status.mode);
      txt_tmp += F("\r\nTrset -> ");txt_tmp += String(air_status.Trset);
      txt_tmp += F("\r\nfan -> ");txt_tmp += String(air_status.fan);
      txt_tmp += F("\r\nvane -> ");txt_tmp += String(air_status.vane) + F("\r\n");
      if (show_bit){
        Serial.print(txt_tmp);
      }
    }
    rmt_detected = 1;//set remote_was_detected
    //MQTTtrigger_get_ir(); // Trigger for sending MQTT
    return true;
  }
}

void IRRemote::trans_air_status(String *bit_, int *power_, int *mode_, int *Trset_, int *fan_, int *vane_){
  //int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  int i = 0,j = 0;
  String remote_data = get_rmt_data_from_eeprom(&rmt_num);  
  uint16_t CDL = get_rmt_const("CDL", &remote_data).toInt();
  uint16_t PWP = get_rmt_const("PWP", &remote_data).toInt();
  uint16_t PWL = get_rmt_const("PWL", &remote_data).toInt();
  uint16_t MDP = get_rmt_const("MDP", &remote_data).toInt();
  uint16_t MDL = get_rmt_const("MDL", &remote_data).toInt();
  uint16_t TMPP = get_rmt_const("TMPP", &remote_data).toInt();
  uint16_t TMPL = get_rmt_const("TMPL", &remote_data).toInt();
  uint16_t FNP = get_rmt_const("FNP", &remote_data).toInt();
  uint16_t FNL = get_rmt_const("FNL", &remote_data).toInt();
  uint16_t VNP = get_rmt_const("VNP", &remote_data).toInt();
  uint16_t VNL = get_rmt_const("VNL", &remote_data).toInt();
  
  /*
   * convert code_format
   */
  if (get_rmt_const("CDF", &remote_data) == "LSB8"){
    String bit_temp = "";
    //Serial.println(F("MSB_code need to be transform in LSB8"));
    for (i = 0; i < CDL/8; i++)
    {
      for (j = 0; j < 8; j++)
      {
        bit_temp += (*bit_).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
      }      
    }
    *bit_ = bit_temp;
    if (show_bit){
      Serial.print(F("LSB = ")); Serial.println((*bit_));
    }
    bit_temp = "";
  }
  else if (show_bit){
    Serial.print(F("MSB = ")); Serial.println((*bit_));
  }
  
  /* 
   *  trans_air_status with constant code, always with return at the end
  */
  if (spc_bit_rx(bit_,power_,mode_,Trset_,fan_, vane_ ,&remote_data)){
    return;
  }

  //--> test power
//  Serial.println(F(""));
//  Serial.print(F("power on/off testing"));
//  Serial.println(F(""));
//  Serial.print((*bit_).substring(PWP - 1, PWP - 1 + PWL));
//  Serial.println(F(""));
//  Serial.print(get_rmt_const("ON", &remote_data));
//  Serial.println(F(""));
//  Serial.print(get_rmt_const("OFF", &remote_data));      
  /* normal trans_air_status*/    
  //---> power
  if ((*bit_).substring(PWP, PWP + PWL) == get_rmt_const("ON", &remote_data) && (get_rmt_const("ON", &remote_data) != "?")) {
    *power_ = 1;
  }
  else if ((*bit_).substring(PWP, PWP + PWL) == get_rmt_const("ON2", &remote_data) && (get_rmt_const("ON2", &remote_data) != "?")) {
    *power_ = 1;
  }
  else if ((*bit_).substring(PWP, PWP + PWL) == get_rmt_const("ON3", &remote_data) && (get_rmt_const("ON3", &remote_data) != "?")) {
    *power_ = 1;
  }  
  else if ((*bit_).substring(PWP, PWP + PWL) == get_rmt_const("ON4", &remote_data) && (get_rmt_const("ON4", &remote_data) != "?")) {
    *power_ = 1;
  }    
  else if ((*bit_).substring(PWP, PWP + PWL) == get_rmt_const("OFF", &remote_data) && (get_rmt_const("OFF", &remote_data) != "?")) {
    *power_ = 0;
  }
  else {
    *power_ = -1;
  }
  
  //---> mode
  if ((*bit_).substring(MDP, MDP + MDL) == get_rmt_const("AUTO", &remote_data) && (get_rmt_const("AUTO", &remote_data) != "?")) {
    *mode_ = 0;//--> auto
  }
  else if ((*bit_).substring(MDP, MDP + MDL) == get_rmt_const("COOL", &remote_data) && (get_rmt_const("COOL", &remote_data) != "?")) {
    *mode_ = 1;//--> cool
  }
  else if ((*bit_).substring(MDP, MDP + MDL) == get_rmt_const("DRY", &remote_data) && (get_rmt_const("DRY", &remote_data) != "?")) {
    *mode_ = 2;//--> dry
  }
  else if ((*bit_).substring(MDP, MDP + MDL) == get_rmt_const("FAN", &remote_data) && (get_rmt_const("FAN", &remote_data) != "?")) {
    *mode_ = 3;//--> fan
  }
  else if ((*bit_).substring(MDP, MDP + MDL) == get_rmt_const("AUTO2", &remote_data) && (get_rmt_const("AUTO2", &remote_data) != "?")) {
    *mode_ = 0;//--> auto2
  }
  else if ((*bit_).substring(MDP, MDP + MDL) == get_rmt_const("COOL2", &remote_data) && (get_rmt_const("COOL2", &remote_data) != "?")) {
    *mode_ = 1;//--> cool2
  }
  else if ((*bit_).substring(MDP, MDP + MDL) == get_rmt_const("DRY2", &remote_data) && (get_rmt_const("DRY2", &remote_data) != "?")) {
    *mode_ = 2;//--> dry2
  }
  else if ((*bit_).substring(MDP, MDP + MDL) == get_rmt_const("FAN2", &remote_data) && (get_rmt_const("FAN2", &remote_data) != "?")) {
    *mode_ = 3;//--> fan2
  }  
  else {
    *mode_ = -1;
  }
    
  //---> temperature
  
  for (i = 15; i < 33 ;i++){
    if ((*bit_).substring(TMPP, TMPP + TMPL) == get_rmt_const("T"+String(i), &remote_data) && (get_rmt_const("T"+String(i), &remote_data) != "?")) {
      *Trset_ = i;
      break;
    }
    else if (i == 32){
      *Trset_ = -1; 
    }
    else if (rmt_num == 501){
      if ((*bit_).substring(TMPP, TMPP + TMPL) == get_rmt_const("T*", &remote_data) && (get_rmt_const("T*", &remote_data) != "?")) {
        *Trset_ = 25;
        break;
      }
      if ((*bit_).substring(TMPP, TMPP + TMPL) == get_rmt_const("T+", &remote_data) && (get_rmt_const("T+", &remote_data) != "?")) {
        *Trset_ = 26;
        break;
      }
      if ((*bit_).substring(TMPP, TMPP + TMPL) == get_rmt_const("T++", &remote_data) && (get_rmt_const("T++", &remote_data) != "?")) {
        *Trset_ = 27;
        break;
      }
      if ((*bit_).substring(TMPP, TMPP + TMPL) == get_rmt_const("T-", &remote_data) && (get_rmt_const("T-", &remote_data) != "?")) {
        *Trset_ = 24;
        break;
      }
      if ((*bit_).substring(TMPP, TMPP + TMPL) == get_rmt_const("T--", &remote_data) && (get_rmt_const("T--", &remote_data) != "?")) {
        *Trset_ = 23;
        break;
      }
    }
  }
  
  //---> fan
  if ((*bit_).substring(FNP, FNP + FNL) == get_rmt_const("FA", &remote_data) && (get_rmt_const("FA", &remote_data) != "?")) {
    *fan_ = 0;
  }
  else if ((*bit_).substring(FNP, FNP + FNL) == get_rmt_const("F1", &remote_data) && (get_rmt_const("F1", &remote_data) != "?")) {
    *fan_ = 1;
  }
  else if ((*bit_).substring(FNP, FNP + FNL) == get_rmt_const("F2", &remote_data) && (get_rmt_const("F2", &remote_data) != "?")) {
    *fan_ = 2;
  }
  else if ((*bit_).substring(FNP, FNP + FNL) == get_rmt_const("F3", &remote_data) && (get_rmt_const("F3", &remote_data) != "?")) {
    *fan_ = 3;
  }
  else if ((*bit_).substring(FNP, FNP + FNL) == get_rmt_const("F4", &remote_data) && (get_rmt_const("F4", &remote_data) != "?")) {
    *fan_ = 4;
  }
  else if ((*bit_).substring(FNP, FNP + FNL) == get_rmt_const("F5", &remote_data) && (get_rmt_const("F5", &remote_data) != "?")) {
    *fan_ = 5;
  }
  else if ((*bit_).substring(FNP, FNP + FNL) == get_rmt_const("FS", &remote_data) && (get_rmt_const("FS", &remote_data) != "?")) {
    *fan_ = 0;//--> another fan speed return to FA
  }   
  else {
    *fan_ = -1;
  }
  
  //---> vane
  if ((*bit_).substring(VNP, VNP + VNL) == get_rmt_const("VA", &remote_data) && (get_rmt_const("VA", &remote_data) != "?")) {
    *vane_ = 0;
  }
  else if ((*bit_).substring(VNP, VNP + VNL) == get_rmt_const("V1", &remote_data) && (get_rmt_const("V1", &remote_data) != "?")) {
    *vane_ = 1;
  }
  else if ((*bit_).substring(VNP, VNP + VNL) == get_rmt_const("V2", &remote_data) && (get_rmt_const("V2", &remote_data) != "?")) {
    *vane_ = 2;
  }
  else if ((*bit_).substring(VNP, VNP + VNL) == get_rmt_const("V3", &remote_data) && (get_rmt_const("V3", &remote_data) != "?")) {
    *vane_ = 3;
  }
  else if ((*bit_).substring(VNP, VNP + VNL) == get_rmt_const("V4", &remote_data) && (get_rmt_const("V4", &remote_data) != "?")) {
    *vane_ = 4;
  }
  else if ((*bit_).substring(VNP, VNP + VNL) == get_rmt_const("V5", &remote_data) && (get_rmt_const("V5", &remote_data) != "?")) {
    *vane_ = 5;
  }
  else if ((*bit_).substring(VNP, VNP + VNL) == get_rmt_const("VR", &remote_data) && (get_rmt_const("VR", &remote_data) != "?")) {
    *vane_ = 6;
  }       
  else {
    *vane_ = -1;
     
  }

  //--> panasonic_128, waiting for comp_on, because we have same signal for on/off
  if (spc_bit_rx(bit_,power_,mode_,Trset_,fan_, vane_ ,&remote_data) && (rmt_num == 701)){
    return;
  }


  if (show_data){
    if (*power_ == -1){
      Serial.println(F("ERROR -> do not change power_status"));
    }
    if (*mode_ == -1){
      Serial.println(F("ERROR -> do not change mode"));
    }
    if (*Trset_ == -1){
      Serial.println(F("ERROR -> do not change temperatur"));   
    }
    if (*fan_ == -1){
      Serial.println(F("ERROR -> do not change fan")); 
    }
    if (*vane_ == -1){
      Serial.println(F("ERROR -> do not change vanetilator"));
    }
  }
}

uint16_t IRRemote::GetZero(uint16_t ps_cnt){
  //int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  int i = 0, y = 0;
  uint16_t jj = 0, xx = 0;
  jj = ps_buffer[1];
  xx = 0;//sum
  y = 0;//counter for sum

  /* 1. finding minimum*/
  for (i = 1; i < (ps_cnt / 2 - 1); i = i + 2){
    if ((ps_buffer[i] > 200) && (ps_buffer[i] < jj)){
      jj = ps_buffer[i];
    }
  }
  /* 2. calc average*/
  for (i = 1; i < (ps_cnt / 2 - 1); i = i + 2){
    if (ps_buffer[i] <= jj + 160){
      xx += ps_buffer[i]; //adding it to calc average
      y = y + 1;
      if (y >= 8){
        xx = xx / 8;
        break;
      }
    }
  }
  return xx; //--> zeroSpace
}

uint16_t IRRemote::GetOne(uint16_t ps_cnt){
  //int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  int i = 0, y = 0;
  uint16_t jj = 0, xx = 0, yy = 0;
  yy = GetZero(ps_cnt);
  xx = 0;//--> sum
  y = 0;//--> counter for sum
  jj = ps_buffer[1];
  /* 1. finding maximum, that bigger than zero*/
  for (i = 1; i < ps_cnt -1; i = i + 2){
    if (ps_buffer[i] > jj){
      jj = ps_buffer[i];
    }
  } 

  //-->2. finding minimum
  for (i = 1; i < ps_cnt -1; i = i + 2){
    if ((ps_buffer[i] < jj) && (ps_buffer[i] > yy + 300)){
      jj = ps_buffer[i];
    }
  } 
  /* 3. calc average*/
  for (i = 1; i < ps_cnt - 1; i = i + 2){
    if ((jj <= ps_buffer[i]) && (ps_buffer[i] <= jj + 400)){   
      xx += ps_buffer[i]; //adding it to calc average
      y = y + 1;
      if (y >= 4){

        xx = xx / 4;
        break;
      }
    }
  }
  return xx; //--> One
}

String IRRemote::get_rmt_data_from_eeprom(uint16_t *rmt_num){
  switch (*rmt_num){
    case 101:
      return String(FPSTR(remote_table[0])); break;
    case 102:
      return String(FPSTR(remote_table[1])); break;
    case 103:
      return String(FPSTR(remote_table[2])); break;
    case 104:
      return String(FPSTR(remote_table[3])); break;
    case 105:
      return String(FPSTR(remote_table[4])); break;
    case 201:
      return String(FPSTR(remote_table[5])); break;
    case 202:
      return String(FPSTR(remote_table[6])); break;
    case 203:
      return String(FPSTR(remote_table[7])); break;
    case 204:
      return String(FPSTR(remote_table[8])); break;
    case 205:
      return String(FPSTR(remote_table[9])); break;
    case 301:
      return String(FPSTR(remote_table[10])); break;
    case 302:
      return String(FPSTR(remote_table[11])); break;
    case 303:
      return String(FPSTR(remote_table[12])); break;
    case 304:
      return String(FPSTR(remote_table[13])); break;
    case 305:
      return String(FPSTR(remote_table[14])); break;
    case 401:
      return String(FPSTR(remote_table[15])); break;
    case 402:
      return String(FPSTR(remote_table[16])); break;
    case 403:
      return String(FPSTR(remote_table[17])); break;
    case 404:
      return String(FPSTR(remote_table[18])); break;
    case 405:
      return String(FPSTR(remote_table[19])); break;
    case 501:
      return String(FPSTR(remote_table[20])); break; 
    case 502:
      return String(FPSTR(remote_table[21])); break;  
    case 503:
      return String(FPSTR(remote_table[22])); break;  
    case 504:
      return String(FPSTR(remote_table[23])); break;  
    case 505:
      return String(FPSTR(remote_table[24])); break;  
    case 601:
      return String(FPSTR(remote_table[25])); break;  
    case 602:
      return String(FPSTR(remote_table[26])); break;  
    case 603:
      return String(FPSTR(remote_table[27])); break;  
    case 604:
      return String(FPSTR(remote_table[28])); break;  
    case 605:
      return String(FPSTR(remote_table[29])); break;  
    case 701:
      return String(FPSTR(remote_table[30])); break;  
    case 702:
      return String(FPSTR(remote_table[31])); break;  
    case 703:
      return String(FPSTR(remote_table[32])); break;
    case 704:
      return String(FPSTR(remote_table[33])); break;
    case 705:
      return String(FPSTR(remote_table[34])); break;
    case 801:
      return String(FPSTR(remote_table[35])); break;
    case 802:
      return String(FPSTR(remote_table[36])); break;
    case 803:
      return String(FPSTR(remote_table[37])); break;
    case 804:
      return String(FPSTR(remote_table[38])); break;
    case 805:
      return String(FPSTR(remote_table[39])); break;
    case 901:
      return String(FPSTR(remote_table[40])); break;
    case 902:
      return String(FPSTR(remote_table[41])); break;
    case 903:
      return String(FPSTR(remote_table[42])); break;
    case 904:
      return String(FPSTR(remote_table[43])); break;
    case 905:
      return String(FPSTR(remote_table[44])); break;
    case 1001:
      return String(FPSTR(remote_table[45])); break;
    case 1002:
      return String(FPSTR(remote_table[46])); break;
    case 1003:
      return String(FPSTR(remote_table[47])); break;
    case 1004:
      return String(FPSTR(remote_table[48])); break;
    case 1005:
      return String(FPSTR(remote_table[49])); break;
    case 1101:
      return String(FPSTR(remote_table[50])); break;
    case 1102:
      return String(FPSTR(remote_table[51])); break;
    case 1103:
      return String(FPSTR(remote_table[52])); break;
    case 1104:
      return String(FPSTR(remote_table[53])); break;
    case 1105:
      return String(FPSTR(remote_table[54])); break;
    case 1201:
      return String(FPSTR(remote_table[55])); break;
    case 1202:
      return String(FPSTR(remote_table[56])); break;
    case 1203:
      return String(FPSTR(remote_table[57])); break;
    case 1204:
      return String(FPSTR(remote_table[58])); break;
    case 1205:
      return String(FPSTR(remote_table[59])); break;
    case 1301:
      return String(FPSTR(remote_table[60])); break;
    case 1302:
      return String(FPSTR(remote_table[61])); break;
    case 1303:
      return String(FPSTR(remote_table[62])); break;
    case 1304:
      return String(FPSTR(remote_table[63])); break;
    case 1305:
      return String(FPSTR(remote_table[64])); break;
    case 1401:
      return String(FPSTR(remote_table[65])); break;
    case 1402:
      return String(FPSTR(remote_table[66])); break;
    case 1403:
      return String(FPSTR(remote_table[67])); break;
    case 1404:
      return String(FPSTR(remote_table[68])); break; 
    case 1405:
      return String(FPSTR(remote_table[69])); break;           
    default:
      return "none"; break;                            
  }
}

/************************************************************************************************************
 * spc_bit_rx()
 * 1. this function just comparing bit with constant such as LG off, that is difference between another bit
 * **********************************************************************************************************
 */
bool IRRemote::spc_bit_rx(String *spec_bit_, int *spec_power_, int *spec_mode_, int *spec_temp_, int *spec_fan_, int *spec_vane_, String *remote_data){
  //--> lg_32 swing
  if (((*spec_bit_).substring(2,27) == get_rmt_const("SWI", remote_data).substring(2,27)) && (rmt_num == 501)){
    if (show_data){
      Serial.println(F("LG swing"));
    }
    *spec_power_ = air_status.power;
    *spec_mode_ = air_status.mode;
    *spec_temp_ = air_status.Trset;
    *spec_fan_ = air_status.fan;
    *spec_vane_ = air_status.vane;
    return 1;
  }

  //--> toshiba_96 swing
  if (((*spec_bit_).substring(48,96) == get_rmt_const("SWI", remote_data).substring(48,96)) && (rmt_num == 101)){
    if (show_data){
      Serial.println(F("toshiba_96 swing"));
    }
    *spec_power_ = air_status.power;
    *spec_mode_ = air_status.mode;
    *spec_temp_ = air_status.Trset;
    *spec_fan_ = air_status.fan;
    *spec_vane_ = air_status.vane;
    return 1;
  }

  //--> toshiba_144 off
  if (((*spec_bit_).substring(get_rmt_const("PWP", remote_data).toInt(),get_rmt_const("PWP", remote_data).toInt() + get_rmt_const("PWL", remote_data).toInt()) == get_rmt_const("OFF", remote_data)) && (rmt_num == 102)){
    if (show_data){
      Serial.println(F("toshiba_144 power_off"));
    }
    *spec_power_ = 0;
    *spec_mode_ = air_status.mode;
    *spec_temp_ = air_status.Trset;
    *spec_fan_ = air_status.fan;
    *spec_vane_ = air_status.vane;
    return 1;
  }

  //--> toshiba_144 swing|fix
  if ((((*spec_bit_).indexOf(get_rmt_const("SWI1", remote_data)) != -1) || ((*spec_bit_).indexOf(get_rmt_const("SWI2", remote_data)) != -1) || ((*spec_bit_).indexOf(get_rmt_const("FIX", remote_data)) != -1)) && (rmt_num == 102)){
    if (show_data){
      Serial.println(F("toshiba_144 swing|fix"));
    }
    *spec_power_ = air_status.power;
    *spec_mode_ = air_status.mode;
    *spec_temp_ = air_status.Trset;
    *spec_fan_ = air_status.fan;
    *spec_vane_ = air_status.vane;
    return 1;
  }

  //--> lg_32 off
  if (((*spec_bit_).substring(2,27) == get_rmt_const("PWF", remote_data).substring(2,27)) && (rmt_num == 501)){
    if (show_data){
      Serial.println(F("lg_32 power_off"));
    }
    *spec_power_ = 0;
    *spec_mode_ = air_status.mode;
    *spec_temp_ = air_status.Trset;
    *spec_fan_ = air_status.fan;
    *spec_vane_ = air_status.vane;    
    return 1; 
  }

  //--> toshiba_96 off num = 101
  if (((*spec_bit_).substring(48,96) == get_rmt_const("PWF", remote_data).substring(48,96)) && (rmt_num == 101)){
    if (show_data){
      Serial.println(F("toshiba power_off"));
    }
    *spec_power_ = 0;
    *spec_mode_ = air_status.mode;
    *spec_temp_ = air_status.Trset;
    *spec_fan_ = air_status.fan;
    *spec_vane_ = air_status.vane;    
    return 1; 
  }

  //--> toshiba_144 off num = 102
  if (((*spec_bit_).substring(72,144) == get_rmt_const("PWF", remote_data).substring(72,144)) && (rmt_num == 102)){
    if (show_data){
      Serial.println(F("toshiba power_off"));
    }
    *spec_power_ = 0;
    *spec_mode_ = air_status.mode;
    *spec_temp_ = air_status.Trset;
    *spec_fan_ = air_status.fan;
    *spec_vane_ = air_status.vane;    
    return 1; 
  }

  if (((*spec_bit_).substring(16,21) == get_rmt_const("PWF", remote_data)) && (rmt_num == 701)){
    *spec_power_ = 0;
    *spec_mode_ = air_status.mode;
    *spec_temp_ = air_status.Trset;
    *spec_fan_ = air_status.fan;
    *spec_vane_ = air_status.vane;
    if (air_status.Trset == 0){
      *spec_temp_ = 25;
    }
  }
  return 0;
}

/*****************************
 * send_ir_control()
 * 1. use cool 30°C to cut
 * ***************************
 */

void IRRemote::cut(){
  if (rmt_num == 903){
    rmt_num = 902; min_bit_length = 56;
  }
  if (rmt_num == 902){//cut for samsung
    send_ir(1, 3, 30, air_status.fan, air_status.vane);
  }
  else if (rmt_num == 1002){//cut for sharp
    send_ir(1, 1, 32, air_status.fan, air_status.vane);
  }
  else{
    send_ir(1, 1, 30, air_status.fan, air_status.vane);
  }
  last_cut_t = system_get_rtc_time();
  //Serial.println(show_data);
  if (show_data){
    Serial.println(F("do smart cut!"));
  }
  //delay(800); // delay to prevent to recieve own signal in IR-Rx
  
}

/*************************************************************************************
 * cool mode
 * 1. samsung has 2 difference remote, they need to switch with each other
 * ***********************************************************************************
 */
void IRRemote::cool(){
  if (rmt_num == 903){
    rmt_num = 902; min_bit_length = 56;
  }
  send_ir(1, 1, air_status.Trset, air_status.fan, air_status.vane);//send ir use same status, but tr - diff_btw_tr_and_tcool, because we make sure that a coil will not cut it self further than s-air
  last_cut_t = system_get_rtc_time();
  if (show_data){
    Serial.println(F("do cool mode!"));
  }
  //delay(800); // delay to prevent to recieve own signal in IR-Rx
}



/********************************************************************************************************************************************************
 * send_ir(int Power, int Mode, int Temp, int Fan, int Vane )
 * 1. this function replace PFB = prefixbit with remote data bit before we calling "get_checksum(String *bit_ir)" in "ir tx final bit modify"
 * ******************************************************************************************************************************************************
 */
void IRRemote::send_ir(int Power, int Mode, int Temp, int Fan, int Vane){
  
  //--------------------------------------------------------\\
  //This function will calulate a bit_code before sending it\\
  //--------------------------------------------------------\\

  String bit_ir = "";
  String bit_ir_temp = "";
  String remote_data = get_rmt_data_from_eeprom(&rmt_num);
  uint16_t CDL = get_rmt_const("CDL", &remote_data).toInt();
  uint16_t PWP = get_rmt_const("PWP", &remote_data).toInt();
  uint16_t PWL = get_rmt_const("PWL", &remote_data).toInt();
  uint16_t MDP = get_rmt_const("MDP", &remote_data).toInt();
  uint16_t MDL = get_rmt_const("MDL", &remote_data).toInt();
  uint16_t TMPP = get_rmt_const("TMPP", &remote_data).toInt();
  uint16_t TMPL = get_rmt_const("TMPL", &remote_data).toInt();
  uint16_t FNP = get_rmt_const("FNP", &remote_data).toInt();
  uint16_t FNL = get_rmt_const("FNL", &remote_data).toInt();
  uint16_t VNP = get_rmt_const("VNP", &remote_data).toInt();
  uint16_t VNL = get_rmt_const("VNL", &remote_data).toInt();
  bit_ir = get_rmt_const("PFB", &remote_data);// prefix_bit
  //Serial.println(bit_ir);
  Serial.print("Mode>");Serial.println(Mode);

  //--> calling spec function
  if (Mode == 3){
    bit_ir = get_const_bit_fan(Fan, Vane);
    if (show_data){
      Serial.print(F("use fan mode = ")); Serial.println(bit_ir);
    }
    bit_to_ps_then_send(&bit_ir, &remote_data); // Send twice, hier is the first one
    Serial.println("Fan mode");
    delay(500); // delay before next send
  }
  else{
    //---------------------------------------------------------- ON/OFF
    bit_ir_temp = bit_ir.substring(0, PWP);//replacing   
    //power_status ON or OFF
    if (Power){
      bit_ir_temp += get_rmt_const("ON", &remote_data);
    }
    else {
      bit_ir_temp += get_rmt_const("OFF", &remote_data);
    } 
    bit_ir_temp += bit_ir.substring(bit_ir_temp.length());
    bit_ir = bit_ir_temp;
    //ON/OFF
    //Serial.println(bit_ir);
  
    //---------------------------------------------------------- MODE
    bit_ir_temp = bit_ir.substring(0, MDP);//replacing
    if (Mode == 0){
      bit_ir_temp += get_rmt_const("AUTO", &remote_data);
    }
    else if (Mode == 1){
      bit_ir_temp += get_rmt_const("COOL", &remote_data); 
    }
    else if (Mode == 2){
      bit_ir_temp += get_rmt_const("DRY", &remote_data);
    }
    else if (Mode == 3){
      bit_ir_temp += get_rmt_const("FAN", &remote_data); 
    }        
    bit_ir_temp += bit_ir.substring(bit_ir_temp.length());
    bit_ir = bit_ir_temp;
    //MODE
    //Serial.println(bit_ir);
      
    //---------------------------------------------------------- TEMP
    bit_ir_temp = bit_ir.substring(0, TMPP);//replacing
    bit_ir_temp += get_rmt_const("T" + String(Temp), &remote_data);     
    bit_ir_temp += bit_ir.substring(bit_ir_temp.length());
    bit_ir = bit_ir_temp;       
    //TEMP
    //Serial.println(bit_ir);
    
    //---------------------------------------------------------- FAN
    bit_ir_temp = bit_ir.substring(0, FNP);//replacing
    if (Fan == 0){
      bit_ir_temp += get_rmt_const("FA", &remote_data);
    }
    else if (Fan == 1){
      bit_ir_temp += get_rmt_const("F1", &remote_data);  
    }
    else if (Fan == 2){
      bit_ir_temp += get_rmt_const("F2", &remote_data); 
    }
    else if (Fan == 3){
      bit_ir_temp += get_rmt_const("F3", &remote_data); 
    }
    else if (Fan == 4){
      bit_ir_temp += get_rmt_const("F4", &remote_data); 
    }
    else if (Fan == 5){
      bit_ir_temp += get_rmt_const("F5", &remote_data); 
    }            
    bit_ir_temp += bit_ir.substring(bit_ir_temp.length());
    bit_ir = bit_ir_temp;
    //FAN
    //Serial.println(bit_ir);
    
    //---------------------------------------------------------- VANE
    bit_ir_temp = bit_ir.substring(0, VNP);//replacing
    if (Vane == 0)
    {
      bit_ir_temp += get_rmt_const("VA", &remote_data);
    }
    else if (Vane == 1){
      bit_ir_temp += get_rmt_const("V1", &remote_data);  
    }
    else if (Vane == 2){
      bit_ir_temp += get_rmt_const("V2", &remote_data); 
    }
    else if (Vane == 3){
      bit_ir_temp += get_rmt_const("V3", &remote_data);  
    }
    else if (Vane == 4){
      bit_ir_temp += get_rmt_const("V4", &remote_data);  
    }
    else if (Vane == 5){
      bit_ir_temp += get_rmt_const("V5", &remote_data);  
    }
    else if (Vane == 6){
      bit_ir_temp += get_rmt_const("VR", &remote_data);  
    }   
    bit_ir_temp += bit_ir.substring(bit_ir_temp.length());
    bit_ir = bit_ir_temp;//code_without_check_sum       
    get_checksum(&bit_ir);
    if (show_bit){
      Serial.print(F("finish code = "));Serial.println(bit_ir);
    }
    //Serial.println("bit_to_ps_then_send");
    bit_to_ps_then_send(&bit_ir, &remote_data);
  }
}



/*********************************************************************************
 * bit_to_ps_then_send(String *bit_code_, String *remote_data)
 * 1. this function convert bit to ir signal and do some modify
 * 2. after converting we send ir signal to ait-condition
 * *******************************************************************************
 */
void IRRemote::bit_to_ps_then_send(String *bit_code_, String *remote_data){
  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  //---------------------------------------------------\\
  //This function calculate ps_buffer and send IR_Signal\\
  //---------------------------------------------------\\

  String additional_mark_space_pos = "",additional_mark,additional_space = "";//value of parameter  
  uint16_t CDL = get_rmt_const("CDL", remote_data).toInt();
  int bit_mark = 0, one_space = 0, zero_space = 0,additional_mark_space_cout = 0, repeat_time = 1;
  for (i = 0; i < 586; i++){//--> clearing big_array
    ps_buffer[i] = 0;
  }
  if (rmt_num == 501){
    repeat_time = 3;    
  }
  else if (rmt_num == 702){
    repeat_time = 2;
  }
  for (x = 0; x < repeat_time; x++){
    j = 0;//pointer of ps_buffer
    /*
     * reading data
     */
    additional_mark_space_cout = get_rmt_const("APSCNT", remote_data).toInt();
    additional_mark_space_pos = get_rmt_const("APSP", remote_data);
    additional_mark = get_rmt_const("APL", remote_data);
    additional_space = get_rmt_const("ASL", remote_data);
    bit_mark = get_rmt_const("BPL", remote_data).toInt();
    one_space = get_rmt_const("OSL", remote_data).toInt();
    zero_space = get_rmt_const("ZSL", remote_data).toInt();
    if (rmt_num == 203){
      for (j = 0; j < 10; j = j + 2){
        //ps_buffer[j] = bit_mark + 100;
        ps_buffer[j] = bit_mark;
        ps_buffer[j+1] = zero_space;
      }
      j = 10;
    }
    for (i = 0; i < CDL; i++){
      while ((i == additional_mark_space_pos.substring(additional_mark_space_pos.indexOf("[")+1,additional_mark_space_pos.indexOf("]")).toInt()) && (additional_mark_space_pos.length() != 0)){
        ps_buffer[j] = additional_mark.substring(additional_mark.indexOf("[")+1,additional_mark.indexOf("]")).toInt() - 100;
        additional_mark = additional_mark.substring(additional_mark.indexOf("]")+1);//delete first mark
        j++;
        ps_buffer[j] = additional_space.substring(additional_space.indexOf("[")+1,additional_space.indexOf("]")).toInt() + 80;
        additional_space = additional_space.substring(additional_space.indexOf("]")+1);//delete first space
        j++;               
        additional_mark_space_pos = additional_mark_space_pos.substring(additional_mark_space_pos.indexOf("]")+1);//delete first pos 
      }
      if ((*bit_code_).substring(0,1) == "0"){
        //ps_buffer[j] = bit_mark + 100;
        ps_buffer[j] = bit_mark;
        j++;
        ps_buffer[j] = zero_space;
        j++;      
        (*bit_code_) = (*bit_code_).substring(1);//delete first bit
      }
      else{
        //ps_buffer[j] = bit_mark + 50;
        ps_buffer[j] = bit_mark;
        j++;
        ps_buffer[j] = one_space;
        j++;      
        (*bit_code_) = (*bit_code_).substring(1);//delete first bit
      }
    }
    if (rmt_num == 701){ //--> adding last pulses for panasonic_128 only
      ps_buffer[j] = 3600;
      j++;
      ps_buffer[j] = 3340;
      j++;
    }
    else if (rmt_num == 601){
      ps_buffer[j] = 580;
      j++;
      ps_buffer[j] = 7400;
      j++;
    }
    //ps_buffer[j] = bit_mark + 50;//adding last mark
    ps_buffer[j] = bit_mark;//adding last mark
    j++;
    if(show_signal){
      Serial.println("IR>");
      for (i = 0; i < j + 2; i++){
        Serial.print(ps_buffer[i]);
        Serial.print(',');
      }
      Serial.println();
    }
    irsend.sendRaw(ps_buffer, j + 2, khz);//sending ir_signal
    delay(300); // delay between each repeat
  }
}

/*************************************************************************************
 * get_checksum(String *bit_ir)
 * 1. this function is very important it modify bit of each remote before sending
 * 2. all remote and all new remote must be programmed in this function
 * ***********************************************************************************
 */
void IRRemote::get_checksum(String *bit_ir){
  //int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  int h = 0,i = 0,j = 0, x = 0;
  String temp = "";
  int sum = 0;
  String sum_bin = "";
  switch (rmt_num){
    /*
     * toshiba_96
     */
    case (101):
      *bit_ir = (*bit_ir).substring(0,72) + get_F_minus((*bit_ir).substring(64,68)) + get_F_minus((*bit_ir).substring(68,72)) + (*bit_ir).substring(80,88) + get_F_minus((*bit_ir).substring(80,84)) + get_F_minus((*bit_ir).substring(84,88));
      *bit_ir = (*bit_ir ).substring(48,96) + (*bit_ir ).substring(48,96);
      temp = "";
      //Serial.print(F("toshiba_96 in MSB --> ")); Serial.println(*bit_ir);
      break;

    /*
     * toshiba_144
     */
    case (102):
      temp = "";
      for (i = 0; i < 8; i++){
        x = 0;
        for (j = 9; j < 17; j++){
          if ((*bit_ir).substring(j*8 + i,j*8 + i + 1) == "1"){
            x++;
          }
        }
        temp += String(x%2);
      }
      *bit_ir = (*bit_ir).substring(72,136) + temp + (*bit_ir).substring(72,136) + temp;
      //Serial.print(F("toshiba_96 in MSB --> ")); Serial.println(*bit_ir);
      break;   

    /*    
     *  daikin_64
     */
    case (201):
      for (i = 0; i < 14; i++){
        temp = (*bit_ir).substring(i*4,i*4+4);//copy_bit to calculate
        for (j = 0; j < 4; j++) 
        {
          sum += temp.substring(3-j,3-j+1).toInt()*pow(2,j);
        }
      }
      sum = sum + 4;
      temp = String(sum, BIN);
      *bit_ir = (*bit_ir).substring(0,56) + temp.substring(temp.length() - 4);
      if (air_status.power){
        *bit_ir += F("11");
      }
      else{
        *bit_ir += F("01");
      }
      if (air_status.fan){
        *bit_ir += F("01");
      }
      else{
        *bit_ir += F("00");
      }
      temp = "";
      for (i = 0; i < 8; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      break;

    /*  
     *   daikin_216
     */
    case (202):
      for (i = 0; i < 26; i++){
        temp = (*bit_ir).substring(i*8,i*8+8);//copy_bit to calculate
        for (j = 0; j < 8; j++){
          sum += temp.substring(7-j,7-j+1).toInt()*pow(2,j);
        }
      }
      //last time sum = sum - 5,at 03/06/2018
      sum = sum - 4;
      temp = String(sum, BIN);
      *bit_ir = (*bit_ir).substring(0,208) + temp.substring(temp.length() - 8);
      temp = "";
      for (i = 0; i < 27; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      //Serial.print(F("daikin_216 in MSB = ")); Serial.println(*bit_ir);
      break;

    /*
     * daikin_280
     */
    case (203):
      for (i = 21; i < 34; i++){
        temp = (*bit_ir).substring(i*8,i*8+8);//copy_bit to calculate
        for (j = 0; j < 8; j++){
          sum += temp.substring(7-j,7-j+1).toInt()*pow(2,j);
        }
      }
      sum = sum + 18;
      temp = String(sum, BIN);
      *bit_ir = (*bit_ir).substring(0,272) + temp.substring(temp.length() - 8);
      //Serial.print(F("daikin_280 in LSB8 = ")); Serial.println(*bit_ir);
      temp = "";
      for (i = 0; i < 35; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      //Serial.print(F("daikin_280 in MSB = ")); Serial.println(*bit_ir);
      break;  
      
    /*
     * heier_112
     */
    case (301):
      for (i = 0; i < 13; i++){
        temp = (*bit_ir).substring(i*8,i*8+8);//copy_bit to calculate
        for (j = 0; j < 8; j++){
          sum += temp.substring(7-j,7-j+1).toInt()*pow(2,j);
        }
      }
      temp = String(sum, BIN);
      *bit_ir = (*bit_ir).substring(0,108) + temp.substring(temp.length() - 8);
      break;

    /*
     * hitashi_104
     */
    case (401):
      for (i = 10; i < 14; i++){
        temp = (*bit_ir).substring(i*4,i*4+4);//copy_bit to calculate
        for (j = 0; j < 4; j++) {
          sum += temp.substring(3-j,3-j+1).toInt()*pow(2,j);
        }
      }
      temp = String(sum, BIN);
      switch (temp.length()){
        case 0:
          temp = "00000000" + temp; break;
        case 1:
          temp = "0000000" + temp; break;
        case 2:
          temp = "000000" + temp; break;
        case 3:
          temp = "00000" + temp; break;
        case 4:
          temp = "0000" + temp; break;
        case 5:
          temp = "000" + temp; break;
        case 6:
          temp = "00" + temp; break;
        case 7:
          temp = "0" + temp; break;
      }
      *bit_ir = (*bit_ir).substring(0,96) + temp;
      //Serial.print(F("hitashi_104 in LSB8 --> ")); Serial.println(*bit_ir);
      temp = "";
      for (i = 0; i < 13; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      //Serial.print(F("hitashi_104 in MSB --> ")); Serial.println(*bit_ir);
      break;  

    /*
     * hitashi_264
     */
    case (402):
      *bit_ir = (*bit_ir).substring(0,96) + get_F_minus((*bit_ir).substring(88,92)) + get_F_minus((*bit_ir).substring(92,96)) + (*bit_ir).substring(104,112)
      + get_F_minus((*bit_ir).substring(104,108)) + get_F_minus((*bit_ir).substring(108,112)) + (*bit_ir).substring(120,208) + get_F_minus((*bit_ir).substring(200,204)) + get_F_minus((*bit_ir).substring(204,208))
      + (*bit_ir).substring(216,224) + get_F_minus((*bit_ir).substring(216,220)) + (*bit_ir).substring(228,264);
      temp = "";
      if (show_bit){
        Serial.print(F("LSB = ")); Serial.println(*bit_ir);
      }
      for (i = 0; i < 33; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      //Serial.print(F("mitsubishi_48 in MSB --> ")); Serial.println(*bit_ir);
      break;
         
    /*
     * lg_28
     */
    case (501):
      for (i = 0; i < 6; i++){
        temp = (*bit_ir).substring(i*4,i*4+4);//copy_bit to calculate
        for (j = 0; j < 4; j++){
          sum += temp.substring(3-j,3-j+1).toInt()*pow(2,j);
        }
      }
      sum = sum - 1;
      temp = String(sum, BIN);
      *bit_ir = (*bit_ir).substring(0,24) + temp.substring(temp.length() - 4);
      sum++;
      temp = String(sum, BIN);
      *bit_ir = *bit_ir + (*bit_ir).substring(0,24) +  temp.substring(temp.length() - 4);
      sum++;
      temp = String(sum, BIN);
      *bit_ir = *bit_ir + (*bit_ir).substring(0,24) +  temp.substring(temp.length() - 4);
      //Serial.print(F("lg_28 in MSB --> ")); Serial.println(*bit_ir);
      break;

    /*
     * mitsu 48
     */
    case (601):
      *bit_ir = (*bit_ir).substring(0,8) + get_F_minus((*bit_ir).substring(0,4)) + get_F_minus((*bit_ir).substring(4,8)) + (*bit_ir).substring(16,24) + get_F_minus((*bit_ir).substring(16,20)) + get_F_minus((*bit_ir).substring(20,24)) + (*bit_ir).substring(32,48);
      temp = "";
      //Serial.print(F("mitsubishi_48 in LSB8 --> ")); Serial.println(*bit_ir);
      for (i = 0; i < 6; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      //Serial.print(F("mitsubishi_48 in MSB --> ")); Serial.println(*bit_ir);
      break;  

      /*
       * mitsubishi_88
       */
    case (602):
      if ((air_status.vane != 0) && (air_status.vane != 1) && (air_status.vane != 6)){
        *bit_ir = (*bit_ir).substring(0,44) + "110100000010" + (*bit_ir).substring(56,88);
      }
      else{
        *bit_ir = (*bit_ir).substring(0,44) + "111100000000" + (*bit_ir).substring(56,88);
      }
      *bit_ir = (*bit_ir).substring(0,64) + get_F_minus((*bit_ir).substring(56,60)) + (*bit_ir).substring(68,80) + get_F_minus((*bit_ir).substring(72,76)) + get_F_minus((*bit_ir).substring(76,80));
      temp = "";
      if (show_bit){
        Serial.print(F("LSB8 = ")); Serial.println(*bit_ir);
      }
      for (i = 0; i < 11; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      break;  
     /*
       * mitsubishi_112
       */
    case (603):
      for (i = 0; i < 13; i++){
        temp = (*bit_ir).substring(i*8,i*8+8);//copy_bit to calculate
        for (j = 0; j < 8; j++){
          sum += temp.substring(7-j,7-j+1).toInt()*pow(2,j);
        }
      }
      sum = sum;
      temp = String(sum, BIN);
      *bit_ir = (*bit_ir).substring(0,104) + temp.substring(temp.length() - 8);
      temp = "";
      if (show_bit){
        Serial.print(F("LSB8 = ")); Serial.println(*bit_ir);
      }
      for (i = 0; i < 14; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      //Serial.print(F("mitsubishi_288 in MSB --> ")); Serial.println(*bit_ir);
      break;   

      /*
       * mitsubishi_136
       */
    case (604):
      *bit_ir = (*bit_ir).substring(0,88) + get_F_minus((*bit_ir).substring(40,44)) + get_F_minus((*bit_ir).substring(44,48)) + get_F_minus((*bit_ir).substring(48,52))
       + get_F_minus((*bit_ir).substring(52,56)) + get_F_minus((*bit_ir).substring(56,60)) + get_F_minus((*bit_ir).substring(60,64)) + (*bit_ir).substring(112,136);
      temp = "";
      if (show_bit){
        Serial.print(F("LSB8 = ")); Serial.println(*bit_ir);
      }
      for (i = 0; i < 17; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      break;    
      
      /*
       * mitsubishi_288
       */
    case (605):
      for (i = 18; i < 35; i++){
        temp = (*bit_ir).substring(i*8,i*8+8);//copy_bit to calculate
        for (j = 0; j < 8; j++) {
          sum += temp.substring(7-j,7-j+1).toInt()*pow(2,j);
        }
      }
      //sum = sum - 1; was chaged at 03/06/2918
      sum = sum;
      temp = String(sum, BIN);
      *bit_ir = (*bit_ir).substring(144,280) + temp.substring(temp.length() - 8);
      *bit_ir = *bit_ir + *bit_ir;
      temp = "";
      for (i = 0; i < 36; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      break;            
    
    /*
     * panasonic_128 need to find PWF
     */
    case (701):
      temp = "";
      temp += (*bit_ir).substring(8,16);//--> fan&temp
      temp += (*bit_ir).substring(8,16);
      temp += (*bit_ir).substring(16,32);//--> mode
      temp += (*bit_ir).substring(8,16);//--> fan&temp
      temp += (*bit_ir).substring(8,16);
      temp += (*bit_ir).substring(16,32);//--> mode
      temp += (*bit_ir).substring(64,80);//--> vane
      temp += (*bit_ir).substring(80,96);//--> on
      temp += (*bit_ir).substring(64,80);
      temp += (*bit_ir).substring(80,96);
      (*bit_ir) = temp;
      temp = "";
      for (i = 0; i < 16; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
//    Serial.print(F("panasonic_128 in LSB8 --> ")); Serial.println(bit_code);
      break;
    
    /*
     * panasonic_216, thare are two wave
     */
    case (702):
      for (i = 0; i < 26; i++){
        temp = (*bit_ir).substring(i*8,i*8+8);//copy_bit to calculate
        for (j = 0; j < 8; j++){
          sum += temp.substring(7-j,7-j+1).toInt()*pow(2,j);
        }
      }
      sum = sum - 13;
      //Serial.println(sum);
      temp = String(sum, BIN);
      switch (temp.length()){
        case 0:
          temp = "00000000" + temp; break;
        case 1:
          temp = "0000000" + temp; break;
        case 2:
          temp = "000000" + temp; break;
        case 3:
          temp = "00000" + temp; break;
        case 4:
          temp = "0000" + temp; break;
        case 5:
          temp = "000" + temp; break;
        case 6:
          temp = "00" + temp; break;
        case 7:
          temp = "0" + temp; break;
      }
      //Serial.println(temp);
      *bit_ir = (*bit_ir).substring(0,208) + temp.substring(temp.length() - 8);

      //second wave inc sum, then calculate
      sum++;
      temp = String(sum, BIN);
      switch (temp.length()){
        case 0:
          temp = "00000000" + temp; break;
        case 1:
          temp = "0000000" + temp; break;
        case 2:
          temp = "000000" + temp; break;
        case 3:
          temp = "00000" + temp; break;
        case 4:
          temp = "0000" + temp; break;
        case 5:
          temp = "000" + temp; break;
        case 6:
          temp = "00" + temp; break;
        case 7:
          temp = "0" + temp; break;
      }
      *bit_ir = *bit_ir + (*bit_ir).substring(0,208) + temp.substring(temp.length() - 8);
      temp = "";
      for (i = 0; i < 54; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      break;

    /*        
     * saijo_64        
     */
    case (801):
      (*bit_ir) = (*bit_ir).substring(0,40);
      for (i = 2; i < 6; i++){
        temp = (*bit_ir).substring(i*4,i*4+4);//copy_bit to calculate
        sum = 0;
        for (j = 0; j < 4; j++){
          sum += temp.substring(3-j,3-j+1).toInt()*pow(2,j);
        }
        temp = String(15-sum,BIN);
        if (temp.length() == 0){
          *bit_ir += F("0000");
        }
        else if (temp.length() == 1){
          *bit_ir += F("000");
        }
        else if (temp.length() == 2){
          *bit_ir += F("00");
        }
        else if (temp.length() == 3){
          *bit_ir += F("0");
        }
        *bit_ir += temp;
      }
      *bit_ir += F("10110111");
      break;   

    /*
     * saijo_128
     */
    case (802):
      *bit_ir = (*bit_ir).substring(0,8) + (*bit_ir).substring(72,79) + "0" + (*bit_ir).substring(80,88) + (*bit_ir).substring(24,40) + get_F_minus((*bit_ir).substring(72,76)) + get_F_minus((*bit_ir).substring(76,79) + "0") 
      + get_F_minus((*bit_ir).substring(80,84)) + get_F_minus((*bit_ir).substring(84,88)) + (*bit_ir).substring(56,104) + get_F_minus((*bit_ir).substring(72,76)) + get_F_minus((*bit_ir).substring(76,80)) + get_F_minus((*bit_ir).substring(80,84)) + get_F_minus((*bit_ir).substring(84,88)) + "11111111";
      break;    

    /*  
     *  samsung_112   
     */
    case (902):
      for ( i = 68; i < 112; i++){
        if ((*bit_ir).substring(i, i+1) == "1"){
          sum = sum + 1;
        }      
      }  
//      Serial.println(F(""));
//      Serial.print(F("sum: "));
//      Serial.print(sum);
      sum = sum%16;
      switch (sum){
        case 1:sum_bin = F("0000"); break;
        case 2:sum_bin = F("1111"); break;
        case 3:sum_bin = F("1110"); break;
        case 4:sum_bin = F("1101"); break;
        case 5:sum_bin = F("1100"); break;
        case 6:sum_bin = F("1011"); break;
        case 7:sum_bin = F("1010"); break;
        default: break;
      } 
//      Serial.println(F(""));
//      Serial.print(F("sum_bin: "));Serial.print(sum_bin);
      temp = (*bit_ir).substring(0, 64);
      temp += sum_bin;
      temp += (*bit_ir).substring(68, 128);
      (*bit_ir) = temp;
      if (sum_bin == "0000"){
        temp = (*bit_ir).substring(0, 76);
        temp += F("1111");
        temp += (*bit_ir).substring(80, 128);
        (*bit_ir) = temp;
      }
//      Serial.println(F(""));
//      Serial.print(F("LSB8_code need to be transform in MSB"));
      temp = "";
      for (i = 0; i < 14; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      (*bit_ir) = temp;
      break;

    /*
     * sharp_96
     */
    case (1001):
      temp = "0000" + get_F_minus((*bit_ir).substring(36,40)) + get_F_minus((*bit_ir).substring(40,44)) + get_F_minus((*bit_ir).substring(44,48)) + get_F_minus((*bit_ir).substring(48,52))
      + get_F_minus((*bit_ir).substring(52,56)) + get_F_minus((*bit_ir).substring(56,60)) + get_F_minus((*bit_ir).substring(60,64));
      *bit_ir = temp + (*bit_ir).substring(32,64) + temp;
      break;
      
    /*
     * toshiba_144
     */
    case (1002):
      temp = "";
      for (i = 0; i < 4; i++){
        x = 0;
        for (j = 0; j < 24; j++){
          if ((*bit_ir).substring(j*4 + i,j*4 + i + 1) == "1"){
            x++;
          }
        }
        temp += String(x%2);
      }
      if (temp.substring(3,4) == "1"){
        temp = temp.substring(0,3) + "0";
      }
      else{
        temp = temp.substring(0,3) + "1";
      }
      *bit_ir = (*bit_ir).substring(0,96) + temp + "0001";
      temp = "";
      for (i = 0; i < 13; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      //Serial.print(F("toshiba_96 in MSB --> ")); Serial.println(*bit_ir);
      break;     
      
    /*  
     *   turbocool_112
     */
    case (1201):
      for (i = 0; i < 13; i++){
        temp = (*bit_ir).substring(i*8,i*8+8);//copy_bit to calculate
        for (j = 0; j < 8; j++){
          sum += temp.substring(7-j,7-j+1).toInt()*pow(2,j);
        }
      }
      sum = sum + 12;
      temp = String(sum, BIN);
      *bit_ir = (*bit_ir).substring(0,112) + temp.substring(temp.length() - 8);
      temp = "";
      for (i = 0; i < 14; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp;
      break;  

    /*  
     *   universal_44
     */
    case (1401):
      for (i = 0; i < 4; i++){
        temp = (*bit_ir).substring(i*4,i*4+4);//copy_bit to calculate
        for (j = 0; j < 4; j++){
          sum += temp.substring(3-j,3-j+1).toInt()*pow(2,j);
        }
      }
      //Serial.println(sum);
      sum = 31 - sum;
      //Serial.println(sum);
      temp = String(sum, BIN);
      if (temp.length() == 0){
        temp = "0000" + temp;
      }
      else if (temp.length() == 1){
        temp = "000" + temp;
      }
      else if (temp.length() == 2){
        temp = "00" + temp;
      }
      else if (temp.length() == 3){
        temp = "0" + temp;
      }
      //Serial.println(temp);
      *bit_ir = (*bit_ir).substring(0,40) + temp.substring(temp.length() - 4);
      break;
    case (1402):
      for (i = 0; i < 5; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp + "0000";
      break;
    case (1403):
      for (i = 0; i < 5; i++){
        for (j = 0; j < 8; j++){
          temp += (*bit_ir).substring(i*8, (i+1)*8).substring(7-j,7-j+1);
        }       
      }
      *bit_ir = temp + "0000";
      break;         
    default:
      break;  
  }
  if (show_bit){
    Serial.println("MSB = " + (*bit_ir));
  }
}

String IRRemote::get_F_minus(String bit_ir){
  if (bit_ir == "0000"){
    return F("1111");
  }
  else if (bit_ir == "0001"){
    return F("1110");
  }
  else if (bit_ir == "0010"){
    return F("1101");
  }
  else if (bit_ir == "0011"){
    return F("1100");
  }
  else if (bit_ir == "0100"){
    return F("1011");
  }
  else if (bit_ir == "0101"){
    return F("1010");
  }
  else if (bit_ir == "0110"){
    return F("1001");
  }
  else if (bit_ir == "0111"){
    return F("1000");
  }
  else if (bit_ir == "1000"){
    return F("0111");
  }
  else if (bit_ir == "1001"){
    return F("0110");
  }
  else if (bit_ir == "1010"){
    return F("0101");
  }
  else if (bit_ir == "1011"){
    return F("0100");
  }
  else if (bit_ir == "1100"){
    return F("0011");
  }
  else if (bit_ir == "1101"){
    return F("0010");
  }
  else if (bit_ir == "1110"){
    return F("0001");
  }
  else if (bit_ir == "1111"){
    return F("0000");
  }
  // v2.1: Added missing return statement
  return "";
}

String IRRemote::get_const_bit_fan(int Fan, int Vane){//this function return const fan-mode, if remote has it.
//    case (702):  <--------------------------use this as vorlage
//      switch (Fan*10 + Vane){
//        case (0): return F("");
//        case (1): return F("");
//        case (2): return F("");
//        case (3): return F("");
//        case (4): return F("");
//        case (5): return F("");
//        case (10): return F("");
//        case (11): return F("");
//        case (12): return F("");
//        case (13): return F("");
//        case (14): return F("");
//        case (15): return F("");
//        case (20): return F("");
//        case (21): return F("");
//        case (22): return F("");
//        case (23): return F("");
//        case (24): return F("");
//        case (25): return F("");
//        case (30): return F("");
//        case (31): return F("");
//        case (32): return F("");
//        case (33): return F("");
//        case (34): return F("");
//        case (35): return F("");
//        default: return F("");
//      }
//      break;
  switch (rmt_num){
    case (902)://samsung_128
      if (Fan == 1){
        if (Vane == 0){
          return F("0100000001001001111100000000000000000000000000000000111110000000010000110111111110001110000000011010110000001111");
        }
        else{
          return F("0100000001001001111100000000000000000000000000000000111110000000010001110111010110001110000000011010110000001111");
        }
      }
      else if (Fan == 2){
        if (Vane == 0){
          return F("0100000001001001111100000000000000000000000000000000111110000000010000110111111110001110000000011001110000001111");
        }
        else{
          return F("0100000001001001111100000000000000000000000000000000111110000000010001110111010110001110000000011001110000001111");
        }
      }
      else if (Fan == 3){
        if (Vane == 0){
          return F("0100000001001001111100000000000000000000000000000000111110000000010011010111111110001110000000011101110000001111");
        }
        else{
          return F("0100000001001001111100000000000000000000000000000000111110000000010010110111010110001110000000011101110000001111");
        }
      }
      else{
        if (Vane == 0){
          return F("0100000001001001111100000000000000000000000000000000111110000000010011010111111110001110000000011101110000001111");
        }
        else{
          return F("0100000001001001111100000000000000000000000000000000111110000000010010110111010110001110000000011101110000001111");
        }
      }
      break;
    default:
      break;
  }
  // v2.1: Added missing return statement
  return "";
}

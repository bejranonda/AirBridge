/*
 * ------- state_function ---------
 */
void state_function(){
  switch (mem.state){
    case 0:
      state_0(); break;
    case 1:
      state_1(); break;  
    case 2:
      state_2(); break;
    case 3:
      state_3(); break;
    case 4:
      state_4(); break;
    case 5:
      state_5(); break;
    case 6:
      state_6(); break;
    case 7:
      state_7(); break;
    case 8:
      state_8(); break; 
    case 9:
      state_9(); break;   
    default:
      mem.state = 0; break;                                          
  }
}

void rmt_state_control(){
  String txt_tmp = "";
  Serial.println(F("***rmt_state_control"));
  if (!rmt.getPower() || (rmt.getMode() == 1)){//s-air will work for only power of or cool mode
    led_green.init(300, 2);//blink green 2 times
    led_yellow.stop();
    if (rmt.getTrset() < min_trset){//we do set tcool = troom - 3 and min.troom = 18 for all remote, so if user set troom = 19, then we set to 21, sothat we can set tcool = 18
      rmt.setTrset(min_trset);
      delay(100);
      rmt.cool();
    }
    else if (rmt.getTrset() > max_trset){
      rmt.setTrset(max_trset);
      delay(100);
      rmt.cool();
    }
    if (!rmt.getPower()){//power off go to state 2, to checking !comp_on
      

      //blink yellow led, if power off
      if (!(((mem.state == 0) || (mem.state == 1) || ((mem.state == 2) && !mem.compressor_state[2])) && (rmt.getNum() == 701))){//if remote != panasonic_701 and coff, then blink yellow led, else blink green led, because 701 on = off
        led_green.stop();
        led_yellow.init(300, 6);//blink yellow 6 times
      }
      
      if (mem.state == 9){//if state 9 and power off, then go state 1
        txt_tmp = F("power off -> go state 1\r\n");
        mem.state = 1; mem.state_cnt = 0;
      }
      else{//go state 2
        txt_tmp = F("power off -> go state 2\r\n");
        mem.state = 2; mem.state_cnt = 0;
      }
    }
    
    /*
     * change state for all air-cindition
     */
    else if  ((mem.state == 5) || (mem.state == 6) || (mem.state == 7)){
      mem.state = 4; mem.state_cnt = 0;
      txt_tmp = F("change air_status -> go state 4\r\n");
    }  
    else if ((mem.state == 0) || (mem.state == 1) || (mem.state == 9)){
      mem.state = 2; mem.state_cnt = 0;
      txt_tmp = F("power on -> go state 2\r\n");     
    }
    else if ((mem.state == 2) || (mem.state == 4)){
      mem.state_cnt = 0;
      txt_tmp = F("already in state 2 or 4 and change ait_status -> state_cnt = 0\r\n");     
    }
  }
  else{
    mem.state = 2; mem.state_cnt = 0;
    txt_tmp = F("no cool mode -> go state 2\r\n");   
  }
  if (show_data){
    Serial.print(txt_tmp);
  }
}

float get_temperature(byte t_index){
  float t_buffer = sensors.getTempCByIndex(t_index);
  if(t_buffer < 0){
    t_buffer = default_temperature_when_no_sensor;
  }
  return t_buffer;
}




//read_temp_slope wiil calculate all av_tr/tc and slope to detect con
//Index 0 = room sensor in box
//      1 = coil sensor        
void read_temp_slope_compressor(){//20181116
  static byte av_t_cnt = 0;
  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  mem.loop_cnt_considering_cut++;
  sensors.requestTemperatures();

  Serial.println(F("****** read_temp_slope_compressor"));

  mem.tc[0] = mem.tc[1];//push old temperature
  mem.tc[1] = mem.tc[2];
  mem.tr[0] = mem.tr[1];
  mem.tr[1] = mem.tr[2];

  //read new temperature
  // v2.1: Fixed - use real sensor readings instead of simulation
  if (mem.tsensors_select){
    mem.tc[2] = get_temperature(0);  // Coil temperature sensor
    mem.tr[2] = get_temperature(1);  // Room temperature sensor
    if ((mem.tr[2] <= default_min_tc_to_detect_compressor_on) && (mem.tc[2] > default_min_tc_to_detect_compressor_on)){
      mem.tsensors_select = 0;
      switch_tc_tr();
    }
  }
  else{
    mem.tc[2] = get_temperature(1);  // Coil temperature (swapped)
    mem.tr[2] = get_temperature(0);  // Room temperature (swapped)


    if ((mem.tr[2] <= default_min_tc_to_detect_compressor_on) && (mem.tc[2] > default_min_tc_to_detect_compressor_on)){
      mem.tsensors_select = 1;
      switch_tc_tr();
    }
  }

  //test temp broken
  //mem.tc[2] = 100.0;
  //mem.tr[2] = 100.0;
  //-> if >=2 time recored, then calculating Taverage
  if (mem.loop_cnt_considering_cut >= 2){
    mem.av_tc[0] = mem.av_tc[1];
    mem.av_tc[1] = mem.av_tc[2];
    mem.av_tr[0] = mem.av_tr[1];
    mem.av_tr[1] = mem.av_tr[2];

    mem.av_tc[2] = (mem.tc[2] + mem.tc[1])/2;//calculate new average
    mem.av_tr[2] = (mem.tr[2] + mem.tr[1])/2;

    if (av_t_cnt >= 3){

      /*
       * finding Tc characters
       */
      // First initial record, lowest_tc = 99, then record (lowest_tc > 90)
      if(mem.lowest_tc > 90){
        mem.lowest_tc = mem.av_tc[2];
      }
      
      // Revise lowest Tc, if the peak is less than lowest_tc then record
      if (mem.av_tc[2] < mem.lowest_tc){
        mem.lowest_tc = mem.av_tc[2];
      }
      
      // Found the lower peak of minimum av_tc each loop
//      if ((mem.av_tc[2] > mem.av_tc[1]) && (mem.av_tc[1] <= mem.av_tc[0])){ //set lowest_tc, [0] = min for all time, [1] = min today
//        Serial.println(F("Minium point Tc"));
//      }

      // Revise the average peak of av_tc each cycle
      if(mem.slope_tr[5] > 0){ // tr is increasing
        if ((mem.av_tc[2] < mem.av_tc[1]) && (mem.av_tc[1] >= mem.av_tc[0])){
          Serial.println(F("Max point Tc"));
          if(mem.avgmax_tc == 0){
            mem.avgmax_tc = mem.av_tc[1];
          }else{
            mem.avgmax_tc = (mem.av_tc[1] + mem.avgmax_tc)/2;
          }
        }
      }



    /*
     * finding Tr characters
     */

      // First initial record, lowest_tc = 99, then record (lowest_tc > 90)
      if(mem.max_tr_this_cycle > 90){
        mem.max_tr_this_cycle = mem.av_tr[2];
        mem.min_tr_this_cycle = mem.av_tr[2];
        if(mem.lowest_tr > 90){
          mem.lowest_tr = mem.av_tr[2];
        }
        
      }
      
      // Revise lowest value of tr, if the peak is less than min_tr[1] then record
      if (mem.av_tr[2] < mem.lowest_tr){
        mem.lowest_tr = mem.av_tr[2];
      }

      // Revise min each cycle during compressor on
      if(mem.compressor_state[2]){

        // just turn on, then reset
        if(!mem.compressor_state[1]){
          mem.min_tr_this_cycle = mem.av_tr[2];
        }
        
        // Finding lowest value as normal
        if (mem.av_tr[2] < mem.min_tr_this_cycle){       
          mem.min_tr_this_cycle = mem.av_tr[2];
        }
        
      // Revise max each cycle during compressor on
      }else{

        // just turn off, then reset
        if(!mem.compressor_state[1]){
          mem.max_tr_this_cycle = mem.av_tr[2];
        }

        // Finding highest value as normal
        if (mem.av_tr[2] > mem.max_tr_this_cycle){
          mem.max_tr_this_cycle = mem.av_tr[2];
        }        
      }
  

      
//      // Found the low peak of av_tc each loop
//      if ((mem.av_tr[2] > mem.av_tr[1]) && (mem.av_tr[1] <= mem.av_tr[0])){ 
//        Serial.println(F("low point Tr"));
//      }

//      // Found the high peak of av_tc each loop
//      if ((mem.av_tr[2] < mem.av_tr[1]) && (mem.av_tr[1] <= mem.av_tr[0])){ 
//        Serial.println(F("high point Tr"));
//      }
      
    }else{
      av_t_cnt++; // count more
    }
      
    if (av_t_cnt>=2){//calculating slope after 2 time av_t 

      /*
       * calculating slope_tc/tr
       */
      for (i = 0; i <= 4; i++){
        mem.slope_tc[i] = mem.slope_tc[i+1];
        mem.slope_tr[i] = mem.slope_tr[i+1];
      }
      mem.slope_tc[5] = (mem.av_tc[2] - mem.av_tc[1])*3; //per minute 
      mem.slope_tr[5] = (mem.av_tr[2] - mem.av_tr[1])*3; //per minute
      mem.slope_cnt++;
      


      if (mem.slope_cnt >= 3){
        /*
         * Very Very Very Very Very Very Very Very Very Very Very Very important to find min_slope_tc()
         */
        if ((mem.slope_tc[5] > mem.slope_tc[4]) && (mem.slope_tc[4] <= mem.slope_tc[3]) && (mem.slope_tc[4] < mem.min_tc_slope_considering_compressor_on)){
          if (mem.slope_tc[4] < mem.min_slope_tc[2]){
            mem.min_slope_tc[2] = mem.slope_tc[4];
          }
          if (mem.slope_tc[4] < mem.min_slope_tc[1]){
            mem.min_slope_tc[1] = mem.slope_tc[4];
          }
          if ((mem.min_slope_tc[0] == 0.0) || (mem.min_slope_tc[1] <= mem.min_slope_tc[0])){
            mem.min_slope_tc[0] = mem.min_slope_tc[1];
          }
        }

        if ((mem.slope_tr[5] > mem.slope_tr[4]) && (mem.slope_tr[4] <= mem.slope_tr[3]) && (mem.slope_tr[4] < 0.0)){//set min slope tr
          if (mem.slope_tr[4] < mem.min_slope_tr[1]){
            mem.min_slope_tr[1] = mem.slope_tr[4];
          }
          if ((mem.min_slope_tr[0] == 0.0) || (mem.min_slope_tr[1] <= mem.min_slope_tr[0])){
            mem.min_slope_tr[0] = mem.min_slope_tr[1];
          }
        }

        // find best tc slope, it must be turn on
        if((mem.best_tc_slope > mem.min_slope_tr[1]) && (rmt.getPower())){
          mem.best_tc_slope =  mem.min_slope_tr[1];
        }
      }
      
      if (mem.slope_cnt >= 2){//calculating con after 2 time slope
        mem.compressor_state[0] = mem.compressor_state[1];
        mem.compressor_state[1] = mem.compressor_state[2];
    
        // check compressor on or off
        compressor_state_change();
        
//        if (mem.compressor_state[2] && !mem.compressor_state[1] && !mem.compressor_state[0] && (mem.compressor_off_cnt >= 3)){//set total_cycle;
//          mem.total_cycle++;
//        }
      }
    }
  }
}


// check compressor on or off
boolean compressor_state_change(){
  static byte count_comp_on_criteria = 0;// marker for loop_cycle
  boolean is_comp_on_criteria = false;// marker for loop_cycle
  static byte count_comp_off_criteria = 0;// marker for loop_cycle
  boolean is_comp_off_criteria = false;// marker for loop_cycle
    
  ////////////
  // If compressor is now on, considering for turning off
  ////////////
  if (mem.compressor_state[2]){

    //Serial.print(F("-- count_comp_off_criteria: "));Serial.println(count_comp_off_criteria);
    //Serial.println(F("-- Comp On --> Considering slop to turn off0"));
    if(rmt.getPower() && (rmt.getMode() == 1)){ // power on and mode is cool
      mem.compressor_on_cnt++; // record compressor on, if power on and mode is cool
      mem.compressor_on_this_cycle++; // record compressor on for this cycle
      //mem.compressor_on_cnt += 2001; // testing
      //Serial.println(F("-- Comp On --> Considering slop to turn off1 - rmt.getMode() == 1"));
      if(mem.slope_tc[5] >= max_tc_slope_compressor_off){ // slope of tc is moore than max_tc_slope_compressor_off
        //Serial.println(F("-- Comp On --> Considering slop to turn off2 - max_tc_slope_compressor_off"));
        if(count_comp_off_criteria >= 1){ // slope_tc[5] muss more than max_tc_slope_compressor_off, two times succesively
          
          compressor_off_record();
          count_comp_off_criteria = 0; // reset count              
          
        }else{ // slope_tc[5] less than mem.min_tc_slope_considering_compressor_on but not enough count, then count for next
          count_comp_off_criteria++;
          is_comp_off_criteria = true;
        }
      }
      if(!is_comp_off_criteria){
        count_comp_off_criteria = 0; // if slope of tc is too low, then reset count_comp_off_criteria
      }
    }
    // if not power on and not cool mode
    else{
      compressor_off_record();
    }
  }
  
  ////////////
  // If compressor is now off, considering for turning on
  ////////////
  else{

    //Serial.print(F("-- count_comp_on_criteria: "));Serial.println(count_comp_on_criteria);
    //Serial.println(F("-- Comp Off --> Considering slop to turn on0"));

    if(rmt.getMode() == 1){ // if mode is cool
      //Serial.println(F("-- Comp Off --> Considering slop to turn on1 - rmt.getMode() == 1"));
      
      if(rmt.getPower()){ // if power on, then record compressor_off_cnt
       //Serial.println(F("-- Comp Off --> compressor_off_cnt+ to turn on2 - rmt.getPower()"));
        mem.compressor_off_cnt++; // record compressor off, if power on and mode is cool
        mem.compressor_off_this_cycle++; // record compressor off for this cycle
        //mem.compressor_off_cnt += 2001; // testing
      }
      
      if (mem.av_tc[2] <= mem.min_tc_to_detect_compressor_on){ // tc < min_tc_to_detect_compressor_on
        //Serial.println(F("-- Comp Off --> Considering slop to turn on2 - min_tc_to_detect_compressor_on"));
        if(abs(mem.av_tr[2] - mem.av_tc[2]) >= default_tdiff_tr_tc_compressor){ //  difference between tc and tr is enough
          //Serial.println(F("-- Comp Off --> Considering slop to turn on3 - default_tdiff_tr_tc_compressor"));

          if(mem.slope_tc[5] <= mem.min_tc_slope_considering_compressor_on){ // slope of tc is less than mem.min_tc_slope_considering_compressor_on
            //Serial.println(F("-- Comp Off --> Considering slop to turn on4 - mem.min_tc_slope_considering_compressor_on"));
            if(count_comp_on_criteria >= 1){ // slope_tc[5] muss less than mem.min_tc_slope_considering_compressor_on, two times succesively
              // if power on, then calculate as normal
              if(rmt.getPower()){
                compressor_on_record(); // trigger for compressor on
                count_comp_on_criteria = 0; // reset count
              }
              // if power off, then calculate wheter the slope meet criteria for longer time then, announce for Power On
              else if(count_comp_on_criteria >= 6){ // slope_tc[5] muss less than mem.min_tc_slope_considering_compressor_on, more times succesively
                compressor_on_record(); // trigger for compressor on
                count_comp_on_criteria = 0; // reset count
                rmt.setPower(1);
                MQTTtrigger_log(F("detect"),F("unexpected,comp_on"));  // Sending info to server           
                if (rmt.getTrset() == 0){//if troom not set but coil is on set troom = 25  // removed because place this detection in compressor detection already
                    rmt.setTrset(default_trset);  // removed because place this detection in compressor detection already
                }
              }
              
            }else{ // slope_tc[5] less than mem.min_tc_slope_considering_compressor_on but not enough count, then count for next
              count_comp_on_criteria++;
              is_comp_on_criteria = true;
            }
          }
        }
      }
    }
    
    if(!is_comp_on_criteria){
      count_comp_on_criteria = 0; // if slope of tc is too high, then reset count_comp_on_criteria
    }

  }
  // v2.1: Added missing return statement
  return false;
}


// trigger for compressor off
void compressor_off_record(){
  mem.compressor_state[2] = 0; // compressor is now off
  mem.min_slope_tc[2] = 0;//if compressor off, then set min_slope_tc[2] = 0
  //mem.compressor_off_cnt++;
  mem.total_cycle++; // count cycle of compressor off
  mem.compressor_off_this_cycle = 2;
  //mem.total_cycle += 1000; // testing
  Serial.println(F("** Comp Off"));
}

// trigger for compressor on
void compressor_on_record(){
  mem.compressor_state[2] = 1; // compressor is now on
  //mem.compressor_on_cnt++;
  mem.total_cycle++; // count cycle of compressor on
  mem.compressor_on_this_cycle = 2;
  //mem.compressor_off_this_cycle = 0;
  
  //mem.total_cycle += 1; // testing
  Serial.println(F("** Comp On"));
}

///////////////
// Revision for detecting vompressor 
//////////////
 
/*
 * Find min_tc_to_detect_compressor_on, from min Tc -- tc[1]
 */


void revise_compressor_on_all(){
  //find_max_tc_this_comp_off_cycle(); // find max of tc each loop
  
  if(mem.state_cnt > considering_change_detect_compressor_on_after_loop){ // cannot detect compressor on
    //
    // If cannot detect anything
    //
    if(mem.compressor_on_this_cycle < 1){ // state_cnt is beyond the limit
  
      //
      // Revise mem.min_tc_to_detect_compressor_on
      //
      
      // Need minimum gap between min and max of Tc
      if(mem.avgmax_tc - mem.lowest_tc > minimum_gap_max_min_tc_considering_revision){
        // Find min_tc_to_detect_compressor_on, from min Tc -- tc[1]
        if((mem.lowest_tc > mem.min_tc_to_detect_compressor_on) || (mem.lowest_tc - mem.min_tc_to_detect_compressor_on > min_gap_tcmin_and_tdetect_considering_new_detect)){
        // min_tc is higher than min_tc_to_detect_compressor_on  || gap betwenn min_tc and min_tc_to_detect_compressor_on is larger than defined, then revise new min_tc_to_detect_compressor_on
          mem.min_tc_to_detect_compressor_on = mem.lowest_tc + abs(25 - mem.lowest_tc)/2; // calculate the new min_tc_to_detect_compressor_on by using min_tc and last average Troom
        }
        // otherwise keep the current min_tc_to_detect_compressor_on  
  
        //
        // Revise mem.min_tc_slope_considering_compressor_on
        //
        if((mem.avgmax_tc > 0)){
          if(mem.min_tc_slope_considering_compressor_on - mem.best_tc_slope > considering_change_slope_compressor_on_by_gap_tc_max_min_beyond){
            mem.min_tc_slope_considering_compressor_on = defaultmin_min_tc_slope_considering_compressor_on;
          }
        }
      }
      
    //
    // Revise slope for better detection
    // even can detect compressor, but have very low gap between low and max slope of Tc, then revise
    //
    }else{
      if(mem.min_tc_slope_considering_compressor_on - mem.best_tc_slope > considering_change_slope_compressor_on_by_gap_tc_max_min_beyond){
        mem.min_tc_slope_considering_compressor_on = mem.best_tc_slope*0.8;
        // if slope limite is too sensitive to detect, then remain the default min slope
        if(mem.min_tc_slope_considering_compressor_on > defaultmin_min_tc_slope_considering_compressor_on){
          mem.min_tc_slope_considering_compressor_on = defaultmin_min_tc_slope_considering_compressor_on;
        }
      }
    }
    
  }
}



float get_min_float(float* array, int size){
  float minimum = array[0];
  for (int i = 0; i < size; i++){
    if (array[i] < minimum) 
    minimum = array[i];
  }
  return minimum;
}

float get_max_float(float* array, int size){
  float maximum = array[0];
  for (int i = 0; i < size; i++){
    if (array[i] > maximum) 
    maximum = array[i];
  }
  return maximum;  
}

/*
 * state 0 initial / webserver
 * 1. saving ssid, pass and e-mail in EEPROM
 * 2. clearing ssid, pass and e-mail
 * 3. check if user need report
 * 4. send ir signal to tell user, that remote is working
 */
void state_0(){
  Serial.print("0");
  web_install();
}


void state_1(){//change date 2018.09
  Serial.println(F("***state_1"));
  if (mem.loop_cnt_considering_cut > 0){
//    if (mem.user_need_email && mem.bl_log.finish){
//      //close_web();
//      stamp_hr_log_t_full_log_dy_full_log_mo_full_send_email();
//      start_web();
//    }  
    mem.reset_internal_vars();
  }
  //check_batt();//if use usb, then start web server. if low batt, then close webserver and go state 1
}

/*
 * state 2
 *   1. read temperature, then calculate slope
 *   2. checking compressor
 *   3. if con = 1 and in last period 60 second or state_cnt = 90 and con = 1 in last 1 minute, then go state 3. 
 *   4. if con = 0 in 90 cnt = 30 minute, then go state 1
 *   5. if user need report and loop_cnt_considering_cutime less than 90 and bl_log.finish = 0 go state 8 instead
 */

void state_2(){
  Serial.println(F("***state_2"));
  read_temp_slope_compressor();
  if (mem.state_cnt == 0){
    Serial.println(F("state_2 -> tcstamp = mem.tc[2]"));
    mem.tcstamp = mem.tc[2];
  }
  // first compressor on
  if ((mem.compressor_state[2] && !mem.compressor_state[1] && !mem.compressor_state[0]) || ((mem.state_cnt >= 30*one_mi) && mem.compressor_state[2] && mem.compressor_state[1] && mem.compressor_state[0])){
    //rmt.setPower(1); // removed because place this detection in compressor detection already
    
//    if (rmt.getTrset() == 0){//if troom not set but coil is on set troom = 25  // removed because place this detection in compressor detection already
//      rmt.setTrset(default_trset);  // removed because place this detection in compressor detection already
//    }  // removed because place this detection in compressor detection already

    //if (!mem.bl_log.finish && mem.loop_cnt_considering_cut <= state_2_duration*one_mi && mem.user_need_email){//con = on and !bl_recording_is_finish, go state 8 and loop_cnt_considering_cut less than state_2_duration minute
    if (!mem.bl_log.finish && mem.loop_cnt_considering_cut <= state_2_duration*one_mi){//con = on and !bl_recording_is_finish, go state 8 and loop_cnt_considering_cut less than state_2_duration minute
      Serial.println(F("state_2 -> mem.state = 8"));
      mem.state = 8;  mem.state_cnt = 0;
      // v2.1: Commented out - proprietary v1.0 function not in v2.x
      //set_trstart();
    }
    else{
      Serial.println(F("state_2 -> mem.state = 3"));
      mem.state = 3;  mem.state_cnt = 0;
    }
  }
  //compressor never turn on for state_2_duration, then set to Power off
//  else if ((mem.state_cnt >= state_2_duration*one_mi) && ((!mem.compressor_state[2] && !mem.compressor_state[1] && !mem.compressor_state[0]) || (mem.tc[2] - mem.tcstamp >= 10.0))){  
//    rmt.setPower(0);
//    mem.state = 1;
//  }


  // if cannot detect compressor on for a certain loop, revise the 
  if((mem.state_cnt > considering_change_detect_compressor_on_after_loop) && (mem.compressor_on_this_cycle < 1)){
    if(mem.state_cnt > considering_change_detect_compressor_on_after_loop){
      revise_compressor_on_all(); // Find min_tc_to_detect_compressor_on, from min Tc -- tc[1]
    }
  }
  
  check_power_off();

  //system checking
  check_trset();
  check_broke_temp();//if some temp-sensor is broken, then go state 9
  //check_batt();//if use usb, then start web server. if low batt, then close webserver and go state 1
  
  //led part
  //if (mem.loop_cnt_considering_cut%one_mi == 1){
    led_green.init(300, 1);
  //}
  mem.state_cnt++;
}

/*
 * state 3
 *   1. wait for pre smart cut period and check if con = 0 or compressor_on_cnt >=8, then go state 5
 *   2. if loop_cnt_considering_cutime >= pre_smart_cut_period and check if con = 0 or compressor_on_cnt >=8,go state 5
 */

void state_3(){
  Serial.println(F("***state_3"));
  read_temp_slope_compressor();
  if ((mem.state_cnt >= initial_first_loop_to_considering_cut) || (mem.loop_cnt_considering_cut >= initial_first_loop_to_considering_cut) && (!mem.compressor_state[2] || (mem.compressor_state[2] && mem.compressor_on_cnt >= (3*one_mi - 1)))){//wait for initial_first_loop_to_considering_cut
    rmt.cut();
    delay(1000);
    rmt.cut();
    if (show_data){
      Serial.println(F("state 3-pre cut's finish -> use cut, go state 5"));
    }
    //log_raw_data_algo();
    mem.state = 5;  mem.state_cnt = 0;
  }

  //system checking
  check_trset();
  check_dirty();
  check_broke_temp();//if some temp-sensor is broken, then go state 9
  //check_batt();
  
  //led part
  //if (mem.loop_cnt_considering_cut%one_mi == 1){
    led_green.init(300, 1);
  //}
  mem.state_cnt++;
}

/*
 * state_4
 * 1. checking (if slope_tc now less than min_slope_tc*cutpercent and con = 1 in last 1 minute and c_on_cnt >=8) or (tr is less than tr_set + setpoint/2.0) to do smart cut
 */

void state_4(){//change date: 20181008
  mem.cut_now = false;
  Serial.println(F("***state_4"));
  read_temp_slope_compressor(); // min_slope will be detect hier in this function
  
  //Condition to cut
  //compressor_on_this_cycle > min_comp_on_loop_to_start_cut (3 mintues) 
  if (mem.compressor_on_this_cycle >= (min_comp_on_loop_to_start_cut - 1)){
    
    if((mem.compressor_on_this_cycle >= (min_comp_on_loop_to_start_cut - 1)) && 
    (mem.compressor_state[2] && mem.compressor_state[1] && mem.compressor_state[0]) && 
    (mem.av_tr[2] <= float(rmt.getTrset()) + diff_btw_tr_and_trset_to_start_cut) &&
    ((mem.slope_tc[5] > (mem.min_slope_tc[2]*mem.percent_to_cut)) && 
    (mem.slope_tc[5] > mem.slope_tc[4]))){
    //Condition to cut
    //last 3 loops are compressor on
    //current Troom < Tset + diff_btw_tr_and_trset_to_start_cut
    //current Tc slope > min Tslope * percent_to_cut
    //current Tc slope > previous Tc slope
    
      Serial.println(F("*** saturated -> cut, go state 5"));
      String cut_msg = F("saturated,");
      cut_msg += String(mem.percent_to_cut*100.0);
      MQTTtrigger_log(F("cut"), cut_msg);  // Sending info to server
      mem.total_heavyduty_cut = 0; // reset this loop if cut by this method
      mem.cut_now = true;
      
    }
    else if((mem.av_tr[2] <= (float(rmt.getTrset()) - tr_less_than_trset_to_cut)) && (mem.av_tr[1] <= (float(rmt.getTrset()) - tr_less_than_trset_to_cut)) && (mem.av_tr[0] <= (float(rmt.getTrset()) - tr_less_than_trset_to_cut))){
    //Condition to cut
    //Current Troom < Tset - tr_less_than_trset_to_cut, for last 3 loops
    
      Serial.println(F("*** low_tr -> cut, go state 5"));
      MQTTtrigger_log(F("cut"), F("low_tr"));  // Sending info to server
      mem.total_heavyduty_cut = 0; // reset this loop if cut by this method
      mem.cut_now = true;
    }
    
    else if (mem.compressor_on_this_cycle >= considering_heavy_cut_after_loop){
    //check heavy_duty every cycle
    //Considering to heavy duty after this loop considering_heavy_cut_after_loop
    
      if(mem.slope_tr[5] > heavy_duty_tr_slope_boundary){
      //Current Troom slop higher (cannot decrease Troom temeprature continously) than  heavy_duty_tr_slope_boundary, then count for heavy dutry
        mem.heavy_cnt++;

        if(mem.heavy_cnt >= heavy_duty_count_loop_max){
        // If count for heavy dutry reaching the limit then start heavy duty mode
          Serial.println(F("*** heavy_duty -> cut, go state 5"));
          String cut_msg = F("heavy_duty,");
          cut_msg += String(mem.total_heavyduty_cut+1);
          MQTTtrigger_log(F("cut"), cut_msg);  // Sending info to server
          mem.cut_now = true;
          mem.heavy_cnt = 0;

          // Considering changing Trset
          if (mem.total_heavyduty_cut >= total_heavyduty_cut_to_adjust_trset){
          // if total cut by heavy duty reach total_heavyduty_cut_to_adjust_trset (3 loops) in a row)
            if(rmt.getTrset() < max_trset_to_adjust_by_heavy_duty){
              mem.total_heavyduty_cut = 0; // reset this loop if trigger the rmt.incTrset() already
              rmt.incTrset();  
            }
          }else{
            mem.total_heavyduty_cut++; // increase loop for considering incTrset
          }
        }
      }
      else{
      //Current Troom slop still enough (can decrease Troom temeprature) than  heavy_duty_tr_slope_boundary, then count for heavy dutry
        mem.heavy_cnt = 0;
      }
      
      

      
    }
  }

  // Check if compressor is off continously, it is possible that is turn off without RC signal, then move to standby state 1
  check_power_off();

  if(mem.cut_now){
    rmt.cut();
    mem.state = 5;  mem.state_cnt = 0;
  }
  //log_raw_data_algo();

  //system checking
  check_trset(); // check if Tset to have value
  if(mem.compressor_on_this_cycle < 1){
    if(mem.state_cnt > considering_change_detect_compressor_on_after_loop){
      revise_compressor_on_all(); // Find min_tc_to_detect_compressor_on, from min Tc -- tc[1]
    }
  }
  check_percent_to_cut();
  check_dirty();
  check_broke_temp();//if some temp-sensor is broken, then go state 9
  //check_batt();
  
  //led part
  //if (mem.loop_cnt_considering_cut%one_mi == 1){
    if (mem.dirty_cnt >= dirty_min_slope_not_pass_cnt){//warning dirty air, dirty_cnt will set in sys_mainten/check_dirty
      led_yellow.init(300, 2);
    }
    else if ((mem.compressor_on_cnt >= (heavy_warning_after_min*one_mi)) && (mem.slope_tr[2] > heavy_duty_tr_slope_boundary)){//check warning heavy
      led_yellow.init(300, 1);
    }
    else{
      led_green.init(300, 1);
    }
  //}
  mem.state_cnt++;     
}

/*
 * state_5
 *   1. check_compressor and trying to send signal to set con = 1
 *   2. calling check brok ir in sys_mainten, if we cannot set con = 1, it meen ir-tx is broken yellow,then LED 3 times blinken every minute
 */

void state_5(){//change date: 20181005
  Serial.println(F("***state_5"));
  read_temp_slope_compressor();
  if ((mem.state_cnt <= (broke_ir_warning_after_min[0]*one_mi*2) - 1) && (!mem.compressor_state[2] && !mem.compressor_state[1])){//if comp = off, then go to state 6
    mem.state = 6;  mem.state_cnt = 0;
  }
  else if (mem.state_cnt == (broke_ir_warning_after_min[0]*one_mi) - 1){
    rmt.cut();
    delay(1000);
    rmt.cut();
    //log_raw_data_algo();
  }
  //checking_system
  check_trset();
  check_percent_to_cut();
  check_dirty();
  check_broke_temp();//if some temp-sensor is broken, then go state 9
  //check_batt();
  
  //led part
  //if (mem.loop_cnt_considering_cut%one_mi == 1){
    if (mem.state_cnt > (broke_ir_warning_after_min[0]*one_mi*2) - 1){//check warning broke ir
      if (show_hardware_warning){
        led_yellow.init(300, 3);
      }
      else{
        led_green.init(300, 1);
      }
    }
    else{
      led_green.init(300, 1);
    }
  //}
  mem.state_cnt++;  
}

/*
 * state_6
 * 1. checking temp if av_tr >= tr_set + set_point, then turn c_on = 1
 * 2. if c_on = 1 in lsat 1 minute, then go state 7 do smart cut it could be that we cannot recieve user remote
 */

void state_6(){//change date: 20181005
  Serial.println(F("***state_6"));
  read_temp_slope_compressor();
  if (!mem.compressor_state[2] && !mem.compressor_state[1] && !mem.compressor_state[1] && (mem.av_tr[2] > (float(rmt.getTrset()) + diff_btw_tr_and_trset_to_start_cut))){
    if (show_data){
      Serial.println(F("Troom is to warm -> use cool mode, go state 7"));
    }
    rmt.cool();
    //log_raw_data_algo();
    mem.state = 7;  mem.state_cnt = 0;
  }
  else if (mem.compressor_state[2] && mem.compressor_state[1]){//if con go state 4
    if (show_data){
      Serial.println(F("Coil is on -> go state 4"));
    }
    mem.state = 4;  mem.state_cnt = 0;
  }
  else{
    // Check if compressor is off continously, it is possible that is turn off without RC signal, then move to standby state 1
    check_power_off();//if waiting more than xx, then go state 1
  }
  
  //system checking
  check_trset();
  check_percent_to_cut();
  check_dirty();
  check_broke_temp();//if some temp-sensor is broken, then go state 9
  //check_batt();
  
  //led part
  //if (mem.loop_cnt_considering_cut%one_mi == 1){
    if (mem.dirty_cnt >= dirty_min_slope_not_pass_cnt){//warning dirty air, dirty_cnt will set in sys_mainten/check_dirty
      led_yellow.init(300, 2);
    }
    else{
      led_green.init(300, 1);
    }
  //}
  mem.state_cnt++;     
}

/*
 * state 7
 *   1. check_compressor and trying to send signal to set con = 1
 *   2. calling check brok ir in sys_mainten, if we cannot set con = 1, it meen ir-tx is broken yellow,then LED 3 times blinken every minute
 */

void state_7(){//change date: 20181008
  Serial.println(F("***state_7"));
  read_temp_slope_compressor();
  if (mem.compressor_state[2] && mem.compressor_state[1]){
    mem.state = 4;  mem.state_cnt = 0;
  }
  else if ((mem.state_cnt == (broke_ir_warning_after_min[1]*one_mi) - 1) || (mem.state_cnt == (broke_ir_warning_after_min[1]*one_mi*2) - 1)){
    if (mem.av_tr[2] > (float(rmt.getTrset()) + diff_btw_tr_and_trset_to_start_cut)){
      rmt.cool();
      delay(1000);
      rmt.cool();
      //log_raw_data_algo();
    }
    else {
      rmt.cut();  // first cut
      delay(800);  
      rmt.cut(); // repeat cut
      //log_raw_data_algo();
      mem.state = 6;  mem.state_cnt = 0;
    }
  }

  //system checking
  check_trset();
  check_percent_to_cut();
  check_dirty();
  check_broke_temp();//if some temp-sensor is broken, then go state 9
  //check_batt();
  
  //led part
  //if (mem.loop_cnt_considering_cut%one_mi == 1){
    if (mem.state_cnt > (broke_ir_warning_after_min[1]*one_mi*2) - 1){//check warning broke ir
      if (show_hardware_warning){
        led_yellow.init(300, 3);
      }
      else{
        led_green.init(300, 1);
      }
    }
    else{
      led_green.init(300, 1);
    }
  //}
  mem.state_cnt++;     
}

/*
 * state_8
 * 1. record base line for mem_bl_length hour, then connect internet to do time stamp
 * 2. after base line finish send ir-signal 2 time to tell user, that base line is finish, then go state 5
 * 3. even if base line not finish or cannot connect internet, go state 5
 */
void state_8(){//20181006
  Serial.println(F("***state_8"));
  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  read_temp_slope_compressor();
  if (mem.compressor_state[2]){
    mem.compressor_log++;
  }
  else{
    mem.coff_log++;
  }
  if (mem.compressor_log + mem.coff_log >= one_hr){
    for (i = 0; i < mem_bl_length; i++){
      if ((mem.bl_log.duty[i] == 0.0) && (mem.bl_log.trset[i] == 0)){
        mem.bl_log.trset[i] = rmt.getTrset();
        mem.bl_log.duty[i] = float(mem.compressor_log)/float(mem.compressor_log + mem.coff_log);
        break;
      }
    }
    mem.compressor_log = 0;
    mem.coff_log = 0;
  }

  // record trstart
//  if(mem.trstart == 0){
//    set_trstart();
//  }

  //
  // Considering, whether baseline is finished or not
  //
  //if ((mem.bl_log.duty[mem_bl_length - 1] != 0.0) && (mem.bl_log.trset[mem_bl_length - 1] != 0) && (!mem.compressor_state[2] || (mem.compressor_state[2] && mem.compressor_on_cnt >= (3*one_mi - 1)))){
  if (mem.state_cnt >= baseline_finish_state_cnt_normal){ // baseline is longer record is longer than baseline_finish_minute, minimum compressor on loop
    Serial.println(F("baseline normal meet"));
//    Serial.print(F("(mem.compressor_on_cnt >= baseline_comp_on_loop)"));Serial.println(mem.compressor_on_cnt >= baseline_comp_on_loop);
//    Serial.print(F("(mem.total_cycle >= baseline_cycle_compressor)"));Serial.println(mem.total_cycle >= baseline_cycle_compressor);
//    Serial.print(F("(mem.state_cnt > baseline_finish_state_cnt_max)"));Serial.println(mem.state_cnt > baseline_finish_state_cnt_max);
//    Serial.print(F("(mem.total_cycle >= 1)"));Serial.println(mem.total_cycle >= 1);
    if(((mem.compressor_on_cnt >= baseline_comp_on_loop) && (mem.total_cycle >= baseline_cycle_compressor)) || ((mem.state_cnt > baseline_finish_state_cnt_max) && (mem.total_cycle >= 1))){
      // Basline finish criteria
      // 0) state_cnt > baseline_finish_state_cnt_normal
      // 1.1) comprresor on more than baseline_comp_on_loop && cycle of compressor on and off is more than baseline_cycle_compressor
      // or
      // if 1.1 criteria not fit and time reearching baseline_finish_state_cnt_max
      // 1.2) state_cnt > baseline_finish_state_cnt_max and has at least one turn on of compressor
//    if (mem.use_usb){
      //close_web();
//    }
//    if (mem.user_need_email){

      // recording baseline data
      Serial.println(F("recording baseline"));
      mem.bl_log.compressor_on_cnt = mem.compressor_on_cnt;
      mem.bl_log.compressor_off_cnt = mem.compressor_off_cnt;
      mem.bl_log.total_cycle = mem.total_cycle;
      //mem.bl_log.hr = get_hr(&mem.last_tstamp); // record no time
      mem.bl_log.trstart = mem.trstart;
      //mem.bl_log.av_tr = get_av_tr_from_log();
      mem.bl_log.av_tr = (mem.av_tr[0] + mem.av_tr[1] + mem.av_tr[2])/3;
      mem.bl_log.lowest_tc = mem.lowest_tc;
      mem.bl_log.best_tc_slope = mem.best_tc_slope;
      mem.bl_log.finish = 1;
      saving_vars_to_eeprom(); //saving vars in eeprom: Temperature, current remote status, will be restored after next restart
      MQTTtrigger_log(F("base"),F("end"));  // Sending info to server
      
      //
      // send baseline together with the next temperature send
      //
//      if(!MQTTtrigger_send_baseline()){
//        Serial.println(F("MQTTtrigger_send_baseline2"));
//        if(check_internet_and_connect_MQTT()){ // if cannot send check internet and send again
//          Serial.println(F("** has internet for sending MQTT baseline"));
//          Serial.println(F("MQTTtrigger_send_baseline3"));
//          if(MQTTtrigger_send_temperature()){
//            mem.sent_baseline_already = true;
//          }
//        }
//      }else{
//        mem.sent_baseline_already = true;
//      }
//        
//      if (show_data){
//        Serial.println(F("** connot send MQTT baseline"));
//      }

      // Get time form internet
//      double t_from_internet = 0; 
//      t_from_internet = get_t_from_internet();
//      Serial.print(F("t_from_internet"));Serial.println(t_from_internet);
//      if (t_from_internet > 1526774400){
//      mem.last_tstamp = time_t(t_from_internet);
//      if (get_mi(&mem.last_tstamp) > 30){
//        mem.last_tstamp = mem.last_tstamp + time_t(3600);
//      }
//      t_from_internet = t_from_internet - time_t(3600*mem_bl_length);
//        mem.bl_log.compressor_on_cnt = get_yr(&mem.last_tstamp);
//        mem.bl_log.compressor_off_cnt = get_mo(&mem.last_tstamp);
//        mem.bl_log.total_cycle = get_dy(&mem.last_tstamp);
//        mem.bl_log.hr = get_hr(&mem.last_tstamp);
//      mem.bl_log.hr = get_hr(&mem.last_tstamp); // record time from internet
      //send_email();

//    }
//    else{
//      mem.bl_log.trstart = mem.trstart;
//      mem.bl_log.finish = 1;
//    }
//    if (mem.use_usb){
      //start_web();
//    }
  
        
  
      //rmt.cut();
      //delay(1000);
      //rmt.cut();
      //log_raw_data_algo();
      mem.state = 5;mem.state_cnt = 0;
    }
    
  }

  //system checking
  check_trset();
  check_power_off();
  check_broke_temp();//if some temp-sensor is broken, then go state 9
  //check_batt();
  
  //led part
  //if (mem.loop_cnt_considering_cut%one_mi == 1){
    if ((mem.compressor_on_cnt >= (heavy_warning_after_min*one_mi)) && (mem.slope_tr[2] > heavy_duty_tr_slope_boundary)){//check warning heavy
      led_yellow.init(300, 1);
    }
    else{
      led_green.init(300, 1);
    }
  //}
  mem.state_cnt++;
}

/*
 * state 9
 * state 9 and state_9_case will be call in sys_mainten--> check_brok_temp
 *   1. read temperature
 *   2. state_9_case =  0 --> tr is broken, do normal smart cut, but turn c on after duty = 70% of av_bl_log_duty
 *                      1 --> tc is broken, c on after tr > trset°C, c off after tr < trset -0,5°C 
 *                      2 --> tr,tc are broken, try to do duty = 70% of av_bl_log_duty
 * change date 2018.09.21,13:15, Germany                      
 */

void state_9(){
  Serial.println(F("***state_9"));
  read_temp_slope_compressor();
  if ((mem.state_cnt >= initial_first_loop_to_considering_cut) || (mem.loop_cnt_considering_cut >= initial_first_loop_to_considering_cut)){
    float av_bl_log_duty = (mem.bl_log.duty[mem_bl_length - 2] + mem.bl_log.duty[mem_bl_length - 1])/2.0;
    if ((av_bl_log_duty == 0.0) || (av_bl_log_duty > 1.00)){
      av_bl_log_duty = state_9_av_bl_log_duty_default;
    }

    /*
     * tr is broken, we can do only smart cut but cannot see tr
     */
    if (mem.state_9_case == 0){
      if (((mem.compressor_on_cnt >= (3*one_mi - 1)) && mem.compressor_state[2] && mem.compressor_state[1] && ((mem.slope_tc[5] > (mem.min_slope_tc[2]*mem.percent_to_cut)) && (mem.slope_tc[5] > mem.slope_tc[4]))) && mem.state_9_compressor_mrk){
        rmt.cut();
        //log_raw_data_algo();
        mem.state_9_compressor_mrk = 0;
      }
      else if ((mem.compressor_off_cnt == (10*one_mi - int(round(av_bl_log_duty*state_9_percent_of_compressor_from_av_bl_log_duty*10.0*float(one_mi))))) && !mem.state_9_compressor_mrk){
        rmt.cool();
        //log_raw_data_algo();
        mem.state_9_compressor_mrk = 1;
      }
      check_power_off();
      
    }
  
    /*
     * tc is broken, we cannot do smart we just see tr and control tr, sothat it is not too warm or too cold
     */
    else if (mem.state_9_case == 1){
      if (((mem.state_cnt == int(round(av_bl_log_duty*state_9_percent_of_compressor_from_av_bl_log_duty*10.0*float(one_mi)))) || (mem.av_tr[2] <= float(rmt.getTrset()) + diff_btw_tr_and_trset_to_start_cut)) && mem.state_9_compressor_mrk){
        rmt.cut();
        //log_raw_data_algo();
        mem.state_9_compressor_mrk = 0;
      }
      else if ((mem.av_tr[2] > (float(rmt.getTrset()) + diff_btw_tr_and_trset_to_start_cut)) && !mem.state_9_compressor_mrk){
        rmt.cool();
        //log_raw_data_algo();
        mem.state_9_compressor_mrk = 1;
        mem.state_cnt = 0;
      }
    }
  
    /*
     * tr, tc are broken, we try to set duty cycle == 70% of base line duty
     */
    else{
      if ((mem.state_cnt%(10*one_mi) == int(round(av_bl_log_duty*state_9_percent_of_compressor_from_av_bl_log_duty*10.0*float(one_mi)))) && mem.state_9_compressor_mrk){
        rmt.cut();
        //log_raw_data_algo();
        mem.state_9_compressor_mrk = 0;
      }
      else if ((mem.state_cnt%(10*one_mi) == 0) && !mem.state_9_compressor_mrk){
        rmt.cool();
        //log_raw_data_algo();
        mem.state_9_compressor_mrk = 1;
      }
    }
  }

  //system checking
  check_broke_temp();//if some temp-sensor is broken, then go state 9
  //check_batt();
  
  //led part
  //if (mem.loop_cnt_considering_cut%one_mi == 1){
    if (show_hardware_warning){
      led_yellow.init(300, 3);
    }
    else{
      led_green.init(300, 1);
    }
  //}
  mem.state_cnt++;
}

void switch_tc_tr(){
  float vars_old[6];
  vars_old[0] = mem.tc[0];//switch tr and tr
  vars_old[1] = mem.tc[1];
  vars_old[2] = mem.tc[2];
  mem.tc[0] = mem.tr[0];
  mem.tc[1] = mem.tr[1];
  mem.tc[2] = mem.tr[2];
  mem.tr[0] = vars_old[0];
  mem.tr[1] = vars_old[1];
  mem.tr[2] = vars_old[2];

  vars_old[0] = mem.av_tc[0];//switch av_tr and av_tr
  vars_old[1] = mem.av_tc[1];
  vars_old[2] = mem.av_tc[2];
  mem.av_tc[0] = mem.av_tr[0];
  mem.av_tc[1] = mem.av_tr[1];
  mem.av_tc[2] = mem.av_tr[2];
  mem.av_tr[0] = vars_old[0];
  mem.av_tr[1] = vars_old[1];
  mem.av_tr[2] = vars_old[2];

  //vars_old[0] = mem.min_tc[0];//switch min_tr and min_tr
  //vars_old[1] = mem.min_tc[1];
  //mem.min_tc[0] = mem.min_tr[0];
  //mem.min_tc[1] = mem.min_tr[1];
  //mem.min_tr[0] = vars_old[0];
  //mem.min_tr[1] = vars_old[1];

  vars_old[0] = mem.slope_tc[0];//switch slope
  vars_old[1] = mem.slope_tc[1];
  vars_old[2] = mem.slope_tc[2];
  vars_old[3] = mem.slope_tc[3];
  vars_old[4] = mem.slope_tc[4];
  vars_old[5] = mem.slope_tc[5];
  mem.slope_tc[0] = mem.slope_tr[0];
  mem.slope_tc[1] = mem.slope_tr[1];
  mem.slope_tc[2] = mem.slope_tr[2];
  mem.slope_tc[3] = mem.slope_tr[3];
  mem.slope_tc[4] = mem.slope_tr[4];
  mem.slope_tc[5] = mem.slope_tr[5];
  mem.slope_tr[0] = vars_old[0];
  mem.slope_tr[1] = vars_old[1];
  mem.slope_tr[2] = vars_old[2];
  mem.slope_tr[3] = vars_old[3];
  mem.slope_tr[4] = vars_old[4];
  mem.slope_tr[5] = vars_old[5];

  vars_old[0] = mem.min_slope_tc[0];//switch min_slope
  vars_old[1] = mem.min_slope_tc[1];
  mem.min_slope_tc[0] = mem.min_slope_tr[0];
  mem.min_slope_tc[1] = mem.min_slope_tr[1];
  mem.min_slope_tr[0] = vars_old[0];
  mem.min_slope_tr[1] = vars_old[1];
  mem.min_slope_tc[2] = get_min_float(mem.slope_tc, 6);
}

void show_op_status(){
  int h = 0,i = 0,j = 0,k = 0,x = 0,y = 0,z = 0;
  if (show_data){
    String txt_tmp = "";
    txt_tmp = F("-----------------------------------------");txt_tmp += rn;
    //txt_tmp += F("loop_cnt= ");txt_tmp += String(loop_cnt) + rn;
    txt_tmp += F("tc= ");txt_tmp += String(mem.tc[2]) + " tr= " + String(mem.tr[2]) + rn;
    txt_tmp += F("state = ");txt_tmp += String(mem.state) + rn;
    //txt_tmp += F("rmt_detected= ");txt_tmp += String(rmt.rmt_detected) + rn;
    txt_tmp += F("Power= ");txt_tmp += String(rmt.getPower()) + rn;
    txt_tmp += F("Remote= ");txt_tmp += String(rmt.getNum()) + rn;
    txt_tmp += F("Trset= ");txt_tmp += String(rmt.getTrset()) + rn;
    txt_tmp += F("Baseline finished= ");txt_tmp += String(mem.bl_log.finish) + rn;
    txt_tmp += F("sent_baseline_already = ");txt_tmp += String(mem.sent_baseline_already) + rn;
    txt_tmp += F("state_cnt = ");txt_tmp += String(mem.state_cnt) + rn;
    txt_tmp += F("cut_now = ");txt_tmp += String(mem.cut_now) + rn;
    txt_tmp += F("--");txt_tmp += rn;
    txt_tmp += F("compressor_state = ");
    if (mem.compressor_state[2]){
      txt_tmp += F("On"); //txt_tmp += String(mem.compressor_on_cnt/one_hr) + "h" + String((mem.compressor_on_cnt%one_hr)/one_mi) + "m" + String((mem.compressor_on_cnt%one_mi)*20) + "s" + rn;
    }
    else{
      txt_tmp += F("Off"); //txt_tmp += String(mem.compressor_off_cnt/one_hr) + "h" + String((mem.compressor_off_cnt%one_hr)/one_mi) + "m" + String((mem.compressor_off_cnt%one_mi)*20) + "s" + rn;  
    }
    txt_tmp += rn;
    txt_tmp += F("compressor_state[3] = |");txt_tmp += String(mem.compressor_state[0]) + "|" + String(mem.compressor_state[1]) + "|" + String(mem.compressor_state[2]) + "|" + rn;
    txt_tmp += F("compressor_on_cnt ");txt_tmp += String(mem.compressor_on_cnt) + rn;
    txt_tmp += F("compressor_off_cnt ");txt_tmp += String(mem.compressor_off_cnt) + rn;
    txt_tmp += F("compressor_on_this_cycle ");txt_tmp += String(mem.compressor_on_this_cycle) + rn;
    txt_tmp += F("compressor_off_this_cycle ");txt_tmp += String(mem.compressor_off_this_cycle) + rn;
    txt_tmp += F("total_cycle = ");txt_tmp += String(mem.total_cycle) + rn;
    txt_tmp += F("best_tc_slope = ");txt_tmp += String(mem.best_tc_slope);
    txt_tmp += F(" | lowest_tc = ");txt_tmp += String(mem.lowest_tc);
    txt_tmp += F(" | avgmax_tc = ");txt_tmp += String(mem.avgmax_tc);
    txt_tmp += F(" | min_tr_this_cycle = ");txt_tmp += String(mem.min_tr_this_cycle);
    txt_tmp += F(" | max_tr_this_cycle = ");txt_tmp += String(mem.max_tr_this_cycle);
    txt_tmp += F(" | min_tc_to_detect_compressor_on = ");txt_tmp += String(mem.min_tc_to_detect_compressor_on) + "°C";
    txt_tmp += F(" | tsensors_select = ");txt_tmp += String(mem.tsensors_select) + rn;
    
    txt_tmp += F("--");txt_tmp += rn;
    txt_tmp += F("heavy_cnt = ");txt_tmp += String(mem.heavy_cnt) ; txt_tmp += F(" | total_heavyduty_cut = ");txt_tmp += String(mem.total_heavyduty_cut) + rn;
    txt_tmp += F("dirty_cnt = ");txt_tmp += String(mem.dirty_cnt) + rn;
    txt_tmp += F("percent_to_cut = ");txt_tmp += String(mem.percent_to_cut*100.0) + "%" + rn;
    txt_tmp += F("--");txt_tmp += rn;

    
    if (mem.state == 2){
      txt_tmp += F("tctamp = ");txt_tmp += String(mem.tcstamp) + rn;
    }
    else if (mem.state == 3){
      txt_tmp += F("pre smart cut left ");txt_tmp += String(initial_first_loop_to_considering_cut - mem.loop_cnt_considering_cut) + rn;
    }
    else if (mem.state == 8){
      txt_tmp += F("recording base line time left ");txt_tmp += String((mem_bl_length*one_hr - mem.state_cnt)/one_hr) + "h" + String(((mem_bl_length*one_hr - mem.state_cnt)%one_hr)/one_mi) + "m" + String(((mem_bl_length*one_hr - mem.state_cnt)%one_mi)*20) + "s" + rn;
      txt_tmp += F("bl_log.duty[] = |");
      for (i = 0; i < mem_bl_length; i++){
        txt_tmp += String(mem.bl_log.duty[i]) + "|";
      }
      txt_tmp += rn;
      txt_tmp += F("bl_log.tr_set[] = |");
      for (i = 0; i < mem_bl_length; i++){
        txt_tmp += String(mem.bl_log.trset[i]) + "|";
      }
      txt_tmp += rn;
    }
    else if (mem.state == 9){
      txt_tmp += F("state_9_case = ");txt_tmp += String(mem.state_9_case) + rn;
      if (mem.loop_cnt_considering_cut < initial_first_loop_to_considering_cut){
        txt_tmp += F("pre smart cut time left ");txt_tmp += String(initial_first_loop_to_considering_cut - mem.loop_cnt_considering_cut) + rn;
      }
    }
    txt_tmp += F("loop_cnt_considering_cut = ");txt_tmp += String(mem.loop_cnt_considering_cut) + rn; 
    


    

    ////txt_tmp += F("esaving_log_full_stack = ");txt_tmp += String(mem.esaving_log_full_stack) + rn;
    txt_tmp += F("free_memory = |");txt_tmp += String(system_get_free_heap_size()) + "Bytes|" + String((float(system_get_free_heap_size())/81920.0)*100) + "%|" + rn;
    //txt_tmp += F("system_adc_read() = |");txt_tmp += String(system_adc_read()) + "|" + String(float(system_adc_read()*4.3)/1024.0) + "V|" + rn;
    //txt_tmp += F("use_usb_cnt = ");txt_tmp += String(mem.use_usb_cnt) + rn;
    //txt_tmp += F("use_usb&access_point_on = |");txt_tmp += String(mem.use_usb) + "|" + String(mem.access_point_on) + "|" + rn;
    txt_tmp += F("access_point_on = ");txt_tmp += String(mem.access_point_on) + rn;

    txt_tmp += F("av_tc[3] = |");txt_tmp += String(mem.av_tc[0]) + "|" + String(mem.av_tc[1]) + "|" + String(mem.av_tc[2]) + "|" + rn;
    txt_tmp += F("av_tr[3] = |");txt_tmp += String(mem.av_tr[0]) + "|" + String(mem.av_tr[1]) + "|" + String(mem.av_tr[2]) + "|" + rn;
    //txt_tmp += F("min_tc[2] = |");txt_tmp += String(mem.min_tc[0]) + "|" + String(mem.min_tc[1]) + "|" + rn;
    //txt_tmp += F("min_tr[2] = |");txt_tmp += String(mem.min_tr[0]) + "|" + String(mem.min_tr[1]) + "|" + rn;
    txt_tmp += F("slope_tc[6] = |");txt_tmp += String(mem.slope_tc[0]) + "|" + String(mem.slope_tc[1]) + "|" + String(mem.slope_tc[2]) + "|" + String(mem.slope_tc[3]) + "|" + String(mem.slope_tc[4]) + "|" + String(mem.slope_tc[5]) + "|" + rn;
    txt_tmp += F("slope_tr[6] = |");txt_tmp += String(mem.slope_tr[0]) + "|" + String(mem.slope_tr[1]) + "|" + String(mem.slope_tr[2]) + "|" + String(mem.slope_tr[3]) + "|" + String(mem.slope_tr[4]) + "|" + String(mem.slope_tr[5]) + "|" + rn;
    txt_tmp += F("min_slope_tc[3] = |");txt_tmp += String(mem.min_slope_tc[0]) + "|" + String(mem.min_slope_tc[1]) + "|" + String(mem.min_slope_tc[2]) + "|" + rn;
    txt_tmp += F("--");txt_tmp += rn;
    txt_tmp += F("mem.bl_log.compressor_on_cnt = ");txt_tmp += String(mem.bl_log.compressor_on_cnt); txt_tmp += F("| compressor_off_cnt = ");txt_tmp += String(mem.bl_log.compressor_off_cnt); txt_tmp += F("| total_cycle  = ");txt_tmp += String(mem.bl_log.total_cycle); txt_tmp += F("| hr = ");
    txt_tmp += F("| trstart  = ");txt_tmp += String(mem.bl_log.trstart); txt_tmp += F("| av_tr = ");txt_tmp += String(mem.bl_log.av_tr); txt_tmp += F("| lowest_tc = ");txt_tmp += String(mem.bl_log.lowest_tc); txt_tmp += F("| best_tc_slope = ");txt_tmp += String(mem.bl_log.best_tc_slope); txt_tmp += F("| finish = "); txt_tmp += String(mem.bl_log.finish) + rn;
    txt_tmp += F("trigging_warning_sys_ssid = ");txt_tmp += String(mem.trigging_warning_sys_ssid) ; txt_tmp += F(" | trigging_warning_sys_mqtt = ");txt_tmp += String(mem.trigging_warning_sys_mqtt);txt_tmp += F(" | count_warning_sys_mqtt = ");txt_tmp += String(mem.count_warning_sys_mqtt) + rn;
    txt_tmp += F("==");txt_tmp += rn;
    Serial.print(txt_tmp);
    delay(10);
  }
}

void sleep_long(){
  if (show_data){
    Serial.print("S");
  }
  led_yellow.lo();
  led_green.lo();
  wifi_fpm_close();
  wifi_fpm_open();
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  gpio_pin_wakeup_disable();
  gpio_pin_wakeup_enable(GPIO_ID_PIN(4), GPIO_PIN_INTR_LOLEVEL);
  wifi_fpm_do_sleep(0xFFFFFFF); // Sleep for longlong
  delay(20);
  wifi_fpm_close();
  wifi_fpm_open();
  wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
  wifi_fpm_do_sleep(0xFFFFFFF);
  delay(1);
  rmt.reset();
  if (show_data){
    Serial.print(F("W"));
  }
}

void sleep_20s(){
  if (show_data){
    Serial.print(F("S"));
  }
  led_yellow.lo();
  led_green.lo();
  wifi_fpm_close();
  wifi_fpm_open();
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  gpio_pin_wakeup_enable(GPIO_ID_PIN(4), GPIO_PIN_INTR_LOLEVEL);
  gpio_pin_wakeup_enable(GPIO_ID_PIN(13), GPIO_PIN_INTR_LOLEVEL);
  wifi_fpm_do_sleep(0xFFFFFFF); // Sleep for longlong
  delay(20);
  wifi_fpm_close();
  wifi_fpm_open();
  wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
  wifi_fpm_do_sleep(0xFFFFFFF);
  delay(1);
  rmt.reset();
  if (show_data){
    Serial.print("W");
  }
}

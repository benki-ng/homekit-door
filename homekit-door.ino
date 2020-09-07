#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"


#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

#define PIN_SWITCH D0


// access your HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;

extern "C" homekit_characteristic_t cha_lock_target_state;
extern "C" homekit_characteristic_t cha_lock_current_state;

static uint32_t next_heap_millis = 0;


// Timeout in seconds to open lock for
const int unlock_period = 4;  // 5 seconds

static bool currentState = false;
static uint32_t next_state_millis = 0;


void updateHomekitValues(){
   homekit_characteristic_notify(&cha_lock_target_state, cha_lock_target_state.value);
   homekit_characteristic_notify(&cha_lock_current_state, cha_lock_current_state.value);
   LOG_D("[*] Sent Homekit status to clients!");
}

void updateRelay(bool state) {
  // true for on, false for off.
  digitalWrite(PIN_SWITCH, !state); // inverted because relays trigger low
  
  if(state){
    LOG_D("[*] Turning on the relay");
  }else{
    LOG_D("[*] Turning off the relay");
  }
  
}

void setup() {
	Serial.begin(115200);
	wifi_connect(); // in wifi_info.h
	//homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example

    pinMode(PIN_SWITCH, OUTPUT);
  updateRelay(false);
  
  cha_lock_target_state.setter = cha_switch_on_setter;
  
  arduino_homekit_setup(&config);

  updateHomekitValues();
}

void loop() {
	  arduino_homekit_loop();
    const uint32_t t = millis();


   if(currentState && (t > next_state_millis) ){
    LOG_D("Turning off the lock");
    currentState = false;
    cha_lock_target_state.value.uint8_value = 1;
    cha_lock_current_state.value.uint8_value = 1;
    updateRelay(currentState);
    updateHomekitValues();
   }
  
  if (t > next_heap_millis) {
    // show heap info every 5 seconds
    next_heap_millis = t + 5 * 1000;
    LOG_D("Free heap: %d, HomeKit clients: %d",
        ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

  }
  
	delay(10);
}





//Called when the switch value is changed by iOS Home APP
void cha_switch_on_setter(const homekit_value_t target) {
/*
    lock_state_unsecured = 0,
    lock_state_secured = 1,
    lock_state_jammed = 2,
    lock_state_unknown = 3,
 */

if(target.bool_value){
  LOG_D("Lock request made by homekit client");
}else{
  LOG_D("Unlock request made by homekit client");
}

   LOG_D("[-] Unlocking the door...");

   currentState = true;
   const uint32_t t = millis();
   next_state_millis = t + unlock_period * 1000;
   
   updateRelay(currentState);

   cha_lock_target_state.value.uint8_value = 0;
   cha_lock_current_state.value.uint8_value = 0;
   updateHomekitValues();


}

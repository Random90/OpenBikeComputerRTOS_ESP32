#include <Arduino.h>
/**
 * Simple arduino code to generate impulses imitating speeding bike.
 * It is hard to debug while on bike, is it? 
 */
#define MOCK_REED_SWITCH 13
#define DEFAULT_DELAY 500
#define SWITCH_DEBOUNCE 10

int delay_val = DEFAULT_DELAY;

void setup() {
   
   pinMode(MOCK_REED_SWITCH,OUTPUT);
}

void loop() {
   digitalWrite(MOCK_REED_SWITCH,LOW);
   delay(delay_val - SWITCH_DEBOUNCE);
   digitalWrite(MOCK_REED_SWITCH,HIGH);
   delay_val += random(-100,100);
   if(delay_val < 100) {
      delay_val = 100;
   }
   //well switching speed depends on how fast tyre is spinning;
   //im to lazy to calculate that. It doesn't matter any way, does it?
   delay(SWITCH_DEBOUNCE); 
}
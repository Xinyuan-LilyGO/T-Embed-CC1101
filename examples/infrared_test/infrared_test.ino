#include <IRremote.h>
#include "utilities.h"
 
int RECV_PIN = BOARD_IR_RX;
int LED_PIN = 13;
int IR_EN_PIN = BOARD_IR_EN;
 
IRrecv irrecv(RECV_PIN);
 
decode_results results;
 
void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(IR_EN_PIN, OUTPUT);
  digitalWrite(IR_EN_PIN, HIGH);
}
 
void loop() {
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    if (results.value == 0xFFA25D) //开灯的值
    {
      digitalWrite(LED_PIN, LOW);
    } else if (results.value == 0xFF629D) //关灯的值
    {
      digitalWrite(LED_PIN, HIGH);
    }
    irrecv.resume(); // Receive the next value
  }
  delay(100);
}
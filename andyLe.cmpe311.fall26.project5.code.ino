//#include "andyLe.cmpe311.fall26.project5.mymilli.h"
#include "andyLe.cmpe311.fall26.project5.myfan.h"

#include <Arduino_FreeRTOS.h>
#include <queue.h> 

void TaskLedBlink1( void *pvParameters );
void TaskLedBlink2( void *pvParameters );
void TaskFan( void *pvParameters );
void TaskSerialParse( void *pvParameters );


/*
  Blink without Delay

  Turns on and off a light emitting diode (LED) connected to a digital pin,
  without using the delay() function. This means that other code can run at the
  same time without being interrupted by the LED code.

  The circuit:
  - Use the onboard LED.
  - Note: Most Arduinos have an on-board LED you can control. On the UNO, MEGA
    and ZERO it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN
    is set to the correct LED pin independent of which board is used.
    If you want to know what pin the on-board LED is connected to on your
    Arduino model, check the Technical Specs of your board at:
    https://docs.arduino.cc/hardware/

  created 2005
  by David A. Mellis
  modified 8 Feb 2010
  by Paul Stoffregen
  modified 11 Nov 2013
  by Scott Fitzgerald
  modified 9 Jan 2017
  by Arturo Guadalupi

  This example code is in the public domain.

  https://docs.arduino.cc/built-in-examples/digital/BlinkWithoutDelay/
*/

#define FAN_PIN     11   /* Arduino D11 = PB3 = OC2A */
#define FAN_BUTTON_PIN   4   /* Arduino D4  = PD4        */

#define LED_PIN_1 2
#define LED_PIN_2 3

#define FAN_WAIT_WINDOW_MS 25

struct ledBlinkMS {
  int ledPin;
  int interval; 
};

QueueHandle_t ledQueue1;
QueueHandle_t ledQueue2;

// user input
char lineBuf[16];
int lineLen = 0;
int selectedLed = 0;              
bool ifWaitInterval = false;

void setup() {
  // set the digital pin as output:
  //init_mymilli();

  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  digitalWrite(LED_PIN_1, LOW);
  digitalWrite(LED_PIN_2, LOW);

  // // initalize the fan and it sbutton 
  init_myfan();
  init_myfanbutton();
  

  ledQueue1 = xQueueCreate(10, sizeof(struct ledBlinkMS));
  ledQueue2 = xQueueCreate(10, sizeof(struct ledBlinkMS));                                        

  if (ledQueue1 != NULL && ledQueue2 != NULL) {
    // Task set up for Leds 
    xTaskCreate(
    TaskSerialParse
    ,  "TaskSerialParse"
    ,  96 // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL );

    xTaskCreate(
    TaskLedBlink1
    ,  "TaskLed1Blink"   // A name just for humans
    ,  96  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

    xTaskCreate(
    TaskLedBlink2
    ,  "TaskLed1Blink"   // A name just for humans
    ,  96  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  }


  xTaskCreate(
    TaskFan
    ,  "TaskFan"   // A name just for humans
    ,  96  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );
}

void loop() {}

void TaskLedBlink1( void *pvParameters ){
  (void) pvParameters;

  struct ledBlinkMS newLedBlink;
  int currPin = LED_PIN_1;
  int currInterval = 0;
  bool ledState = false; 

  for (;;){ 
    TickType_t waitState = (currInterval > 0) ? pdMS_TO_TICKS(currInterval) : portMAX_DELAY;

    if(xQueueReceive(ledQueue1, &newLedBlink, waitState) == pdPASS){
      // new LED blink interval
      currPin = newLedBlink.ledPin;
      currInterval = newLedBlink.interval;
      digitalWrite(currPin, LOW);
      ledState = false; 
    } else {
      // else continue previous interval
      ledState = !ledState;
      digitalWrite(currPin, ledState ? HIGH : LOW);
    }

    // One tick delay (15ms) in between reads for stability
    vTaskDelay(1);
  }
}

void TaskLedBlink2( void *pvParameters ){
  (void) pvParameters;

  struct ledBlinkMS newLedBlink;
  int currPin = LED_PIN_2;
  int currInterval = 0;
  bool ledState = false; 

  for (;;){ 
    TickType_t waitState = (currInterval > 0) ? pdMS_TO_TICKS(currInterval) : portMAX_DELAY;

    if(xQueueReceive(ledQueue2, &newLedBlink, waitState) == pdPASS){
      // new LED blink interval
      currPin = newLedBlink.ledPin;
      currInterval = newLedBlink.interval;
      digitalWrite(currPin, LOW);
      ledState = false; 
    } else {
      // else continue previous interval
      ledState = !ledState;
      digitalWrite(currPin, ledState ? HIGH : LOW);
    }

    // One tick delay (15ms) in between reads for stability
    vTaskDelay(1);
  }
}

void TaskFan( void *pvParameters ){
  (void) pvParameters;

  Serial.begin(9600);

  for (;;) {
    fanButtonPressRTOS();

    // taskDelay synced to debounce in myfan
    vTaskDelay(pdMS_TO_TICKS(FAN_WAIT_WINDOW_MS));
  }
}

void TaskSerialParse( void *pvParameters ){
  (void) pvParameters;

  Serial.begin(9600);
  Serial.println("What LED? (1 or 2)");

  for (;;){ 
    while (Serial.available() > 0) {
      char c = Serial.read();

      if (c == '\n' || c == '\r') {
        if (lineLen > 0) {
          lineBuf[lineLen] = '\0';
          int value = atoi(lineBuf);
          lineLen = 0;

          if (!ifWaitInterval) {
            // Expecting LED number
            if (value == 1 || value == 2) {
              selectedLed = value;
              ifWaitInterval = true;
              Serial.println("What interval (Millisecond)?");
            } else {
              Serial.println("Invalid LED!  What LED? (1 or 2)");
            }
          } else {
            // Expecting interval
            if (value > 0) {
              int interval = (int)value;
              int half = interval / 2;
              if (half == 0) half = 1;

              struct ledBlinkMS newLedBlink; 
              newLedBlink.ledPin = (selectedLed == 1) ? LED_PIN_1 : LED_PIN_2;
              newLedBlink.interval = half; 

              if (selectedLed == 1) {
                xQueueSend(ledQueue1, &newLedBlink, portMAX_DELAY);
              } else {
                xQueueSend(ledQueue2, &newLedBlink, portMAX_DELAY);
              }

              ifWaitInterval = false;      
              selectedLed   = 0;
            
              Serial.println("What LED? (1 or 2)");
          } else {
            Serial.println("Invalid interval. What interval (Millisecond)?");
          }
            
          }
        }
      } else {
        if (lineLen < sizeof(lineBuf) - 1) {
          lineBuf[lineLen++] = c;
        }
      }
    }

    // One tick delay (15ms) in between reads for stability
    vTaskDelay(1);
  }
}


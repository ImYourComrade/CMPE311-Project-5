#include "andyLe.cmpe311.fall26.project5.myfan.h"
//#include "andyLe.cmpe311.fall26.project5.mymilli.h"
#include <Arduino_FreeRTOS.h>



// Registers 
// timer 
#define MY_TCCR2B ((volatile uint8_t*) 0xB1)
#define MY_TCCR2A ((volatile uint8_t*) 0xB0)
#define MY_TCNT2 ((volatile uint8_t*) 0xB2)
#define MY_OCR2A ((volatile uint8_t*) 0xB3)

// Pins for fan and 
#define MY_DDRB    ((volatile uint8_t *)0x24)
#define MY_PORTB   ((volatile uint8_t *)0x25)
#define MY_PINB    ((volatile uint8_t *)0x23)

/* Port D: PD4 = Arduino D4 = fan button */
#define MY_DDRD    ((volatile uint8_t *)0x2A)
#define MY_PORTD   ((volatile uint8_t *)0x2B)
#define MY_PIND    ((volatile uint8_t *)0x29)

#define FAN_OFF 0
#define FAN_LOW_SPEED 50    
#define FAN_MED_SPEED 155
#define FAN_HIGH_SPEED 250 

static const uint8_t fanSpeedTable[6] = {
  FAN_OFF,
  FAN_LOW_SPEED,   
  FAN_MED_SPEED,   
  FAN_HIGH_SPEED,  
  FAN_MED_SPEED,   
  FAN_LOW_SPEED   
};

static uint8_t currFanSpeed = 0;

// variables for fanButtonPress - ensures fan does not interfere with LEDs and works properly
#define FAN_WAIT_WINDOW_MS 25
static uint8_t lastFanButtonRead     = 1;
static uint8_t fanButtonState  = 1;
static unsigned long lastFanButtonPressMS = 0;



// function to initalize mymilli's registers  
void init_myfan(void){	
  // TCCR2A
	unsigned int bitPosCOM2A1 = 7; // Clear OC2A on compare match, set at BOTTOM
	unsigned int bitPosWGM21  = 1; // Fast PWM mode bit 1
	unsigned int bitPosWGM20  = 0; // Fast PWM mode bit 0
	// TCCR2B
	unsigned int bitPosCS20 = 0;   // No prescaler
	// DDRB
	unsigned int bitPosPB3 = 3;    // PB3 = OC2A = Arduino D11

  /* Step 1: clear timer2 control registers */
  *MY_TCCR2A = 0;
  *MY_TCCR2B = 0;
  *MY_TCNT2  = 0;

  /* Step 2: set data direction reg for OC2A pin (PB3 = D11) */
  *MY_DDRB |= (1 << bitPosPB3);

  /* Step 3: set count for speed cycle (start OFF) */
  *MY_OCR2A = 0;

  /* Step 4: TCCR2A
     Non-Inverting Fast PWM mode 3 using OCR A unit,
     clear on compare match */
  *MY_TCCR2A |= (1 << bitPosCOM2A1) | (1 << bitPosWGM21) | (1 << bitPosWGM20);

  /* Step 5: TCCR2B -- no prescaler
     f_PWM = 16 MHz / 256 = 62.5 kHz */
  *MY_TCCR2B |= (1 << bitPosCS20);
}

// function to initalize fan button
void init_myfanbutton(void){
	// Bit positions
	unsigned int bitPosPD4 = 4; // PD4 = Arduino D4

	// Masks
	unsigned int maskDDRD  = 1 << bitPosPD4; // used to CLEAR input bit
	unsigned int maskPORTD = 1 << bitPosPD4; // used to SET pull-up bit

	// PD4 as input (clear DDD4)
	*MY_DDRD = *MY_DDRD & (uint8_t)~maskDDRD;

	// Enable pull-up on PD4 (set PORTD4)
	*MY_PORTD = *MY_PORTD | maskPORTD;
}

// function to read fan button
static uint8_t fanButtonRead(void){
	// Bit position
	unsigned int bitPosPD4 = 4;

	// Read entire port D, then extract PD4
	uint8_t pinValue = *MY_PIND;

	return (pinValue >> bitPosPD4) & 1;
}

// fan btton press - non=blocking button to ensure led 
// blinks in conjunction with fan, without any delay
uint8_t fanButtonPress(void){
	uint8_t reading = fanButtonRead();

  // if fanButtonRead does not
	if (reading != lastFanButtonRead) {
		lastFanButtonPressMS = mymilli();
		lastFanButtonRead = reading;
	}

	if ((mymilli() - lastFanButtonPressMS) >= FAN_WAIT_WINDOW_MS) {
		if (reading != fanButtonState) {
			fanButtonState = reading;

		
			if (fanButtonState == 0) {
				advanceFanCycle();
				return 1;
			}
		}
	}

	return 0;
}

// fan button press - non-blocking button to ensure led 
// blinks in conjunction with fan, without any delay
// now uses FreeRTOS functions rather than mymilli
uint8_t fanButtonPressRTOS(void){
	uint8_t reading = fanButtonRead();

  // if fanButtonRead does not
	if (reading != lastFanButtonRead) {
		lastFanButtonPressMS = pdTICKS_TO_MS(xTaskGetTickCount());
		lastFanButtonRead = reading;
	}

	if ((pdTICKS_TO_MS(xTaskGetTickCount()) - lastFanButtonPressMS) >= FAN_WAIT_WINDOW_MS) {
		if (reading != fanButtonState) {
			fanButtonState = reading;

		
			if (fanButtonState == 0) {
				advanceFanCycle();
				return 1;
			}
		}
	}

	return 0;
}

void setFanSpeed(uint8_t speed){
  currFanSpeed = speed % 6;
  *MY_OCR2A = fanSpeedTable[currFanSpeed];
; 
}

// function to advance to the next fan state in the 6-state cycle
void advanceFanCycle(void){
  setFanSpeed(currFanSpeed + 1);
}


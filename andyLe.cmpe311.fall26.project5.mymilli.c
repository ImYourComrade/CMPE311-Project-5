#include "andyLe.cmpe311.fall26.project5.mymilli.h"

// #define MY_SREG volatile uint8_t* 0x3F
// Registers 
#define MY_SREG ((volatile uint8_t*) 0x5F)
#define MY_TCNT ((**volatile uint8_t*) 0x46)
#define MY_TCCR0B ((volatile uint8_t*) 0x45)
#define MY_TCCR0A ((volatile uint8_t*) 0x44)
#define MY_TIMSK0 ((volatile uint8_t*) 0x6E)


// based on the offical implementation of milli
#ifndef MY_F_CPU
#define MY_F_CPU 16000000UL // 16 MHz Clock for Arduino UNO R3
#endif

// Arduino's wiring.c already uses vector_16, so we use vector_14 in its place
#define MY_TIM0_OVF_vect __vector_14
// As a result we have to use OCR0A to disable ti 
#define MY_OCR0A  ((volatile uint8_t*) 0x47)

#define MY_ISR(vector) \
    void vector(void) __attribute__ ((signal, used, externally_visible)); \
    void vector(void)

#define MY_CYCLES_PER_US() ( MY_F_CPU / 1000000L )
#define MY_CYCLES_TO_US(a)( (a) / MY_CYCLES_PER_US() )

#define MY_MICROSEC_PER_TIMER0_OVERFLOW (MY_CYCLES_TO_US(64 * 256)) // prescaler - set so timer0 ticks every 64 clock cycles + overflow handler called every 256 ticks
#define MY_MILLIS_INC (MY_MICROSEC_PER_TIMER0_OVERFLOW / 1000) //  whole number of milliseconds per timer0 overflow

// the fractional number of milliseconds per timer0 overflow
// shift right by three to fit these numbers into byte
#define MY_FRACT_INC  ((MY_MICROSEC_PER_TIMER0_OVERFLOW % 1000) >> 3) 
#define MY_FRACT_MAX  (1000 >> 3)

volatile unsigned long my_timer_overflow_count = 0;
volatile unsigned long my_timer_millis = 0;
volatile unsigned long my_timer_fract = 0;

// using inline assembly, recreates sei() and cli() from avr library
# define MY_SEI()  __asm__ __volatile__ ("sei" ::: "memory") //  Global intrerupt enable 
# define MY_CLI()  __asm__ __volatile__ ("cli" ::: "memory") //  Global interrupt disable 

// based off of "Write your own Arduino millis() in Rust" blog + offical implementation 
// Clock is not a perfect millisecond, theres a tiny fraction that adds up over time
// This checks for overflow in order for milliseconds to be as accurate as it could be
#if defined(MY_TIM0_OVF_vect)
MY_ISR(MY_TIM0_OVF_vect)
#else
MY_ISR(MY_TIMER0_OVF_vect)
#endif
{
	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	unsigned long m = my_timer_millis;
	unsigned char f = my_timer_fract;

	m += MY_MILLIS_INC;
	f += MY_FRACT_INC;
	if (f >= MY_FRACT_MAX) {
		f -= MY_FRACT_MAX;
		m += 1;
	}

	my_timer_fract = f;
	my_timer_millis = m;
	my_timer_overflow_count++;
}

// function to initalize mymilli's registers  
void init_mymilli(void){	
	// Bit positions - As found in hardware sheet for registers used for timing 
	// TCCR0A
	unsigned int bitPosWGM01 = 1; 
	unsigned int bitPosWGM00 = 0; 
	
	// TCCR0B
	unsigned int bitPosWGM02 = 3; 
	unsigned int bitPosCS02 = 2;
	unsigned int bitPosCS01 = 1;
	unsigned int bitPosCS00 = 0; 

	// TIMSK0 
	unsigned int bitPosTOIE0 = 0; 
	
	unsigned int bitPosOCIE0A = 1; // OCIE0A (Output Compare Match A)

	// Masks - (bitmask) - Pattern to selectively to modify specific bits within registers 
	// TCCR0A
	// Fast PWM
	unsigned int maskTCCR0A = (1 << bitPosWGM01) | (1 <<bitPosWGM00); 
	// unsigned int maskWGM00 = 1 << bitPosWGM02; 
	
	// TCCR0B
	// 54 Prescaler 
	unsigned int maskTCCR0B = (0 << bitPosWGM02) | (0 << bitPosCS02) | (1 << bitPosCS01) | (1 << bitPosCS00); 
	// unsigned int maskCS02 = 1 << bitPosCS02;
	// unsigned int maskCS01 = 1 << bitPosCS01;
	// unsigned int maskCS00 = 1 << bitPosCS00; 

	// TIMSK0 
	//OCIE0A: Output Compare Match A
	// we have to use this as we're using vector_14

	unsigned int maskTIMSK0 = 1 << bitPosOCIE0A; 
	 

	// Assigning flags 
	*MY_OCR0A = 0; // Set compare match threshold to 0

	*MY_TCCR0B = *MY_TCCR0B | maskTCCR0B; // 
	// unsigned int MY_TCCR0B = MY_TCCR0B | maskCS02;
	// unsigned int MY_TCCR0B = MY_TCCR0B | maskCS01;
	// unsigned int MY_TCCR0B = MY_TCCR0B | maskCS00;

	*MY_TCCR0A = *MY_TCCR0A | maskTCCR0A;
	// unsigned int MY_TCCR0A = MY_TCCR0B | maskWHM00;

	*MY_TIMSK0 = *MY_TIMSK0 | maskTIMSK0; 

	MY_SEI(); // Enables interrupt
}

unsigned long mymilli()
{
	unsigned long milli;
	// saves current SREG in order to preserve interrupt state
	uint8_t oldSREG = *MY_SREG;

	// disable interrupts while we read timer0_millis or we might get an
	// inconsistent value (e.g. in the middle of a write to timer0_millis)
	MY_CLI(); 

	milli = my_timer_millis;
	// restores og SREG register state
	*MY_SREG = oldSREG;

	return milli;
}
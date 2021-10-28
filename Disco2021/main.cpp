/*
 * Disco2021.cpp
 *
 * Created: 20-10-2021 17:24:45
 * Author : Ch4oZ
 */ 

#include <avr/io.h>
#include <avr/sfr_defs.h>

#include "ffft.h"

int16_t                 capture[FFT_N];                         // Wave captureing buffer
complex_t               bfly_buff[FFT_N];                       // FFT buffer
uint16_t                spectrum[FFT_N / 2];                    // Spectrum output buffer
unsigned int            channel[8];
unsigned int            cnt, ch;
int                     sample, tmp;

const uint8_t spectrumIndexB[6] = { 4, 8, 24, 6, 17, 30 };
const uint8_t spectrumIndexD[5] = { 10, 13, 20,22, 26 };

//pwm
int pwm_phase =0;

int16_t danceflooron = 0;
uint8_t audiodetect = 0;
uint8_t noAudio = 0;
uint8_t lowAudio = 0;
bool fadingLedActive = false;
bool motorActive = false;
bool fadingLed1Active = false;
bool fadingLed2Active = false;
uint8_t fadingLed1Countdown =0;
uint8_t fadingLed2Countdown =0;

void set_PORTC_bit(int position, bool value)
{
	// Sets or clears the bit in position 'position'
	// either high or low (1 or 0) to match 'value'.
	// Leaves all other bits in PORTC unchanged.	
	if (value == true)
	{
		PORTC |= (1 << position);       // Set high, leave others alone
	}
	else
	{
		PORTC &= ~(1 << position);      // Set bit position low
	}
}

void setLed_B(int poort, int value) {
	if( value > 0 )
	{
		PORTB &= ~(1<<poort); // active low LED aan
	} else {
		PORTB |= (1<<poort); // active low LED uit
	}
}

void setLed_D(int poort, int value) {
	if( value > 0 )
	{
		PORTD &= ~(1<<poort); // active low LED aan
	} else {
		PORTD |= (1<<poort); // active low LED uit
	}
}
void activeFadingLed1(bool active) {
	if (active) {
		fadingLed1Active = true;
		set_PORTC_bit(PC2,1); 
		fadingLed1Countdown = 200;
		
		if (fadingLedActive == false)
			fadingLedActive = true;
		
		return;
	}
	
	if (fadingLed1Active == true) {	
		set_PORTC_bit(PC2,0);
		fadingLed1Active = false;
		
		if (fadingLed2Active == false)		
			fadingLedActive = false;
	}
}

void activeFadingLed2(bool active) {
	if (active) {
		fadingLed2Active = true;
		set_PORTC_bit(PC3,1);
		fadingLed2Countdown = 200;
	
		if (fadingLedActive == false)
			fadingLedActive = true;
	
		return;
	}
	
		
	if (fadingLed2Active == true) {
		set_PORTC_bit(PC3,0);
		fadingLed2Active = false;
		
		if (fadingLed1Active == false)
			fadingLedActive = false;
	}
}


int main(void)
{
	int16_t *buffer;
		
	ADCSRA = 0;
	ADMUX = (1<<REFS0); /* | (1<<MUX0)*/;  // = Analog input channel :  Ref = 5V; ADC0 = ingang
	// 0, 1, AVCC with external capacitor at AREF pin
	
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2) | (1<<DDB3) | (1<<DDB4) | (1<<DDB5); // = outputs voor rgb
	DDRC = (1<<PC4) | (1<<PC5) | (1<<PC2) | (1<<PC3);  // = output voor normaal licht en motor / fading leds
	DDRD = (1<<DDD2) | (1<<DDD3) | (1<<DDD4) | (1<<DDD5) | (1<<DDD6) | (1<<DDD7);  // = outputs overig
	
	set_PORTC_bit(PC5,0);	// motor uit
	set_PORTC_bit(PC4,1);
	activeFadingLed1(false);
	activeFadingLed2(false);
	
    /* Replace with your application code */
    while (1) 
    {
		buffer = capture;
		for (cnt = 0; cnt < FFT_N; cnt++) {
			ADCSRA = (1<<ADEN) | (1<<ADSC) | (1<<ADIF) | (1<<ADPS2) ;				   
			// ADCSRA = AD Status Register ???
			// ADEN = AD ENabled
			// ADSC = AD Start Conversion
			//  ADPS =  ADC Prescaler Select
				   
			while (bit_is_clear(ADCSRA, ADIF));                 // Sampelen ADIF = AD Interrupt Flag
			*buffer = ADC - 32768;
			buffer++;
		}
		ADCSRA = 0;
			   
		PORTD &= ~(/*(1<<PD2) |   (1<<PD3) |(1<<PD4) | (1<<PD5) | (1<<PD6) |*/ (1<<PD7));
			   		   
		//PORTB |= _BV(PB1);
		fft_input(capture, bfly_buff);                          // Bewerken
		fft_execute(bfly_buff);
		fft_output(bfly_buff, spectrum);
		//PORTB &= ~(_BV(PB0));

		if (spectrum[2] > 15) {
			setLed_D(PD7, 0); // = aan
			audiodetect |= (1<<6);
		} else {
			audiodetect &= ~(1<<6);
		}
 		
		for (uint8_t ix = 0;ix<6;ix++) {
			channel[ix] = spectrum[spectrumIndexB[ix]];
		}
		   
		for (uint8_t x=0;x<6;x++) {
			if (channel[x] > 20) {
				audiodetect |= (1<<x);
				if (danceflooron > 0) {
					setLed_B(x, 0);  // aan
				} else {	
					setLed_B(x, 1);  //uit
				}										
			} else {
				audiodetect &= ~(1<<x);
				setLed_B(x, 1); // uit
			}				   				   
		}
		   
		if (danceflooron > 0) {
			uint8_t ix =0;
			for (uint8_t portd_pin=2;portd_pin<7;portd_pin++) {
				if (spectrum[spectrumIndexD[ix]] > 15) {
					setLed_D(portd_pin, 1); // = aan
			   	} else {
					setLed_D(portd_pin, 0); // = uit
			   	}
				ix++;
			}
		}						  
						  	
		if (audiodetect > 0) {			
			noAudio = 0;			
			
			if (motorActive == false) {
				set_PORTC_bit(PC5,1); // motor on
				motorActive = true;
			}
			if (danceflooron == 0) {				
				set_PORTC_bit(PC4,0); // light off
			}
			danceflooron = 500;						
			if (fadingLedActive == true) {
				if (fadingLed1Countdown > 0) {
					fadingLed1Countdown--;
					
					if (fadingLed1Countdown ==0) {
						activeFadingLed1(false); // fading led off
					}
				}
				if (fadingLed2Countdown > 0) {
					fadingLed2Countdown--;
					
					if (fadingLed2Countdown ==0) {
						activeFadingLed2(false); // fading led off
					}
				}				
				
			}		      		
			
			if (audiodetect < 32) {
				if (lowAudio < 255) {
					lowAudio++;
				}
				
				
				if (lowAudio == 175 || lowAudio == 225){
					if ( (audiodetect & (1<<0)) == 0) {
						if (fadingLed1Active == false) {
							activeFadingLed1(true);
							} else {
							activeFadingLed2(true);
						}
						} else {
						if (fadingLed2Active == false) {
							activeFadingLed2(true);
							} else {
							activeFadingLed1(true);
						}
					}
				}
			} else {
				lowAudio =0;
			}
				   
		} else {

			
			if (danceflooron > 0) {
				if (noAudio < 255) {
					noAudio++;
				}							
				
				danceflooron--;
					
				if (danceflooron == 480) {
					activeFadingLed2(true); // fading led on
				} else if (danceflooron == 445) {
					activeFadingLed1(true); // fading led on
				} else if (danceflooron == 50) {					
					set_PORTC_bit(PC5,0); // motor off
					motorActive = false;
				} else if (danceflooron == 0) {					
					set_PORTC_bit(PC4,1); // light on
					activeFadingLed1(false); // fading led off
					activeFadingLed2(false); // fading led off
						 
					// disco leds ook ALLEMAAL uit:
					PORTD &= ~((1<<PD2) | (1<<PD3) |(1<<PD4) | (1<<PD5) | (1<<PD6) | (1<<PD7));
				}						
			}		   
		}		
    }
}
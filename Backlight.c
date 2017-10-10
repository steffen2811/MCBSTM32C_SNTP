#include "GPIO_STM32F10x.h"
#include "Backlight.h"

/*
Example 2:
==========
Using timer 3 to Dim the Backlight in Display

Mode : Continues PWM mode to PB0. Section 15.3.9 page 370
    For details see: http://mars.tekkom.dk/mediawiki/index.php/STM32F107VC/Using_MCBQVGA-TS-Display-v12

Caveats: Assumes TIMxCLK = 72 MHz (See RCC - Reset And Clock Control)
*/

void tim3init( void ) {
	GPIOB->CRL |= 0xa;  // Set PB0 as Alternate function Push-Pull 
	RCC->APB1ENR |= 2; //Enable timer 3 clock 	(RCC->APB1ENR bit 1 = 1)
		
    
	TIM3->PSC = 72;	  // Prescaler: With timer 3 input clock 72 Mhz divide by 72 to 1 Mhz
	TIM3->ARR = 1000; // Auto Reload Register set to 1000 divide 1Mhz/1000 = 1 Khz
	TIM3->CCR3 = 100; //Compare Register for channel 3
	TIM3->CCMR2 |= 7 << 4; //PWM mode 1
	TIM3->CCMR2 |= 1 << 3;
	TIM3->EGR |= 1; //Update generate
	TIM3->CCER |= 1<<8; //Polarity
	TIM3->CCER |= 1<<9;
	TIM3->CR1 |= 1;
}

/* Change Display Backlight Intensity
 input: 0 - 17 ( 0 = 0% backlight (All dark) to 17 = 100% backlight (Brigth)
 Caveats: No errorcontrol
             When 0 the timer should be turned off and the backlight turned off (PB0=0)
             when 17 the timer should be turned off and the backlight turned on (PB0=1)
*/
void backlight( int intensity ) {
	int disvalarr[] = {0,5,10,15,25,35,50,65,80,100,120,150,200,300,400,600,800,1000};
	TIM3->CCR3 = disvalarr[intensity];
}

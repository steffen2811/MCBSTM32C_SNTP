/*----------------------------------------------------------------------------*
 * Name:    timer.c /HeTh@mercantec.dk                                                           *
 * Purpose: Initialize TIM2 and enable IRQHandler for TIM2                    *
 * Alias:   Driver for TIM2                                                   *
 * Function to be called to initialize TIM2:  timer_init()                    *
 *----------------------------------------------------------------------------*/
#include "GPIO_STM32F10x.h"  
#include "rtc.h"
#include "cmsis_os.h"

extern struct TimeStruct t;
extern osThreadId TID_UpdateTimeDisplay;
extern struct TimeStruct InternetTime;
/*
 * TIM_DIER_UIE   == 1
 * TIM_CR1_CEN   == 1
 */
/*----------------------------------------------------------------------------
 * TIM2 initializer 'timer_init': used to initialize TIM2
 *---------------------------------------------------------------------------*/
void timer_init() {

   // Enable "timer 2" clock   (RCC->APB1ENR bit 0 = 1)
   RCC->APB1ENR |= 1;

   // Setting prescale value (72.000.000 / 7200 = 10000 Hz) (Side 403 Ref Manual)
   TIM2->PSC = 7200 - 1;
   
   // Autoload Register 10000 Hz - 1 (it starts at 0) (Side 354 Ref Manual)
   TIM2->ARR = 10000 - 1;
   
   // Enable IRQ interrupt for TIM2
   NVIC_EnableIRQ(TIM2_IRQn);
   
   // Enable interrupt on update
   TIM2->DIER |= TIM_DIER_UIE; // TIM_DIER_UIE = 1
   
   // Enable Counter     (TIM2_CR1 bit 0 = 1)
   TIM2->CR1 |= TIM_CR1_CEN; // TIM_CR1_CEN = 1
   
}







/*----------------------------------------------------------------------------
 * TIM2 IRQHANDLER 'TIM2_IRQHandler': run when timer interrupts.
 *---------------------------------------------------------------------------*/
void TIM2_IRQHandler(void) __irq{

  TIM2->SR = 0;
	t.epoch++;
	InternetTime.epoch++;
	osSignalSet (TID_UpdateTimeDisplay, 0x02);
}

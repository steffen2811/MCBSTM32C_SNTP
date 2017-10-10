/**************************************************************************
  #     #
  ##   ##  ######  #####    ####     ##    #    #   #####  ######   ####
  # # # #  #       #    #  #    #   #  #   ##   #     #    #       #    #
  #  #  #  #####   #    #  #       #    #  # #  #     #    #####   #
  #     #  #       #####   #       ######  #  # #     #    #       #
  #     #  #       #   #   #    #  #    #  #   ##     #    #       #    #
  #     #  ######  #    #   ####   #    #  #    #     #    ######   ####
 
***************************************************************************
Author..: Henrik Thomsen <heth@mercantec.dk>
Company.: House of Technology at Mercantec ( http://www.mercantec.dk )
date....: 2012 Jan. 14
***************************************************************************
Abstract: This module contains utility functions to operate the RTC
- Real Time Clock - on a ST Microelectronics STM32F107VC micro-controller.
The target board Keil MCBSTM32C contains battery and 32,768 Khz X-tal.

Purpose.: 1: First time initialization of the RTC or after battery replacement
          2: Setting/changing/reading the time, date and timezone
	  3: Converting between epoch time and human time
	     epoch time: seconds elapsed since 1970 jan 1 - 00:00:00
	     human time: year, month, date, weekday, hour, minute and seconds
	  4: Setting/canceling alarms
	  5: Handling leap-years 
	    (and perhaps leap-seconds - Not implemented/decided yet)
***************************************************************************
ToDo....: Missing timezone handling
          Missing alarm handling
	  Missing leap-second implementation
          Code is messy. Clean up necessary.
***************************************************************************
Modification log: 


                
***************************************************************************
License:  Free open software but WITHOUT ANY WARRANTY.
Terms..:  see http://www.gnu.org/licenses
***************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include "rtc.h"
/****************************** Defines ***********************************/
#define TRUE  1
#define FALSE 0
#define COUNT_TO 30000000 // Almost randomly delay count (Experimentation)
/*************************** Used registers *******************************/
//Define where in the memory the RTC start address peripheral is located
#define RTC_BASE         0x40002800 // See reference manual page 52
 
//Define the RTC register map. See reference manual section 18.4.7 page 474
uint32_t volatile * const rtc_crh  = (uint32_t *) (RTC_BASE + 0x0); 
uint32_t volatile * const rtc_crl  = (uint32_t *) (RTC_BASE + 0x4);
uint32_t volatile * const rtc_prlh = (uint32_t *) (RTC_BASE + 0x8);
uint32_t volatile * const rtc_prll = (uint32_t *) (RTC_BASE + 0xc);
uint32_t volatile * const rtc_divh = (uint32_t *) (RTC_BASE + 0x10);
uint32_t volatile * const rtc_divl = (uint32_t *) (RTC_BASE + 0x14);
uint32_t volatile * const rtc_cnth = (uint32_t *) (RTC_BASE + 0x18);
uint32_t volatile * const rtc_cntl = (uint32_t *) (RTC_BASE + 0x1c);
uint32_t volatile * const rtc_alrh = (uint32_t *) (RTC_BASE + 0x20);
uint32_t volatile * const rtc_alrl = (uint32_t *) (RTC_BASE + 0x24);

//Define where in the memory the RCC start address peripheral is located
#define RCCBASE         0x40021000
//unsigned int volatile * const rcc_apb1en = (unsigned int *) 0x4002101c;
uint32_t volatile * const rcc_apb1en = (uint32_t *) (RCCBASE + 0x1c);
uint32_t volatile * const rcc_bdcr = (uint32_t *) (RCCBASE + 0x20);

//Define where in the memory the PWR start address peripheral is located
#define PWR_BASE         0x40007000
uint32_t volatile * const pwr_cr = (uint32_t *) PWR_BASE + 0x0;

/****************************** Constants *********************************/
//Days in normal year, jan,feb .. dec (Not leap year)
uint8_t const daysmd[12] =  {31,28,31,30,31,30,31,31,30,31,30,31};

char const * const rtctxts[] = {
    "RTC: Illegal year. Valid range 1970 to 2106.",
    "RTC: Illegal month. Valid range 0 to 11. (0 = January, 1 = febrary ...)",
    "RTC: Illegal date in month.",
    "RTC: Illegal date in month. Not a leap year",
    "RTC: Illegal hour. Valid range 00 to 23",
    "RTC: Illegal minut. Valid range 00 to 59",
    "RTC: Illegal second. Valid range 00 to 59",
    "RTC: ERROR: RTC never ready",
    "RTC: ERROR in rtcsetcnt 1: RTOFF never 1",
    "RTC: ERROR in rtcsetcnt 2: RTOFF never 1",
    "RTC: ERROR could not set counter in 10 attempts",
    "RTC: Enabling and programming the RTC (Real Time Clock) for first time use",
    "RTC: ERROR in rtccnt: RSF never 1",

};
/*************************** FUNCTION HEADER ******************************
Name....: rtc_protect
Author..: heth
***************************************************************************
Abstract: Enable or disable STM32F107VC RTC write protect
          The RTC is automatically write protected against parasitic write
          access. To write to the RTC registers write protect must be disabled.
          
Inputs..: uint8_t wr_enable  0     : Disable writeprotect 
                             >=1   : Enable writeprotect
Outputs.: none 
ToDo....: Wanted enhancements
***************************************************************************
Modification log:
**************************************************************************/
void rtc_protect(uint8_t wr_enable) {
	if ( wr_enable == TRUE ) {
			*pwr_cr 	&= ~(1 << 8); 	// Enable Write protect
	} else {
			*pwr_cr 	|= 1 << 8;	// Disable write protect
	}
}

/*************************** FUNCTION HEADER ******************************
Name....: rtc_init
Author..: heth
***************************************************************************
Abstract: Initialize STM32F107VC rtc
           - If rtc not started initialize it for first time use
           - If rtc already started and running - just enable it
Inputs..: none
Outputs.: Returns 0 on succes - pointer to textstring on error 
ToDo....: Wanted enhancements
***************************************************************************
Modification log:
**************************************************************************/
char const *rtc_init(void) {
	uint32_t i;
	*rcc_apb1en |= 1 << 27;  	// BKPEN = 1 (Backup interface clock enable)
	*rcc_apb1en |= 1 << 28; 	// PWREN = 1 (Power interface clock enable)
	*pwr_cr     |= 0x3 << 5;  // PLS[2:0] = 011 Threshold 2,5 volt (VDD = 3.3 volt)
	*pwr_cr			|= 1 << 4;		// Enable Power voltage detector
	
	if ( (*rcc_bdcr &= 1 << 15) == 0 || (*rcc_bdcr &= 1 << 1) == 0 ) { // If RTC not running
		// Programm the RTC for first time use - or after battery swap
			rtc_protect(FALSE);			// Disable write protect
			*rcc_bdcr	|= 0x1 << 8;	// Select external XTAL - LSE
			*rcc_bdcr	|= 1;		 			// LSEON
			*rcc_bdcr	|= 1 << 15;		// RTC on
			for (i=0; (i < COUNT_TO) && ( (*rcc_bdcr & (1 << 1) ) == 0 ); i++);// Wait LSE ready
			if (i == COUNT_TO) {
				// ERROR: LSE oscillator never ready 
				rtc_protect(TRUE);	// Enable write protect
				return( rtctxts[7] ); 
			}
	
			*rtc_prlh = 0x7fff;			// Setting prescaler to 32767 (0x7fff)
			// Clock not set - eventually prompt user for time here
			rtc_protect(TRUE);	// Enable write protect
	}
			//5. Poll RSF, wait until its value goes to ‘1’ to ensure registers are synchronized.


	return(0);
}
// ToDO: Prettyfy to end
/* 1. Poll RTOFF, wait until its value goes to ‘1’
2. Set the CNF bit to enter configuration mode
3. Write to one or more RTC registers
4. Clear the CNF bit to exit configuration mode
5. Poll RTOFF, wait until its value goes to ‘1’ to check the end of the write operation.
*/

char const *rtcsetcnt( uint32_t value) {
	uint32_t i;
	uint8_t attempts;
	
	/* To avoid overflow from from rtc_cntl to rtc_cnth when writing
	to the registers. A max of 10 attempts setting the clock.*/
	attempts = 0;
	do {
		rtc_protect(FALSE); 		// Disable Write protect PWR_CR.DBP=1

		// 1. Poll RTOFF, wait until its value goes to ‘1’
		for( i=0; ( i < COUNT_TO ) && !( *rtc_crl & 1<<5); i++ ); 
		if ( i == COUNT_TO ) { return( rtctxts[8] ); }	//ERROR RTCOFF never ready

		// 2. Set the CNF bit to enter configuration mode
		*rtc_crl |= 1 << 4;

		// 3: Write to counter high and low. Check high not changed in loop
		*rtc_cnth = (value >> 16) & 0xffff;
		*rtc_cntl = value & 0xffff;
	
		// 4. Clear the CNF bit to exit configuration mode
		*rtc_crl &= ~(1 << 4); // The write operation starts when CNF is cleared
	
		//5. Poll RTOFF, wait until its value goes to ‘1’ to check the end of the write operation.
		for( i=0; ( i < COUNT_TO ) && !( *rtc_crl & 1<<5) ;i++ );
		if ( i == COUNT_TO ) { return( rtctxts[9] ); }	//ERROR RTCOFF never ready

		*pwr_cr &= ~(1 << 8);	// Enable write protect

		//if ( (*rtc_cnth == (value >> 16) & 0xffff) && ( *rtc_cntl == (value &= 0xffff)) ) {
		if ( rtccnt() == value ) { // Did we succed in writing the counter
			return(0);			   //Yes we did
		}
		attempts++;
	} while ( attempts < 10 );
	return( rtctxts[10] );
}
 
/*************************** FUNCTION HEADER ******************************
Name....: rtccnt
Author..: heth
***************************************************************************
Abstract: Reads the 32 bit counter of the RTC on STM ARM STM32F107VC uc
          The 32 bit counter is contained in two 16 bit registers rtc_cnth 
          (highest 16 bits) and rtc_cntl (lowest 16 bits)
Inputs..: none 
Outputs.: Returns uint32_t containing rtc counter value
Status..: Local
ToDo....: 
***************************************************************************
Modification log:
**************************************************************************/
uint32_t rtccnt( void ) {	
	unsigned int low,high,i;
	unsigned char count;
	
	count = 0;
	
	// The do-while block below checks if the RTC counter - which are living
	// in another time domain - changes higher 16 bits before we read lower 16
	// bits. (If lower contains 0xffff and high contains for example 0x0100
	// If we read 0x0100 from higher and before we read lower it increments 
	// from 0xffff to 0x0000 we would erroneos belive the counter was
	// 0x0100If low transits to 0x0000 before we reads 
	// higher 16 bits - 0x0100 0000 instead of 0x0100 fffff or 0x0101 0000
	do {
		*rtc_crl &= ~(1<<3);	//Clear RSF and wait for synchronization 
		for( i=0; ( i < COUNT_TO ) && !( *rtc_crl & 1<<3) ;i++ ); // Wait for Registers synchronized
		if ( i == COUNT_TO ) { return( 4 ); }	//ERROR RSF never ready - 4 seconds means this error
		
		high = (int) *rtc_cnth & 0xffff;
		low  = (int) *rtc_cntl & 0xffff;
		
		count++;

	} while ( ( high != (*rtc_cnth & 0xffff) ) && ( count <= 2 ) ); 
	// If high and low reads wrong in two attempts - somwthing is wrong ;-( 
	if ( count >= 2 ) { // 
		// Error condition RTC unstable..
		// Put error reporting/recovery code here....
		printf("ERROR: RTC rtccnt() error. Count reach %i. (High = %i low = %i )\n",count,high, low );
		return(0); //Time not valid return epoch
	}
	return( (high << 16) | low );
}

/* time2epoch  calculates epoch from initialized TimeStruct structure t.
t must contain year, mon, mday, hour, min, sec
returns pointer to string if error 0 if succesfull.
Returned string is a null-terminated error description.
Only first discovered error returned  */

char const *time2epoch( struct TimeStruct *t) {
        int i;
        t->epoch = 0; // initialize to 1. jan 1970 00:00:00

        /* Add year in seconds */
        if ( ( t->year < 1970 ) || ( t->year  > 2106 ) ) { // Check valid year
                return( rtctxts[0]);
        }
        for ( i = 1970;  t->year > i; i++ ) { // Counting from 1970 to current year

                if ( i % 4 == 0 ) { // Is year a possible leap year
                        t->epoch += SECONDSDAY; // Possible leap year add a day in seconds
                                                                                                // Note 1970 is not a leap year. t->epoch newer negative
                        if (  ( i % 100 == 0 ) && ( i % 400 != 0 ) ) { // 400 years rule
                                t->epoch -= SECONDSDAY;    // Divisble by 100 but not 400 remove the leap day again
                        }
                }
                t->epoch += SECONDSYEAR;  // add a normal 365 day year
        }

        /* Add month(s) in year in seconds */
        if ( t->mon  > 11 ) { // Check valid month
                return( rtctxts[1] );
        }
        for (i=0; t->mon > i; i++) {

        	/* If february and leapyear add one day */
        	if ( ( i == 1) && ( t->year % 4 == 0 ) ) { // Possible leap year
            	    t->epoch += SECONDSDAY; // Possible leap year substract a day in seconds

                	if (  ( t->year % 100 == 0 ) && ( t->year % 400 != 0 ) ) { // 400 years rule
                	t->epoch -= SECONDSDAY;    // Divisble by 100 but not 400 remove the leap day again
            	}
        	}
        t->epoch += daysmd[i]*SECONDSDAY;
        }

        /* Add day in month in seconds */
        if ( ( t->mday < 1 ) || ( t->mday > daysmd[ t->mon ] ) ) { // Check valid date
                if (  (t->year % 4 != 0) || ( t->mon != 1 ) ) {  // Not a leap year allowing 29. of february
                        return( rtctxts[2] );
                }
                // february and possible leap year passing here. (Check for 100 and 400 years rule)
                if ( (t->year % 100 == 0 ) && ( t->year % 400 != 0 ) ) {
                        if ( t->mday == 29 ) {
                                return( rtctxts[3] );
                        } else {
                                return( rtctxts[2] );
                        }
                }
                // February and leap-year passing here
            if ( t->mday > 29 ) {
                return( rtctxts[2] );
                }
        // February 29. in a leap year is legal
        }
        t->epoch += ( ( t->mday - 1 ) * SECONDSDAY); // Substract one day. Day not passed yet!

        /* Add hours in seconds */
        if (   t->hour  > 23  ) { // Check valid hour 00 -> 23
                return( rtctxts[4] );
        }
        t->epoch += t->hour * 60 * 60;

        /* Add minutes in seconds */
        if (  t->min  > 59  ) { // Check valid minute 00 -> 59
                return( rtctxts[5] );
        }
        t->epoch += t->min * 60;

        /* Add  seconds */
        if (  t->sec  > 59  ) { // Check valid second 00 -> 59
                return( rtctxts[6] );
        }
        t->epoch += t->sec;

        /* Adjust for timezone */
        t->epoch -= t->tz;
        return(0);
}

/* No errorchecks performed. */
void epoch2time( struct TimeStruct *t) {
			int i;
			uint32_t epoch;
        
        epoch = t->epoch + t->tz; // Adjust to local timezone
        t->year=1970;   // Initialize structure

        /* Find weekday */
        /* 0 = Sunday, 1 = Monday....*/
        /* epoch 1. jan 1970 was a thursday = 4 */
        t->wday = ( (epoch / SECONDSDAY) + 4 ) % 7;

        /* Find year */
        while ( epoch >= SECONDSYEAR ) {  
            if ( t->year % 4 == 0 ) { // Is year a possible leap year
            	if (  ( t->year % 100 == 0 ) && ( t->year % 400 != 0 ) ) { // 400 years rule
                   	epoch -= SECONDSYEAR;    // Divisble by 100 but not 400. Not a leap year
					t->year++;
                } else { // A leap year
				/* Need to consider when it is 31. december in a leap year.
				   epoch will be >= SECONDSYEAR */
					if ( epoch >= ( SECONDSYEAR + SECONDSDAY ) ) { // If it is a full leapyear
						epoch -= SECONDSYEAR; // A full year
						epoch -= SECONDSDAY;  // A leap day
						t->year++;
					} else {
						break;  // 31. december in a leap year.  SECONDSYEAR + SECONDSDAY <= epoch >= SECONDSYEAR
					}
				}	
            } else { // Not a leap year
            	epoch -= SECONDSYEAR;  // Substract a normal 365 day year
            	t->year++; // Add a year
			}
        }
        /* Find month */
        for (t->mon=0; epoch >= daysmd[t->mon]*SECONDSDAY; t->mon++) { // 0=jan, 1=feb...
          /* If february and leapyear substract one day. Except if it is the 29. of february */
					if ( (( t->year % 4 == 0 ) && ( t->year % 100 != 0 )) || ( t->year % 400 == 0 ) ) { // A leap year
						if ( t->mon == 1 ) {
							if ( epoch > (daysmd[t->mon]*SECONDSDAY) + SECONDSDAY ) { // Are we NOT in february
								epoch -= SECONDSDAY;
							} else {
								break; // 29'th of february in a leap year
							}
						}
					}	
          epoch -= daysmd[t->mon]*SECONDSDAY;
        }


        /* find date */
        t->mday=1;
        for (i=0; epoch >= SECONDSDAY; i++ ) {
                t->mday++;
                epoch -= SECONDSDAY;
        }

        /* find hour */
        t->hour=0;
        while ( epoch >= 60*60 ) {
                t->hour++;
                epoch -= 60*60;
        }

        /* find minute */
        t->min=0;
        while ( epoch >= 60 ) {
                t->min++;
                epoch -= 60;
        }

        /* Remaining must be second */
        t->sec = epoch;

}

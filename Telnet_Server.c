/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network
 * Copyright (c) 2004-2015 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    Telnet_Server.c
 * Purpose: Telnet Server example
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "rl_net.h"    									// Keil.MDK-Pro::Network:CORE
#include "sntp.h"
#include "Board_ADC.h"                  // ::Board Support:A/D Converter
#include "Board_LED.h"                  // ::Board Support:LED
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "GLCD_Config.h"                // Keil.MCBSTM32C::Board Support:Graphic LCD
#include "rtc.h"
#include "Backlight.h"
#include "Timer.h"

extern GLCD_FONT GLCD_Font_16x24;
extern struct TimeStruct t;
extern struct TimeStruct InternetTime;

bool LEDrun;
char lcd_text[2][20+1] = { "LCD line 1",
                           "LCD line 2" };
uint32_t epoch2;
extern int32_t udp_sock;
													 
// Thread IDs
osThreadId TID_Display;
osThreadId TID_Led;
osThreadId TID_UpdateTimeDisplay;

// Thread definitions
static void BlinkLed (void const *arg);
static void Display (void const *arg);
static void UpdateTimeDisplay (void const *arg);

osThreadDef(BlinkLed, osPriorityNormal, 1, 0);
osThreadDef(Display, osPriorityNormal, 1, 0);
osThreadDef(UpdateTimeDisplay, osPriorityNormal, 1, 0);

// Mutex definition 
osMutexDef (uart_mutex);    // Declare mutex
osMutexId  (uart_mutex_id); // Mutex ID
													 
/// Read analog inputs
uint16_t AD_in (uint32_t ch) {
  int32_t val = 0;

  if (ch == 0) {
    ADC_StartConversion();
    while (ADC_ConversionDone () < 0);
    val = ADC_GetValue();
  }
  return (val);
}

/// IP address change notification
//void netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len) {

  //if (opt == dhcpClientIPaddress) {
  //if (option == NET_DHCP_OPTION_IP_ADDRESS) {
    //osSignalSet (TID_Display, 0x01);
  //}
//}

/*----------------------------------------------------------------------------
  Thread 'UpdateTimeDisplay': LCD display handler
 *---------------------------------------------------------------------------*/
static void UpdateTimeDisplay (void const *arg) {
	static char    buf[40];
	static char    buf1[40];
	
	
	while(1)
	{		
		osSignalWait (0x02, osWaitForever);
		if (InternetTime.epoch < 10000)
		{
			send_udp_data();
		}
		epoch2time(&t);
		epoch2time(&InternetTime);
		
		sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d", t.mday, t.mon + 1, t.year, t.hour, t.min, t.sec);
		sprintf(buf1, "%02d/%02d/%04d %02d:%02d:%02d", InternetTime.mday, InternetTime.mon + 1, InternetTime.year, InternetTime.hour + 2, InternetTime.min, InternetTime.sec);
		
		osMutexWait(uart_mutex_id, osWaitForever);
		
		GLCD_DrawString (0, 3*24, buf);
		GLCD_DrawString (0, 0*24, buf1);
		
		osMutexRelease(uart_mutex_id);
	}
}


/*----------------------------------------------------------------------------
  Thread 'Display': LCD display handler
 *---------------------------------------------------------------------------*/
static void Display (void const *arg) {
  static uint8_t ip_addr[NET_ADDR_IP6_LEN];
  static char    ip_ascii[40];
  static char    buf[24];
	
  // Print Link-local IPv6 address
  netIF_GetOption (NET_IF_CLASS_ETH,
                   netIF_OptionIP6_LinkLocalAddress, ip_addr, sizeof(ip_addr));

  netIP_ntoa(NET_ADDR_IP6, ip_addr, ip_ascii, sizeof(ip_ascii));

  

  while(1) {
		
    /* Wait for signal from DHCP */
    //osSignalWait (0x01, osWaitForever);
    /* Retrieve and print IPv4 address */
    netIF_GetOption (NET_IF_CLASS_ETH,
                     netIF_OptionIP4_Address, ip_addr, sizeof(ip_addr));

    netIP_ntoa (NET_ADDR_IP4, ip_addr, ip_ascii, sizeof(ip_ascii));

		osMutexWait(uart_mutex_id, osWaitForever);
		
		GLCD_DrawString         (0, 1*24, "       MDK-MW       ");
		GLCD_DrawString         (0, 2*24, "Telnet Serv. example");

		
		sprintf (buf, "IP6:%.16s", ip_ascii);
		GLCD_DrawString (0, 5*24, buf);
		sprintf (buf, "%s", ip_ascii+16);
		GLCD_DrawString (10*16, 6*24, buf);
		
    sprintf (buf, "IP4:%-16s",ip_ascii);
    GLCD_DrawString (0, 4*24, buf);
		
    sprintf (buf, "%-20s", lcd_text[0]);
    GLCD_DrawString (0, 7*24, buf);
    sprintf (buf, "%-20s", lcd_text[1]);
    GLCD_DrawString (0, 8*24, buf);
		
		osMutexRelease(uart_mutex_id);
  }
}

/*----------------------------------------------------------------------------
  Thread 'BlinkLed': Blink the LEDs on an eval board
 *---------------------------------------------------------------------------*/
static void BlinkLed (void const *arg) {
  const uint8_t led_val[16] = { 0x48,0x88,0x84,0x44,0x42,0x22,0x21,0x11,
                                0x12,0x0A,0x0C,0x14,0x18,0x28,0x30,0x50 };
  int cnt = 0;																
										
  LEDrun = true;
																
  while(1) {
		
    // Every 100 ms
    if (LEDrun == true) {
      LED_SetOut (led_val[cnt]);
      if (++cnt >= sizeof(led_val)) {
        cnt = 0;
      }
    }
    osDelay (100);
  }
}



/*----------------------------------------------------------------------------
  Main Thread 'main': Run Network
 *---------------------------------------------------------------------------*/
int main (void) {
  LED_Initialize ();
  ADC_Initialize ();
	rtc_init();
  netInitialize ();
	timer_init();
	
	t.epoch = rtccnt();


	  // Initialize UDP socket and open port 2000
  udp_sock = netUDP_GetSocket (udp_cb_func);
		if (udp_sock >= 0) {
			netUDP_Open (udp_sock, 123);
		}
	
		
	GLCD_Initialize         ();
  GLCD_SetBackgroundColor (GLCD_COLOR_BLUE);
  GLCD_SetForegroundColor (GLCD_COLOR_WHITE);
  GLCD_ClearScreen        ();
  GLCD_SetFont            (&GLCD_Font_16x24);
	
	
		
  TID_Led     = osThreadCreate (osThread(BlinkLed), NULL);
  TID_Display = osThreadCreate (osThread(Display),  NULL);
	TID_UpdateTimeDisplay = osThreadCreate (osThread(UpdateTimeDisplay),  NULL);
	
	uart_mutex_id = osMutexCreate(osMutex(uart_mutex));
	
  while(1) {
    osSignalWait (0, osWaitForever);
  }
}

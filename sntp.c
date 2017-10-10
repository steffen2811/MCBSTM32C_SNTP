#include "rl_net.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE
#include "Board_LED.h"
#include "sntp.h"
#include "string.h"
#include "rtc.h"

 
int32_t udp_sock;                       // UDP socket handle
extern bool LEDrun;

 
struct SNTPstruct sntps2;
unsigned int sec;
struct TimeStruct InternetTime;

int changed_endian(int num)
{
 int byte0, byte1, byte2, byte3;
 byte0 = (num & 0x000000FF) >> 0 ;
 byte1 = (num & 0x0000FF00) >> 8 ;
 byte2 = (num & 0x00FF0000) >> 16 ;
 byte3 = (num & 0xFF000000) >> 24 ;
 return((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | (byte3 << 0));
}

// Notify the user application about UDP socket events.
uint32_t udp_cb_func (int32_t socket, const NET_ADDR *addr, const struct SNTPstruct *buf, uint32_t len) {
	sec = changed_endian(buf->sec);
	sec -= 2208988800;
	InternetTime.epoch = sec;
  return (0);
}
 
// Send UDP data to destination client.
void send_udp_data (void) {
  struct SNTPstruct *sntppoi;
	uint8_t *buf;
  if (udp_sock >= 0) {
    // IPv4 address: 192.168.138.111
		//NTP: 194, 239, 123, 230 
    NET_ADDR addr = { NET_ADDR_IP4, 123, 194, 239, 123, 230  };
                  		
    sntppoi = (struct SNTPstruct *) netUDP_GetBuffer(sizeof(struct SNTPstruct));
    
		sntppoi->VN = 0x01;
		sntppoi->Mode = 0x03;

    netUDP_Send (udp_sock, &addr, (uint8_t * )sntppoi, (sizeof(struct SNTPstruct)));
  }
}


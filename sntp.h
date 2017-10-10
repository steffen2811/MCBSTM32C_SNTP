#include <stdio.h>

struct SNTPstruct {
	unsigned Mode	: 3;
  unsigned VN : 3;   
	unsigned LI : 2;
  unsigned Stratum	: 8;
	unsigned Poll	: 8;
	unsigned Precision	: 8;
	uint32_t Root_Delay;
	uint32_t Root_Dispersion;
	uint32_t Root_Identifier;
	unsigned long long Reference_Timestamp;
	unsigned long long Originate_Timestamp;
	uint32_t sec;
	uint32_t deleafsec;
	//unsigned long long Receive_Timestamp;
	unsigned long long Transmit_Timestamp;
	uint32_t Key_Identifier;
	unsigned long long Message_Digest[2];
};

extern void send_udp_data (void);
extern uint32_t udp_cb_func (int32_t socket, const NET_ADDR *addr, const struct SNTPstruct *buf, uint32_t len);

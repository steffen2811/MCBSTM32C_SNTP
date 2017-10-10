#include <stdint.h>
extern char const * const rtctxts[];

/* Days in month's non-leap year jan, feb...*/
extern const uint8_t daysmd[];
/* Calculting current time from epoch in seconds.
Epoch: 1/1-1970 00:00:00

Leap year: In a leap year february has 29 days instead of normal 28.
 A leap year is defined as a year evenly divisble by 4 or 400. Execpt for years
 evenly divisble by 100 but not 400.
*/

#define SECONDSDAY  (24*60*60)
#define SECONDSYEAR (365*SECONDSDAY)

struct TimeStruct {
        uint32_t    epoch;      // Seconds since 1. jan 1970 00:00:00
				uint32_t     tz;         // Timezone +/- x seconds from GMT
        uint32_t    year;       // year                 1970 to 2106
        uint32_t     sec;        // seconds                          00 to 59
        uint32_t     min;        // minutes                          00 to 59
        uint32_t     hour;       // hours                            00 to 23
        uint32_t     mday;       // day of the month         1 to 31
        uint32_t     mon;        // month                            0 to 11
        uint32_t     wday;       // day of the week          0=sunday, 1=monday...
        uint32_t    isdst;       // is Daylight Saving Time (DST)
                                 // isdst: 0 = not dst 1 = dst  2=Not used
        uint32_t     dstOnMon;   // Month (0-11) when DST starts
        uint32_t     dstOnMday;  // Day of month when DST starts
        uint32_t     dstOnHour;  // Hour of the day when DST starts
        uint32_t     dstOnMin;   // Minute of the hour when DST starts
        uint32_t     dstOffMon;  // Month (0-11) when DST ends
        uint32_t     dstOffMday; // Day of month when DST ends
        uint32_t     dstOffHour; // Hour of the day when DST ends
        uint32_t     dstOffMin;  // Minute of the hour when DST ends
};

extern char const *time2epoch( struct TimeStruct *t);
extern void epoch2time( struct TimeStruct *t);
extern char const *rtc_init(void);
extern unsigned int rtccnt( void );
extern char const *rtcsetcnt( uint32_t value);

/*=============================================================================
 * File        : rtc.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Thin wrapper around the LPC2148's built-in Real-Time Clock
 *               peripheral. No external RTC chip is used; this relies
 *               entirely on the microcontroller's internal RTC block
 *               (typically clocked from a 32.768 kHz crystal on XTAL1/2 or
 *               derived from PCLK via the prescaler below).
 *
 * CCR (Clock Control Register) values used:
 *   CCR = 0x02 -> CCALEN + reset: clears/resets the RTC clock divider
 *   CCR = 0x01 -> CLKEN : enable the RTC to start counting
 *===========================================================================*/
#include <lpc214x.h>
#include "rtc.h"

/* Reset the RTC's clock divider, program the prescaler (PREINT/PREFRAC)
 * so a 1-second tick is generated from PCLK, then enable the RTC. */
void rtc_init(void)
{
    CCR = 0x02;          /* Reset the RTC's internal prescaler/divider */
    PREINT  = 456;        /* Prescaler integer part                     */
    PREFRAC = 25024;      /* Prescaler fractional part                  */
    CCR = 0x01;           /* Enable (start) the RTC                     */
}

/* Set the current time. The RTC clock is briefly disabled while the
 * HOUR/MIN/SEC registers are written, then re-enabled, to avoid a
 * rollover mid-update. */
void rtc_set_time(u8 hh, u8 mm, u8 ss)
{
    CCR &= ~0x01;   /* Pause the RTC while updating */
    HOUR = hh;
    MIN  = mm;
    SEC  = ss;
    CCR |= 0x01;    /* Resume the RTC */
}

/* Set the current date (day of month, month, 4-digit year). Same
 * pause/update/resume pattern as rtc_set_time(). */
void rtc_set_date(u8 dd, u8 mon, u16 yy)
{
    CCR &= ~0x01;
    DOM   = dd;
    MONTH = mon;
    YEAR  = yy;
    CCR |= 0x01;
}

/* Build a fixed-width, human-readable timestamp string in the form
 * "DD/MM/YYYY HH:MM:SS\0" (20 bytes, including the null terminator) by
 * reading the current RTC registers and converting each field to ASCII
 * digits directly (no sprintf, to keep the code small and fast). */
void rtc_get_stamp(char *buf)
{
    u8 d   = (u8)DOM;
    u8 m   = (u8)MONTH;
    u16 y  = (u16)YEAR;
    u8 hh  = (u8)HOUR;
    u8 mm  = (u8)MIN;
    u8 ss  = (u8)SEC;

    buf[0]  = (d / 10) + '0';         /* Day, tens digit    */
    buf[1]  = (d % 10) + '0';         /* Day, units digit    */
    buf[2]  = '/';
    buf[3]  = (m / 10) + '0';         /* Month, tens digit   */
    buf[4]  = (m % 10) + '0';         /* Month, units digit  */
    buf[5]  = '/';
    buf[6]  = (y / 1000) + '0';       /* Year, thousands     */
    buf[7]  = ((y / 100) % 10) + '0'; /* Year, hundreds      */
    buf[8]  = ((y / 10) % 10) + '0';  /* Year, tens          */
    buf[9]  = (y % 10) + '0';         /* Year, units         */
    buf[10] = ' ';
    buf[11] = (hh / 10) + '0';        /* Hour, tens digit    */
    buf[12] = (hh % 10) + '0';        /* Hour, units digit   */
    buf[13] = ':';
    buf[14] = (mm / 10) + '0';        /* Minute, tens digit  */
    buf[15] = (mm % 10) + '0';        /* Minute, units digit */
    buf[16] = ':';
    buf[17] = (ss / 10) + '0';        /* Second, tens digit  */
    buf[18] = (ss % 10) + '0';        /* Second, units digit */
    buf[19] = '\0';
}

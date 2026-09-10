/*=============================================================================
 * File        : security.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Ties together tamper detection, RTC-timestamped access
 *               logging (sent out over UART0 for a PC to capture), and
 *               one-time initialisation of the default EEPROM passwords.
 *
 * Wiring:
 *   Tamper switch -> P0.4, active LOW (switch pulls the pin to GND when
 *                    the enclosure is opened/tampered with; an external
 *                    pull-up resistor keeps the pin HIGH when closed)
 *===========================================================================*/
#include <lpc214x.h>
#include <string.h>
#include "security.h"
#include "defines.h"
#include "lcd.h"
#include "uart.h"
#include "rtc.h"
#include "eeprom.h"
#include "buzzer.h"
#include "delay.h"

#define TAMPER_PIN   (1UL << 4)   /* P0.4 - tamper switch input */

/* Configure the tamper switch pin as a GPIO input. */
void security_init(void)
{
    PINSEL0 &= ~(3UL << 8);   /* P0.4 as plain GPIO (not an alternate function) */
    IO0DIR  &= ~TAMPER_PIN;   /* Configure as input */
}

/* Read the current state of the tamper switch.
 * The switch is active LOW, so a HIGH pin reading means "not tampered"
 * (returns 0) and a LOW reading means "tampered" (returns 1). */
u8 tamper_detected(void)
{
    if (IO0PIN & TAMPER_PIN) return 0;   /* Pin HIGH -> switch closed -> OK       */
    else                     return 1;   /* Pin LOW  -> switch open  -> tampered  */
}

/* Print a single timestamped log line over UART0 in the form:
 * "[DD/MM/YYYY HH:MM:SS] <message>\r\n"
 * This is intended to be captured by a PC terminal for an audit trail. */
void log_event(const char *msg)
{
    char stamp[20];
    rtc_get_stamp(stamp);

    uart0_string("[");
    uart0_string(stamp);
    uart0_string("] ");
    uart0_string(msg);
    uart0_string("\r\n");
}

/* Poll the tamper switch once and, on a fresh LOW->tampered transition
 * (edge-detected via 'old_state' so the alert only fires once per event
 * rather than continuously while the switch stays open), show an alert
 * on the LCD, log the event, and sound the buzzer. */
void check_tamper_and_alert(void)
{
    static u8 old_state = 0;

    if (tamper_detected())
    {
        if (old_state == 0)   /* Only trigger on the transition into "tampered" */
        {
            old_state = 1;
            lcd_clear();
            lcd_string("TAMPER ALERT");
            log_event("Tamper detected");
            buzzer_alert(5);
        }
    }
    else
    {
        old_state = 0;   /* Switch closed again -> re-arm for the next event */
    }
}

/*------------------------------------------------------------
 * Login lockout state (merged in from EnviroTime's security.c,
 * adapted to SecureLocker's two-level Bluetooth+keypad login).
 *------------------------------------------------------------*/
static u8 wrong_attempts = 0U;   /* Consecutive failed login attempts */

/* Call after a fully successful login (both Level-1 and Level-2
 * matched). Resets the consecutive-failure counter back to zero. */
void security_register_success(void)
{
    wrong_attempts = 0U;
}

/* Call after any failed login step (wrong Level-1 Bluetooth password,
 * or wrong Level-2 keypad password). Once MAX_WRONG_ATTEMPTS consecutive
 * failures have been registered, this locks the system out: it shows
 * "SYSTEM LOCKED" on the LCD, logs the lockout, sounds the buzzer, and
 * blocks for LOCK_DURATION_MS (30 seconds) before automatically
 * resetting the counter and returning control to the caller. */
void security_register_failure(void)
{
    wrong_attempts++;

    if (wrong_attempts >= MAX_WRONG_ATTEMPTS)
    {
        lcd_clear();
        lcd_string("SYSTEM LOCKED");
        lcd_gotoxy(1,0);
        lcd_string("WAIT 30 SECS");

        log_event("System locked: 3 consecutive failed login attempts");
        buzzer_alert(5);

        delay_ms(LOCK_DURATION_MS);   /* Blocking 30-second lockout */

        wrong_attempts = 0U;
        log_event("System unlocked, attempt counter reset");
    }
}

/* On first boot (or after a blank/corrupted EEPROM), write the 4-byte
 * "LKR1" magic marker plus the two factory-default passwords
 * (Level-1/Bluetooth = "1234", Level-2/keypad = "5678") into the EEPROM.
 * On subsequent boots the marker will already match and this function
 * does nothing, preserving any passwords the admin has since changed. */
void ensure_default_passwords(void)
{
    char magic[5];

    eeprom_read_str(EEPROM_MAGIC_ADDR, magic, 4);

    if (strcmp(magic, "LKR1") != 0)
    {
        eeprom_write_str(EEPROM_MAGIC_ADDR, "LKR1", 4);
        eeprom_write_str(EEPROM_L1_ADDR, "1234", PWD_LEN);
        eeprom_write_str(EEPROM_L2_ADDR, "5678", PWD_LEN);
        log_event("EEPROM initialized with default passwords");
    }
}

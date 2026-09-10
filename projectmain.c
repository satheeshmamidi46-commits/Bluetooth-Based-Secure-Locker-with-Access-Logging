/*=============================================================================
 * File        : projectmain.c
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Target      : NXP/Philips LPC2148 (ARM7TDMI-S) @ 60 MHz core clock
 *
 * SYSTEM OVERVIEW
 * ----------------
 * A two-factor locker: the user first sends a 4-digit password from a
 * phone over Bluetooth (Level-1), and if correct is then prompted to
 * enter a second 4-digit password on a physical keypad (Level-2). Only
 * when both match does the microcontroller drive a DC motor (via an
 * L293D H-bridge) to open, then automatically close, the locker. Every
 * significant event (boot, tamper, password results, locker open/close,
 * admin actions) is timestamped using the on-chip RTC and streamed out
 * over UART0 as a simple audit log for a PC to capture.
 *
 * An admin push-button (external interrupt EINT2) opens a separate menu
 * (see menu.c) for CLK Setting (time/date/day), an Alarm (checked every
 * main-loop pass via check_alarm()), and changing either stored password;
 * a tamper switch is continuously monitored and triggers an alarm (LCD
 * message + buzzer + log entry) if the enclosure is opened. The CLK
 * Setting/Alarm menu was merged in from the separate "EnviroTime"
 * project and rewritten to use this project's own LCD/keypad/buzzer/
 * RTC/EEPROM drivers; no other SecureLocker file needed to change for
 * that merge beyond the single check_alarm() call in the main loop below.
 *
 * At power-up the LCD splash screen reads "Bluetooth" / "Secure System".
 *
 * PIN CONNECTIONS
 * ----------------
 *   LCD (16x2, 4-bit mode):
 *     RS               -> P0.16
 *     EN               -> P0.17
 *     D4 .. D7         -> P0.18 .. P0.21
 *
 *   UART0 (PC debug / access-log console):
 *     TXD0             -> P0.0
 *     RXD0             -> P0.1
 *
 *   I2C0 (AT24C256 EEPROM - password storage):
 *     SCL0             -> P0.2
 *     SDA0             -> P0.3
 *     EEPROM VCC       -> 3V3
 *
 *   Tamper switch (active LOW, external pull-up to 3V3):
 *     Signal           -> P0.4
 *
 *   Admin push-button (EINT2, falling edge):
 *     Signal           -> P0.7
 *
 *   UART1 / HC-05 Bluetooth module:
 *     TXD1 (MCU -> HC-05 RXD) -> P0.8
 *     RXD1 (MCU <- HC-05 TXD) -> P0.9
 *     HC-05 GND        -> GND
 *     HC-05 VCC        -> 5V
 *
 *   4x4 matrix keypad:
 *     Rows (outputs)   -> P1.16 .. P1.19
 *     Cols (inputs)    -> P1.20 .. P1.23
 *
 *   DC motor (locker latch actuator, via L293D H-bridge):
 *     IN1              -> P1.24
 *     IN2              -> P1.25
 *
 *   Buzzer (audible alert):
 *     Signal           -> P1.26
 *
 * DEFAULT / FACTORY PASSWORDS (written to EEPROM on first boot only)
 * ----------------
 *   Level-1 (Bluetooth, sent as "1234#" from the phone app) = 1234
 *   Level-2 (keypad, entered after Level-1 succeeds)          = 5678
 *===========================================================================*/
#include <lpc214x.h>
#include <string.h>

#include "types.h"
#include "defines.h"
#include "delay.h"
#include "lcd.h"
#include "keypad.h"
#include "buzzer.h"
#include "motor.h"
#include "uart.h"
#include "bluetooth.h"
#include "eeprom.h"
#include "rtc.h"
#include "security.h"
#include "menu.h"

static void pll_feed(void);
static void SystemInit_SecureLocker(void);
static void print2(u8 val);
static void display_live_clock(void);
static void DisplayStandby(void);
static void DisplayAccessGranted(void);
static void DisplayAccessDenied(const char *reason);
static void read_keypad_password(char *buf);
static void open_locker_sequence(void);
static void system_lockout(void);

/* 3-letter day-of-week names, indexed by the RTC's DOW register (0=SUN
 * .. 6=SAT), used by display_live_clock() below. */
static const char * const dow_names[7] = { "SUN","MON","TUE","WED","THU","FRI","SAT" };

/* Number of consecutive failed authentication attempts (a wrong
 * Level-1 Bluetooth password OR a wrong Level-2 keypad password each
 * count as one). Reset to 0 on any fully successful open, or after a
 * lockout period completes. See system_lockout() below. */
#define MAX_FAILED_ATTEMPTS   3
static u8 fail_count = 0;

/* Required "feed" sequence (0xAA then 0x55 written to PLL0FEED) that must
 * follow any write to the PLL control/configuration registers for the
 * change to actually take effect; this is a fixed hardware requirement
 * of the LPC2148's PLL, not project-specific logic. */
static void pll_feed(void)
{
    PLL0FEED = 0xAA;
    PLL0FEED = 0x55;
}

/* One-time hardware bring-up: configure the PLL for a 60 MHz core clock
 * from a 12 MHz crystal, set up the Memory Accelerator Module (MAM) for
 * that clock speed, then initialise every peripheral driver used by the
 * project (LCD, both UARTs, I2C/EEPROM, RTC, keypad, buzzer, motor,
 * tamper input, and the admin-button interrupt). */
static void SystemInit_SecureLocker(void)
{
    /* PLL setup:
       Crystal = 12MHz
       CCLK    = 60MHz
       PCLK    = 15MHz */
    MAMCR = 0x00;              /* Disable the MAM while changing clock speed */

    PLL0CON = 0x00;            /* Disable the PLL before reconfiguring it     */
    pll_feed();

    PLL0CFG = 0x24;            /* M=5, P=2 -> CCLK = 12MHz * (M+1) = 60MHz    */
    pll_feed();

    PLL0CON = 0x01;            /* Enable the PLL (not yet connected)          */
    pll_feed();

    while (!(PLL0STAT & (1UL << 10)));   /* Wait for the PLL to lock */

    PLL0CON = 0x03;            /* Enable AND connect the PLL as the clock source */
    pll_feed();

    VPBDIV = 0x00;             /* PCLK = CCLK / 4 = 15MHz (VPB divider = 1/4) */

    MAMTIM = 0x04;             /* MAM fetch cycles tuned for 60MHz operation  */
    MAMCR  = 0x02;             /* Re-enable the MAM in fully-enabled mode      */

    /* Bring up every peripheral driver used by the system. */
    lcd_init();
    uart0_init(9600);
    bluetooth_init(9600);
    i2c_init();
    rtc_init();
    keypad_init();
    buzzer_init();
    motor_init();
    security_init();
    admin_int_init();
}

/* Print a byte as two zero-padded decimal digits (e.g. 7 -> "07"). */
static void print2(u8 val)
{
    lcd_data((val / 10) + '0');
    lcd_data((val % 10) + '0');
}

/* Read the RTC directly (HOUR/MIN/SEC/DOM/MONTH/YEAR/DOW registers, the
 * same registers rtc.c and menu.c already use) and refresh the idle
 * screen in place - no lcd_clear() here, so the digits are simply
 * overwritten each call and the display doesn't flicker.
 *
 * Line 1: HH:MM:SS
 * Line 2: DD/MM/YYYY DOW
 *
 * This is only ever called from the idle/standby wait loop in main()
 * below - never while a Level-1 or Level-2 password is actually being
 * entered - so it refreshes the clock without ever interrupting or
 * re-issuing a password prompt mid-entry. */
static void display_live_clock(void)
{
    u8 hh = (u8)HOUR, mm = (u8)MIN, ss = (u8)SEC;
    u8 dd = (u8)DOM,  mo = (u8)MONTH;
    u16 yy = (u16)YEAR;
    u8 dow = (u8)DOW;

    lcd_gotoxy(0, 0);
    print2(hh); lcd_data(':');
    print2(mm); lcd_data(':');
    print2(ss);
    lcd_string("        ");   /* Pad out the rest of line 1 (clears any leftovers) */

    lcd_gotoxy(1, 0);
    print2(dd); lcd_data('/');
    print2(mo); lcd_data('/');
    lcd_data((yy / 1000) % 10 + '0');
    lcd_data((yy /  100) % 10 + '0');
    lcd_data((yy /   10) % 10 + '0');
    lcd_data( yy         % 10 + '0');
    lcd_data(' ');
    lcd_string((dow <= 6) ? dow_names[dow] : "???");
}

/* Show the idle/waiting screen while the system waits for a Bluetooth
 * password attempt: a continuously-updating live clock/date/day
 * display (merged in from the "EnviroTime" project's clock screen),
 * instead of a static message. */
static void DisplayStandby(void)
{
    lcd_clear();
    display_live_clock();
}

/* Show the "access granted" message on the LCD. */
static void DisplayAccessGranted(void)
{
    lcd_clear();
    lcd_string("ACCESS GRANTED");
}

/* Show the "access denied" message on the LCD, log the specific reason
 * for the audit trail, and sound the buzzer as feedback. */
static void DisplayAccessDenied(const char *reason)
{
    lcd_clear();
    lcd_string("ACCESS DENIED");
    log_event(reason);
    buzzer_alert(5);
}

/* Read the Level-2 password from the keypad with basic line-editing:
 *   0-9  -> append the digit (masked with '*' on the LCD)
 *   '*'  -> backspace: erase the previous digit from both the buffer
 *           and the LCD
 *   '#'  -> clear the entire entry and start again from the beginning
 * Any other key is ignored. Returns once exactly PWD_LEN digits have
 * been accepted. */
static void read_keypad_password(char *buf)
{
    u8 i = 0;
    char k;

    while (i < PWD_LEN)
    {
        k = keypad_getkey();
        if(k>='0' && k<='9')
        {
          buf[i++]=k;
          lcd_data('*');
        }
        else if(k == '*')          /* Backspace: remove the last entered digit */
        {
          if(i>0)
          {
            i--;
            buf[i] ='\0';

            lcd_gotoxy(1, i);       /* Move cursor back to the erased position */
            lcd_data(' ');           /* Blank out the '*' that was shown there   */
            lcd_gotoxy(1, i);        /* Leave the cursor ready for the next digit */
          }
        }
        else if(k == '#')          /* Clear: erase the whole entry so far */
        {
          while(i>0)
          {
             i--;
             buf[i] = '\0';

             lcd_gotoxy(1, i);
             lcd_data(' ');
          }
          lcd_gotoxy(1, 0);
        }
        // buf[i++] = k;
      //  lcd_data('*');
    }
    buf[PWD_LEN] = '\0';
}

/* Runs the physical locker actuation sequence once both passwords have
 * matched: show "access granted", turn the motor forward ONCE for a
 * single ~180-degree turn to open the locker (gate-style), hold it open,
 * then turn the motor in reverse ONCE for the same duration to bring it
 * back to its original (closed) position. Every stage is written to the
 * audit log.
 *
 * A "busy" guard prevents this function from ever being re-entered or
 * overlapped (e.g. if it were accidentally called again while already
 * running) - only one forward pulse and one reverse pulse can ever be in
 * flight, which is what stops the motor from appearing to run forward
 * and reverse "at the same time". */
static void open_locker_sequence(void)
{
    static u8 locker_busy = 0;

    if (locker_busy)
        return;         /* Already mid-sequence - ignore a duplicate call */
    locker_busy = 1;

    DisplayAccessGranted();
    log_event("Access granted, opening locker");

    /* --- Single forward pulse: turn the gate/latch open ~180 degrees --- */
    motor_stop();                 /* Make sure we start from a full stop   */
    motor_forward();
    delay_ms(MOTOR_ROTATE_MS);    /* One short, tuned pulse - NOT a multi-second
                                    * continuous run, so the motor turns once
                                    * instead of spinning through several
                                    * full rotations. */
    motor_stop();

    log_event("Locker opened");

    lcd_clear();
    lcd_string("LOCKER OPEN");
    lcd_gotoxy(1,0);
    lcd_string("WAIT...");
    delay_ms(5000);              /* Hold the gate open for the visitor    */

    lcd_clear();
    lcd_string("LOCKER CLOSE");

    /* --- Brief full-stop settle time before reversing direction --------
     * Reversing an H-bridge output straight from one direction to the
     * other while the motor still has momentum can cause a mechanical/
     * electrical jolt; a short stop first avoids that. */
    delay_ms(MOTOR_SETTLE_MS);

    /* --- Single reverse pulse: return the gate/latch to its original
     * (closed) position, using the exact same duration as the opening
     * pulse so it ends up back where it started. --- */
    motor_reverse();
    delay_ms(MOTOR_ROTATE_MS);
    motor_stop();

    log_event("Locker closed");

    locker_busy = 0;
}

/*============================================================
 * system_lockout
 * Called once fail_count reaches MAX_FAILED_ATTEMPTS (3 consecutive
 * wrong Level-1 or Level-2 password attempts). Discards any
 * Bluetooth command that may already be sitting in the receive
 * buffer (so a password sent during the lockout can't sneak
 * through the instant it ends), shows "SYSTEM LOCKED" on the LCD,
 * sounds the buzzer, and then blocks for 30 seconds - broken into
 * 1-second steps so the tamper switch is still monitored the whole
 * time. Once the 30 seconds elapse, the failed-attempt counter is
 * reset and normal operation resumes.
 *============================================================*/
static void system_lockout(void)
{
    u8 i;

    log_event("System locked: 3 consecutive failed attempts");

    bluetooth_clear();     /* Discard anything already buffered/in-flight */

    lcd_clear();
    lcd_string("SYSTEM LOCKED");
    lcd_gotoxy(1,0);
    lcd_string("WAIT 30 SEC");
    buzzer_alert(3);

    for (i = 0; i < 30; i++)
    {
        check_tamper_and_alert();   /* Tamper detection stays active during the lockout */
        delay_ms(1000);
    }

    fail_count = 0;
    log_event("System unlocked after 30s lockout");

    lcd_clear();
    lcd_string("SYSTEM UNLOCKED");
    delay_ms(1000);
}

int main(void)
{
    char bt_cmd[BT_BUF_SIZE];
    char l1_pwd[PWD_LEN + 1];
    char l2_pwd[PWD_LEN + 1];
    char kp_pwd[PWD_LEN + 1];

    SystemInit_SecureLocker();

    /* Set a fixed default date/time at boot; the admin can later correct
     * this via the admin menu (see menu.c -> edit_rtc()), since the
     * on-chip RTC has no battery backup information available here. */
    rtc_set_date(1, 1, 2024);
    rtc_set_time(12, 0, 0);

    /* On the very first boot (or if the EEPROM was blank/corrupted),
     * populate it with the factory-default passwords. */
    ensure_default_passwords();

    lcd_clear();
    lcd_string("Bluetooth");
    lcd_gotoxy(1,0);
    lcd_string("Secure System");

    uart0_string("\r\n====================================\r\n");
    uart0_string(" Bluetooth Secure Locker Started\r\n");
    uart0_string("====================================\r\n");

    log_event("System booted");

    delay_ms(2000);

    while (1)
    {
        /* Highest priority: if the admin button was pressed, service
         * the admin menu before doing anything else. */
        if (admin_flag)
            admin_menu();

        check_tamper_and_alert();
        check_alarm();   /* From the merged EnviroTime menu module: ring the buzzer if the alarm time is reached */

        DisplayStandby();

        /* Wait for a Bluetooth command to arrive, while continuously
         * refreshing the live clock (so it keeps ticking, not just a
         * single frozen snapshot), still polling the tamper switch, and
         * bailing out early if the admin button is pressed during the
         * wait. */
        while (!bluetooth_available())
        {
            check_tamper_and_alert();
            check_alarm();
            display_live_clock();   /* Refresh HH:MM:SS / date / day in place, no re-clear */

            if (admin_flag)
                break;

            delay_ms(100);
        }

        if (admin_flag)
             continue;   /* Go back to the top of the loop and open the admin menu */

        bluetooth_read_command(bt_cmd);

        if (bt_cmd[0] == '\0')
            continue;   /* Ignore an empty command (e.g. a stray terminator) */

        log_event("Bluetooth authentication request received");

        /* --- Level 1: check the Bluetooth password --- */
        eeprom_read_str(EEPROM_L1_ADDR, l1_pwd, PWD_LEN);

        /* DEBUG: print exactly what was received over Bluetooth and what
         * is currently stored in EEPROM as the Level-1 password, so a
         * mismatch (e.g. stray characters, or a password that was
         * previously changed via the admin menu) is immediately visible
         * on a UART0 serial terminal instead of only showing "ACCESS
         * DENIED" on the LCD. Safe to remove once you've confirmed
         * everything matches as expected. */
        uart0_string("DEBUG: received='");
        uart0_string(bt_cmd);
        uart0_string("' stored L1='");
        uart0_string(l1_pwd);
        uart0_string("'\r\n");

        if (strcmp(bt_cmd, l1_pwd) == 0)
        {
            lcd_clear();
            lcd_string("LEVEL1 OK");
            lcd_gotoxy(1,0);
            lcd_string("ENTER L2");

            log_event("Level-1 Bluetooth password matched");
            delay_ms(1000);

            lcd_clear();
            lcd_string("KEYPAD PWD:");
            lcd_gotoxy(1,0);

            /* --- Level 2: prompt for and check the keypad password --- */
            read_keypad_password(kp_pwd);

            eeprom_read_str(EEPROM_L2_ADDR, l2_pwd, PWD_LEN);

            if (strcmp(kp_pwd, l2_pwd) == 0)
            {
                log_event("Level-2 keypad password matched");
                fail_count = 0;         /* Full success - clear the failed-attempt counter */
                open_locker_sequence();
            }
            else
            {
                DisplayAccessDenied("Wrong Level-2 keypad password");

                fail_count++;
                if (fail_count >= MAX_FAILED_ATTEMPTS)
                    system_lockout();
            }
        }
        else
        {
            DisplayAccessDenied("Wrong Level-1 Bluetooth password");

            fail_count++;
            if (fail_count >= MAX_FAILED_ATTEMPTS)
                system_lockout();
        }

        delay_ms(1000);
    }
}

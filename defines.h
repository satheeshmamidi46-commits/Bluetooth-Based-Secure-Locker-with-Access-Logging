/*=============================================================================
 * File        : defines.h
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Global project-wide constants shared by several modules
 *               (EEPROM memory map, password length, Bluetooth buffer size,
 *               and the admin push-button interrupt configuration).
 *===========================================================================*/
#ifndef DEFINES_H
#define DEFINES_H

#include <lpc214x.h>   /* LPC2148 peripheral register map (SFRs)          */
#include "types.h"      /* u8 / u16 / u32 / s8 / s16 / s32 aliases         */

/* Number of digits in every password (Bluetooth Level-1 and keypad Level-2) */
#define PWD_LEN            4

/* --- EEPROM (AT24C256) address map -----------------------------------------
 * A small "magic" marker is stored first; if it does not match the expected
 * value the EEPROM is considered blank/uninitialised and is (re)loaded with
 * the two factory-default passwords the first time the system boots.
 * ------------------------------------------------------------------------ */
#define EEPROM_MAGIC_ADDR  0x0000   /* 4-byte marker "LKR1"                */
#define EEPROM_L1_ADDR     0x0010   /* Level-1 (Bluetooth) password, 4 digits */
#define EEPROM_L2_ADDR     0x0020   /* Level-2 (keypad) password, 4 digits */

/* Size of the ring/line buffer used to collect Bluetooth (HC-05) commands  */
#define BT_BUF_SIZE        32

/* --- Locker motor (gate-style open/close) timing --------------------------
 * The locker motor is a plain DC motor driven through an L293D (no encoder
 * or servo positioning), so "how far it turns" is controlled purely by how
 * long it is driven for. MOTOR_ROTATE_MS is the ON-time used for a single
 * open (forward) or close (reverse) movement.
 *
 * TUNE THIS VALUE FOR YOUR HARDWARE: start low (e.g. 300-500 ms) and
 * increase gradually while checking the physical rotation, until one pulse
 * turns the gate/latch about half a turn (180 degrees). Driving the motor
 * for several seconds (as a raw "hold direction" delay) lets it complete
 * many full rotations instead of a single controlled half-turn, which is
 * what previously made it look like it was spinning repeatedly.
 * MOTOR_SETTLE_MS is a short full-stop pause inserted before reversing
 * direction, to avoid mechanically/electrically shocking the motor and
 * H-bridge by reversing it while it still has momentum.
 * ------------------------------------------------------------------------ */
#define MOTOR_ROTATE_MS    500   /* ON-time for one ~180-degree open/close turn */
#define MOTOR_SETTLE_MS    200   /* Full-stop pause before reversing direction  */

/* --- Login lockout (merged in from the "EnviroTime" project's security
 * module) ---------------------------------------------------------------
 * After MAX_WRONG_ATTEMPTS consecutive failed login attempts (a failed
 * attempt is either a wrong Level-1 Bluetooth password, or a correct
 * Level-1 followed by a wrong Level-2 keypad password), the system locks
 * out for LOCK_DURATION_MS milliseconds, showing "SYSTEM LOCKED" on the
 * LCD, before automatically resetting the attempt counter.
 * ------------------------------------------------------------------------ */
#define MAX_WRONG_ATTEMPTS   3U       /* Lock after this many consecutive failures */
#define LOCK_DURATION_MS     30000U   /* Lock duration: 30 seconds                 */

/* --- Admin push-button (EINT2) configuration --------------------------------
 * The admin button is wired to P0.7 which, when configured for its
 * alternate function, becomes external interrupt input EINT2.
 * ------------------------------------------------------------------------ */
#define ADMIN_EINT_BIT     2    /* Bit position of EINT2 in EXTINT/EXTMODE/EXTPOLAR */
#define ADMIN_VIC_SRC      16   /* VIC channel number for the EINT2 interrupt source */

/* U1IER (UART1 Interrupt Enable Register) is not defined in every version
 * of the vendor header <lpc214x.h>, so it is defined manually here as a
 * safety net to guarantee the project always compiles. */
#ifndef U1IER
#define U1IER (*((volatile unsigned long *)0xE0010004))
#endif

#endif

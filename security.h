/*=============================================================================
 * File        : security.h
 * Project     : Bluetooth-Based Secure Locker with Access Logging
 * Description : Public API for tamper detection, the UART/RTC-timestamped
 *               access-logging helper, and first-boot password provisioning.
 *===========================================================================*/
#ifndef SECURITY_H
#define SECURITY_H

#include "types.h"

void security_init(void);              /* Configure the tamper-switch input pin           */
u8   tamper_detected(void);             /* Returns 1 if the tamper switch is currently open */
void check_tamper_and_alert(void);      /* Poll the tamper switch and raise an alert on change */
void ensure_default_passwords(void);    /* Load factory-default passwords on first boot     */
void log_event(const char *msg);        /* Print a timestamped event line over UART0         */

/*-------------- Login lockout (merged in from "EnviroTime") -------------
 * Call security_register_failure() after ANY failed login step (wrong
 * Level-1 Bluetooth password, or wrong Level-2 keypad password). Once
 * MAX_WRONG_ATTEMPTS in a row have failed, this function itself shows
 * "SYSTEM LOCKED" on the LCD and blocks for LOCK_DURATION_MS before
 * resetting the counter - the caller does not need to do anything else.
 * Call security_register_success() after a fully successful login (both
 * levels matched) to reset the failed-attempt counter back to zero.
 * ------------------------------------------------------------------------ */
void security_register_failure(void);
void security_register_success(void);

#endif


# 🔐 Bluetooth-Based Secure Locker with Access Logging

A secure embedded locker system built on the LPC2148 ARM7 microcontroller featuring Bluetooth authentication, keypad-based verification, EEPROM credential storage, RTC-based event management, access logging, alarm support, and motorized lock control.
2r
---

## 📖 Overview

This project implements a secure electronic locker using a dual-factor authentication mechanism. Access is granted only after successful Bluetooth and keypad verification. User credentials are stored in non-volatile EEPROM memory, ensuring persistence across power cycles.t c

The system integrates multiple embedded peripherals including UART communication, I²C EEPROM storage, RTC timekeeping, LCD user interaction, keypad input, buzzer alerts, interrupt-driven administration, and motorized locking.

---

## ✨ Key Features

* Dual-factor authentication

  * Bluetooth password verification
  * Keypad password verification
* EEPROM-based credential storage
* RTC-based clock and calendar management
* Access event logging
* Administrator configuration menu
* Password update functionality
* Alarm configuration and monitoring
* Interrupt-driven admin access
* Motorized lock control
* LCD-based user interface
* Buzzer-based security alerts
* Persistent settings and credentials
* Tamper and invalid-access indication

---

## 🏗 System Architecture
```text
                    +-------------------+
                    |   Mobile Device   |
                    +---------+---------+
                              |
                              | Bluetooth
                              |
                       +------+------+
                       |    HC-05    |
                       +------+------+
                              |
                              |
                     UART Communication
                              |
                              v

+------------------------------------------------+
|                  LPC2148 MCU                   |
+------------------------------------------------+
|                                                |
|  UART1  <----> HC-05 Bluetooth Module          |
|  I²C    <----> AT24C256 EEPROM                 |
|  RTC    <----> Time \\\& Date Management          |
|  GPIO   <----> LCD Display                     |
|  GPIO   <----> 4x4 Keypad                      |
|  GPIO   <----> Buzzer                          |
|  GPIO   <----> L293D Motor Driver              |
|  EINT2  <----> Admin Push Button               |
|                                                |
+------------------------------------------------+
                    |
                    |
                    v

             Motorized Lock System
```



## 🔒 Authentication Flow

```text
User Request
      |
      v
Bluetooth Authentication
      |
      v
Valid ?
  |        |
 No       Yes
  |         |
  v         v
Alert    Keypad Authentication
             |
             v
          Valid ?
        |         |
       No        Yes
        |          |
        v          v
     Alert     Access Granted
                   |
                   v
             Unlock Locker
                   |
                   v
               Log Event
```

---

## 🛠 Hardware Components

| Component              | Purpose                 |
| ---------------------- | ----------------------- |
| LPC2148                | Main Controller         |
| HC-05 Bluetooth Module | Wireless Authentication |
| AT24C256 EEPROM        | Credential Storage      |
| RTC Peripheral         | Time & Date Management  |
| 16x2 LCD               | User Interface          |
| 4x4 Keypad             | Password Entry          |
| Buzzer                 | Audible Alerts          |
| L293D                  | Motor Driver            |
| DC Motor               | Locker Control          |
| Push Button            | Admin Interrupt         |
| Power Supply           | System Power            |

---

## 🔌 Peripheral Interfaces

### UART

UART communication is used for:

* Bluetooth communication
* Command transmission
* Authentication data exchange

### I²C

I²C communication is used for:

* EEPROM read/write operations
* Credential storage
* Configuration persistence

### RTC

The Real-Time Clock subsystem provides:

* Timekeeping
* Date management
* Day tracking
* Alarm support
* Timestamp generation

---

## 📺 LCD Interface

The LCD interface provides:

* Authentication prompts
* System status messages
* Configuration menus
* Alarm notifications
* Administrative settings
* Error messages

---

## ⌨️ Keypad Interface

The keypad is used for:

* Password entry
* Menu navigation
* Administrative configuration
* Alarm configuration
* Clock configuration

---

## 🔔 Alarm System

The integrated alarm subsystem supports:

* Alarm time configuration
* Alarm enable/disable control
* Audible buzzer alerts
* Administrative reset
* RTC-triggered activation

---

## ⚙️ Administrative Features

Administrative access is provided through an interrupt-driven menu system.

Available options include:

### Clock Settings

* Set Time
* Set Date
* Set Day

### Alarm Settings

* Configure Alarm Time
* Enable Alarm
* Disable Alarm
* Reset Alarm

### Password Management

* Update Bluetooth Password
* Update Keypad Password
* Credential Verification
* EEPROM Persistence Check

---

## 💾 Credential Management

Passwords are stored in external EEPROM memory.

Capabilities include:

* Persistent storage
* Password updates
* Read-back verification
* Power-loss retention
* Configuration persistence

---

## 🔔 Security Features

* Dual-level authentication
* EEPROM-backed credentials
* Invalid password detection
* Audible security alerts
* Administrative control
* Alarm support
* Access-event tracking
* Non-volatile configuration storage

---

## 🚀 Build Environment

### Software

* Embedded C
* Keil µVision
* Flash Magic

### Firmware Modules

* LCD Driver
* Keypad Driver
* UART Driver
* EEPROM Driver
* RTC Driver
* Buzzer Driver
* Security Module
* Menu System
* Delay Utilities

---

## 🧪 Recommended Validation

Before deployment:

1. Verify GPIO connections.
2. Verify LCD operation.
3. Verify keypad scanning.
4. Verify Bluetooth communication.
5. Verify EEPROM read/write operations.
6. Verify RTC functionality.
7. Verify buzzer operation.
8. Verify motor direction.
9. Verify interrupt operation.
10. Verify authentication workflow.
11. Verify alarm functionality.
12. Verify power-cycle persistence.

---

## 🔮 Future Enhancements

* Encrypted credential storage
* Authentication rate limiting
* Temporary account lockout
* Enhanced tamper detection
* Protected audit records
* Motor position sensing
* Door status sensing
* Watchdog-based recovery
* Mobile application integration
* Wi-Fi connectivity
* Cloud-based access logs
* Biometric authentication

---

## ⚠️ Important Notes

This repository represents an embedded-system project and should be validated against the target hardware platform.

Actual behavior depends on:

* LPC2148 hardware configuration
* EEPROM device configuration
* Bluetooth module settings
* RTC configuration
* LCD wiring
* Keypad wiring
* Motor driver implementation

Hardware verification is recommended before deployment.

---

## 📜 License

Select and add an appropriate open-source license before distributing the repository publicly.

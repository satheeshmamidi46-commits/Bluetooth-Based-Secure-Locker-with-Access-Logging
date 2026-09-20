# 🔐 Bluetooth-Based Secure Locker with Access Logging

<p align="center">
  <img src="https://img.shields.io/badge/MCU-LPC2148%20%7C%20ARM7-blue" alt="MCU"/>
  <img src="https://img.shields.io/badge/IDE-Keil%20%C2%B5Vision-red" alt="IDE"/>
  <img src="https://img.shields.io/badge/License-MIT-green" alt="License"/>
  <img src="https://img.shields.io/badge/Language-Embedded%20C-orange" alt="Language"/>
</p>

> An **embedded security system** built on the **NXP LPC2148 (ARM7)** microcontroller that unlocks a physical locker using **two-factor authentication** — a **Bluetooth password** from your smartphone 📱 + a **keypad password** ⌨️ — while logging every event with real-time timestamps 🕐.

<p align="center">
  <img width="1379" height="1141" alt="vasudia" src="https://github.com/user-attachments/assets/a518c344-3098-4076-add9-c400769ab665" />
</p>

---

## 📖 Table of Contents

- [What Is This Project?](#-what-is-this-project)
- [Components & Peripherals Guide](docs/Components.md)
- [Features](#-features)
- [How It Works (Simple Explanation)](#-how-it-works-simple-explanation)
- [Demo Flow](#-demo-flow)
- [Hardware Required](#-hardware-required)
- [Pin Connections](#-pin-connections)
- [Project Structure](#-project-structure)
- [Default Passwords](#-default-passwords)
- [Admin Mode](#%EF%B8%8F-admin-mode)
- [Step-by-Step: Build & Run Your First Time](#-step-by-step-build--run-your-first-time)
  - [Step 1 — Install the Software](#step-1--install-the-software)
  - [Step 2 — Build the Firmware](#step-2--build-the-firmware)
  - [Step 3 — Flash the Firmware to the Board](#step-3--flash-the-firmware-to-the-board)
  - [Step 4 — Wire the Hardware](#step-4--wire-the-hardware)
  - [Step 5 — Watch the Logs on Your PC](#step-5--watch-the-logs-on-your-pc)
  - [Step 6 — Use the Locker](#step-6--use-the-locker)
- [Building with ARM GNU Toolchain (CLI — No Keil Required)](#-building-with-arm-gnu-toolchain-cli--no-keil-required)
  - [What Is the ARM GNU Toolchain?](#what-is-the-arm-gnu-toolchain)
  - [Step 1 — Install the ARM GNU Toolchain](#step-1--install-the-arm-gnu-toolchain)
  - [Step 2 — Build with One Command](#step-2--build-with-one-command)
  - [Makefile Targets Reference](#makefile-targets-reference)
  - [What Happens Under the Hood](#what-happens-under-the-hood)
  - [Keil vs GNU — Which Should You Use?](#keil-vs-gnu--which-should-you-use)
- [Configuration](#%EF%B8%8F-configuration)
- [Troubleshooting (FAQ)](#-troubleshooting-faq)
- [What You Can Learn From This Project](#-what-you-can-learn-from-this-project)
- [Future Improvements](#-future-improvements)
- [Documentation](#-documentation)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🤔 What Is This Project?

Imagine a **locker** (like a gym locker or a safe) that opens **only when you prove who you are in two different ways**:

1. **Something you have** — your smartphone, which sends a secret password over **Bluetooth**.
2. **Something you know** — a second password typed on a **physical keypad** attached to the locker.

Only if **both** passwords are correct does the locker open. And here's the cool part: **every single event** — successful unlocks, wrong passwords, timeouts, admin changes — is **recorded with the exact date & time** and sent to a PC, like a security camera for your data.

This is a complete, real-world **embedded systems project** — perfect for students and hobbyists learning **ARM7 microcontrollers, peripheral drivers, and IoT-style security**.

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| 🔑 **Two-Factor Authentication** | Bluetooth password (Level-1) **+** keypad password (Level-2) — both required |
| 📡 **HC-05 Bluetooth** | Receive passwords wirelessly from any phone Bluetooth-terminal app |
| ⌨️ **4x4 Matrix Keypad** | Physical password entry with 10-second timeout |
| 🖥️ **16x2 LCD Display** | Live status messages guiding the user at every step |
| 💾 **EEPROM Storage** | Passwords saved in AT24C256 chip — survive power loss |
| 🕐 **Real-Time Clock (RTC)** | On-chip RTC timestamps every event accurately |
| 📝 **Access Logging** | Full event log streamed to a PC over UART (9600 baud) |
| ⚙️ **Motorized Latch** | L293D motor driver opens/closes the locker automatically |
| 🔊 **Buzzer Feedback** | Beeps on success and alerts on denied access |
| 🛠️ **Admin Mode** | Secret button to set the clock and change passwords |
| ⏱️ **Smart Timeouts** | Auto-cancels if the user takes too long (30 s BT / 10 s keypad) |

---

## 🧠 How It Works (Simple Explanation)

```
 ┌──────────────┐      ┌────────────────┐
 │  Smartphone  │─────▶│  HC-05 BT      │──┐
 │  (BT app)    │      │  (Level-1 pwd) │  │      ┌─────────────┐      ┌──────────┐
 └──────────────┘      └────────────────┘  ├─────▶│   LPC2148   │─────▶│  L293D   │──▶ 🔓 LOCKER
                                           │      │    (Brain)  │      │  Motor   │    LATCH
 ┌──────────────┐      ┌────────────────┐  │      └─────────────┘      └──────────┘
 │  4x4 Keypad  │─────▶│  Keypad driver │──┘            │
 │  (Level-2 pwd)│     │                │               ▼
 └──────────────┘      └────────────────┘      ┌──────────────────┐
                                               │  UART0 → PC Log  │
 ┌──────────────┐      ┌────────────────┐      │  (with RTC time) │
 │ Admin Button │─────▶│ EINT1 interrupt│─────▶└──────────────────┘
 └──────────────┘      └────────────────┘
```

**The authentication journey, step by step:**

1. 🟢 The system boots up, shows *"BT Secure Locker"* on the LCD, and waits.
2. 📱 You pair your phone with the **HC-05** module and send your **Level-1 password** (e.g., `1234`) using any Bluetooth terminal app.
3. ✅ If correct → LCD shows *"LEVEL-1 PASSED"* and asks for the keypad password.
4. ⌨️ You type your **Level-2 password** (e.g., `5678`) on the 4x4 keypad.
5. 🔓 If correct → the buzzer beeps, the **motor opens the locker**, it stays open for **5 seconds**, then closes automatically.
6. ❌ If either password is wrong or you take too long → *"ACCESS DENIED"* on the LCD, a warning beep, and the attempt is **logged**.
7. 📝 Meanwhile, the PC terminal receives a timestamped record of everything that happened.

---

## 🎬 Demo Flow

**Successful unlock:**
```
LCD: "BT SECURE LOCKER"  →  "Send BT Password"  →  "LEVEL-1 PASSED"
     →  "Enter L2 PWD"   →  "ACCESS GRANTED!"   →  "LOCKER OPEN"
     →  "Closing in 5s"  →  back to standby
```

**PC log for that event:**
```
========================================
LOCKER ACCESS
Time : 12/09/2026 14:32:07
Status : SUCCESS
Action : Opening Locker
========================================
```

**Failed attempt:**
```
LCD: "ACCESS DENIED" / "Invalid Bluetooth Password"  +  🔊 buzzer
PC : "Status : FAILED / Reason : Invalid Bluetooth Password"
```

---

## 🧰 Hardware Required

| # | Component | Qty | Purpose | Approx. Cost (₹) |
|---|-----------|-----|---------|------------------|
| 1 | LPC2148 dev board (ARM7) | 1 | The "brain" of the system | 800–1500 |
| 2 | HC-05 Bluetooth module | 1 | Receives password from phone | 300–400 |
| 3 | 16x2 LCD display | 1 | Shows status messages | 150–200 |
| 4 | 4x4 matrix keypad | 1 | Physical password entry | 100–150 |
| 5 | AT24C256 EEPROM module (HW-669) | 1 | Stores passwords permanently | 50–100 |
| 6 | L293D motor driver IC | 1 | Drives the locker motor | 50–100 |
| 7 | DC motor (locker latch) | 1 | Physically opens the locker | 100–300 |
| 8 | Active buzzer | 1 | Sound feedback | 10–20 |
| 9 | Push button + 10 kΩ resistor | 1 | Admin mode entry | 10 |
| 10 | 10 kΩ potentiometer | 1 | LCD contrast adjustment | 10 |
| 11 | USB-UART adapter (CP2102/FTDI) | 1 | PC logging + firmware flashing | 150–250 |
| 12 | 32.768 kHz crystal | 1 | Accurate RTC timekeeping | 20 |
| 13 | 5 V power supply (≥1 A) | 1 | Powers everything | 100–200 |
| 14 | Jumper wires + breadboard | — | Connections | 100 |

**Total: roughly ₹2,000–3,500** — a very affordable embedded project! 💰

---

## 🔌 Pin Connections

### Quick Pin Map

| Peripheral | LPC2148 Pins |
|------------|--------------|
| **LCD (8-bit)** | DATA = P0.16–P0.23, RS = P0.4, EN = P0.5, RW = P0.18 |
| **Keypad** | Rows = P1.16–P1.19, Cols = P1.20–P1.23 |
| **UART0 → PC log** | TXD = P0.0, RXD = P0.1 (9600 baud) |
| **UART1 → HC-05** | TXD = P0.8, RXD = P0.9 (9600 baud) |
| **I2C0 → EEPROM** | SCL = P0.2, SDA = P0.3 |
| **Motor (L293D)** | IN1 = P0.12, IN2 = P0.13 |
| **Buzzer** | P0.11 |
| **Admin button (EINT1)** | P0.14 (falling edge, 10 kΩ pull-up) |

> ⚠️ **Beginner warning — pin conflict:** The code header lists **UART1 TXD = P0.8**, which is the **same pin as LCD data line D0**. Check your actual board wiring and `uart1.c` before assembling.

### Key Wiring Rules for Beginners

- 🔀 **UART is always crossed:** MCU TXD → module RXD, and MCU RXD ← module TXD.
- ⚡ **HC-05 RXD pin is 3.3 V logic** — use a simple voltage divider (1 kΩ / 2 kΩ) from the MCU's 5 V TX line to be safe.
- 🌍 **All grounds must be connected together** (MCU, motor supply, HC-05, everything).
- 🔩 The AT24C256 module (HW-669) already has I2C pull-up resistors onboard — no extras needed.

---

## 📁 Project Structure

The project is organized into **module folders** — each hardware peripheral has its own folder containing its driver (`.c`), header (`.h`), and pin/register defines (`*_defines.h`):

```
BT_SECURE-LOCKER/
│
├── core/                            # 🧠 Core application files
│   ├── main.c                       #     Main logic: auth flow, admin menu, locker control
│   ├── config.h                     #     All settings: passwords, timings, EEPROM addresses
│   ├── define.h                     #     Global defines
│   ├── types.h / type.h             #     Custom types (u8, u32, s32...)
│   ├── Startup.s                    #     ARM7 startup code for Keil (armasm syntax)
│   ├── startup_gnu.S                #     ARM7 startup code for GNU toolchain (gas syntax)
│   └── LPC21xx.h                    #     LPC2148 register definitions
│
├── drivers/                         # ⚙️ Hardware drivers (one folder per module)
│   ├── uart/                        # 📡 Serial communication
│   │   ├── uart0.c / uart0.h        #     UART0 → PC logging
│   │   ├── uart0_defines.h          #     UART0 register/pin defines
│   │   └── uart1.c / uart1.h        #     UART1 → HC-05 Bluetooth (interrupt-driven)
│   │
│   ├── lcd/                         # 🖥️ Display
│   │   ├── lcd.c / lcd.h            #     16x2 LCD driver (8-bit mode)
│   │   └── lcd_defines.h            #     LCD pin defines & commands
│   │
│   ├── keypad/                      # ⌨️ Input
│   │   ├── kpm.c / kpm.h            #     4x4 matrix keypad driver
│   │   └── kpm_defines.h            #     Keypad row/col pin defines
│   │
│   ├── i2c/                         # 🔌 Communication bus
│   │   ├── i2c.c / i2c.h            #     I2C0 protocol driver
│   │   └── i2c_defines.h            #     I2C register defines
│   │
│   ├── eeprom/                      # 💾 Persistent storage
│   │   └── eeprom.c / eeprom.h      #     AT24C256 password read/write
│   │
│   ├── rtc/                         # 🕐 Timekeeping
│   │   ├── rtc.c / rtc.h            #     On-chip RTC driver
│   │   └── rtc_defines.h            #     RTC register defines
│   │
│   ├── motor/                       # ⚙️ Actuation
│   │   ├── motor.c / motor.h        #     L293D motor + buzzer control
│   │   └── motor_defines.h          #     Motor/buzzer pin defines
│   │
│   ├── eint/                        # ⚡ Interrupts
│   │   └── eint.c / eint.h          #     External interrupt (admin button)
│   │
│   └── delay/                       # ⏱️ Timing
│       └── delay.c / delay.h        #     Software delay functions
│
├── Makefile                         # 🔨 GNU toolchain CLI build (arm-none-eabi-gcc)
├── ldscript/
│   └── lpc2148.ld                   # 📍 GNU linker script (flash/RAM memory map)
├── blutooth.uvproj                  # 🔨 Keil µVision project (open this!)
├── docs/
│   ├── Hardware.md                  # 🔌 Detailed wiring guide
│   └── Components.md                # 📚 Component & peripheral deep-dive guide
├── CHANGELOG.md                     # 📜 Version history
├── CONTRIBUTING.md                  # 🤝 How to contribute
├── LICENSE                          # 📄 MIT License
└── README.md                        # 📖 You are here
```

> 💡 **How the Keil project is organized:** Inside µVision you'll see the same structure as neat groups — `Startup`, `Application`, `Drivers\UART`, `Drivers\LCD`, `Drivers\Keypad`, `Drivers\I2C_EEPROM`, `Drivers\RTC`, `Drivers\Motor_Buzzer`, `Drivers\Interrupts`, `Drivers\Delay`. All include paths are pre-configured in the project file.

> 💡 Build outputs (`.hex`, `.axf`, `.o`, `.map`...) are **not** stored in the repo — they're generated when you build. See `.gitignore`.

---

## 🔑 Default Passwords

| Level | How You Enter It | Default | Stored At (EEPROM) |
|-------|------------------|---------|--------------------|
| **Level-1** | Bluetooth (phone app) | `1234` | `0x0000` |
| **Level-2** | 4x4 keypad | `5678` | `0x0010` |

> 🔒 **Change these immediately** using Admin Mode (see below) — anyone who reads this README knows the defaults!

---

## 🛠️ Admin Mode

Press the **push button on P0.14** to enter Admin Configuration Mode:

```
┌────────────────────────┐
│   1.RTC  2.PWDS        │
│   3.EXIT               │
└────────────────────────┘
```

| Option | What It Does |
|--------|--------------|
| `1` | **Set Date & Time** — enter hour, min, sec, day, month, year on the keypad (validated, e.g., month must be 1–12) |
| `2` | **Change Passwords** — choose `1` for Bluetooth password or `2` for keypad password; requires the **old password** + **confirmation** of the new one |
| `3` | **Exit** back to normal mode |

- ⏱️ Every admin menu auto-exits after **10 seconds** of inactivity (security!).
- 📝 All admin actions are logged to the PC with timestamps.

---

## 🚀 Step-by-Step: Build & Run Your First Time

*New to embedded systems? No problem — follow these steps in order.*

### Step 1 — Install the Software

1. **Keil µVision (MDK-ARM)** — download from [keil.com](https://www.keil.com/download/product/) (free MDK-Lite edition works).
   - During install, make sure **ARM7 (LPC21xx) device pack** support is included.
2. **Flash Magic** — free tool to upload firmware to the board: [flashmagictool.com](https://www.flashmagictool.com/).
3. **A serial terminal** — any one of: [PuTTY](https://www.putty.org/), [Tera Term](https://ttssh2.osdn.jp/), or the Arduino IDE's Serial Monitor.

### Step 2 — Build the Firmware

1. Clone or download this repository:
   ```bash
   git clone https://github.com/21vasundhara/BT_SECURE-LOCKER.git
   cd BT_SECURE-LOCKER
   ```
2. Double-click **`blutooth.uvproj`** — it opens in Keil µVision.
3. Press **F7** (or Project → Build Target).
4. You should see `0 Error(s)` in the Build Output window. ✅
   - The firmware file **`blutooth.hex`** is now generated in the project folder.

### Step 3 — Flash the Firmware to the Board

**Using Flash Magic (easiest for beginners):**

1. Connect the **USB-UART adapter** to the board: adapter TXD → P0.1, adapter RXD → P0.0, GND → GND.
2. Put the board into **ISP mode** (hold the ISP/BOOT button while pressing reset — check your board's manual).
3. Open Flash Magic:
   - Device: **LPC2148**
   - COM Port: your adapter's COM port (check Device Manager)
   - Baud rate: **9600**
   - Hex file: browse to **`blutooth.hex`**
4. Click **Start**. Done! 🎉

**Alternative:** In Keil, use **Flash → Download** if you have a ULINK/J-Link debugger.

### Step 4 — Wire the Hardware

Follow the [Pin Connections](#-pin-connections) table above and the detailed diagrams in [docs/Hardware.md](docs/Hardware.md).

**Beginner checklist:**
- [ ] LCD contrast pot adjusted (turn it until you see characters)
- [ ] HC-05 TX/RX **crossed** correctly
- [ ] EEPROM module A0–A2 pins to GND
- [ ] 10 kΩ pull-up on the admin button (P0.14)
- [ ] All grounds connected together
- [ ] Motor on a separate supply (or at least with decoupling capacitors)

### Step 5 — Watch the Logs on Your PC

1. Keep the USB-UART adapter connected (P0.0/P0.1).
2. Open your serial terminal at **9600 baud, 8N1**.
3. Power the board — you'll see the boot log and every event live! 📺

### Step 6 — Use the Locker

1. 📱 On your phone, pair with **HC-05** (PIN is usually `1234` or `0000`).
2. Open a Bluetooth terminal app (e.g., *"Serial Bluetooth Terminal"* on Android).
3. Send `1234` (the default Level-1 password).
4. ⌨️ Type `5678` on the keypad within 10 seconds.
5. 🔓 Watch the locker open for 5 seconds, then close!
6. 🛠️ Press the admin button to set the RTC clock and change the passwords.

---

## 🐧 Building with ARM GNU Toolchain (CLI — No Keil Required)

> 🆓 **Prefer free & open-source tools?** This project can be built **entirely from the command line** using the **ARM GNU toolchain** — no Keil license, no IDE, works on **Windows, Linux, and macOS**. Perfect for CI, VS Code users, and Linux enthusiasts!

### What Is the ARM GNU Toolchain?

The **Arm GNU Toolchain** (formerly *GNU Arm Embedded Toolchain*) is a free, open-source collection of tools maintained by Arm:

| Tool | Command | What It Does |
|------|---------|--------------|
| Compiler | `arm-none-eabi-gcc` | Compiles C into ARM7 machine code |
| Assembler | `arm-none-eabi-as` | Assembles startup code (invoked by gcc for `.S` files) |
| Linker | `arm-none-eabi-ld` (via gcc) | Combines objects using our linker script |
| Converter | `arm-none-eabi-objcopy` | Produces `.hex` / `.bin` files for flashing |
| Size report | `arm-none-eabi-size` | Shows flash/RAM usage |

The `none-eabi` part means *"bare-metal"* — no operating system, exactly what a microcontroller needs.

### Step 1 — Install the ARM GNU Toolchain

**Windows:**
- Download the **Arm GNU Toolchain** from [developer.arm.com/downloads/-/gnu-rm](https://developer.arm.com/downloads/-/gnu-rm) (choose `arm-none-eabi` Windows .exe/.zip), **or**
- Easiest: install via [xPack](https://xpack-dev-tools.github.io/arm-none-eabi-gcc-xpack/):
  ```bash
  npm install -g xpm
  xpm install --global @xpack-dev-tools/arm-none-eabi-gcc
  ```
- Add the `bin` folder to your **PATH**, then verify:
  ```bash
  arm-none-eabi-gcc --version
  ```
- You'll also need **make** — on Windows, install [GnuWin Make](http://gnuwin32.sourceforge.net/packages/make.htm) or use `make` bundled with Git Bash / MSYS2.

**Linux (Debian/Ubuntu):**
```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi make
```

**macOS (Homebrew):**
```bash
brew install --cask gcc-arm-embedded
```

### Step 2 — Build with One Command

From the project root:

```bash
make
```

That's it! 🎉 You'll see output like:

```
gcc -mcpu=arm7tdmi -marm -O2 ... -c core/main.c -o build/core/main.o
gcc -mcpu=arm7tdmi -marm -O2 ... -c drivers/uart/uart0.c -o build/drivers/uart/uart0.o
...
Linked: build/blutooth_gnu.elf
Created: build/blutooth_gnu.hex
Created: build/blutooth_gnu.bin
   text    data     bss     dec     hex filename
  12345      120    1200   13665    3561 build/blutooth_gnu.elf
```

**Output files (in `build/`):**

| File | Purpose |
|------|---------|
| `blutooth_gnu.elf` | ELF with debug symbols (for GDB/OpenOCD debugging) |
| `blutooth_gnu.hex` | Intel HEX — flash with **Flash Magic** or `lpc21isp` |
| `blutooth_gnu.bin` | Raw binary — flash with OpenOCD/J-Link |
| `blutooth_gnu.map` | Linker map — shows exactly what went where in memory |

### Makefile Targets Reference

| Command | What It Does |
|---------|--------------|
| `make` | Build everything (ELF + HEX + BIN + size report) |
| `make size` | Show flash/RAM usage of the last build |
| `make flash` | Flash via `lpc21isp` — set your port first: `make flash PORT=COM3` (Windows) or `make flash PORT=/dev/ttyUSB0` (Linux) |
| `make clean` | Delete the entire `build/` folder |
| `make help` | List available targets |

**Useful variations:**
```bash
make flash PORT=/dev/ttyUSB0        # Linux flashing
make C_SRCS="core/main.c ..."       # override source list (advanced)
```

### What Happens Under the Hood

The GNU build uses three extra files that the Keil build doesn't need:

1. **`core/startup_gnu.S`** — The Keil startup (`core/Startup.s`) uses **armasm syntax** (`AREA`, `EXPORT`, `EQU`...) which GNU's assembler doesn't understand. `startup_gnu.S` is the **same startup logic rewritten in GNU (gas) syntax**: exception vector table at address 0, mode stack setup, PLL → 60 MHz, MAM enable, copy `.data` from flash to RAM, zero `.bss`, then call `main()`.
2. **`ldscript/lpc2148.ld`** — Tells the linker the LPC2148 memory map: **512 KB flash @ 0x00000000**, **32 KB RAM @ 0x40000000**, and where each section goes.
3. **`Makefile`** — Orchestrates compiling all `core/` + `drivers/` sources with the right flags:
   - `-mcpu=arm7tdmi -marm` — target the LPC2148's ARM7TDMI core in ARM mode
   - `-ffreestanding` — bare-metal build (no OS)
   - `-ffunction-sections -fdata-sections` + `-Wl,--gc-sections` — strips unused code for a smaller binary

**Portability note:** The interrupt service routines use a portable macro in `core/types.h`:
```c
#ifdef __ARMCC_VERSION
    #define IRQ_HANDLER __irq                                        /* Keil ARMCC */
#else
    #define IRQ_HANDLER __attribute__((interrupt("IRQ")))            /* GNU GCC   */
#endif
```
So the **same C source builds correctly with both toolchains** — no code duplication.

### Keil vs GNU — Which Should You Use?

| | Keil µVision | ARM GNU (CLI) |
|---|---|---|
| **Cost** | Free up to 32 KB code (MDK-Lite) | 100% free & open source |
| **Interface** | GUI IDE | Command line / any editor |
| **Platforms** | Windows only | Windows, Linux, macOS |
| **Debugger** | Built-in (ULINK/J-Link) | GDB + OpenOCD (setup required) |
| **Best for** | Beginners, classroom labs | CI/CD, Linux users, VS Code workflows |
| **Output** | `blutooth.axf` / `.hex` | `blutooth_gnu.elf` / `.hex` / `.bin` |

> 💡 **Both builds produce functionally identical firmware** — same startup configuration (60 MHz PLL, MAM enabled), same drivers, same behavior on hardware. Pick whichever fits your workflow!

---

## ⚙️ Configuration

All tunable settings live in one file: [`config.h`](config.h) — no need to dig through driver code!

```c
#define PASSWORD_LENGTH         5      // Max password buffer size
#define DEFAULT_BT_PASSWORD   "1234"   // Factory Level-1 (Bluetooth) password
#define DEFAULT_KP_PASSWORD   "5678"   // Factory Level-2 (Keypad) password

#define L1_ADDR               0x0000   // EEPROM address: BT password
#define L2_ADDR               0x0010   // EEPROM address: keypad password

#define LOCKER_OPEN_TIME_MS   5000     // How long the locker stays open
#define MOTOR_RUN_TIME_MS     3000     // Motor actuation duration

#define BUZZER_BEEP_COUNT     5        // Beeps per alert
#define BUZZER_ON_TIME        200      // Beep ON duration (ms)
#define BUZZER_OFF_TIME       200      // Beep OFF duration (ms)
```

After editing, rebuild (F7) and re-flash to apply changes.

---

## 🩺 Troubleshooting (FAQ)

<details>
<summary><b>LCD shows only black boxes / nothing</b></summary>

Turn the **contrast potentiometer** slowly until characters appear. Also double-check RS (P0.16), EN (P0.17), and data lines P0.8–P0.15.
</details>

<details>
<summary><b>Phone connects to HC-05 but the locker doesn't respond</b></summary>

1. Make sure you're sending the password with a **newline/enter** if required.
2. Check the HC-05 baud rate is **9600** (factory default).
3. Verify TX/RX are **crossed** (HC-05 TXD → P0.9).
4. Remember: the system waits only **30 seconds** for the BT password.
</details>

<details>
<summary><b>Keypad presses do nothing</b></summary>

Check the row/column wiring order (P1.16–P1.23). If wrong characters appear, your rows and columns are probably swapped — swap the wires.
</details>

<details>
<summary><b>EEPROM errors / passwords reset after power off</b></summary>

Verify the AT24C256 module: VCC = 5 V, A0/A1/A2 = GND, WP = GND, SCL = P0.2, SDA = P0.3. If using a bare chip (not the module), add 4.7 kΩ pull-ups on SCL and SDA.
</details>

<details>
<summary><b>The board resets whenever the motor runs</b></summary>

Classic power problem! The motor draws too much current. Use a **separate supply for the motor** (L293D VCC2), add a 100 µF capacitor near the L293D, and make sure all grounds are common.
</details>

<details>
<summary><b>Admin button doesn't work</b></summary>

Check the **10 kΩ pull-up** resistor on P0.14 to 3.3 V, and the button connecting P0.14 to GND when pressed.
</details>

<details>
<summary><b>Serial log shows garbage characters</b></summary>

Wrong baud rate — set your terminal to exactly **9600 baud, 8 data bits, no parity, 1 stop bit**.
</details>

<details>
<summary><b>RTC shows wrong time after every power cycle</b></summary>

The on-chip RTC needs the **32.768 kHz crystal** fitted and VBAT backed up. Without it, time resets on power loss.
</details>

---

## 🎓 What You Can Learn From This Project

This project is a fantastic learning resource covering:

- 🧩 **Modular embedded C** — one driver per peripheral, clean separation of concerns
- 📡 **UART programming** — polling (UART0) and interrupt-driven (UART1) approaches
- 🔌 **I2C protocol** — talking to external EEPROM memory
- ⏰ **RTC peripherals** — real-time clock configuration and reading
- ⚡ **External interrupts (EINT)** — reacting to hardware buttons
- 🔐 **Embedded security concepts** — two-factor auth, non-volatile credential storage, timeouts
- ⚙️ **Motor control** — H-bridge (L293D) direction control
- 🖥️ **State-machine thinking** — standby → auth → grant/deny → admin flows

---

## 🔮 Future Improvements

- [ ] Limit password attempts (e.g., 3 tries → lockout period)
- [ ] Only write default passwords on true first boot (EEPROM blank-check)
- [ ] Resolve the UART1 TXD / LCD D0 pin overlap (P0.8)
- [ ] Add GSM module for SMS alerts on failed attempts
- [ ] Password masking (`*` on LCD while typing)
- [ ] Store access log history in EEPROM, not just UART stream


---

## 📚 Documentation

| Document | Contents |
|----------|----------|
| [docs/Components]| 📚 **Deep-dive into every component** — I2C, HC-05, L293D, LCD, keypad, EEPROM, RTC, interrupts — theory + project implementation |
| [docs/Hardware]| Full BOM, wiring diagrams, power notes, troubleshooting |
| [CONTRIBUTING]| How to report bugs and submit improvements |
| [CHANGELOG]| Version history and planned features |

---

<p align="center">
  <i>Built with ❤️ for embedded systems learning — LPC2148 · ARM7 · Keil MDK · HC-05</i><br/>
  ⭐ <b>If this project helped you, please give it a star!</b> ⭐
</p>

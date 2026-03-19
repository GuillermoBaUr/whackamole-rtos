# whackamole-rtos
Whack‑A‑Mole with EXTI buttons, SPI SSD1331 OLED, message queue, timer callback, semaphores, and UART logging.


 small embedded game built in **under 3 hours** for the Spaceium engineering challenge.  
Runs on an **STM32F4 Nucleo** (F401RE/F411RE class) with **FreeRTOS**, an **SSD1331 OLED** via **SPI2**, **EXTI buttons** on `PA8..PA11`, and **UART2 (115200)** for logs.

> Core concepts: **multitasking**, **message queues**, **software timers**, **semaphores**, **GPIO interrupts**, and **SPI display I/O**.

The main firmware logic is located in [`main.c`](Embedded-Software-Engineer-SPACEIUM/Core/Src/main.c).

## 📸 Media

<p align="center">
  <img src="Img_project/Whack-A-Mole.jpeg" width="520" />
</p

Watch the video demonstration of the application, titled "Whack‑A‑Mole on STM32 (FreeRTOS)" [here](https://www.youtube.com/watch?v=1Qv2hCJcYMA).

---

## Features

- FreeRTOS **tasks** for game loop, display updates, and countdown HUD
- **Message queue** moves button events from ISR → worker task
- **One‑shot software timer** picks the next LED and paces turns
- **Semaphores** coordinate OLED updates and timer completion
- **Debounced EXTI** inputs on `PA8..PA11`
- **Randomized** LED target (never repeats twice in a row)
- **SSD1331** OLED HUD: score + `mm:ss.t` countdown
- **UART** console logs @ 115200

---

## 🧩 Game Rules

- Press the **Blue buttn** to **start**.
- A colored **LED** turns on—press the **matching color button**:
  - Correct → score **+1**
  - Wrong → score **–1** (min 0)
- Each turn **gets faster**.
- Game lasts **~26 s**.

---

## Architecture

**RTOS Objects**
- **Tasks**
  - `startOledTask` — draws time/score (serializes OLED access)
  - `startGame` — game loop: starts timer, waits for turn done
  - `startCountingPoints` — dequeues button events, updates score
  - `StartCounterTask` — countdown and HUD time string
  - `StartDefaultTask` — idle placeholder
    
- **Timer**
  - `myTimer01` (one‑shot) → `selectLEDCallback` picks next LED and releases `timerBinarySem`
    
- **Queue**
  - `myGameQueue` — carries color codes from button ISR
    
- **Semaphores**
  - `timerBinarySem` — signals end of turn
  - `oledSemaphore` — allows OLED updates (capacity 4)

**Interrupts & I/O**
- **EXTI** on `PA8..PA11`: falling edge → debounced → enqueue `COLOR_*`
- **B1** (user start button, e.g., `PC13`)
- **SSD1331** on **SPI2** (`CS/RES/DC` on `GPIOB`)
- **LEDs** on `GPIOB`: BLUE, GREEN, YELLOW, RED
- **UART2** at **115200** for logs

## Hardware

- **MCU**: STM32F4 Nucleo (tested on F401RE/F411RE class)
- **Display**: 0.96" **SSD1331 OLED** (SPI)
- **Inputs**:
  - **B1** start button (e.g., `PC13`)
  - **4x color buttons** on `PA8`, `PA9`, `PA10`, `PA11` (pull‑ups, falling edge)
- **Outputs**: `GPIOB` LEDs: BLUE, GREEN, YELLOW, RED
- **Serial**: **USART2 @ 115200** for logs

> Pin macros (e.g., `BLUE_LED_Pin`, `SSD1331_CS_Pin`) live in `main.h` (CubeMX). Adjust for your wiring.

---

📬 Contact
If you have any questions:
📧 badillouribeguillermoca@gmail.com

📄 License
This repository is licensed under the MIT License.

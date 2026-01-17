This repository does not provide ready-to-use firmware intended for
illegal interference with wireless communications.

License: Apache License 2.0

nRF_Jammer_v3
==============

OVERVIEW
--------
Hello!
I would like to share my project nRF_Jammer_v3 — an experimental RF project
based on ESP32 and multiple nRF24 modules.

The project was built mainly for educational, research, and hardware
experimentation purposes. It focuses on multi-module RF handling,
power management, and a clean, logical user interface.

IMPORTANT:
This project is intended for educational and experimental use only.
Usage of RF jamming or interference may be illegal in many countries.
The author does not take responsibility for any misuse of this project.

What has changed in version v3.0.1 compared to v3?
-----------
- Improved the UI interface (cleaner and more intuitive).
- Added support for expanding with additional nRF modules via I2C on ESP32 (TX implemented, RX still in progress).
- Improved jamming range.
- Improved SPI line stability.

KEY FEATURES
------------
- Support for 4× nRF24E01 / nRF24L01 modules
- ESP32-WROOM microcontroller
- 1.3" OLED display with a clear and logical UI
- 3 physical navigation buttons
- Battery-powered (18650 Li‑ion)
- Battery voltage monitoring
- Modular and expandable design
- Possibility to expand with additional NRF modules via I2C (work in progress: transmission over I2C is already implemented,      only reception on the second ESP32 remains).

  Thanks to the use of four nRF24 modules, the device achieves significantly
  improved RF coverage compared to single-module designs.


HARDWARE COMPONENTS
------------------
- ESP32-WROOM
- 4× nRF24E01 / nRF24L01
- 1.3" OLED display (I2C)
- 3× buttons (Left / Right / Select)
- 2× 100kΩ resistors (battery voltage measurement)
- 2× 4.7kΩ pull-up resistors (optional, I2C SDA/SCL)
- 3.7V Li‑ion battery (18650)
- 3.3V voltage regulator
- 5V boost converter (>1A)
- Power switch (>2A)
- 18650 charging module


PIN CONNECTIONS
---------------

nRF24 MODULES
-------------
Pin   | nRF1 | nRF2 | nRF3 | nRF4
--------------------------------
SCK   | 14   | 14   | 18   | 18
MOSI  | 13   | 13   | 23   | 23
MISO  | 12   | 12   | 19   | 19
CE    | 26   | 27   | 4    | 16
CSN   | 15   | 25   | 2    | 17


OLED DISPLAY (I2C)
-----------------
SDA  -> ESP32 pin 21
SCL  -> ESP32 pin 22


BUTTONS
-------
Left button    -> ESP32 pin 33
Right button   -> ESP32 pin 32
Select button  -> ESP32 pin 5


BATTERY VOLTAGE READING
-----------------------
Battery ADC -> ESP32 pin 34


PLANNED FEATURES / ROADMAP
--------------------------
- 2.4 GHz spectrum scanning
- Sleep mode (to remove the need for a power switch, under consideration)
- Improved battery voltage measurement accuracy
- Configuration via Serial Monitor
- IR-based experiments
- Wi‑Fi network scanning (research purposes)
- BLE scanning
- UI and performance optimizations
- Improved RF efficiency and power handling


INTERFACE DESIGN
----------------
The user interface was designed to be clean, logical, and easy to navigate,
optimized for small OLED displays.


DISCLAIMER
----------
This repository is provided for educational and research purposes only.
Any functionality that may interfere with wireless communication must be used
only in controlled environments and in accordance with local laws.


LICENSE
-------
MIT License
Feel free to study, modify, and improve this project responsibly.

AUTHORSHIP AND REDISTRIBUTION
-----------------------------
This project may be used, modified, and sold, including in commercial
products.

However, any redistribution or commercial use MUST clearly credit the
original author and project name.

It is NOT permitted to claim authorship of this project or present it
as an original work created by someone else.

Rebranding, removing author information, or impersonating the original
author is strictly prohibited.



# Legal Disclaimer & Intellectual Property Notice

## 1. Non-Affiliation Disclaimer

**OpenFeatureOS** is an independent, non-commercial, open-source research and educational operating system project developed by individual contributors.

- This project is **not** affiliated with, endorsed by, sponsored by, or in any way associated with **Nokia Corporation**, **HMD Global Oy**, **Unisoc (Shanghai) Technologies Co., Ltd.**, or any of their respective subsidiaries or affiliates.
- The official Nokia website can be found at [nokia.com](https://www.nokia.com).
- The official HMD website can be found at [hmd.com](https://www.hmd.com).
- The official Unisoc website can be found at [unisoc.com](https://www.unisoc.com).

---

## 2. Trademark Notice & Nominative Fair Use

- "Nokia", "Nokia 2660", "Nokia 2660 Flip", and related model designations are registered trademarks of **Nokia Corporation** and/or **HMD Global Oy**.
- "Unisoc", "Spreadtrum", "T107", "UMS9117", and related platform identifiers are trademarks or registered trademarks of **Unisoc (Shanghai) Technologies Co., Ltd.**
- Any use of these trademarks, trade names, or model identifiers within this repository, its documentation, or its source code is strictly for **nominative, descriptive, and identification purposes only**—specifically to indicate the target hardware architecture, peripheral specifications, and interoperability objectives of this operating system.
- Such use falls under the statutory exceptions of **Section 30(2)(d) of the Indian Trade Marks Act, 1999** and the doctrine of **Nominative Fair Use** under applicable international trademark laws.
- This project does not use any official corporate logos, stylized wordmarks, or trade dress of Nokia, HMD Global, or Unisoc.

---

## 3. Reverse Engineering & Interoperability

- Research and reverse engineering conducted in connection with this project are performed in strict accordance with statutory interoperability exceptions, including **Section 52(1)(ab)** and **Section 52(1)(ac) of the Indian Copyright Act, 1957**, as amended by the Copyright (Amendment) Act, 2012.
- The hardware registers, memory maps, bitmasks, timing parameters, and interface definitions documented in this project represent uncopyrightable functional ideas, systems, and methods of operation under copyright law (*Idea-Expression Dichotomy*).
- All device drivers, board support packages, and system software in this repository are **original, clean-room implementations** written from scratch without copying proprietary vendor source code or disassembly listings.

---

## 4. Hardware Safety & Volatile RAM Execution Guarantee

- **Zero Risk of Bricking**: In this phase, OpenFeatureOS executes **strictly from volatile DRAM** at load address `0x80100000`, loaded via USB download mode (BSL/FDL1) with `spd_dump`.
- **No NAND Modification**: On-board SPI NAND flash memory, boot partitions, partition tables, calibration data (NVRAM), and eFuses are **never modified or written to**.
- **Factory Safe**: Disconnecting the USB cable and reseating the battery immediately restores the phone to 100% factory-stock condition.
- OpenFeatureOS does not attempt signature bypass or flash reprogramming in this development phase.

---

## 5. Firmware Redistribution Policy

- **No Proprietary Blobs Included**: This repository does **not** contain, distribute, or host any proprietary vendor firmware binaries, modem/baseband DSP images, BootROM dumps, or commercial assets.
- OpenFeatureOS targets an open, clean-room operating system environment on top of the RT-Thread RTOS core.

---

## 6. Supported Host Platforms & Windows Notice

- **Supported Platforms**: OpenFeatureOS build systems and developer tooling are developed and supported exclusively on **Linux** and **macOS**.
- **No Windows Support**: We do not provide, promise, or guarantee compatibility, support, or bug fixes for native Windows operating systems. Any requests for Windows assistance or issue tickets concerning native Windows toolchains will be **respectfully declined**.
- **User Self-Reliance**: Users on Windows who wish to contribute or build OpenFeatureOS are expected to configure their own execution environment using **WSL2 (Windows Subsystem for Linux)** with Ubuntu/Debian independently without assistance from project maintainers.

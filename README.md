# 6502 CPU Emulator in C++

&#x20;\[]

A **cycle-accurate**, software-based emulator of the classic MOS Technology 6502 microprocessor written in modern C/C++. It faithfully reproduces over 160 instructions and all addressing modes, enabling you to run retro video games and vintage software with authentic timing, memory mapping, stack behavior, and interrupt handling.

---

## 🚀 Table of Contents

* [✨ Features](#✨-features)
* [📐 Architecture & Design](#📐-architecture--design)
* [⚙️ Prerequisites](#⚙️-prerequisites)
* [🔧 Build & Installation](#🔧-build--installation)
* [🎮 Usage](#🎮-usage)
* [🗺️ Memory Map & I/O](#🗺️-memory-map--io)
* [📁 Project Structure](#📁-project-structure)
* [🛠️ Roadmap](#🛠️-roadmap)
* [🤝 Contributing](#🤝-contributing)
* [📄 License](#📄-license)

---

## ✨ Features

* 🔄 **Cycle-Accurate Execution**: Each instruction decrements the cycle counter exactly as on real hardware.
* 📜 **Full Instruction Set**: Implements 160+ opcodes including LDA/STA/ADC/SBC, branching, stack operations, status-flag manipulation, and more.
* 🌐 **All Addressing Modes**: Immediate, Zero Page, Absolute, Indexed, Indirect, and their variants, with proper page-cross cycle penalties.
* 📦 **Memory Abstraction**: 64 KB address space with read/write operators, zero-page optimization, stack page (0x0100–0x01FF) handling.
* ⏸️ **Stack & Interrupts**: Emulates the 6502’s hardware stack, BRK, IRQ, NMI, JSR/RTS, and RTI precisely.
* ⚙️ **Modular CPU Core**: Separate `Mem` and `CPU` structures, making it easy to integrate with video/sound peripherals or host framework.

---

## 📐 Architecture & Design

* **`Mem`**: 64 KB linear memory array with `operator[]` overloads for byte reads/writes.
* **`StatusFlags`**: Bitfield representing the processor status (N, V, B, D, I, Z, C).
* **`CPU`**: Holds registers (A, X, Y, PC, SP), flags, and core methods:

  * **Fetch**: `FetchByte`, `FetchWord`, sign-extended immediate fetch.
  * **Read/Write**: `ReadByte`, `ReadWord`, `WriteByte`, `WriteWord` with cycle accounting.
  * **Stack**: `PushByte`, `PopByte`, `PushWord`, `PopWord` on 0x0100+SP.
  * **Addressing**: Functions for each mode: zero-page, absolute, indexed, indirect, etc., including extra-cycle logic.
  * **Execute Loop**: `Execute(cycles, memory)` reads an opcode and dispatches via a `switch` to lambdas or inlined handlers, updating PC, flags, and cycles.

---

## ⚙️ Prerequisites

* **C++17 compiler**: MSVC (Visual Studio 2019+), GNU g++ 7+, or Clang 6+
* No external dependencies—standard library only.

---

## 🔧 Build & Installation

### Windows (MSVC)

1. Open **Developer Command Prompt for VS**.
2. Navigate to the project root:

   ```bat
   cd C:\path\to\6502-emulator
   ```
3. Compile:

   ```bat
   cl /EHsc /std:c++17 main.cpp
   ```
4. Run:

   ```bat
   main.exe
   ```

### Linux / macOS (g++)

```bash
cd /path/to/6502-emulator
g++ -std=c++17 main.cpp -o 6502emu
./6502emu
```

### CMake (Multi-platform)

```bash
mkdir build && cd build
cmake .. -DCMAKE_CXX_STANDARD=17
cmake --build .
./6502emu
```

---

## 🎮 Usage

By default, `main()` resets CPU & memory then halts. To run a ROM:

```cpp
// In main():
Mem mem;
CPU cpu;
cpu.Reset(mem);

// Load binary (e.g., a .nes PRG ROM) at 0x8000
std::ifstream rom("game.prg", std::ios::binary);
rom.read(reinterpret_cast<char*>(&mem.Data[0x8000]), romSize);

cpu.PC = 0x8000;

// Emulate for N cycles:
cpu.Execute(1'000'000, mem);
```

Integrate this core into your emulator front-end (graphics, APU, input) to play vintage games.

---

## 🗺️ Memory Map & I/O

| Address Range | Description                     |
| ------------- | ------------------------------- |
| 0x0000–0x00FF | Zero Page                       |
| 0x0100–0x01FF | Stack Page (SP)                 |
| 0x0200–0x07FF | Work RAM                        |
| 0x0800–0xFFFF | Cartridge ROM / I/O / Expansion |

*Customize **`Mem`** or add MMU logic to mirror reads/writes into PPU registers or mapper hardware.*

---

## 📁 Project Structure

```text
6502-emulator/
├─ main.cpp         # Reset, ROM loading, driver loop
├─ CPU.h            # CPU struct + instruction dispatch
├─ Mem.h            # Memory array and operators
├─ StatusFlags.h    # Processor status bitfield
├─ Makefile / CMakeLists.txt
└─ README.md        # You are here
```

---

## 🛠️ Roadmap

*

---

## 🤝 Contributing

1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/xyz`).
3. Commit your changes with clear messages.
4. Open a Pull Request—describe your enhancement and test coverage.

Please follow the [Code of Conduct](CODE_OF_CONDUCT.md).

---

## 📄 License

This project is licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

---

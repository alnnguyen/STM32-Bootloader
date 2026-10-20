# STM32 Custom Bootloader

A UART bootloader for the STM32F401RE (NUCLEO-F401RE), written bare metal with CMSIS headers only (no HAL). It is built with GCC and Make and flashed with OpenOCD.

The goal is a bootloader that can receive a new application image over the USB virtual COM port, check it, write it to flash, and jump to it, without needing a debug probe for every update.

## Status

Work in progress. The board currently runs the bare-metal LED blink. UART, the flash driver, the application jump and the update protocol are planned. See the [build guide](docs/build-guide.md) for the step-by-step plan and progress checklist.

## Hardware

- NUCLEO-F401RE (STM32F401RE, Cortex-M4, 512 KB flash, 96 KB RAM)
- USB cable (ST-Link for flashing, virtual COM port for updates)

## Requirements

- `arm-none-eabi-gcc` toolchain
- `make`
- OpenOCD
- Python 3 with `pyserial` (for the host uploader, once it exists)

## Build and flash

```sh
make              # build the bootloader   -> build/bootloader.elf / .bin
make flash        # build and flash the bootloader (0x08000000)
make app          # build the application  -> build/app.elf
make flash-app    # flash the application (0x08010000)
make clean
```

`make app` and `make flash-app` need the application sources, which are not written yet.

## Flash layout

| Sectors | Address      | Size   | Contents                               |
| ------- | ------------ | ------ | -------------------------------------- |
| 0–1     | `0x08000000` | 32 KB  | Bootloader                             |
| 2       | `0x08008000` | 16 KB  | Metadata (is a valid app installed?)   |
| 3       | `0x0800C000` | 16 KB  | Unused                                 |
| 4–7     | `0x08010000` | 448 KB | Application                            |

## Project layout

```
bootloader/   bootloader sources
common/       startup code shared by bootloader and application
linker/       linker scripts
Drivers/      CMSIS core and device headers
docs/         build guide
```

## Documentation

- [Build guide](docs/build-guide.md): the step-by-step plan, from blink to a robust UART update protocol.

## References

- RM0368: STM32F401 Reference Manual
- PM0214: Cortex-M4 Programming Manual
- UM1724: Nucleo-64 User Manual

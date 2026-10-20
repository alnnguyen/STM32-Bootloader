# STM32 Custom Bootloader — build from scratch

Board: NUCLEO-F401RE · Bare metal (CMSIS headers only, no HAL) · GCC + Make + OpenOCD

You build the bootloader one small step at a time. **Every step ends with something you can see on the board.** Finish a step before starting the next one.

The finished version is in `../STM32 Custom Bootloader/`. Use it only to **check your work after you try a step yourself**. Each step names the file to compare against.

---

## Commands

```sh
make            # build bootloader  -> build/bootloader.elf / .bin
make flash      # build + flash bootloader (0x08000000)
make app        # build application -> build/app.elf  (from step 3)
make flash-app  # flash application (0x08010000)       (from step 3)
make clean
```

Serial terminal (from step 1): `screen /dev/cu.usbmodem* 115200`. To quit, press `Ctrl-A` then `K`, then `Y`.

Read a word of memory without stopping the chip:

```sh
openocd -f board/st_nucleo_f4.cfg -c "init; mdw 0x08010000 4; exit"
```

## Reference documents (download from st.com)

- **RM0368**: STM32F401 Reference Manual. It describes every register. You'll use it constantly.
- **PM0214**: Cortex-M4 Programming Manual. It covers the vector table, VTOR and MSP.
- **UM1724**: Nucleo-64 User Manual. It gives the pinout: LED = PA5, button = PC13, VCP UART = PA2/PA3.

## Flash memory map (the plan for the whole project)

| Sectors | Address      | Size   | Owner                                  |
| ------- | ------------ | ------ | -------------------------------------- |
| 0–1     | `0x08000000` | 32 KB  | Bootloader                             |
| 2       | `0x08008000` | 16 KB  | Metadata ("is a valid app installed?") |
| 3       | `0x0800C000` | 16 KB  | Unused                                 |
| 4–7     | `0x08010000` | 448 KB | Application                            |

---

## Step 0 — Blink (already done ✅)

`bootloader/main.c` and `common/startup.c` are your blink project. Run `make flash`, and LD2 should blink.

> Flashing this replaces the old bootloader in sectors 0–1 on the board. That's fine: you can reinstall it anytime with `make flash-bootloader` in the old folder.

**Make sure you can explain:** the vector table, `Reset_Handler`, `.data`/`.bss`, `RCC->AHB1ENR`, `MODER`, `ODR`.

---

## Step 1 — UART transmit: print "hello"

**Goal:** the board prints `hello\r\n` once a second, and you see it in `screen`.

**Concepts:** alternate functions, baud rate, status flags.

**You write:** `common/board.h` and `common/board.c` with `uart_init()` and `uart_write(const uint8_t *data, unsigned n)`.

**Hints:**

1. Enable the clocks: GPIOA is on AHB1, and **USART2 is on APB1** (`RCC->APB1ENR`).
2. Set PA2 (TX) and PA3 (RX) to **alternate function mode**, `MODER = 10`.
3. Pick **AF7** (USART2) for PA2/PA3 in `GPIOA->AFR[0]`. Each pin gets 4 bits, so PA2 is bits 11:8.
4. Baud: `USART2->BRR = 16000000 / 115200` (rounded to 139).
5. Turn on `UE | TE | RE` in `USART2->CR1`.
6. To send a byte, wait until `SR & USART_SR_TXE`, then write `DR`.

**Done when:** `hello` scrolls in the terminal.
**RM0368 chapters:** GPIO (AFR), USART (BRR, SR, DR, CR1). **Compare:** `common/board.c`

---

## Step 2 — UART receive + a millisecond timer

**Goal:** what you type in `screen` is echoed back, while the LED toggles exactly every 1000 ms **with no busy-wait delay**.

**You write:**

- `int uart_read(void)`: returns a byte, or `-1` if nothing has arrived. Check `USART_SR_RXNE`.
- `uint32_t millis(void)` using **TIM5**, a 32-bit timer: prescaler `PSC = 16000 - 1` gives 1 tick per ms. Set `ARR = 0xFFFFFFFF`, write `EGR = UG` so the prescaler loads, then set `CEN`. `millis()` just returns `TIM5->CNT`.
- A main loop that echoes bytes and toggles the LED when `millis() - last >= 1000`.

**Why it matters:** the bootloader needs timeouts ("no byte for 500 ms → give up on this frame") without interrupts.

**Done when:** typing echoes instantly, and the blink stays steady while you type.
**Compare:** `common/board.c` (`uart_read`, `millis`)

---

## Step 3 — Two programs: bootloader jumps to application ⭐ core concept

**Goal:**

- The bootloader starts, turns the LED **on solid for 1 s**, then jumps to a separate application.
- The application blinks **fast** (100 ms).
- Both are flashed with the ST-Link for now.

**You write:**

1. `linker/application.ld`: copy `bootloader.ld`, then change FLASH to `ORIGIN = 0x08010000, LENGTH = 448K`.
2. `application/main.c`: a fast blink.
3. In the app's startup, set `SCB->VTOR` to the address of its own vector table. Otherwise its interrupts would use the bootloader's handlers.
4. In the bootloader, the jump:
   - Read the app's first word, `*(uint32_t *)0x08010000`. That's its **initial stack pointer**.
   - Read the second word at `0x08010004`. That's its **Reset_Handler address**.
   - Disable interrupts (`__disable_irq()`), set `SCB->VTOR = 0x08010000`, load MSP with `__set_MSP(sp)`, then call the reset address as a function pointer.

**Problem to solve:** `make app` compiles `common/startup.c` into both images, so both get the same startup code. That's fine. Ask yourself why the _same_ `startup.c` ends up at two different addresses. (Answer: the linker script decides where it goes.)

**Check it:**

- `arm-none-eabi-objdump -s -j .isr_vector build/app.elf`: the first word should be `0x20018000`, and the second should be an odd address in `0x0801xxxx`.
- Bonus: hold B1 (PC13 reads 0 when pressed) during reset to **stay** in the bootloader with the LED on solid.

**Done when:** reset shows the solid LED for 1 s, then the fast blink.
**Compare:** `bootloader/handoff.c`, `linker/application.ld`
**Read:** PM0214, "Vector table" and "VTOR".

---

## Step 4 — Flash driver: erase and write

**Goal:** the bootloader erases sector 4 and writes `0xDEADBEEF` at `0x08010000`. You confirm it with OpenOCD's `mdw` command.

> ⚠️ Never erase sectors 0–1. That's the code currently running. Add a check that refuses those sector numbers.

**You write:** `bootloader/flash.c`:

- **unlock:** write `0x45670123` then `0xCDEF89AB` to `FLASH->KEYR`.
- **erase(sector):** wait while `FLASH->SR & BSY`. Set `CR = PSIZE_1 | SER | (sector << 3)`, then set `STRT`. Wait for BSY to clear, then check the error flags in `SR`.
- **program(addr, word):** set `CR = PSIZE_1 | PG`, write the word to the address like a normal pointer, wait for BSY, **read it back** and compare.
- **lock:** set `CR = FLASH_CR_LOCK` when you're finished.

**Facts to understand:**

- Erasing sets every bit to **1** (`0xFF` bytes).
- Programming can only change 1 → 0. To rewrite anything, you must erase the whole sector.

**Done when:** `mdw 0x08010000 1` prints `deadbeef`.
**RM0368 chapter:** Embedded Flash memory (FLASH_KEYR, SR, CR, sector table). **Compare:** `bootloader/flash.c`

---

## Step 5 — First real update over UART

**Goal:** upload `build/app.bin` from your Mac over USB serial, with **no ST-Link programming**. Change the blink speed, rebuild, upload again, and see it change.

**You write:**

1. **A simple protocol (v0).** Make up your own; here's a starting point:
   - Host sends: 4-byte image size.
   - Bootloader erases the sectors it needs, then replies `'R'` (ready).
   - Host sends the image in 256-byte chunks. After each chunk, the bootloader programs it and replies `'K'`.
   - Host waits for `'K'` before sending the next chunk. The CPU **stalls while flash is busy**, so bytes sent during that time would be lost.
   - At the end, the bootloader jumps to the app.
2. **`host/upload.py`** using pyserial (`pip3 install pyserial`):
   ```python
   import serial
   s = serial.Serial('/dev/cu.usbmodemXXXX', 115200, timeout=5)
   ```

**Done when:** you can update the app twice in a row, using only the USB serial connection.

---

## Step 6 — Don't boot a broken app (metadata + CRC)

**Problem:** unplug the cable halfway through step 5. The bootloader will jump into half an app and crash.

**Goal:** the bootloader only jumps when it can prove the app is complete. Otherwise it stays in update mode with the LED on solid.

**You write:**

1. A **CRC-32** function (the same algorithm as Python's `zlib.crc32`). Test it: `"123456789"` must give `0xCBF43926`.
2. The host sends the image CRC along with the size.
3. Once the whole image is written, the bootloader recomputes the CRC **from flash**. If it matches, it writes a small record to sector 2 (`0x08008000`): `magic, size, crc`, with the **magic word written last**.
4. At the start of every update, **erase sector 2 first**, before touching the app.
5. At boot: if the magic is wrong, or the CRC of the app in flash doesn't match the record, stay in the bootloader.

**Think about:** why must the magic be written _last_, and the record erased _first_?

**Done when:** unplugging mid-upload leaves the board in update mode (LED on solid), and uploading again fixes it.
**Compare:** `bootloader/engine.c` (`installed_valid`, END), `docs/architecture.md`, "Update commit ordering"

---

## Step 7 — A clean handoff (interrupts in the app)

**Problem:** so far the app hasn't used interrupts, so a wrong VTOR or a leftover bootloader setting wouldn't show up.

**Goal:** the app blinks using a **TIM2 interrupt**. If VTOR or the handoff is wrong, it will crash or hang, so a working blink proves the jump is clean.

**You write:**

- Expand `startup.c` to the full vector table: 16 core + 85 peripheral entries, with TIM2 at IRQ 28. With that many entries the table must be aligned to 512 bytes: `__attribute__((aligned(512)))`.
- Before jumping, the bootloader turns off what it used:
  - stop TIM5 and USART2, and reset them with `RCC->APB1RSTR`
  - disable and clear all NVIC interrupts
  - stop SysTick
- Write the final jump in **assembly**: set MSP, then `bx` to the reset address. Why? After MSP changes, compiled C may still read local variables from the old stack.

**Done when:** the TIM2-driven app blinks after an update over UART.
**Compare:** `bootloader/handoff.c`, `bootloader/trampoline.S`, `application/main.c`

---

## Step 8 — Make it robust (optional, interview-level)

Once steps 0–7 work, harden the protocol in this order:

1. **Packets:** header (command, sequence number, length), payload, CRC per packet. Bad packets are rejected before they reach flash.
2. **Retries:** the host resends if no ACK arrives. The bootloader recognizes a duplicate (same sequence number) and doesn't write it twice.
3. **COBS framing:** a `0x00` byte marks the end of each frame, so the receiver can always find the start of the next one.
4. **Tests on your Mac:** compile the protocol logic (no registers) with plain `cc` and drive it from Python. The storage is replaced by a fake in RAM.

**Compare:** `common/codec.c`, `bootloader/engine.c`, `host/uploader.py`, `tests/`, `docs/protocol.md`

---

## Progress

- [x] 0 Blink
- [ ] 1 UART TX
- [ ] 2 UART RX + millis
- [ ] 3 Bootloader → app jump
- [ ] 4 Flash erase/write
- [ ] 5 Update over UART
- [ ] 6 Metadata + CRC validation
- [ ] 7 Clean handoff with interrupts
- [ ] 8 Robust protocol + tests

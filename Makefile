CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
BUILD   = build
CMSIS   = Drivers/CMSIS

CFLAGS  = -mcpu=cortex-m4 -mthumb -std=c11 -Og -g3 -Wall -Wextra \
          -ffreestanding -fno-tree-loop-distribute-patterns \
          -ffunction-sections -fdata-sections \
          -DSTM32F401xE -Icommon \
          -I$(CMSIS)/Include -I$(CMSIS)/Device/ST/STM32F4xx/Include
LDFLAGS = -nostdlib -Wl,--gc-sections

# Every .c / .S file in these folders is compiled automatically.
COMMON_SRC = $(wildcard common/*.c common/*.S)
BOOT_SRC   = $(wildcard bootloader/*.c bootloader/*.S) $(COMMON_SRC)
APP_SRC    = $(wildcard application/*.c application/*.S) $(COMMON_SRC)
HEADERS    = $(wildcard common/*.h bootloader/*.h application/*.h)

OPENOCD = openocd -f board/st_nucleo_f4.cfg

all: $(BUILD)/bootloader.bin

# Step 3 onward: the application image, linked at 0x08010000.
app: $(BUILD)/app.bin

$(BUILD):
	mkdir -p $@

$(BUILD)/bootloader.elf: $(BOOT_SRC) $(HEADERS) linker/bootloader.ld | $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) -T linker/bootloader.ld -Wl,-Map=$(BUILD)/bootloader.map $(BOOT_SRC) -lgcc -o $@
	$(SIZE) $@

$(BUILD)/app.elf: $(APP_SRC) $(HEADERS) linker/application.ld | $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) -T linker/application.ld -Wl,-Map=$(BUILD)/app.map $(APP_SRC) -lgcc -o $@
	$(SIZE) $@

$(BUILD)/%.bin: $(BUILD)/%.elf
	$(OBJCOPY) -O binary $< $@

# ELF files carry their own load address, so OpenOCD writes each to the right place.
flash: $(BUILD)/bootloader.elf
	$(OPENOCD) -c "program $< verify reset exit"

flash-app: $(BUILD)/app.elf
	$(OPENOCD) -c "program $< verify reset exit"

clean:
	rm -rf $(BUILD)

.PHONY: all app flash flash-app clean

TARGET = firmware

CC      = arm-none-eabi-gcc
CXX     = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

INCLUDES = -I../shared -Isrc

CFLAGS = -mcpu=cortex-m4 \
         -mthumb \
         -mfloat-abi=hard \
         -mfpu=fpv4-sp-d16 \
         -Wall \
         -Wextra \
         -O0 \
         -g \
         -ffreestanding \
         -fdata-sections \
         -ffunction-sections \
         -nostdlib \
         $(INCLUDES)

CXXFLAGS = $(CFLAGS) \
           -std=c++17 \
           -fno-exceptions \
           -fno-rtti

LDFLAGS = -T linker.ld \
          -Wl,-Map=$(TARGET).map \
          -Wl,--gc-sections \
          -nostdlib

C_SRCS   = $(wildcard src/*.c)
CXX_SRCS = $(wildcard src/*.cpp)
OBJS     = $(C_SRCS:.c=.o) $(CXX_SRCS:.cpp=.o)

all: $(TARGET).elf $(TARGET).bin

$(TARGET).elf: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).map

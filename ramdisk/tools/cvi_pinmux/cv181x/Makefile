ifeq ($(SDK_VER),musl_riscv64)
	CC = $(TOP_DIR)/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-gcc
	CFLAGS += -mcpu=c906fdv -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d -static
else ifeq ($(SDK_VER),64bit)
	CC = $(TOP_DIR)/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
else ifeq ($(SDK_VER),32bit)
	CC = $(TOP_DIR)/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
else
$(error Unknown SDK_VER)
endif
source  = cvi_pinmux.c devmem.c
object  = $(source:%.cc=%.o)



all: $(object)
	$(CC) $(CFLAGS) $(object) -o cvi_pinmux

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) *.o cvi_pinmux

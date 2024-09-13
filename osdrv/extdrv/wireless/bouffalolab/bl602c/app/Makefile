
# CC=cc -mfloat-abi=hard
CC=riscv64-unknown-linux-musl-gcc -mcpu=c906fdv -march=rv64imafdcv0p7xthead

CFLAGS := -DBUILD_USERSPACE -g

.PHONY: all clean
all: blutl
clean:
	rm -rf blutl

blutl:
	$(CC) -o $@ $(CFLAGS)  bl_util.c

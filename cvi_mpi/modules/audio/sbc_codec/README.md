##Code download address: https://github.com/pschatzmann/arduino-libsbc

1. Unzip parduino-libsbc-main.tar.gz
    tar -xzvf arduino-libsbc-main.tar.gz
2. Compile and Build libsbc.a libsbc.so [arm or riscv]
    cd arduino-libsbc-main
    cmake .
    make
3. Replace the libsbc.so in $(MW_3RD_LIB)
4. sample use the sbc command:
    ./sample_audio 1 --list -r 16000 -R 16000 -c 2 -p 128 -C 5 -V 0 -F Cvi_16k_2chn.sbc -T 10
    ./sample_audio 3 --list -r 16000 -R 16000 -c 2 -p 152 -C 5 -V 0 -F Cvi_16k_2chn.sbc -T 10


cmd_drivers/rtc/built-in.o :=  /opt/gm8136/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-ld -EL    -r -o drivers/rtc/built-in.o drivers/rtc/rtc-lib.o ; scripts/mod/modpost drivers/rtc/built-in.o

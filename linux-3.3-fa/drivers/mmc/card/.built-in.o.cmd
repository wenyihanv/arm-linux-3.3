cmd_drivers/mmc/card/built-in.o :=  /opt/gm8136/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-ld -EL    -r -o drivers/mmc/card/built-in.o drivers/mmc/card/mmc_block.o ; scripts/mod/modpost drivers/mmc/card/built-in.o

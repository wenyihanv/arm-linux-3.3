cmd_drivers/net/ethernet/faraday/built-in.o :=  /opt/gm8136/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-ld -EL    -r -o drivers/net/ethernet/faraday/built-in.o drivers/net/ethernet/faraday/gmac100.o ; scripts/mod/modpost drivers/net/ethernet/faraday/built-in.o
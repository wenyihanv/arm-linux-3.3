cmd_net/sched/built-in.o :=  /opt/gm8136/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-ld -EL    -r -o net/sched/built-in.o net/sched/sch_generic.o net/sched/sch_mq.o ; scripts/mod/modpost net/sched/built-in.o

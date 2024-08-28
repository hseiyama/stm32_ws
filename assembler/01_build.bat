arm-none-eabi-as -g 01_sample01_base.S -o 01_sample01_base.o
arm-none-eabi-ld 01_sample01_base.o -o 01_sample01_base.elf -T 01_stm32l4a6zg.ld
arm-none-eabi-objdump -h -S 01_sample01_base.elf  > "01_sample01_base.list"
arm-none-eabi-objcopy  -O srec 01_sample01_base.elf  "01_sample01_base.srec"

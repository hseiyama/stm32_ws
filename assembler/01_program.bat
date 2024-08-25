openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "program 01_sample01_base.elf verify reset exit"

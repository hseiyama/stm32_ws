PREFIX=arm-none-eabi-
#S_SOURCES = vectortable.S startup.S program.S
#S_OBJECTS = vectortable.o startup.o program.o
S_SOURCES = vectortable.S program.S
S_OBJECTS = vectortable.o program.o
LDSCRIPT = stm32l4a6zg.ld

program.elf : $(S_OBJECTS) $(LDSCRIPT)
	$(PREFIX)ld -o program.elf $(S_OBJECTS) -T $(LDSCRIPT)
	$(PREFIX)objdump -h -S program.elf > program.list
	$(PREFIX)objcopy -O ihex program.elf program.hex
	$(PREFIX)objcopy -O srec program.elf program.srec
	$(PREFIX)objdump -d -s program.elf > program.dump
	$(PREFIX)nm program.elf > program.symbol
	$(PREFIX)readelf -S program.elf

#$(S_OBJECTS) : $(S_SOURCES) stm32l4a6.inc
$(S_OBJECTS) : $(S_SOURCES)
	$(PREFIX)as -g -o vectortable.o vectortable.S
#	$(PREFIX)as -g -o startup.o startup.S
	$(PREFIX)as -g -o program.o program.S

.PHONY : gdbsrv program gdb clean

gdbsrv :
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg

program : program.elf
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "program program.elf verify reset exit"

gdb : program.elf
	$(PREFIX)gdb program.elf

clean :
	del program.elf $(S_OBJECTS) program.list program.hex program.srec program.dump program.symbol

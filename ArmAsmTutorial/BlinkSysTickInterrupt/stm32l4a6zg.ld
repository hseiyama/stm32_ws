_StackEnd = ORIGIN(SRAM) + LENGTH(SRAM);

MEMORY {
	FLASH	: ORIGIN = 0x08000000,	LENGTH = 1M
	SRAM	: ORIGIN = 0x20000000,	LENGTH = 320K
}

SECTIONS {
	.VectorTable : {
		*(.VectorTable)
	} >FLASH

	.text : {
		*(.text)
		. = ALIGN(4);
	} >FLASH

	.data : {
		_DataStart = .;
		*(.data);
		. = ALIGN(4);
		_DataEnd = .;
	} >SRAM AT >FLASH

	_DataLoad = LOADADDR(.data);

	.bss (NOLOAD) : {
		_BssStart = .;
		*(.bss);
		. = ALIGN(4);
		_BssEnd = .;
	} >SRAM
}

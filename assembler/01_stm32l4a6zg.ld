_StackEnd = ORIGIN(SRAM) + LENGTH(SRAM);

MEMORY {
	FLASH	: ORIGIN = 0x08000000,	LENGTH = 1M
	SRAM	: ORIGIN = 0x20000000,	LENGTH = 320K
}

SECTIONS {
	.text : {
		*(.VectorTable)
		*(.text)
	} >FLASH
	
	.data (NOLOAD) : {
		*(.data)
	} >SRAM
}

/*****************************************************************************/
/*                                                                           */
/* Sistemas Empotrados                                                       */
/*                                                                           */
/* Linker script para la Redwire EconoTAG                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Punto de entrada                                                          */
/*****************************************************************************/

ENTRY(_start)

/*
 * Mapa de memoria de la placa
 */
MEMORY{	/* Space between name and ':' is needed, error if not there */
	ram0 :	org = 0x00400000,		len = 0x00002000		/* (8 KB) */
	ram1 :	org = 0x00402000,		len = 0x00006000		/* (24 KB) */
	ram2 :	org = 0x00408000,		len = 0x00008000		/* (32 KB) */
	ram3 :	org = 0x00410000,		len = 0x00008000		/* (32 KB) */
	gpio :  org = 0x80000000,       len = 0x00000068
}

SECTIONS{
	/* Código del programa */
	.text : {
		*(.text);
	} > ram0

	.data : {
		*(.data);
	} > ram0

	/*
		.ram1 : {
			s1 = .;
			*(.ram1);
		} > ram1

		.ram2 : {
			s2 = .;
			*(.ram2);
		} > ram2

		.ram3 : {
			s3 = .;
			*(.ram3);
		} > ram3
	*/
	.gpio : {
		gpio_base = . ;
		gpio_pad_dir0 = gpio_base + 0x00;
		gpio_pad_dir1 = gpio_base + 0x04;
		gpio_data0 = gpio_base + 0x08;
		gpio_data_set0 = gpio_base + 0x48;
		gpio_data_set1 = gpio_base + 0x4c;
		gpio_data_reset1 = gpio_base + 0x54;
	} > gpio
}

/*
 * Sistemas Empotrados
 * Linker script para la Redwire EconoTAG
 * Runtime de C para ser cargado por la BIOS de la placa
 */

/*
 * Punto de entrada
 */
ENTRY(_vector_table)

/*
 * Mapa de memoria de la placa
 */
MEMORY{
	ram : org = 0x00400000, len = 0x00018000		/*  96 KB */
}

SECTIONS{
	/* Imagen del firmware */
	/* Generar una sección al principio de la RAM que organice las secciones del firmware al comienzo de la RAM de la plataforma */
	boot : {
		_boot_start = .;
		obj/hal/crt0.o(.text);
		. = ALIGN(4);
		_boot_end = .;
	} > ram

	.text : {
		_text_start = .;
		*(.text);
		_text_end = .;
	} > ram

	.data : {
		_data_start = .;
		*(.data);
		. = ALIGN(4);
		_data_end = .;
	} > ram

	.rodata : {
		_rodata_start = .;
		*(.rodata);
		. = ALIGN(4);
		_rodata_end = .;
	} > ram

	/* Sección .bss */
	/* Generamos una sección para las variables globales sin inicializar */
	.bss : {
		_bss_start = .;
		*(.bss);
		. = ALIGN(4);
		*(COMMON);
		. = ALIGN(4);
		_bss_end = . ;
	} > ram

	/* Gestión del heap */
	/* Generar una sección que ocupe el espacio entre la sección .bss y las pilas para el heap, con los símbolos de inicio y fin del heap */
	.heap : {
		_heap_start = .;
		*(.heap);
		_heap_end = .;
	} > ram

	/* Gestión de las pilas */
	/* Generar una sección al final de la RAM para las pilas de cada modo y definir símbolos para el tope de cada pila */
	.stacks : {
		_sys_stack_top = .;
		_svc_stack_top = .;
		_abt_stack_top = .;
		_und_stack_top = .;
		_irq_stack_top = .;
		_fiq_stack_top = .;
	} > ram
}


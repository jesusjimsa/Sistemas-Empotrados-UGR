@
@ Sistemas Empotrados
@ El "hola mundo" en la Redwire EconoTAG
@

@
@ Constantes
@

	@ Registro de control de dirección del GPIO00-GPIO31
	.set GPIO_PAD_DIR0,		0x80000000

	@ Registro de control de dirección del GPIO32-GPIO63
	.set GPIO_PAD_DIR1,		0x80000004

	@ Registro para consultar si se pulsa un botón del GPIO00-GPIO31
	.set GPIO_DATA0,		0x80000008

	@ Registro de activación de bits del GPIO00-GPIO31
	.set GPIO_DATA_SET0,	0x80000048

	@ Registro de activación de bits del GPIO32-GPIO63
	.set GPIO_DATA_SET1,	0x8000004c

	@ Registro de limpieza de bits del GPIO32-GPIO63
	.set GPIO_DATA_RESET1,	0x80000054

	@ El led rojo está en el GPIO 44 (el bit 12 de los registros GPIO_X_1)
	.set LED_RED_MASK,		(1 << (44-32))

	@ El led verde está en el GPIO 45 (el bit 13 de los registros GPIO_X_1)
	.set LED_GREEN_MASK,	(1 << (45-32))

	@ Switches
	.set SW2_INPUT_MASK,	(1 << 27)
	.set SW3_INPUT_MASK,	(1 << 26)
	.set SW2_OUTPUT_MASK,	(1 << 23)
	.set SW3_OUTPUT_MASK,	(1 << 22)

	@ Retardo para el parpadeo
	.set DELAY,				0x00080000

@
@ Punto de entrada
@

	.code	32
	.text
	.global	_start
	.type	_start, %function

_start:
	bl	gpio_init

gpio_init:
	@ Configuramos el GPIO44 para que sea de salida
	ldr		r4, =GPIO_PAD_DIR1
	ldr		r5, =(LED_RED_MASK|LED_GREEN_MASK)
	str		r5, [r4]

	@ Set SW pins to HIGH
	ldr		r5, =(SW2_OUTPUT_MASK|SW3_OUTPUT_MASK)
	ldr		r8, =GPIO_DATA_SET0
	str		r5, [r8]
	
	@ Direcciones de los registros GPIO_DATA_SET1 y GPIO_DATA_RESET1
	ldr		r6, =GPIO_DATA_SET1
	ldr		r7, =GPIO_DATA_RESET1

	@ Estado inicial de los LED
	ldr		r5, =(LED_RED_MASK|LED_GREEN_MASK)
	str		r5, [r7]
	ldr		r5, =LED_RED_MASK
	b		loop
	
	bl		loop

loop:
	@ Comprobamos los botones
	bl		test_buttons

	@ Encendemos el led rojo
	str		r5, [r6]

	@ Pausa corta
	ldr		r0, =DELAY
	bl		pause

	@ Apagamos el led rojo
	str		r5, [r7]

	@ Pausa corta
	ldr		r0, =DELAY
	bl		pause

	@ Comprobamos los botones
	bl		test_buttons

	@ Bucle infinito
	b		loop
	
@ Función que produce un retardo
@ r0: iteraciones del retardo
	.type	pause, %function

pause:
	subs	r0, r0, #1
	bne		pause
	mov		pc, lr

test_buttons:
	mov		r9, lr
	ldr		r4, =GPIO_DATA0
	ldr		r4, [r4]	@ [] significa dirección de memoria
	ldr		r8, =SW3_INPUT_MASK
	tst		r8, r4
	blne	green_led
	ldr		r4, =GPIO_DATA0
	ldr		r4, [r4]
	ldr		r8, =SW2_INPUT_MASK
	tst		r8, r4
	blne	red_led
	mov		pc, r9

green_led:
	ldr		r5, =LED_GREEN_MASK
	mov		pc, lr

red_led:
	ldr		r5, =LED_RED_MASK
	mov		pc, lr

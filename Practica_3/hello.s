/*
	Sistemas Empotrados
	El "hola mundo" en la Redwire EconoTAG
*/

/*
	Variables globales
*/
.data
	@ El led rojo está en el GPIO 44 (el bit 12 de los registros GPIO_X_1)
	led_red_mask: .word	(1 << (44-32))

	@ El led verde está en el GPIO 45 (el bit 13 de los registros GPIO_X_1)
	led_green_mask: .word	(1 << (45-32))

	@ Switches
	sw2_input_mask: .word	(1 << 27)
	sw3_input_mask: .word	(1 << 26)
	sw2_output_mask: .word	(1 << 23)
	sw3_output_mask: .word	(1 << 22)

	@ Retardo para el parpadeo
	delay:	.word	0x00080000

	@ Variable que guarda los leds que deben parpadear
	leds:	.word
/*
	Punto de entrada
*/

	.code	32
	.text
	.global	_start
	.type	_start, %function

_start:
	bl		gpio_init

	@ Estado inicial de los LED
	@ Por defecto parpadeará el rojo
	ldr		r0, =led_red_mask
	ldr		r1, [r0]
	ldr		r0, =leds
	str		r1, [r0]

loop:
	@ Comprobamos los botones
	bl		test_buttons

	@ Encendemos los leds
	ldr		r0, =leds
	ldr		r1, [r0]
	ldr		r0, =gpio_data_set1
	str		r1, [r0]

	@ Pausa corta
	ldr		r0, =delay
	ldr		r0, [r0]
	bl		pause

	@ Apagamos los leds
	ldr		r0, =leds
	ldr		r1, [r0]
	ldr		r0, =gpio_data_reset1
	str		r1, [r0]

	@ Comprobamos los botones
	bl		test_buttons

	@ Pausa corta
	ldr		r0, =delay
	ldr		r0, [r0]
	bl		pause

	@ Bucle infinito
	b		loop

/*
	Función que produce un retardo
	r0: iteraciones del retardo
*/
	.type	pause, %function

pause:
	subs	r0, r0, #1
	bne		pause
	mov		pc, lr

test_buttons:
	@ Guardar los leds en los registros r4 y r5 para más tarde
	ldr		r4, =led_green_mask
	ldr     r5, =led_red_mask

	@ Comprobar botón del LED verde
	ldr		r0, =gpio_data0
	ldr		r1, [r0]
	ldr		r0, =sw3_input_mask
	ldr		r2, [r0]
	tst		r1, r2
	ldrne	r1, [r4]

	@ Comprobar botón del LED rojo
	ldr		r0, =gpio_data0
	ldr		r1, [r0]
	ldr		r0, =sw2_input_mask
	ldr		r2, [r0]
	tst		r1, r2
	ldrne   r1, [r5]

	@ Volvemos al bucle que enciende y apaga los LED
	mov		pc, lr

gpio_init:
	@ Configuramos el GPIO44 y el GPIO45 para que sean de salida
	@ Las 5 instrucciones siguientes equivalen a: ldr	r5, =(led_red_mask|led_green_mask)
	ldr		r0, =led_red_mask
	ldr		r1, [r0]
	ldr		r0, =led_green_mask
	ldr		r2, [r0]
	orr		r2, r1, r2	@ r2 = r1 | r2
	ldr		r0, =gpio_pad_dir1
	str		r2, [r0]

	@ Set SW pins to HIGH
	@ Las 5 instrucciones siguientes equivalen a: ldr	r5, =(sw2_output_mask|sw3_output_mask)
	ldr		r0, =sw2_output_mask
	ldr		r1, [r0]
	ldr		r0, =sw3_output_mask
	ldr		r2, [r0]
	orr		r2, r1, r2
	ldr		r0, =gpio_pad_dir0
	str		r2, [r0]

	@ Fijamos a 1 los pines de salida de los botones
	ldr		r0, =gpio_data_set0
	str		r2, [r0]

	mov		pc, lr	@ Retornamos

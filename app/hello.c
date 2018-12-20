/*****************************************************************************/
/*                                                                           */
/* Sistemas Empotrados                                                       */
/* El "hola mundo" en la Redwire EconoTAG en C                               */
/*                                                                           */
/*****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "system.h"

/*
 * Constantes relativas a la plataforma
 */

/* El led rojo está en el GPIO 44 (el bit 12 de los registros GPIO_X_1) */
#define RED_LED gpio_pin_44

/* El led verde está en el GPIO 45 */
#define GREEN_LED gpio_pin_45

uint32_t red_led, green_led;	// Máscara del led que se hará parpadear
uint32_t veces_mensaje = 0;

/*
 * Constantes relativas a la aplicacion
 */
uint32_t const delay = 0x10000;

/*****************************************************************************/

/*
 * Inicialización de los pines de E/S
 */
void gpio_init(void){
	/* Configuramos los GPIO44 y GPIO45 para que sean de salida */
	gpio_set_pin_dir_output (RED_LED);
	gpio_set_pin_dir_output (GREEN_LED);
}

/*****************************************************************************/

/*
 * Enciende el led indicado en el pin
 * @param pin Máscara para seleccionar led
 */
void leds_on(uint32_t pin){
	gpio_set_pin(pin);
}

/*****************************************************************************/

/*
 * Apaga el led indicado en el pin
 * @param pin Máscara para seleccionar led
 */
void leds_off (uint32_t pin){
	gpio_clear_pin(pin);
}

/*****************************************************************************/

/*
 * Retardo para el parpadeo
 */
void pause(void){
	uint32_t i;
	for (i = 0; i < delay; i++);
}

/*****************************************************************************/

/**
 * Imprime una cadena de caracteres por la UART1
 * @param str La cadena
 */
void print_str(char * str){
        uart_send(UART1_ID, str, strlen(str));
}

/*****************************************************************************/

void my_callback(){
	uint32_t len;
	uint32_t i;
	char c;
	char buf[100]; /* Búfer para recibir los datos */

	/* Leemos los datos recibidos por la uart */
	len = uart_receive(uart_1, buf, 100);

	for(i = 0; i < len; i++){
		c = buf[i];

		if (c == 'r' || c == 'R'){

			if (red_led){
				print_str("Desactivando el led rojo\r\n");
			}
			else{
				print_str("Activando el led rojo\r\n");
			}

			red_led = !red_led;
		}
		else{
			if (c == 'g' || c == 'G'){
				if (green_led){
					print_str("Desactivando el led verde\r\n");
				}
				else{
					print_str("Activando el led verde\r\n");
				}

				green_led = !green_led;
			}
			else{
				print_str("Pulsa 'g' o 'r'\r\n");
			}
		}
	}
}

/*****************************************************************************/

/*
 * Programa principal
 */
int main (){
	gpio_init();

	red_led = 1;
	green_led = 1;

	leds_on(RED_LED);
	leds_on(GREEN_LED);

	uart_set_receive_callback(uart_1, my_callback);

	iprintf("Hola mundo!\n");

	while (1){
		if (red_led){
			leds_on(RED_LED);
		}

		if (green_led){
			leds_on(GREEN_LED);
		}

		pause();

		leds_off(RED_LED);
		leds_off(GREEN_LED);

		pause();
	}

	return 0;
}

/*****************************************************************************/


/*****************************************************************************/
/*                                                                           */
/* Sistemas Empotrados                                                       */
/* El "hola mundo" en la Redwire EconoTAG en C                               */
/*                                                                           */
/*****************************************************************************/

#include <stdint.h>
#include "system.h"

/*
 * Constantes relativas a la plataforma
 */

/* El led rojo está en el GPIO 44 (el bit 12 de los registros GPIO_X_1) */
#define RED_LED gpio_pin_44

/* El led verde está en el GPIO 45 */
#define GREEN_LED gpio_pin_45

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
 * Programa principal
 */
int main (){
	uint32_t red_led, green_led;	// Máscara del led que se hará parpadear

	gpio_init();

	//Mensaje de error
	char msg0[38] = "Solo se pueden usar las teclas g y r\r\n";
    char msg1[34] = "De verdad, solo las teclas g y r\r\n";
	char msg2[69] = "Lo hemos hablado muchas veces, solo se pueden usar las teclas g y r\r\n";
	char msg3[10] = "g y r...\r\n";
	char msg30[28] = "Me rindo, ahi te quedas...\r\n";

	uint32_t veces_mensaje = 0;

	red_led = 0;
	green_led = 0;

	while (1){
		char c;

		c = uart_receive_byte(uart_1);

		if(c == 'r'){
			red_led = !red_led;
			veces_mensaje = 0;	// El programa está contento y te perdona (por ahora)

			if(red_led == 0){
				leds_on(RED_LED);
			}
			else{
				leds_off(RED_LED);
			}
		}
		else{
			if(c == 'g'){
				green_led = !green_led;
				veces_mensaje = 0;

				if(green_led == 0){
					leds_on(GREEN_LED);
				}
				else{
					leds_off(GREEN_LED);
				}
			}
			else{
				uint32_t i;
				
				switch(veces_mensaje){
					case 0:
						for(i = 0; i < 38; i++){
							uart_send_byte(uart_1, msg0[i]);
						}

						veces_mensaje++;

						break;
					case 1:
						for(i = 0; i < 34; i++){
							uart_send_byte(uart_1, msg1[i]);
						}

						veces_mensaje++;

						break;
					case 2:
						for(i = 0; i < 69; i++){
							uart_send_byte(uart_1, msg2[i]);
						}

						veces_mensaje++;

						break;
					case 30:
						for(i = 0; i < 28; i++){
							uart_send_byte(uart_1, msg30[i]);
						}

						return 0;	// ¿Cómo te atreves? Has enfadado al programa y se ha ido :(

						break;
					default:
						for(i = 0; i < 10; i++){
							uart_send_byte(uart_1, msg3[i]);
						}

						veces_mensaje++;

						break;
				}
			}
		}
	}

	return 0;
}

/*****************************************************************************/


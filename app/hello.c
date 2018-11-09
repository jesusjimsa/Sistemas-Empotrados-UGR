/*****************************************************************************/
/*                                                                           */
/* Sistemas Empotrados                                                       */
/* El "hola mundo" en la Redwire EconoTAG en C                               */
/*                                                                           */
/*****************************************************************************/

#include <stdint.h>

/*
 * Constantes relativas a la plataforma
 */

/* Dirección del registro de control de dirección del GPIO32-GPIO63 */
volatile uint32_t * const reg_gpio_pad_dir1    = (uint32_t *) 0x80000004;

/* Dirección del registro de activación de bits del GPIO32-GPIO63 */
volatile uint32_t * const reg_gpio_data_set1   = (uint32_t *) 0x8000004c;

/* Dirección del registro de control de botones del GPIO00-GPIO31 */
volatile uint32_t * const reg_gpio_data0 = (uint32_t *) 0x80000008;

/* Dirección del registro de limpieza de bits del GPIO32-GPIO63 */
volatile uint32_t * const reg_gpio_data_reset1 = (uint32_t *) 0x80000054;

/* El led rojo está en el GPIO 44 (el bit 12 de los registros GPIO_X_1) */
uint32_t const led_red_mask = (1 << (44-32));

/* El led verde está en el GPIO 45 */
uint32_t const led_green_mask = (1 << (45-32));

/* Switches */
uint32_t const sw2_input_mask = (1 << 27);
uint32_t const sw3_input_mask = (1 << 26);
uint32_t const sw2_output_mask = (1 << 23);
uint32_t const sw3_output_mask = (1 << 22);

/*
 * Constantes relativas a la aplicacion
 */
uint32_t const delay = 0x10000;

/*
 * Máscara del led que se hará parpadear
 */
uint32_t the_led;
 
/*****************************************************************************/

/*
 * Inicialización de los pines de E/S
 */
void gpio_init(void){
	/* Configuramos los GPIO44 y GPIO45 para que sean de salida */
	*reg_gpio_pad_dir1 = (led_red_mask | led_green_mask);
	/* Set SW pins to high */
	*reg_gpio_data_set1 = (sw2_output_mask | sw3_output_mask);
}

/*****************************************************************************/

/*
 * Enciende los leds indicados en la máscara
 * @param mask Máscara para seleccionar leds
 */
void leds_on (uint32_t mask){
	/* Encendemos los leds indicados en la máscara */
	*reg_gpio_data_set1 = mask;
}

/*****************************************************************************/

/*
 * Apaga los leds indicados en la máscara
 * @param mask Máscara para seleccionar leds
 */
void leds_off (uint32_t mask){
	/* Apagamos los leds indicados en la máscara */
	*reg_gpio_data_reset1 = mask;
}

/*****************************************************************************/

void test_buttons(void){
	if(*reg_gpio_data0 == sw3_input_mask){
		the_led = led_green_mask;
	}
	else{
		if(*reg_gpio_data0 == sw2_input_mask){
			the_led = led_red_mask;
		}
	}
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

/*
 * Programa principal
 */
int main (){
	gpio_init();

    the_led = led_red_mask;

	while (1){
		test_buttons();
		leds_on(the_led);
        pause();

		leds_off(the_led);
        pause();
	}

    return 0;
}

/*****************************************************************************/


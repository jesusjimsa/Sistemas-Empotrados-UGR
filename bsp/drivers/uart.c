/*
 * Sistemas operativos empotrados
 * Driver de las uart
 */

#include <fcntl.h>
#include <errno.h>
#include "system.h"
#include "circular_buffer.h"

/*****************************************************************************/

/**
 * Acceso estructurado a los registros de control de las uart del MC1322x
 */

typedef struct{
	// UART Control Register
	union{
		struct{
			uint32_t TxE		: 1;
			uint32_t RxE		: 1;
			uint32_t PEN		: 1;
			uint32_t EP			: 1;
			uint32_t ST2		: 1;
			uint32_t SB			: 1;
			uint32_t conTx		: 1;
			uint32_t Tx_oen_b	: 1;
			uint32_t			: 2;
			uint32_t xTIM		: 1;
			uint32_t FCp		: 1;
			uint32_t FCe		: 1;
			uint32_t mTxR		: 1;
			uint32_t mRxR		: 1;
			uint32_t TST		: 1;
		};

		uint32_t CON;
	};

	// UART Status Register
	union{
		struct{
			uint32_t SE			: 1;
			uint32_t PE			: 1;
			uint32_t FE			: 1;
			uint32_t TOE		: 1;
			uint32_t ROE		: 1;
			uint32_t RUE		: 1;
			uint32_t RxRdy		: 1;
			uint32_t TxRdy		: 1;
		};

		uint32_t STAT;
	};

	// UART Data Register
	union{
		uint8_t Rx_data;
		uint8_t Tx_data;
		uint32_t DATA;
	};

	// UART RxBuffer Control Register
	union{
		uint32_t RxLevel			: 5;
		uint32_t Rx_fifo_addr_diff 	: 6;
		uint32_t RxCON;
	};

	// UART TxBuffer Control Register
	union{
		uint32_t TxLevel			: 5;
		uint32_t Tx_fifo_addr_diff	: 6;
		uint32_t TxCON;
	};

	// UART CTS Level Control Register
	uint32_t CTS;

	// UART Baud Rate Divider Register
	union{
		struct{
			uint32_t BRMOD	: 16;
			uint32_t BRINC	: 16;
		};

		uint32_t BR;
	};
} uart_regs_t;

/*****************************************************************************/

/**
 * Acceso estructurado a los pines de las uart del MC1322x
 */
typedef struct{
	gpio_pin_t tx,rx,cts,rts;
} uart_pins_t;

/*****************************************************************************/

/**
 * Definición de las UARTS
 */
static volatile uart_regs_t* const uart_regs[uart_max] = {
	UART1_BASE,
	UART2_BASE
};

static const uart_pins_t uart_pins[uart_max] = {
	{
		gpio_pin_14, gpio_pin_15, gpio_pin_16, gpio_pin_17
	},
	{
		gpio_pin_18, gpio_pin_19, gpio_pin_20, gpio_pin_21
	}
};

static void uart_1_isr(void);
static void uart_2_isr(void);

static const itc_handler_t uart_irq_handlers[uart_max] = {
	uart_1_isr,
	uart_2_isr
};

/*****************************************************************************/

/**
 * Tamaño de los búferes circulares
 */
#define __UART_BUFFER_SIZE__	256

static volatile uint8_t uart_rx_buffers[uart_max][__UART_BUFFER_SIZE__];
static volatile uint8_t uart_tx_buffers[uart_max][__UART_BUFFER_SIZE__];

static volatile circular_buffer_t uart_circular_rx_buffers[uart_max];
static volatile circular_buffer_t uart_circular_tx_buffers[uart_max];


/*****************************************************************************/

/**
 * Gestión de las callbacks
 */
typedef struct{
	uart_callback_t tx_callback;
	uart_callback_t rx_callback;
} uart_callbacks_t;

static volatile uart_callbacks_t uart_callbacks[uart_max];

/*****************************************************************************/

/**
 * Inicializa una uart
 * @param uart	Identificador de la uart
 * @param br	Baudrate
 * @param name	Nombre del dispositivo
 * @return		Cero en caso de éxito o -1 en caso de error.
 * 				La condición de error se indica en la variable global errno
 */
int32_t uart_init(uart_id_t uart, uint32_t br, const char *name){
	/* Comprobación de errores */
	if(uart >= uart_max){
		errno = ENODEV;	/* El dispositivo no existe */

		return -1;
	}

	if(!name){
		errno = EFAULT;

		return -1;
	}

	uint32_t mod = 9999;
	uint32_t inc = br * mod / (CPU_FREQ >> 4);

	/* Fijamos los parámetros por defecto y deshabilitamos la uart */
	/* La uart debe estar deshabilitada para fijar la frecuencia */
	uart_regs[uart]->CON = (1 << 13) | (1 << 14);

	/* Fijamos la frecuencia, asumimos un oversampling de 8x */
	uart_regs[uart]->BR = ( inc << 16 ) | mod;

	/* Habilitamos la uart. En el MC1322x hay que habilitar el */
	/* periférico antes fijar el modo de funcionamiento de sus pines */
	uart_regs[uart]->CON |= (1 << 0) | (1 << 1);

	/* Cambiamos el modo de funcionamiento de los pines */
	gpio_set_pin_func(uart_pins[uart].tx, gpio_func_alternate_1);
	gpio_set_pin_func(uart_pins[uart].rx, gpio_func_alternate_1);
	gpio_set_pin_func(uart_pins[uart].cts, gpio_func_alternate_1);
	gpio_set_pin_func(uart_pins[uart].rts, gpio_func_alternate_1);

	/* Fijamos TX y CTS como salidas y RX y RTS como entradas */
	gpio_set_pin_dir_output(uart_pins[uart].tx);
	gpio_set_pin_dir_output(uart_pins[uart].cts);
	gpio_set_pin_dir_input(uart_pins[uart].rx);
	gpio_set_pin_dir_input(uart_pins[uart].rts);

	/* Inicializamos los búferes de circulares */
	circular_buffer_init(&uart_circular_rx_buffers[uart], (uint8_t *) uart_rx_buffers[uart], sizeof(uart_rx_buffers[uart]));
	circular_buffer_init(&uart_circular_tx_buffers[uart], (uint8_t *) uart_tx_buffers[uart], sizeof(uart_tx_buffers[uart]));

	/* Programamos cuando generar las interrupciones */
	uart_regs[uart]->TxLevel = 31;	/* cola envio vacia */
	uart_regs[uart]->RxLevel = 1;	/* llega un byte */

	/* Habilitamos las interrupciones de la uart */
	/* en el controlador de interrupciones del sistema */
	itc_set_priority (itc_src_uart1 + uart, itc_priority_normal);
	itc_set_handler (itc_src_uart1 + uart, uart_irq_handlers[uart]);
	itc_enable_interrupt (itc_src_uart1 + uart);

	/* Por defecto no hay funciones callback */
	uart_callbacks[uart].tx_callback = NULL;
	uart_callbacks[uart].rx_callback = NULL;

	/* Habilitamos interrupciones en la recepción */
	uart_regs[uart]->mRxR = 0;

	/* Registramos el dispositivo. Implementación del driver de nivel 2 */
	bsp_register_dev (name, uart, NULL, NULL, uart_receive, uart_send, NULL, NULL, NULL);

	return 0;
}

/*****************************************************************************/

/**
 * Transmite un byte por la uart
 * Implementación del driver de nivel 0. La llamada se bloquea hasta que transmite el byte
 * @param uart	Identificador de la uart
 * @param c		El carácter
 */
void uart_send_byte(uart_id_t uart, uint8_t c){
	uart_regs[uart]->mTxR = 1;

	if(!circular_buffer_is_empty(&uart_circular_tx_buffers[uart])){
		while(uart_regs[uart]->Tx_fifo_addr_diff > 0){
			uart_regs[uart]->Tx_data = circular_buffer_read(&uart_circular_tx_buffers[uart]);
		}
	}

	/* Esperamos a poder transmitir */
	// Espera hasta que el número de huecos en la cola de escritura sea mayor que 0
	while(uart_regs[uart]->Tx_fifo_addr_diff == 0);

	/* Escribimos el carácter en la cola HW de la uart */
	uart_regs[uart]->Tx_data = c;

	uart_regs[uart]->mTxR = 0;
}

/*****************************************************************************/

/**
 * Recibe un byte por la uart
 * Implementación del driver de nivel 0. La llamada se bloquea hasta que recibe el byte
 * @param uart	Identificador de la uart
 * @return		El byte recibido
 */
uint8_t uart_receive_byte(uart_id_t uart){
	uint8_t value;
	uart_regs[uart]->mRxR = 1;

	if(!circular_buffer_is_empty(&uart_circular_rx_buffers[uart])){
		value = circular_buffer_read(&uart_circular_tx_buffers[uart]);
	}
	else{
		/* Esperamos a poder recibir */
		// Espera hasta que el número de bytes en la cola de lectura sea mayor que 0
		while(uart_regs[uart]->Rx_fifo_addr_diff == 0);

		/* Leemos el byte */
		value = uart_regs[uart]->Rx_data;
	}

	uart_regs[uart]->mRxR = 0;

	return value;
}

/*****************************************************************************/

/**
 * Transmisión de bytes
 * Implementación del driver de nivel 1. La llamada es no bloqueante y se realiza mediante interrupciones
 * @param uart	Identificador de la uart
 * @param buf	Búfer con los caracteres
 * @param count	Número de caracteres a escribir
 * @return	El número de bytes almacenados en el búfer de transmisión en caso de éxito o
 *              -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
ssize_t uart_send(uint32_t uart, char *buf, size_t count){
	if(uart >= uart_max){
		errno = ENODEV;

		return -1;
	}

	if(buf == NULL || count < 0){
		errno = EFAULT;

		return -1;
	}

	uint32_t written = 0;

	/*
		Deshabilitamos la petición de interrupciones del transmisor de la UART
		mientras realizamos la copia de datos al búfer circular
	*/
	uart_regs[uart]->mTxR = 1;

	while(!circular_buffer_is_full(&uart_circular_tx_buffers[uart]) && count > 0){
		circular_buffer_write(&uart_circular_tx_buffers[uart], *buf++);

		written++;
		count--;
	}

	/*
		Volvemos a habilitarla una vez que la copia ha terminado
	*/
	uart_regs[uart]->mTxR = 0;

	return written;
}

/*****************************************************************************/

/**
 * Recepción de bytes
 * Implementación del driver de nivel 1. La llamada es no bloqueante y se realiza mediante interrupciones
 * @param uart	Identificador de la uart
 * @param buf	Búfer para almacenar los bytes
 * @param count	Número de bytes a leer
 * @return	El número de bytes realmente leídos en caso de éxito o
 *              -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
ssize_t uart_receive(uint32_t uart, char *buf, size_t count){
	if(uart >= uart_max){
		errno = ENODEV;

		return -1;
	}

	if(buf == NULL || count < 0){
		errno = EFAULT;

		return -1;
	}

	uint32_t read = 0;

	/*
		Región crítica para el acceso al búfer circular de recepción
	*/
	uart_regs[uart]->mRxR = 1;

	while(!circular_buffer_is_empty(&uart_circular_rx_buffers[uart]) && count > 0){
		*buf++ = circular_buffer_read(&uart_circular_rx_buffers[uart]);

		read++;
		count--;
	}

	/*
		Fin de región crítica
	*/
	uart_regs[uart]->mRxR = 0;

	return read;
}

/*****************************************************************************/

/**
 * Fija la función callback de recepción de una uart
 * @param uart	Identificador de la uart
 * @param func	Función callback. NULL para anular una selección anterior
 * @return	Cero en caso de éxito o -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
int32_t uart_set_receive_callback(uart_id_t uart, uart_callback_t func){
	if(uart >= uart_max){
		errno = ENODEV;

		return -1;
	}

	uart_callbacks[uart].rx_callback = func;

	return 0;
}

/*****************************************************************************/

/**
 * Fija la función callback de transmisión de una uart
 * @param uart	Identificador de la uart
 * @param func	Función callback. NULL para anular una selección anterior
 * @return	Cero en caso de éxito o -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
int32_t uart_set_send_callback(uart_id_t uart, uart_callback_t func){
	if(uart >= uart_max){
		errno = ENODEV;

		return -1;
	}

	uart_callbacks[uart].tx_callback = func;

	return 0;
}

/*****************************************************************************/

/**
 * Manejador genérico de interrupciones para las uart.
 * Cada isr llamará a este manejador indicando la uart en la que se ha
 * producido la interrupción.
 * Lo declaramos inline para reducir la latencia de la isr
 * @param uart	Identificador de la uart
 */
static inline void uart_isr(uart_id_t uart){
	uint32_t status;

	/* Si la interrupción es por un error, la reconocemos */
	/* Limpiamos los bits de error, de momento no gestionamos errores */
	status = uart_regs[uart]->STAT;

	/* Si la interrupción es del receptor */
	if (uart_regs[uart]->RxRdy){
		/* Mandamos al búfer todos los caracteres de la cola HW que podamos */
		while (!circular_buffer_is_full(&uart_circular_rx_buffers[uart]) && (uart_regs[uart]->Rx_fifo_addr_diff > 0)){
			circular_buffer_write (&uart_circular_rx_buffers[uart], uart_regs[uart]->Rx_data);	/* Recibimos un carácter */
		}

		/* Llamamos a la función callback para que la aplicación se haga cargo de los datos del búfer */
		if (uart_callbacks[uart].rx_callback){
			uart_callbacks[uart].rx_callback();
		}

		/* Si el buffer circular está lleno, no podemos aceptar más datos */
		if (circular_buffer_is_full (&uart_circular_rx_buffers[uart])){
			uart_regs[uart]->mRxR =	1;	/* Enmascaramos las interrupciones del receptor para que no nos ofrezca más datos */
		}
	}

	/* Si la interrupción es del transmisor */
	if (uart_regs[uart]->TxRdy){
		/* Mandamos a la cola HW todos los caracteres del búfer que podamos */
		while (!circular_buffer_is_empty(&uart_circular_tx_buffers[uart]) && (uart_regs[uart]->Tx_fifo_addr_diff > 0)){
			uart_regs[uart]->Tx_data = circular_buffer_read (&uart_circular_tx_buffers[uart]);	/* Transmitimos un carácter */
		}

		/* Llamamos a la función callback por si la aplicación quiere mandar más datos al búfer */
		if (uart_callbacks[uart].tx_callback){
				uart_callbacks[uart].tx_callback();
		}

		/* Si el búfer está vacío es que no hay mas datos */
		if (circular_buffer_is_empty (&uart_circular_tx_buffers[uart])){
			uart_regs[uart]->mTxR = 1;	/* Enmascaramos las interrupciones del transmisor para que no nos pida más datos */
		}
	}
}

/*****************************************************************************/

/**
 * Manejador de interrupciones para la uart1
 */
static void uart_1_isr(void){
	uart_isr(uart_1);
}

/*****************************************************************************/

/**
 * Manejador de interrupciones para la uart2
 */
static void uart_2_isr(void){
	uart_isr(uart_2);
}

/*****************************************************************************/

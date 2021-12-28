#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

#include "adc.h"
#include "gpio.h"
#include "i2c.h"
#include "serial.h"
#include "timer.h"

char input[] = "Arturas";
char output[7];
char buffer[7];

int main (void) {

	i2c_init();
	uart_init();

	sei();

            //del 1-2
	for(int i = 0; i < sizeof(input); i++){
		eeprom_write_byte(0x10 + i, input[i]);
	}

	
	for(int s = 0; s < sizeof(output); s++){
		output[s] = eeprom_read_byte(0x10 + s);
		
	}
					// DELUPPGIFT 3 (VG-KRAV)
	eeprom_write_page(DATA_ADDRESS, input);
	eeprom_sequential_read(buffer, DATA_ADDRESS, sizeof(input));

	
	/*for (int i = 0; i < sizeof(buffer); i++)
	{
		printf_P(PSTR("DATA FROM EEPROM: %x\n"), buffer[i]);
	}*/

	while (1) {
		              //del 1-2
		//printf_P(PSTR("%s\n"), output);

						// DELUPPGIFT 3 (VG-KRAV)
		printf_P(PSTR("DATA FROM EEPROM: %s\n"), buffer);

	}

	return 0;
}


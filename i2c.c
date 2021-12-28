#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <string.h>

#include "i2c.h"

void i2c_init(void) {
	TWCR = (1 << TWEN); //enable the 2-wire Serial Interface (TWI)
	TWSR = 0; //prescaler value = 1;
	TWBR = 72; // result in SCL frequency of 100 kHz
	//SCL frequency = CPU Clock frequency/(16+2*(TWBR) * Prescaler value)
	//100 000 = 16 000 000 / (16+2*(TWBR) * Prescaler value)
	//TWBR = 72 = 100 000 = SCL frequency
	//100 000 = 16 000 000 / (16+2*72) * 1)
	

}

void i2c_meaningful_status(uint8_t status) {
	switch (status) {
		case 0x08: // START transmitted, proceed to load SLA+W/R
			printf_P(PSTR("START\n"));
			break;
		case 0x10: // repeated START transmitted, proceed to load SLA+W/R
			printf_P(PSTR("RESTART\n"));
			break;
		case 0x38: // NAK or DATA ARBITRATION LOST
			printf_P(PSTR("NOARB/NAK\n"));
			break;
		// MASTER TRANSMIT
		case 0x18: // SLA+W transmitted, ACK received
			printf_P(PSTR("MT SLA+W, ACK\n"));
			break;
		case 0x20: // SLA+W transmitted, NAK received
			printf_P(PSTR("MT SLA+W, NAK\n"));
				break;
		case 0x28: // DATA transmitted, ACK received
			printf_P(PSTR("MT DATA+W, ACK\n"));
			break;
		case 0x30: // DATA transmitted, NAK received
			printf_P(PSTR("MT DATA+W, NAK\n"));
			break;
		// MASTER RECEIVE
		case 0x40: // SLA+R transmitted, ACK received
			printf_P(PSTR("MR SLA+R, ACK\n"));
			break;
		case 0x48: // SLA+R transmitted, NAK received
			printf_P(PSTR("MR SLA+R, NAK\n"));
			break;
		case 0x50: // DATA received, ACK sent
			printf_P(PSTR("MR DATA+R, ACK\n"));
			break;
		case 0x58: // DATA received, NAK sent
			printf_P(PSTR("MR DATA+R, NAK\n"));
			break;
		default:
			printf_P(PSTR("N/A %02X\n"), status);
			break;
	}
}

inline void i2c_start() {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // send start condition
	while (!(TWCR & (1 << TWINT))); //Wait for TWINT Flag set. This indicates that the START condition has been transmitted
}

inline void i2c_stop() {
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); //Transmit STOP condition
	while ((TWCR & (1 << TWSTO))); // Wait for stop
	
}

inline uint8_t i2c_get_status(void) {
	//TWSR = (1 << TWS7) | (1 << TWS6) | (1 << TWS5) | (1 << TWS4) | (1 << TWS3);
	uint8_t status;
	status = (TWSR & 0xF8); //These 5 bits reflect the status of the TWI logic and the rest three bits are prescaler bits
	return status;
	//return (TWSR & 0xF8);
}

inline void i2c_xmit_addr(uint8_t eepromAddress, uint8_t rw) {
	TWDR = (eepromAddress & 0xfe) | (rw & 0x01); // Load SLA + R/W into TWDR Register, (Setting control byte with adress and rw) 0 = write 1 = read
	TWCR = (1 << TWINT) | (1 << TWEN); //Clear TWINT(interrupt) bit in TWCR to start transmission of address
	while (!(TWCR & (1 << TWINT))); //Wait for TWINT Flag set. This indicates that the SLA + W has been transmitted, and ACK / NACK has been received.
}

inline void i2c_xmit_byte(uint8_t data) {
	TWDR = data; //Load DATA into TWDR Register
	TWCR = (1 << TWINT) | (1 << TWEN); //Clear TWINT bit in TWCR to start transmission of data
	while (!(TWCR & (1 << TWINT))); //Wait for TWINT Flag set. This indicates that the DATA has been transmitted, and ACK / NACK has been received
}

inline uint8_t i2c_read_ACK() {
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA); //Clear TWINT bit in TWCR and enable ACK(TWEA) to receive a byte
	while (!(TWCR & (1 << TWINT))); // wait for TWINT flag set
	return TWDR;
}

inline uint8_t i2c_read_NAK() {
	TWCR = (1 << TWINT) | (1 << TWEN); //Clear TWINT bit in TWCR without ACK(TWEA) to receive a byte and then send NACK
	while (!(TWCR & (1 << TWINT))); // wait for TWINT flag set
	return TWDR;
}

inline void eeprom_wait_until_write_complete() {
	while(i2c_get_status() != 0x18) // wait for SLA+W has been transmitted; ACK has been received
	{
		i2c_start();
		i2c_xmit_addr(EEPROM_ADDRESS, I2C_W); //transmit eeprom addres and write
	}
}

uint8_t eeprom_read_byte(uint8_t dataAddr) {
	uint8_t receivedData;
	i2c_start();                              //start
	//i2c_meaningful_status(i2c_get_status());
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_W); //transmit eeprom addres and write
	i2c_xmit_byte(dataAddr); //transmit data addres
	i2c_start(); //restart
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_R); //transmit eeprom addres and read
	receivedData = i2c_read_NAK(); //receive data
	i2c_stop(); //stop
	//i2c_meaningful_status(i2c_get_status());
	return receivedData;

}

void eeprom_write_byte(uint8_t dataAddr, uint8_t data) {
	i2c_start();
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_W); //transmit eeprom addres and write
	i2c_xmit_byte(dataAddr); //transmit data addres
	i2c_xmit_byte(data); //transmit memory data
	i2c_stop(); //stop
	eeprom_wait_until_write_complete(); // ACK


}



void eeprom_write_page(uint8_t dataAddr, uint8_t *data) {
	i2c_start();
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_W); //transmit eeprom addres and write
	i2c_xmit_byte(dataAddr); //transmit data addres
	for(int i = 0; i < strlen(data); i ++){
		i2c_xmit_byte(*data++);
	}
	i2c_stop(); //stop
	eeprom_wait_until_write_complete(); // ACK

}

void eeprom_sequential_read(uint8_t *buf, uint8_t start_addr, uint8_t len) {
	i2c_start();
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_W); 
	i2c_xmit_byte(start_addr);
	i2c_start();
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_R);

	/*for (int i = 0; i < len -1 ; i++)
	{
		*buf++ = i2c_read_ACK();	
	}
	*buf = i2c_read_NAK(); // no ack
	
	i2c_stop();*/

	
	for (int i = 0; i < len - 1; i++)
	{
		buf[i] = i2c_read_ACK();
	}

	//Receive last data byte 
	buf[len - 1] = i2c_read_NAK();

	//Stop
	i2c_stop();
}

/*
 * AVRExample.cpp
 *
 * Created: 2017/09/02 08:30:42
 * Author : Constructor
 */ 

#define F_CPU 8000000

#include <avr/io.h>
#include <ajmodem.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "lcd.h"

int main(void)
{
	sei();
	
	Modem modem = Modem(F_CPU, 32);
	modem.begin();
	
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();
	char s[32];
	sprintf(s,"Not ready to \nreceive yet.");
	lcd_puts(s);
	DDRD |= (1<<7);
	
    while (1) 
    {
		_delay_ms(100);
		PORTD &= ~(1<<7);
		_delay_ms(100);
		PORTD |= (1<<7);
		
    }
}


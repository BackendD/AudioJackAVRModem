/*
 * AVRExample.cpp
 *
 * Created: 2017/09/02 08:30:42
 * Author : Constructor
 */ 

#define F_CPU 8000000

#include <avr/io.h>
#include <ajmodem.h>

int main(void)
{
	Modem modem = Modem(F_CPU, 32);
	modem.begin();
    while (1) 
    {
    }
}


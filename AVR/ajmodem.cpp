#include "ajmodem.h"

Modem* Modem::activeObject = nullptr;

Modem::Modem(uint32_t freq, uint8_t bufferSize)
{
	setFreq(freq);
	uint8_t buffer[bufferSize];
	_recvBuffer = buffer;
}

Modem::~Modem()
{
	
}

void Modem::setFreq(uint32_t freq)
{
	this->freq = freq;
	baudRate = 1225;
	lowFreq = 4900;
	highFreq = 7350;
}

void Modem::begin(void)
{
	DDRB |= (1<<0);
	PORTB &= ~(1<<0);
	
	DDRB &= ~((1<<3) | (1<<2));
	PORTB &= ~((1<<3) | (1<<2));
	
	_recvBufferHead = _recvBufferTail = 0;
	
	Modem::activeObject = this;
	
	_lastTCNT = TCNT2;
	_lastDiff = _lowCount = _highCount = 0;
	
	TCCR1A &= ~((1<<WGM11) | (1<<WGM10));
	TCCR1B &= ~((1<<WGM13) | (1<<WGM12) | (1<<CS12) | (1<<CS11) | (1<<CS10));
	
	ACSR &= ~(1<<ACIS0);
	ACSR |= ((1<<ACIE) | (1<<ACIS1));
}
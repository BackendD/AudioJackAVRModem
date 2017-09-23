#include "ajmodem.h"
#include <avr/interrupt.h>
#include <stdlib.h>

void * operator new[](size_t size) {
	return malloc(size);
}

void operator delete[](void * ptr) {
	free(ptr);
}

Modem* Modem::activeObject = nullptr;

Modem::Modem(uint32_t freq, uint8_t bufferSize)
{
	setFreq(freq);
	 
	_recvBuffer = new uint8_t[rxBufSize = bufferSize];
}

Modem::~Modem()
{
	
}

enum { START_BIT = 0, DATA_BIT = 8, STOP_BIT = 9, INACTIVE = 0xff };
	
void Modem::setFreq(uint32_t freq)
{
	this->freq = freq;																	//8,000,000
	baudRate = 1225;
	lowFreq = 4900;
	highFreq = 7350;
	
	low_freq_micros			= (uint16_t) (1000000 / lowFreq);							//204
	micros_per_timer_count	= (uint16_t) (8 / (freq/1000000));							//1
	tcnt_low_freq			= low_freq_micros / micros_per_timer_count;					//204
	tcnt_low_th_l			= tcnt_low_freq - (tcnt_low_freq - tcnt_high_freq) / 2;		//173
	tcnt_low_th_h			= tcnt_low_freq + (tcnt_low_freq - tcnt_high_freq) / 2;		//224
	bit_period				= (uint16_t) (1000000 / baudRate);							//816 
	tcnt_bit_period			= bit_period / micros_per_timer_count;						//816
	high_freq_micros		= (uint16_t) (1000000/highFreq);							//136
	tcnt_high_freq			= high_freq_micros / micros_per_timer_count;				//136
	tcnt_high_th_l			= tcnt_high_freq - (tcnt_low_freq - tcnt_high_freq) / 2;	//173
	tcnt_high_th_h			= tcnt_high_freq + (tcnt_low_freq - tcnt_high_freq) / 2;	//156
}

void Modem::begin(void)
{
	DDRB |= (1<<0);
	PORTB &= ~(1<<0);
	
	DDRB &= ~((1<<3) | (1<<2));
	PORTB &= ~((1<<3) | (1<<2));
	
	_recvStat = INACTIVE;
	_recvBufferHead = _recvBufferTail = 0;
	
	Modem::activeObject = this;
	
	_lastTCNT = TCNT1;
	_lowCount = _highCount = 0;
	
	TCCR1A &= ~((1<<WGM11) | (1<<WGM10));
	TCCR1B &= ~((1<<WGM13) | (1<<WGM12) | (1<<CS12) | (1<<CS10));
	TCCR1B |= (1<<CS11);
	
	ACSR &= ~(1<<ACIS0);
	ACSR |= ((1<<ACIE) | (1<<ACIS1));
}

void Modem::demodulate(void)
{
	uint16_t t = TCNT1;
	uint16_t diff;
	diff = t - _lastTCNT;
	if (diff < tcnt_high_th_l)
		return;
	_lastTCNT = t;
	if(diff > tcnt_low_th_h)
		return;
	
	if(tcnt_low_th_l <= diff && diff <= tcnt_low_th_h)
	{
		_lowCount += diff;
		
		if(_recvStat == INACTIVE)
		{
			// Start bit detection
			if(_lowCount >= tcnt_bit_period * 0.5)
			{
				_recvStat = START_BIT;
				_highCount = 0;
				_recvBits  = 0;
				OCR1A = t + tcnt_bit_period - _lowCount;
				TIFR |= (1<<OCF1A);
				TIMSK |= (1<<OCIE1A);
			}
		}
	}
	else if(tcnt_high_th_l <= diff && diff <= tcnt_high_th_h)
	{
		if(_recvStat == INACTIVE)
		{			
			_lowCount = 0;
			_highCount = 0;
		}
		else
		{
			_highCount += diff;
		}
	}
}

// Analog comparator interrupt
ISR(ANA_COMP_vect)
{
	Modem::activeObject->demodulate();
}

void Modem::recv(void)
{
	uint8_t high;
	
	// Bit logic determination
	if(_highCount > _lowCount)
	{
		_highCount = 0;
		high = 0x80;			//0b1000 0000
	}
	else
	{
		_lowCount = 0;
		high = 0x00;			//0b0000 0000
	}
	// Start bit reception
	if(_recvStat == START_BIT)
	{
		if(!high)
		{
			_recvStat++;
		}
		else
		{
			goto end_recv;
		}
	}
	// Data bit reception
	else if(_recvStat <= DATA_BIT) 
	{
		_recvBits >>= 1;
		_recvBits |= high;
		_recvStat++;
	}
	// Stop bit reception
	else if(_recvStat == STOP_BIT)
	{
		if(high)
		{
			// Stored in the receive buffer
			uint8_t new_tail = (_recvBufferTail + 1 == rxBufSize) ? 0 : _recvBufferTail + 1;
			if(new_tail != _recvBufferHead)
			{
				*(_recvBuffer + _recvBufferTail) = _recvBits;
				_recvBufferTail = new_tail;
			}
			else
			{
				;// Overrun error detection
			}
		}
		else
		{
			;// Fleming error detection
		}
		goto end_recv;
	}
	else
	{
end_recv:
		_recvStat = INACTIVE;
		TIMSK &= ~(1<<OCIE1A);
	}
}

ISR(TIMER1_COMPA_vect)
{
	OCR1A += Modem::activeObject->tcnt_bit_period;
	Modem::activeObject->recv();
}

uint8_t Modem::available()
{
	return _recvBufferTail == _recvBufferHead ? 0 : 1;
}

uint8_t Modem::read()
{
	if(_recvBufferHead == _recvBufferTail)
		return -1;
	uint8_t c = *(_recvBuffer + _recvBufferHead);
	_recvBufferHead = (_recvBufferHead + 1 == rxBufSize) ? 0 : _recvBufferHead + 1;
	return c;
}

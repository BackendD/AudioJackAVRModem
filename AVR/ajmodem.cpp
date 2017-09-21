#include "ajmodem.h"
#include <avr/interrupt.h>



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

enum { START_BIT = 0, DATA_BIT = 8, STOP_BIT = 9, INACTIVE = 0xff };
	
void Modem::setFreq(uint32_t freq)
{
	this->freq = freq;															//8,000,000
	baudRate = 1225;
	lowFreq = 4900;
	highFreq = 7350;
	
	low_freq_micros			= (uint16_t) (1000000 / lowFreq);					//204
	micros_per_timer_count	= (uint16_t) (8 / (freq/1000000));					//1
	tcnt_low_freq			= low_freq_micros / micros_per_timer_count;			//204
	tcnt_low_th_l			= tcnt_low_freq * 0.85;								//173
	tcnt_low_th_h			= tcnt_low_freq * 1.10;								//224
	bit_period				= (uint16_t) (1000000 / baudRate);					//816 
	tcnt_bit_period			= bit_period / micros_per_timer_count;				//816
	high_freq_micros		= (uint16_t) (1000000/highFreq);					//136
	tcnt_high_freq			= high_freq_micros / micros_per_timer_count;		//136
	tcnt_high_th_h			= tcnt_high_freq * 1.15;							//156
}

void Modem::begin(void)
{
	DDRB |= (1<<0);
	PORTB &= ~(1<<0);
	
	DDRB &= ~((1<<3) | (1<<2));
	PORTB &= ~((1<<3) | (1<<2));
	
	_recvBufferHead = _recvBufferTail = 0;
	
	Modem::activeObject = this;
	
	_lastTCNT = TCNT1;
	_lowCount = _highCount = 0;
	
	TCCR1A &= ~((1<<WGM11) | (1<<WGM10));
	TCCR1B &= ~((1<<WGM13) | (1<<WGM12) | (1<<CS12) | (1<<CS11) | (1<<CS10));
	
	ACSR &= ~(1<<ACIS0);
	ACSR |= ((1<<ACIE) | (1<<ACIS1));
}

void Modem::demodulate(void)
{
	uint16_t t = TCNT1;
	uint16_t diff;
	diff = t - _lastTCNT;
	_lastTCNT = t;
	if(diff > tcnt_low_th_h)
		return;
	if(diff >= tcnt_low_th_l)
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
				OCR1A = t + tcnt_bit_period - (uint16_t)_lowCount;
				TIFR |= (1<<OCF1A);
				TIMSK |= (1<<OCIE1A);
			}
		}
	}
	else if(diff <= (uint8_t)(tcnt_high_th_h))
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

ISR(TIMER1_COMPA_vect)
{
	
}
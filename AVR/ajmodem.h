#ifndef ajmodem_h
#define ajmodem_h

#include <stddef.h>
#include <stdint-gcc.h>
#include <avr/io.h>

class Modem 
{
public:
	Modem(uint32_t freq, uint8_t bufferSize);
	~Modem();
	void setFreq(uint32_t freq);
	void begin(void);
	static Modem* activeObject;
private:
	uint32_t freq;
	uint32_t baudRate;
	uint32_t lowFreq;
	uint32_t highFreq;
	uint32_t rxBufSize;
	uint8_t* _recvBuffer;
	uint8_t _recvBufferHead;
	uint8_t _recvBufferTail;
	uint8_t _lastTCNT;
	uint8_t _lastDiff;
	uint8_t _lowCount;
	uint8_t _highCount;
};

#endif
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

//Function to initialize the PWM for PB1 and PB2
void pwm_init(){
	TCCR1A = (1<<WGM11) | (1<<COM1A1) | (1<<COM1B1); //Fast-PWM / 64 prescaler
	TCCR1B = (1<<CS10)| (1<<CS11) | (1<<WGM12) | (1<<WGM13);
	
	ICR1 = 4999; // 50 = 16Mhz / (64* (1 + T) --> T = 4999
}

//Initialize ADC
void adc_init(){
	//ADC with AVCC as external voltage reference
	//128 Prescaler selection
	ADMUX = (1<<REFS0);
	ADCSRA = (1<<ADEN) | (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2);
	
	ADCSRA |= (1<<ADSC);
}

void uart_init(){
	//Initialize Serial Communication to Receiver and Transmitter | 8 Bits
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	
	//9600 Baud Rate
	UBRR0L = 103;
}

//Function to create delay using Timer 1 | CTC Mode | 1024 prescalar
//It only accepts some values, n cannot be higher than 2^16 - 1.
//In this project it is only used 1 - 5 ms delays.
void delay(unsigned int delayMs){
	int n = (delayMs * 16000000 / 1000)/1024;
	
	OCR0A = n - 1;
	TCCR0A = 0x02;
	TCCR0B = 0x05;
	
	while((TIFR0 & (1<<OCF0A)) == 0);
	
	TCCR0B = 0;
	TIFR0 = (1<<OCF0A);
}

//Function to retrieve ADC value from ADC Register given a certain pin number.
uint16_t returnADC(uint8_t pin){
	//Clear Pins from MUX bits
	ADMUX &= 0xF8;

	ADMUX |= pin;
	ADCSRA |= (1 << ADSC);
	
	while ((ADCSRA & (1 << ADIF)) == 0) {
		delay(1);
	}

	ADCSRA |= (1 << ADIF);

	return ADC;
}

//Function to send the character to the serial port.
void uart_putchar(char c) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

//Function to convert a string to characters
void uart_puts(const char *str, unsigned int size) {
	for(unsigned int i = 0; i < size; i++){
		uart_putchar(str[i]);
	}
}

int main(void)
{
	//PB1 & PB2 output
	DDRB |= (1<<1) | (1<<2);
	
	adc_init();
	pwm_init();
	uart_init();
	
	uint16_t right = 0;
	uint16_t left = 0;
	uint16_t up = 0;
	uint16_t down = 0;
	//Range variable sets a range for the difference is values between two sensors
	//This helps control oscillations and reduce the number of times that the motor moves
	uint16_t range = 30;
	
	//Initialize the servo motors to a initial position (437 is the middle position, 0 degrees)
	OCR1A = 437;
	OCR1B = 437;
	
	//This is the length of the message if we would like to transmit the light intensity serially
	char message[4];
	
	delay(5);
	
	while (1)
	{
		//Functions to transmit the light intensity serially
		//itoa(left, message, 10);
		//uart_puts(message, sizeof message);
		//uart_putchar('|');
		
		//First get sensor values to later compare them
		left = returnADC(3);
		right = returnADC(0);
		up = returnADC(2);
		down = returnADC(1);
		
		// 1ms = (64 * (1 + bottomOCR1A ))/16Mhz --> BottomOCR1A = 249
		if(left>right && (left-right)>range){
			if(OCR1A > 249){
				--OCR1A;
				delay(5);
			}
			// 2.5ms = (64 * (1 + topOCR1A ))/16Mhz --> topOCR1A = 624
			}else if(right > left && (right - left)>range){
			if(OCR1A < 624){
				++OCR1A;
				delay(5);
			}
		}
		//Here we limited the range of motion of the top servo motor.
		if(up > down && (up - down) >range){
			if(OCR1B > 349){
				--OCR1B;
				delay(5);
			}
			}else if(down > up && (down - up)>range){
			if(OCR1B < 524){
				++OCR1B;
				delay(5);
			}
		}
	}
	return 0;
}
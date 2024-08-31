#define F_CPU 8000000UL /* Define CPU Frequency e.g. here 8MHz */
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define LCD_Data_Dir DDRC	   /* Define LCD data port direction */
#define LCD_Command_Dir DDRD   /* Define LCD command port direction register */
#define LCD_Data_Port PORTC	   /* Define LCD data port */
#define LCD_Command_Port PORTD /* Define LCD data port */
#define RS PD5				   /* Define Register Select (data/command reg.)pin */
#define RW PD6				   /* Define Read/Write signal pin */
#define EN PD7				   /* Define Enable signal pin */

// #define USART_BAUDRATE 9600
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

// **** BUTTON INTERRUPT ****
// SHIFT SCREEN - INT0: INT0 - PD2
// EDIT MODE ON - INT1: INT1 - PD3
// SET VALUE - INT2: INT2 - PB2

// **** SENSOR PINOUTS ****
// SOIL MOISTURE - PA0
// LIGHT SENSOR - PA1
// CO2 SENSOR - PA2
// DHT22 - PB4

// **** MACHINES RELAY'S PINOUTS ****
// HEATER - PA4
// FAN - PA5
// MISTURE - PA6
// WATER PUMP - PA7

//////////////////////////////////////////////////////////////////////////
// Initialize UART
void UART_init(long USART_BAUDRATE);
unsigned char UART_RxChar();
void UART_TxChar(char ch);
void UART_SendString(char *str);
void UART_SendFloat(float number);
void UART_sendData();

///////////////////////////////////////////////////////////////////////////
// PORT initialization function prototype
void port_init();
void ext_Interrupt();
int rotary();
void show_data(uint8_t shift_btn, int counter);

// ADC initialization function prototype
void ADC_Init();
void machine();

// ADC function prototype
int sm_ADC_Read();
float soil_moisture();

int li_ADC_Read();
float light_intensity();

int co_ADC_Read();
float cogas_level();

// LCD Display function Prototypes.......
void LCD_Command(unsigned char cmnd);
void LCD_Char(unsigned char char_data);							 /* LCD data write function */
void LCD_Init(void);											 /* LCD Initialize function */
void LCD_String(const unsigned char *str, unsigned char lenght); /* Send string to LCD function */
void LCD_String_xy(char row, char pos, char *str, char lenght);	 /* Send string to LCD with xy position */
void LCD_Clear();

volatile uint8_t shift_btn = 0; // variable for shiftting screen
volatile uint8_t edit_btn = 0;	// variable for turn on edit mode of set value
volatile uint8_t set_btn = 1;	// variable for set the value of set_variable

// variable defining for rotary encoder
volatile int counter = 0;
volatile uint8_t a_state;
volatile uint8_t b_state;
volatile uint8_t a_last_state;

// variable for store the manually setted values
volatile int set_temp = 30;
volatile int set_humi = 20;
volatile int set_soil = 10;

// variables for Temperature
float temperature = 35;

// variables for Air Humidity
float humidity = 25;

// Variables for run the Soil Moisture function
char sm_array[10];
int sm_adc_value;
float sm_moisture;

// Variables for run the co level function
char co_array[10];
int co_adc_value;
float co_level;

// Variables for run the light intensity function
char li_array[10];
int li_adc_value;
float li_intensity;

// Variables for DHT11 sensor
uint8_t c = 0, I_RH, D_RH, I_Temp, D_Temp, CheckSum;

int main(void)
{

	ext_Interrupt();
	sei();
	UART_init(9600); // Baud Rate of Serial Communication
	port_init();
	LCD_Init(); /* Initialize LCD */
	ADC_Init();
	LCD_Clear();

	while (1)
	{

		show_data(shift_btn, counter);
		if (edit_btn == 0)
		{
			soil_moisture();
			light_intensity();
			cogas_level();
			machine();
			UART_sendData();
		}
		else
		{
			rotary();
		}
	}
}

////////////////////////////////////////////////////////////////////////////
void ext_Interrupt(void)
{
	DDRD = DDRD & ~(1 << DDD2);											// DDRD PIN2 as input - INT0 /CHANGE DISPLAY
	DDRD = DDRD & ~(1 << DDD3);											// DDRD PIN3 as input - INT1 /Turn on edit mode
	DDRB = DDRB & ~(1 << DDB2);											// DDRB PIN2 as input - INT2 / set value - Rotary encoder switch
	GICR |= (1 << INT0) | (1 << INT1) | (1 << INT2);					// enable external interrupt 0,1 & 2 ; INT0 = PD2 , INT1 = PD3
	MCUCR |= (1 << ISC01) | (1 << ISC00) | (1 << ISC11) | (1 << ISC10); // INT1 and INT0 as Raising edge triggered interrupt
	MCUCSR |= (1 << ISC2);												// INT2 as Raising edge triggered interrupt
}

////////////////////////////////////////////////////////////////////////////////
//// USART Functions////
void UART_init(long USART_BAUDRATE)
{
	UCSRB |= (1 << RXEN) | (1 << TXEN);					 /* Turn on transmission and reception */
	UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1); /* Use 8-bit character sizes */
	UBRRL = BAUD_PRESCALE;								 /* Load lower 8-bits of the baud rate value */
	UBRRH = (BAUD_PRESCALE >> 8);						 /* Load upper 8-bits*/
}

////////////////

unsigned char UART_RxChar()
{
	while ((UCSRA & (1 << RXC)) == 0)
		;		  /* Wait till data is received */
	return (UDR); /* Return the byte*/
}

////////////////

void UART_TxChar(char ch)
{
	while (!(UCSRA & (1 << UDRE)))
		; /* Wait for empty transmit buffer*/
	UDR = ch;
}

////////////////

void UART_SendString(char *str)
{
	unsigned char j = 0;

	while (str[j] != 0) /* Send string till null */
	{
		UART_TxChar(str[j]);
		j++;
	}
}

///////////////////

void UART_SendFloat(float number)
{
	char buffer[30];
	dtostrf(number, 5, 3, buffer);
	UART_SendString(buffer);
	memset(buffer, 0, 10);
}

////////////////////

void UART_sendData()
{
	UART_SendFloat(temperature);
	UART_SendString("A");
	UART_SendFloat(humidity);
	UART_SendString("B");
	UART_SendFloat(sm_moisture);
	UART_SendString("C");
	UART_SendFloat(li_intensity);
	UART_SendString("D");
	UART_SendFloat(co_level);
	UART_SendString("E");
	UART_SendString("\n");

	_delay_ms(1000);
}

///////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

// int rotary(a_last_state,a_state,b_state,counter)
int rotary()
{

	if (edit_btn == 1)
	{

		int check_a_last(a_last_state)
		{

			// read Initial state of the output A - a_state
			if (PINB & (1 << PINB0))
			{
				a_last_state = 0;
			}
			else
			{
				a_last_state = 1;
			}

			return a_last_state;
		}

		char count[1];

		while (set_btn != 1)
		{
			// read the updated value of a_state
			if (PINB & (1 << PINB0))
			{
				a_state = 0;
			}
			else
			{
				a_state = 1;
			}

			if (a_state != a_last_state)
			{

				// read the status of the b pin
				if (PINB & (1 << PINB1))
				{
					b_state = 0;
				}
				else
				{
					b_state = 1;
				}

				// check the rotate is clockwise or c_clockwise

				if (a_state != b_state)
				{

					counter = counter + 1;
				}

				else
				{

					counter = counter - 1;
				}

				if (edit_btn == 1)
				{
					itoa(counter, count, 10);
					LCD_String_xy(2, 13, count, 3); // data
				}
			}

			a_last_state = a_state;
			return counter;
		}
	}
	return;
}

void machine()
{
	// temperature low
	if (set_temp < temperature)
	{
		PORTA |= (1 << PA4); // HEATER TURN On
	}
	else if (set_temp > temperature)
	{
		PORTA &= (~(1 << PA4)); // HEATER TURN Off
	}

	// temperature high
	if (set_temp > temperature)
	{
		PORTA |= (1 << PA5); // FAN TURN ON
	}
	else
	{
		PORTA &= (~(1 << PA5)); // FAN TURN OFF
	}

	// humidity
	if (set_humi < humidity)
	{
		PORTA |= (1 << PA6); // TURN ON MISTURE
	}
	else
	{
		PORTA &= (~(1 << PA6)); // TURN OFF MISTURE
	}

	// soil moisture
	if (set_soil < sm_moisture)
	{
		PORTA |= (1 << PA7); // TURN ON WATER PUMP
	}
	else
	{
		PORTA &= (~(1 << PA7)); // TURN OFF WATER PUMP
	}
}

void show_data(uint8_t shift_btn, int counter)
{

	char set_val_temp[1];
	char set_val_hum[1];
	char set_val_soil[1];

	switch (shift_btn)
	{
	case 1:

		LCD_String_xy(1, 0, "  TEMPERATURE", 13);
		LCD_String_xy(2, 0, "Now:", 4);
		LCD_String_xy(2, 9, "Set:", 4);
		itoa(set_temp, set_val_temp, 10);
		if (edit_btn == 0)
		{
			LCD_String_xy(2, 13, set_val_temp, 3);
		}
		break;

	case 2:

		LCD_String_xy(1, 0, "  AIR HUMIDITY ", 15);
		LCD_String_xy(2, 0, "Now:", 4);
		LCD_String_xy(2, 9, "Set:", 4);
		itoa(set_humi, set_val_hum, 10);
		if (edit_btn == 0)
		{
			LCD_String_xy(2, 13, set_val_hum, 3);
		}
		break;

	case 3:

		LCD_String_xy(1, 0, " SOIL  MOISTURE", 15);
		LCD_String_xy(2, 0, "Now:", 4);
		LCD_String_xy(2, 9, "Set:", 4);
		itoa(set_soil, set_val_soil, 10);
		if (edit_btn == 0)
		{
			LCD_String_xy(2, 13, set_val_soil, 3);
		}
		break;

	case 4:
		LCD_String_xy(1, 0, "LIGHT INTENSITY", 15);
		LCD_String_xy(2, 0, "Now:", 4);
		LCD_String_xy(2, 9, "Set:", 4);
		LCD_String_xy(2, 13, "NO ", 3);
		break;

	case 5:
		LCD_String_xy(1, 0, "  CO GAS LEVEL ", 15);
		LCD_String_xy(2, 0, "Now:", 4);
		LCD_String_xy(2, 9, "Set:", 4);
		LCD_String_xy(2, 13, "NO ", 3);
		break;

	default:
		// Handle any other cases or errors if necessary
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////
// ADC Interrupts//
/*ISR(ADC_vect){

	// temperature
	if(set_temp < temperature){
		PORTA &= (~(1 << PA4));  	// HEATER TURN ON
		PORTA |= (1 << PA5);  		// FAN TURN OFF
	} else if(set_temp > temperature ){
		PORTA |= (1 << PA4); 		//HEATER TURN OFF
		PORTA &= (~(1 << PA5)); 	// FAN TURN ON
	}

	// humidity
	if(set_humi < humidity){
		PORTA &= (~(1 << PA6));		// TURN ON MISTURE
	}else{
		PORTA |= (1 << PA6);		// TURN OFF MISTURE
	}

	// soil moisture
	if(set_soil < sm_moisture){
		PORTA &= (~(1 << PA7));		// TURN ON WATER PUMP
	}else {
		PORTA |= (1 << PA7);		// TURN OFF WATER PUMP
	}


}
*/

//////////////////////////////////////////////////////////////////////////////////
// *** button interrupts *1*5*///

// Shift Screen
ISR(INT0_vect)
{

	if (edit_btn != 1)
	{

		LCD_Clear();

		switch (shift_btn)
		{
		case 0:
			shift_btn = 1;
			break;
		case 1:
			shift_btn = 2;
			counter = set_temp;
			break;
		case 2:
			shift_btn = 3;
			counter = set_humi;
			break;
		case 3:
			shift_btn = 4;
			counter = set_soil;
			break;
		case 4:
			shift_btn = 5;
			break;
		case 5:
			shift_btn = 1;
			break;
		default:
			// Handle any other values or errors if necessary
			break;
		}
	}
}

// turn on edit mode
ISR(INT1_vect)
{

	switch (shift_btn)
	{
	case 0:
		edit_btn = 0;
		set_btn = 1;
		break;
	case 1:
		edit_btn = 1;
		set_btn = 0;
		break;
	case 2:
		edit_btn = 1;
		set_btn = 0;
		break;
	case 3:
		edit_btn = 1;
		set_btn = 0;
		break;
	case 4:
		edit_btn = 0;
		set_btn = 1;
		break;
	case 5:
		edit_btn = 0;
		set_btn = 1;
		break;
	default:
		// Handle any other values or errors if necessary
		break;
	}
}

// set variable value
ISR(INT2_vect)
{
	if (set_btn == 0)
	{

		edit_btn = 0;
		set_btn = 1;

		switch (shift_btn)
		{
		case 0:
			break;
		case 1:
			set_temp = counter;
			break;
		case 2:
			set_humi = counter;
			break;
		case 3:
			set_soil = counter;
			break;
		case 4:
			break;
		case 5:
			break;
		default:
			// Handle any other values or errors if necessary
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////
void port_init()
{
	// LCD
	DDRC = 0xFF;									 // Port C- DATA for LCD display
	DDRD |= (1 << DDD5) | (1 << DDD6) | (1 << DDD7); // PD5,PD6,PD7 - INSTRUCTION- output/LCD

	// ROTARY ENCODER -  PB0,PB1 , PB2 set as input pins - that output A and B connected
	DDRB &= (~(1 << DDB0)); // rotary encoder line A
	DDRB &= (~(1 << DDB1)); // rotary encoder line B

	// SENOSRS inputs - ANALOG
	DDRA = 0b11110000;
	// DDRA = DDRA & ~(1 << PA0); // DDRA PIN0 as input - soil
	// DDRA = DDRA & ~(1 << PA1); // DDRA PIN1 as input - light
	// DDRA = DDRA & ~(1 << PA2); // DDRA PIN2 as input - co2

	// external device controll outputs
	// DDRA |= (1 << PA4); // DDRA PIN4 as output - Heater
	// DDRA |= (1 << PA5); // DDRA PIN5 as output - Fan
	// DDRA |= (1 << PA6); // DDRA PIN6 as output - HUMIDITY/ mistures
	// DDRA |= (1 << PA7) ; // DDRA PIN7 as output -  WATER PUMP
}
//////////////////////////////////////////////////////////////////////////////////////////////

//////  ADc register initialization //////////////

void ADC_Init()
{

	ADCSRA = 0x87; //  Enable ADC, fr/128
}

////////////////////////////////////////////////////////////////
// Functions for Soil moisture value//

int sm_ADC_Read()
{

	ADMUX = 0b01000000;	   /* Vref: Avcc, ADC channel: 0  */
	ADCSRA |= (1 << ADSC); /* start conversion  */
	while ((ADCSRA & (1 << ADIF)) == 0)
		;				   /* monitor end of conversion interrupt flag */
	ADCSRA |= (1 << ADIF); /* set the ADIF bit of ADCSRA register */
	return (ADCW);		   /* return the ADCW */
}

float soil_moisture()
{

	sm_adc_value = sm_ADC_Read();						   /* Copy the ADC value */
	sm_moisture = 100 - (sm_adc_value * 100.00) / 1023.00; /* Calculate moisture in % */

	dtostrf(sm_moisture, 3, 1, sm_array);
	strcat(sm_array, "%   "); /* Concatenate unit of % */

	if (shift_btn == 3)
	{

		LCD_String_xy(2, 4, sm_array, 5);
	}

	memset(sm_array, 0, 10);
	_delay_ms(50);
	return sm_moisture;
}

////////////////////////////////////////////////////////////////
// Functions for Light Intensity value//

int li_ADC_Read()
{

	ADMUX = 0b01000001;	   /* Vref: Avcc, ADC channel: 0  */
	ADCSRA |= (1 << ADSC); /* start conversion  */
	while ((ADCSRA & (1 << ADIF)) == 0)
		;				   /* monitor end of conversion interrupt flag */
	ADCSRA |= (1 << ADIF); /* set the ADIF bit of ADCSRA register */
	return (ADCW);		   /* return the ADCW */
}

float light_intensity()
{

	li_adc_value = li_ADC_Read(); /* Copy the ADC value */

	li_intensity = 100 - (li_adc_value * 100.00) / 1023.00; /* Calculate light intensity in % */

	dtostrf(li_intensity, 3, 1, li_array);
	strcat(li_array, "%   "); /* Concatenate unit of % */

	if (shift_btn == 4)
	{

		LCD_String_xy(2, 4, li_array, 5);
	}

	memset(li_array, 0, 10);
	_delay_ms(50);
	return li_intensity;
}

////////////////////////////////////////////////////////////////
// Functions for CO gas level//

int co_ADC_Read()
{

	ADMUX = 0b01000010;	   /* Vref: Avcc, ADC channel: 0  */
	ADCSRA |= (1 << ADSC); /* start conversion  */
	while ((ADCSRA & (1 << ADIF)) == 0)
		;				   /* monitor end of conversion interrupt flag */
	ADCSRA |= (1 << ADIF); /* set the ADIF bit of ADCSRA register */
	return (ADCW);		   /* return the ADCW */
}

float cogas_level()
{

	co_adc_value = co_ADC_Read();						/* Copy the ADC value */
	co_level = 100 - (co_adc_value * 100.00) / 1023.00; /* Calculate light intensity in % */
	dtostrf(co_level, 3, 1, co_array);
	strcat(co_array, "%   "); /* Concatenate unit of % */

	if (shift_btn == 5)
	{

		LCD_String_xy(2, 4, co_array, 4);
	}

	memset(co_array, 0, 10);
	_delay_ms(50);
	return co_level;
}

//////////////////////////////////////////////////////////////////////////////////////////////

/* commands for LCD display */
void LCD_Command(unsigned char cmnd)
{
	LCD_Data_Port = cmnd;
	LCD_Command_Port &= ~(1 << RS); /* RS=0 command reg. */
	LCD_Command_Port &= ~(1 << RW); /* RW=0 Write operation */
	LCD_Command_Port |= (1 << EN);	/* Enable pulse */
	_delay_us(1);
	LCD_Command_Port &= ~(1 << EN);
	_delay_ms(3);
}

/* LCD data write function */
void LCD_Char(unsigned char char_data)
{
	LCD_Data_Port = char_data;
	LCD_Command_Port |= (1 << RS);	/* RS=1 Data reg. */
	LCD_Command_Port &= ~(1 << RW); /* RW=0 write operation */
	LCD_Command_Port |= (1 << EN);	/* Enable Pulse */
	_delay_us(1);
	LCD_Command_Port &= ~(1 << EN);
	_delay_ms(1);
}

/* LCD Initialize function */
void LCD_Init(void)
{
	_delay_ms(20);	   /* LCD Power ON delay always >15ms */
	LCD_Command(0x38); /* Initialization of 16X2 LCD in 8bit mode */
	LCD_Command(0x0C); /* Display ON Cursor OFF */
	LCD_Command(0x06); /* Auto Increment cursor */
	LCD_Command(0x01); /* Clear display */
	LCD_Command(0x80); /* Cursor at home position */
}

/* Send string to LCD function */
void LCD_String(const unsigned char *str, unsigned char lenght)
{
	int i;
	for (i = 0; i < lenght; i++) /* Send each char of string till the NULL */
	{
		LCD_Char(str[i]);
	}
}

/* Send string to LCD with xy position */
void LCD_String_xy(char row, char pos, char *str, char lenght)
{
	if (row == 1 && pos < 16)
	{
		LCD_Command((pos & 0x0F) | 0x80); /* Command of first row and required position<16 */
		LCD_String(str, lenght);		  /* Call LCD string function */
	}

	else if (row == 2 && pos < 16)
	{
		LCD_Command((pos & 0x0F) | 0xC0); /* Command of first row and required position<16 */
		LCD_String(str, lenght);		  /* Call LCD string function */
	}

	_delay_ms(1);
}

void LCD_Clear()
{
	LCD_Command(0x01); /* clear display */
	LCD_Command(0x80); /* cursor at home position */
}

#include "stm32f4xx.h"
#include "LCD_init.h"
#include <string.h>


void tim6_delay(void){
	// enable APB1 bus clock
	RCC->APB1ENR|=RCC_APB1ENR_TIM6EN;
	//TIM6 prescaler set at default to 0 for now
	TIM6->PSC=0; // prescalar
	TIM6->ARR = 21000;  //auto reload register 
	TIM6->CNT=0;   //clear counter register
	TIM6->CR1|=TIM_CR1_CEN;
	//WHEN COUNTER IS DONE THE TIM6_SR REG UIF FLAG IS SET
	while(TIM6->SR==0);
	TIM6->SR=0; //CLEAR uIF FLAG
}

void delay(int ms){
int i;
	for (i=ms; i>0; i--)
	{
		tim6_delay();
	}
}

void LCD_port_init(){
//STEP 1: Enable GPIOD in RCC AHB1ENR register
RCC -> AHB1ENR |= (1<<3); //enable port D clock 
	
//STEP 2: Set MODER of GPIOD Pins 7, 6, 3, 2, 1 & 0 as outputs
	
// MODER: output is 01
	//Pin7: bits 15&14 ....Pin0: bits 1 and 0
	//Pin 7
	GPIOD -> MODER &= ~(1<<15); //bit 15 = 0
	GPIOD -> MODER |= (1<<14); //bit 14 = 1
	//Pin 6
	GPIOD -> MODER &= ~(1<<13); //bit 13 = 0
	GPIOD -> MODER |= (1<<12); //bit 12 = 1
	//Pin 5
	GPIOD -> MODER &= ~(1<<11); //bit 11 = 0
	GPIOD -> MODER |= (1<<10); //bit 10 = 1
	//Pin 4
	GPIOD -> MODER &= ~(1<<9); //bit 9 = 0
	GPIOD -> MODER |= (1<<8); //bit 8 = 1
	//pin 3
	GPIOD -> MODER &= ~(1<<7); //bit 7 = 0
	GPIOD -> MODER |= (1<<6); //bit 6 = 1
	//Pin 2
	GPIOD -> MODER &= ~(1<<5); //bit 5 = 0
	GPIOD -> MODER |= (1<<4); //bit 4 = 1
	//Pin 1
	GPIOD -> MODER &= ~(1<<3); //bit 3 = 0
	GPIOD -> MODER |= (1<<2); //bit 2 = 1
	//Pin 0
	GPIOD -> MODER &= ~(1<<1); //bit 1 = 0
	GPIOD -> MODER |= (1<<0); //bit 0 = 1
	
//STEP 3: Set OTYPER of GPIOD Pins 7, 6, 3, 2, 1 & 0 as push-pull

	//bits 7-0 set to 0 (push pull)
	GPIOD -> OTYPER &=~(1<<7); //Sets bit 7 in OTYPER OFF
	GPIOD -> OTYPER &=~(1<<6);
	GPIOD -> OTYPER &=~(1<<5);
	GPIOD -> OTYPER &=~(1<<4);
	GPIOD -> OTYPER &=~(1<<3);
	GPIOD -> OTYPER &=~(1<<2);
	GPIOD -> OTYPER &=~(1<<1);
	GPIOD -> OTYPER &=~(1<<0);
	
//Done with LCD port Initialization
}

void LCD_init(){
RCC -> AHB1ENR |= (1<<3); //enable port D clock 
	
// STEP 1: Wait for 100ms for power-on-reset to take effect
	delay(76); //delay for *roughly* 100ms
	
// STEP 2: Set RS pin LOW to send instructions
	GPIOD -> ODR &=~(1<<7); //set pin 7 to 0 (RS pin)
	delay(16);//delays *roughly* 20ms
	
// Send instructions using following format:
// Set EN=HIGH; Send 4-bit instruction; Set EN=low; delay 20ms;
// STEP 3a-3d: Set 4-bit mode (takes a total of 4 steps)
	
	GPIOD -> ODR |=(1<<6); //Sets bit 6 (Enable)
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2);//DB6=0
	GPIOD -> ODR |=(1<<1);//DB5=1
	GPIOD -> ODR |=(1<<0);//DB4=1
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
	GPIOD -> ODR |=(1<<6); //Sets bit 6 (Enable)
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2);//DB6=0
	GPIOD -> ODR |=(1<<1);//DB5=1
	GPIOD -> ODR |=(1<<0);//DB4=1
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
	GPIOD -> ODR |=(1<<6); //Sets bit 6 (Enable)
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2);//DB6=0
	GPIOD -> ODR |=(1<<1);//DB5=1
	GPIOD -> ODR |=(1<<0);//DB4=1
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
	// 4-bit: RS=0;RW=0;DB7=0;DB6=0;DB5=1;DB4=0;
	GPIOD -> ODR |=(1<<6); //Sets bit 6 (Enable)
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2);//DB6=0
	GPIOD -> ODR |=(1<<1);//DB5=1
	GPIOD -> ODR &=~(1<<0);//DB4=0
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
// STEP 4: Set 2 line display -- treats 16 char as 2 lines

	//0 0 0 0 1 0 0 0 1 0 * *
	//IN other words: RS=0;7=0;6=0;5=1;4=0
									//RS=0;7=1;6=0;5=0;4=0
	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2); //=0
	GPIOD -> ODR |=(1<<1); //=1
	GPIOD -> ODR &=~(1<<0);//=0
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR |=(1<<3); //DB7=1
	GPIOD -> ODR &=~(1<<2); //=0
	GPIOD -> ODR &=~(1<<1); //=0
	GPIOD -> ODR &=~(1<<0);//=0
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
// STEP 5: Set DISPLAY to OFF

	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2); //6=0
	GPIOD -> ODR &=~(1<<1); //5=0
	GPIOD -> ODR &=~(1<<0);//4=0
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR |=(1<<3); //DB7=1
	GPIOD -> ODR &=~(1<<2); //6=0 (clear)
	GPIOD -> ODR &=~(1<<1); //5=0 (no cursor)
	GPIOD -> ODR &=~(1<<0);//4=0 (no blink)
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
// STEP 6: CLEAR DISPLAY

	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2); //6=0
	GPIOD -> ODR &=~(1<<1); //5=0
	GPIOD -> ODR &=~(1<<0);//4=0
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2); //6=0
	GPIOD -> ODR &=~(1<<1); //5=0
	GPIOD -> ODR |=(1<<0); //4=1
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
// STEP 7: SET ENTRY MODE - Auto increment; no scrolling

	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2); //6=0
	GPIOD -> ODR &=~(1<<1); //5=0
	GPIOD -> ODR &=~(1<<0);//4=0
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //7=0
	GPIOD -> ODR |=(1<<2);//6=1  
	GPIOD -> ODR |=(1<<1);//5=1 
	GPIOD -> ODR &=~(1<<0);//4 =0
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
// STEP 8: Set Display to ON with Cursor and Blink.
	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2); //6=0
	GPIOD -> ODR &=~(1<<1); //5=0
	GPIOD -> ODR &=~(1<<0);//4=0
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR |=(1<<3); //DB7=1
	GPIOD -> ODR |=(1<<2);//6=1  (not cleared)
	GPIOD -> ODR |=(1<<1);//5=1 (cursor on)
	GPIOD -> ODR |=(1<<0);//4=1 (blink)
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
}


void LCD_Delay_String_Write(char string[], int delay_time){
	int size = strlen(string);   // length, not counting '\0'
	if (size > 16){              // LCD only holds 16 characters
		size = 16;
	}
	for (int i = 0; i < size; i++){
		LCD_write(string[i]);        // send one character
		delay(delay_time);           // pause between characters
	}
}

void ClearLCD(){
	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2); //6=0
	GPIOD -> ODR &=~(1<<1); //5=0
	GPIOD -> ODR &=~(1<<0);//4=0
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
	
	GPIOD -> ODR |=(1<<6); //Enable High
	GPIOD -> ODR &=~(1<<7); //RS=0
	GPIOD -> ODR &=~(1<<3); //DB7=0
	GPIOD -> ODR &=~(1<<2); //6=0
	GPIOD -> ODR &=~(1<<1); //5=0
	GPIOD -> ODR |=(1<<0); //4=1
	GPIOD -> ODR &=~(1<<6);//Enable low
	delay(16);//delays *roughly* 20ms
}

void LCD_String_Write(char string[]){
	int size = strlen(string);   // length, not counting '\0'
	if (size > 16){              // LCD only holds 16 characters
		size = 16;
	}
	for (int i = 0; i < size; i++){
		
		LCD_write(string[i]);        // send one character
	}
}

void LCD_write(unsigned char data){
		GPIOD -> ODR |=(1<<7); //set RS to high (DATA REGISTER...No longer the instruction reg)
		GPIOD -> ODR |=(1<<6); //Enable High
	
	//*note: the characters are converted to ASCII, so I need to check each of the bits and set the ODR accordingly
		if (data & (1<<7)){//check if MSB = 1
			GPIOD -> ODR |=(1<<3); //if it is, set the ODR at that bit to 1
		}
		else{
			GPIOD -> ODR &=~(1<<3);//else, set to 0
		}
		if (data & (1<<6)){//check next bit 
			GPIOD -> ODR |=(1<<2); 
		}
		else{
			GPIOD -> ODR &=~(1<<2);
		}
		if (data & (1<<5)){//check is next bit is 1
			GPIOD -> ODR |=(1<<1); 
		}
		else{
			GPIOD -> ODR &=~(1<<1);
		}
		if (data & (1<<4)){//check is next bit is 1
			GPIOD -> ODR |=(1<<0); 
		}
		else{
			GPIOD -> ODR &=~(1<<0);
		}
		GPIOD -> ODR &=~(1<<6);//Enable low
		delay(16);//delays *roughly* 20ms
		
		//last 4 bits
		GPIOD -> ODR |=(1<<6); //Enable High
		if (data & (1<<3)){//check if MSB = 1
			GPIOD -> ODR |=(1<<3); 
		}
		else{
			GPIOD -> ODR &=~(1<<3);
		}
		if (data & (1<<2)){ //check is next bit is 1
			GPIOD -> ODR |=(1<<2);
		}
		else{
			GPIOD -> ODR &=~(1<<2);
		}
		if (data & (1<<1)){//check is next bit is 1
			GPIOD -> ODR |=(1<<1); 
		}
		else{
			GPIOD -> ODR &=~(1<<1);
		}
		if (data & (1<<0)){//check if LSB = 1
			GPIOD -> ODR |=(1<<0); 
		}
		else{
			GPIOD -> ODR &=~(1<<0);
		}
		GPIOD -> ODR &=~(1<<6);//Enable low
		delay(16);//delays *roughly* 20ms

}

void place_lcd_cursor(unsigned char lineno){
	if (lineno ==1){
		// Set DDRAM: DB7=1
		GPIOD -> ODR |=(1<<6); //Enable High
		GPIOD -> ODR |=(1<<3); //DB7=1
		GPIOD -> ODR &=~(1<<2); //6=0
		GPIOD -> ODR &=~(1<<1); //5=0
		GPIOD -> ODR &=~(1<<0);//4=0
		GPIOD -> ODR &=~(1<<6);//Enable low
		delay(16);//delays *roughly* 20ms
		
		GPIOD -> ODR |=(1<<6); //Enable High
		GPIOD -> ODR &=~(1<<3); //7=0
		GPIOD -> ODR &=~(1<<2); //6=0
		GPIOD -> ODR &=~(1<<1); //5=0
		GPIOD -> ODR &=~(1<<0);//4=0
		GPIOD -> ODR &=~(1<<6);//Enable low
		delay(16);//delays *roughly* 20ms
	}
	else if(lineno ==2){
		GPIOD -> ODR |=(1<<6); //Enable High
		GPIOD -> ODR |=(1<<3); //DB7=1
		GPIOD -> ODR |=(1<<2); //6=1
		GPIOD -> ODR &=~(1<<1); //5=0
		GPIOD -> ODR &=~(1<<0);//4=0
		GPIOD -> ODR &=~(1<<6);//Enable low
		delay(16);//delays *roughly* 20ms
		
		GPIOD -> ODR |=(1<<6); //Enable High
		GPIOD -> ODR &=~(1<<3); //7=0
		GPIOD -> ODR &=~(1<<2); //6=0
		GPIOD -> ODR &=~(1<<1); //5=0
		GPIOD -> ODR &=~(1<<0);//4=0
		GPIOD -> ODR &=~(1<<6);//Enable low
		delay(16);//delays *roughly* 20ms
	}
	else{//default 2
		GPIOD -> ODR |=(1<<6); //Enable High
		GPIOD -> ODR |=(1<<3); //DB7=1
		GPIOD -> ODR |=(1<<2); //6=1
		GPIOD -> ODR &=~(1<<1); //5=0
		GPIOD -> ODR &=~(1<<0);//4=0
		GPIOD -> ODR &=~(1<<6);//Enable low
		delay(16);//delays *roughly* 20ms
		
		GPIOD -> ODR |=(1<<6); //Enable High
		GPIOD -> ODR &=~(1<<3); //7=0
		GPIOD -> ODR &=~(1<<2); //6=0
		GPIOD -> ODR &=~(1<<1); //5=0
		GPIOD -> ODR &=~(1<<0);//4=0
		GPIOD -> ODR &=~(1<<6);//Enable low
		delay(16);//delays *roughly* 20ms
	}
}
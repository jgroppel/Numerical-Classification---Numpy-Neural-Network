#ifndef LCD_INIT_H
#define LCD_INIT_H

void tim6_delay(void);
void delay(int ms);
void LCD_port_init(void);
void LCD_init(void);
void LCD_Delay_String_Write(char string[], int delay_time);
void ClearLCD(void);
void LCD_String_Write(char string[]);
void place_lcd_cursor(unsigned char lineno);
void LCD_write(unsigned char data);
#endif
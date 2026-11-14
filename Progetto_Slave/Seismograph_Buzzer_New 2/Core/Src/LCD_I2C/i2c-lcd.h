#ifndef __I2C_LCD_H
#define __I2C_LCD_H

#include "stm32f3xx_hal.h" // Includiamo l'HAL per i tipi come I2C_HandleTypeDef

// Dichiarazioni delle funzioni pubbliche per il display LCD
// La funzione lcd_init ora accetta l'handle I2C e l'indirizzo del display
void lcd_init (I2C_HandleTypeDef *hi2c_handle, uint8_t lcd_address);
void lcd_send_cmd (char cmd);
void lcd_send_data (char data);
void lcd_clear (void);
void lcd_put_cur(int row, int col);
void lcd_send_string (char *str);

#endif /* __I2C_LCD_H */

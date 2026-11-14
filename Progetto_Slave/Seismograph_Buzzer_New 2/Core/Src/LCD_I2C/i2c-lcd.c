/**
  ******************************************************************************
  * @file           : i2c-lcd.c
  * @brief          : Driver per LCD1602 I2C adattato per STM32 HAL (F3/F4/L4 etc.)
  * @author         : Basato su Controllerstech (adattato da Assistente AI)
  * @version        : 1.2 - Adattamento per HAL I2C e correzioni delay
  ******************************************************************************
  * @attention
  *
  * Questo driver è basato sulla libreria originale di Controllerstech,
  * ma è stato modificato per utilizzare le funzioni HAL I2C standard di STM32.
  *
  * Non richiede i file 'i2c.h' e 'delay.h' originali di Controllerstech,
  * ma si basa sull'inclusione di 'stm32f3xx_hal.h' (o l'HAL generico).
  *
  ******************************************************************************
*/

#include "i2c-lcd.h"        // Include il proprio file header
#include "stm32f3xx_hal.h"  // Include l'header HAL specifico per STM32F3

// Dichiarazione delle variabili statiche interne alla libreria
// Queste variabili memorizzeranno l'handle I2C e l'indirizzo del display
// passati durante l'inizializzazione, rendendoli accessibili a tutte le
// funzioni interne della libreria.
static I2C_HandleTypeDef *Lcd_hi2c_handle;
static uint8_t Lcd_i2c_address;

// Implementazione di un micro-delay semplice (potrebbe essere migliorato con un timer)
// Funziona per piccoli ritardi, per ritardi più lunghi usare HAL_Delay
static void lcd_delay_us(uint32_t us)
{
    // Questa è una implementazione molto base e dipende dalla frequenza CPU.
    // Per ritardi precisi in microsecondi, si dovrebbe usare un timer.
    // Per un HSI a 16MHz (come nel tuo SystemClock_Config),
    // ogni iterazione del loop prende circa 4 cicli.
    // Quindi per 'us' microsecondi, il numero di iterazioni è us * (16 / 4) = us * 4
    volatile uint32_t i = 0; // Correzione da uint33_t a uint32_t
    for (i = 0; i < (us * (SystemCoreClock / 1000000UL / 4)); i++);
}


// Funzione di basso livello per inviare dati I2C al display LCD
// Sostituisce la funzione 'LCD_Write' originale di Controllerstech
static void lcd_i2c_transmit (uint8_t *Data, int size)
{
    // Usa la funzione HAL_I2C_Master_Transmit
    // Timeout impostato a 100ms
    HAL_I2C_Master_Transmit(Lcd_hi2c_handle, Lcd_i2c_address, Data, size, 100);
}

// Funzione per inviare un comando al display LCD
void lcd_send_cmd (char cmd)
{
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (cmd&0xf0);
    data_l = ((cmd<<4)&0xf0);
    data_t[0] = data_u|0x0C;  //en=1, rs=0 (Backlight ON)
    data_t[1] = data_u|0x08;  //en=0, rs=0 (Backlight ON)
    data_t[2] = data_l|0x0C;  //en=1, rs=0 (Backlight ON)
    data_t[3] = data_l|0x08;  //en=0, rs=0 (Backlight ON)
    lcd_i2c_transmit (data_t, 4); // Usa la funzione HAL-based
}

// Funzione per inviare un dato (carattere) al display LCD
void lcd_send_data (char data_to_send)
{
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (data_to_send&0xf0);
    data_l = ((data_to_send<<4)&0xf0);
    data_t[0] = data_u|0x0D;  //en=1, rs=1 (Backlight ON)
    data_t[1] = data_u|0x09;  //en=0, rs=1 (Backlight ON)
    data_t[2] = data_l|0x0D;  //en=1, rs=1 (Backlight ON)
    data_t[3] = data_l|0x09;  //en=0, rs=1 (Backlight ON)
    lcd_i2c_transmit (data_t, 4); // Usa la funzione HAL-based
}

// Funzione per pulire il display
void lcd_clear (void)
{
    // Il comando 0x01 pulisce il display e porta il cursore a (0,0)
    lcd_send_cmd (0x01);
    HAL_Delay(3); // Necessario un ritardo per il comando Clear Display
}

// Funzione per impostare la posizione del cursore
void lcd_put_cur(int row, int col)
{
    uint8_t address;
    switch (row)
    {
        case 0: address = 0x80 + col; break;
        case 1: address = 0xC0 + col; break;
        default: address = 0x80 + col; break; // Default alla riga 0
    }
    lcd_send_cmd (address);
}

// Funzione per scrivere una stringa sul display LCD
void lcd_send_string (char *str)
{
    while (*str) lcd_send_data (*str++);
}

// Funzione di inizializzazione del display LCD
// Ora accetta l'handle I2C e l'indirizzo del display
void lcd_init (I2C_HandleTypeDef *hi2c_handle, uint8_t lcd_address)
{
    // Salva l'handle I2C e l'indirizzo per uso futuro nelle altre funzioni
    Lcd_hi2c_handle = hi2c_handle;
    Lcd_i2c_address = lcd_address;

    // Sequenza di inizializzazione del display HD44780 in modalità 4-bit
    HAL_Delay(50);      // Ritardo di accensione (power on delay)
    lcd_send_cmd (0x30);
    lcd_delay_us(4500); // Wait for >4.1ms
    lcd_send_cmd (0x30);
    lcd_delay_us(150);  // Wait for >100us
    lcd_send_cmd (0x30);
    HAL_Delay(10);
    lcd_send_cmd (0x20);  // 4bit mode
    HAL_Delay(10);

    // Configurazione del display
    lcd_send_cmd (0x28); // Function Set: DL=0 (4 bit mode), N = 1 (2 linee), F = 0 (5x8 caratteri)
    HAL_Delay(1);
    lcd_send_cmd (0x08); // Display On/Off Control: D=0, C=0, B=0 (Display OFF, Cursore OFF, Lampeggio OFF)
    HAL_Delay(1);
    lcd_send_cmd (0x01);  // Clear Display (pulisce il contenuto del display)
    HAL_Delay(3);
    lcd_send_cmd (0x06); // Entry Mode Set: I/D = 1 (incrementa cursore), S = 0 (non sposta il display)
    HAL_Delay(1);
    lcd_send_cmd (0x0C); // Display On/Off Control: D=1 (Display ON), C=0 (Cursore OFF), B=0 (Lampeggio OFF)
    HAL_Delay(1);
}

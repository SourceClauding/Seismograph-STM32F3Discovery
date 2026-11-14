/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : SLAVE - Buzzer + Display
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c-lcd.h" // Per il display LCD I2C
#include <stdio.h>   // Per sprintf (formattazione di stringhe)
#include <string.h>  // Per strlen (calcolo lunghezza stringa)
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Definiamo il pin del buzzer per rendere il codice più leggibile
#define BUZZER_PORT GPIOA  // Porta Buzzer
#define BUZZER_PIN  GPIO_PIN_6  // Pin buzzer

// Indirizzo I2C comune del modulo PCF8574 per LCD.
// Molti sono 0x27, alcuni 0x3F.
// '<< 1' è necessario per le funzioni HAL che usano indirizzi a 8 bit.
// Verifica con uno scanner I2C se 0x27 non funziona (es. prova 0x3F << 1).
#define LCD_I2C_ADDRESS (0x27 << 1)  // Indirizzo LCD
#define BUZZER_ON_MS 3000u	// Durata suono buzzer in millisecondi

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;  // I2C1 per LCD
TIM_HandleTypeDef htim2;  // TIM2 per tick 1ms
UART_HandleTypeDef huart1;  // USART1 per ricezione level

/* USER CODE BEGIN PV */
// Variabili globali
uint8_t uart_rx_buffer[1]; // Buffer per la ricezione di 1 byte dalla seriale
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);  // Clock di sistema
static void MX_GPIO_Init(void);  // GPIO buzzer out, abilitazioni clock
static void MX_I2C1_Init(void);  // I2C1 per LCD
static void MX_TIM2_Init(void);  // TIM2 -> tick 1 ms
static void MX_USART1_UART_Init(void);  // USART1 9600 8N1
/* USER CODE BEGIN PFP */
void Manage_Buzzer_On_Vibration(uint8_t level);  // Gestione del livello ricevuto
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  Callback chiamata in automatico quando la ricezione UART è completa.
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)  // ISR
{
  if (huart->Instance == USART1)  // Verifica che provenga da USART1
  {
    uint8_t received_level = uart_rx_buffer[0];  // Leggi byte ricevuto

    Manage_Buzzer_On_Vibration(received_level);  // Chiama la funzione che gestisce la logica del buzzer e ora del display

    HAL_UART_Receive_IT(&huart1, uart_rx_buffer, 1);  // Si rimette in ascolto per il prossimo dato
  }
}

/**
  * @brief  Callback chiamata quando l'impulso del timer in modalità One-Pulse è terminato.
  * Questa funzione viene chiamata UNA SOLA VOLTA, 3 secondi dopo aver avviato il timer.
  */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)  // ISR
{
    if (htim->Instance == TIM2)  //
    {
      // L'impulso di 3 secondi è finito, quindi spegniamo il buzzer.
      // Se la logica è invertita, GPIO_PIN_SET spegne il buzzer.
      HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);

      // NON AGGIUNGERE NESSUNA LOGICA QUI PER IL DISPLAY.
      // La visualizzazione della magnitudo persistente viene gestita
      // in Manage_Buzzer_On_Vibration e rimane fino alla prossima vibrazione.
    }
}

/**
  * @brief  Gestisce la logica di accensione del buzzer per 3 secondi e visualizza la magnitudo.
  * La magnitudo visualizzata è il valore intero del 'level' ricevuto,
  * che si presume corrisponda al punto di inizio dell'intervallo di magnitudo.
  */
void Manage_Buzzer_On_Vibration(uint8_t level)
{
    char mag_str[17]; // Buffer per la stringa del display
    static float displayed_magnitude = 0.0f; // Variabile statica per mantenere l'ultima magnitudo valida

    if (level > 0)
    {
        // Se il level è 1, lo mappiamo a 2.0 (perché 1 LED verde = Mag 2.0)
        // Altrimenti, il level ricevuto corrisponde direttamente alla magnitudo (es. level 3 -> Mag 3.0)
        if (level == 1) { // Caso specifico per "1 verde (LD6)" che è 2.0-2.9
            displayed_magnitude = 2.0f;
        } else {
            displayed_magnitude = (float)level; // Per 0, 3, 4, 5, 6, 7, 8
        }

        HAL_TIM_OnePulse_Stop_IT(&htim2, TIM_CHANNEL_1);  // Controllo che il timer sia fermo prima di riavviarlo

        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);   // LIVELLO > 0: Vibrazione rilevata. Accensione del buzzer

        // Aggiorna il display per indicare la vibrazione e la magnitudo corrente
        lcd_put_cur(0, 0);
        lcd_send_string("Vibration!    "); // Spazi per pulire il resto della riga
        lcd_put_cur(1, 0);
        sprintf(mag_str, "Mag:%.1f", displayed_magnitude); // Visualizza la magnitudo corrente
        lcd_send_string(mag_str);

        for(int i = strlen(mag_str); i < 16; i++) lcd_send_data(' ');  // Pulizia del resto della riga se la stringa è più corta di 16 caratteri

        if (HAL_TIM_OnePulse_Start_IT(&htim2, TIM_CHANNEL_1) != HAL_OK)  // Avvio il timer in modalità One-Pulse con interrupt.
        {
            Error_Handler();
        }
    }
    else // level == 0
    {
        // LIVELLO 0: Nessuna vibrazione o comando di stop esplicito.
        HAL_TIM_OnePulse_Stop_IT(&htim2, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);

        // Aggiorna il display per indicare l'attesa (mantenendo l'ultima magnitudo)
        lcd_put_cur(0, 0);
        lcd_send_string("Seismograph:");
        lcd_put_cur(1, 0);
        sprintf(mag_str, "Mag:%.1f", displayed_magnitude); // Usa la magnitudo precedentemente memorizzata
        lcd_send_string(mag_str);

        for(int i = strlen(mag_str); i < 16; i++) lcd_send_data(' ');  // Pulisci il resto della riga se la stringa è più corta di 16 caratteri
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  char mag_str[17]; // Buffer locale per la stringa del display

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  // All'avvio, garantisce che il buzzer sia spento
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);  // Buzzer spento all'avvio
  HAL_Delay(100); // Ritardo di 100 millisecondi per permettere al pin di stabilizzarsi

  lcd_init(&hi2c1, LCD_I2C_ADDRESS);  // Inizializzazione del display LCD I2C
  HAL_Delay(100); // Piccolo delay dopo l'inizializzazione

  // Test di scrittura iniziale sul display (mostra il messaggio iniziale e Mag: 0.0f)
  lcd_clear();
  lcd_put_cur(0, 0);
  lcd_send_string("Seismograph:"); // Messaggio iniziale
  lcd_put_cur(1, 0);
  sprintf(mag_str, "Mag:%.1f", 0.0f); // Inizializza con 0.0f all'avvio
  lcd_send_string(mag_str);
  for(int i = strlen(mag_str); i < 16; i++) lcd_send_data(' '); // Pulisci la riga

  HAL_UART_Receive_IT(&huart1, uart_rx_buffer, 1);  // Mette lo schedino in ascolto sulla porta seriale per il primo dato

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);  // Processore in modalità sleep, si sveglierà solo per gli interrupt (UART o timer)
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function - MODIFICATA PER ONE-PULSE MODE
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OnePulse_InitTypeDef sOnePulseConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  /*
   * Calcolo per un delay di 3 secondi con un clock di sistema a 16MHz
   * Frequenza Timer = Clock CPU / (Prescaler + 1) -> 16,000,000 / (15999 + 1) = 1000 Hz (1 KHz)
   * Periodo di ogni tick = 1 / 1000 Hz = 1 ms
   * Durata totale = Periodo tick * Period -> 1ms * 3000 = 3000 ms = 3 secondi
   */
  htim2.Init.Prescaler = 15999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 3000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_OnePulse_Init(&htim2, TIM_OPMODE_SINGLE) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sOnePulseConfig.OCMode = TIM_OCMODE_PWM1;
  sOnePulseConfig.Pulse = 3000; // La durata dell'impulso (3 secondi)
  sOnePulseConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
  sOnePulseConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
  sOnePulseConfig.ICPolarity = TIM_ICPOLARITY_RISING;
  sOnePulseConfig.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sOnePulseConfig.ICFilter = 0;
  if (HAL_TIM_OnePulse_ConfigChannel(&htim2, &sOnePulseConfig, TIM_CHANNEL_1, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  // * VERIFICA FONDAMENTALE: Abilita l'interrupt del TIM2 nel NVIC *
  // CubeMX dovrebbe farlo automaticamente, ma assicurati che questa riga (o equivalente) sia presente
  // nel tuo codice di inizializzazione o nel file stm32fxxx_it.c
  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0); // Imposta la priorità dell'interrupt
  HAL_NVIC_EnableIRQ(TIM2_IRQn);         // Abilita l'interrupt per il TIM2

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;  // 8 bit
  huart1.Init.StopBits = UART_STOPBITS_1;  // 1 stop
  huart1.Init.Parity = UART_PARITY_NONE;  // Nessuna parità
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();  // Abilita clock porta A (buzzer)
  __HAL_RCC_GPIOB_CLK_ENABLE();  // Abilita clock porta B (PB6, PB7)

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);  // Per tenere il buzzer spento all'avvio, per la logica è invertita, impostiamo a HIGH.

  /*Configure GPIO pin : PA6 (Buzzer) */
  GPIO_InitStruct.Pin = BUZZER_PIN;  // PA6 -> Buzzer
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);

  /* NVIC per USART1 e TIM2 (interrupt vettorizzati) */
  HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);  // Priorità USART1 (RX)
  HAL_NVIC_EnableIRQ(USART1_IRQn);  // Abilita IRQ USART1
  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);  // Priorità TIM2 (tick 1 ms)
  HAL_NVIC_EnableIRQ(TIM2_IRQn);  // Abilita IRQ TIM2
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);  // Accende il buzzer in caso di errore per segnalarlo visivamente
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  * where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

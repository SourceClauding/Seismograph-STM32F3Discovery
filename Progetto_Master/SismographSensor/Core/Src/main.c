/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : MASTER - Sensore di vibrazione
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MEASUREMENT_WINDOW_MS 100  // Finestra di misurazione per contare gli impulsi del sensore (100 ms)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim2;  // TIM2 usato per realizzare la finestra di misurazione temporale
TIM_HandleTypeDef htim3;  // TIM3 usato come contatore di base
UART_HandleTypeDef huart1; // USART1 su PA9/PA10 per trasmettere il livello al ricevitore
PCD_HandleTypeDef hpcd_USB_FS;

/* USER CODE BEGIN PV */
volatile uint32_t pulse_count = 0;  // Conta gli impulsi (fronte di salita) dal sensore durante la finestra
volatile uint32_t pulse_frequency = 0;  // Frequenza stimata (Hz), aggiornata a fine finestra
volatile uint8_t measurement_complete = 0;  // Flag impostato da TIM2 alla scadenza della finestra temporale
uint8_t current_level = 0;  // Ultimo livello trasmesso

const uint16_t frequency_thresholds[8] = {
    5, 10, 20, 40, 80, 150, 300, 500  // Soglie di frequenza per determinare i livelli di intensità
};

const uint16_t led_pins[8] = {
    LD3_Pin, LD5_Pin, LD7_Pin, LD9_Pin, LD10_Pin, LD8_Pin, LD6_Pin, LD4_Pin  // Array dei LED sulla porta GPIOE. Stabilisce l'ordine di accensione dei LED in base all'intensità registrata
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);  // Configurazione del clock
static void MX_GPIO_Init(void);  // Inizializzazione GPIO
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_TIM2_Init(void);  // Inizializzazione TIM2
static void MX_TIM3_Init(void);  // Inizializzazione TIM3
static void MX_USART1_UART_Init(void);  // Inizializzazione UART1
/* USER CODE BEGIN PFP */
void Update_LED_Display(uint8_t level);  // Funzione per accendere i LED in base al valore di level, mantiene spenti tutti gli altri
uint8_t Map_Frequency_To_Level(uint32_t frequency);  // Funzione per determinare i livelli a partire dagli Hz
void Process_Vibration_Measurement(void);  // Funzione che nella finestra di 100 ms calcola la frequenza, mappa il livello e invia UART
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USB_PCD_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);     // Imposta priorità massima per TIM2
  HAL_NVIC_EnableIRQ(TIM2_IRQn);             // Abilita l'interrupt di TIM2

  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_Base_Start_IT(&htim2);  // Abilito TIM2 in modalità interrupt

  // TEST PER VERIFICARE FUNZIONAMENTO DEI LED
  for(int i = 0; i < 8; i++) {
      HAL_GPIO_WritePin(GPIOE, led_pins[i], GPIO_PIN_SET);
  }
  HAL_Delay(1000);  // TEST LED ACCESI PER 1 SECONDO
  Update_LED_Display(0);  // TEST SPEGNIMENTO DEI LED
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);  // La CPU è in sleep mode e si attiva solo quando arriva un interrupt : EXTI rileva un impulso o TIM2 segnala la fine della finestra

	if(measurement_complete)  // Se la finestra è terminata allora elabora i dati raccolti
	{
		Process_Vibration_Measurement();  // Calcolo e trasmissione
		measurement_complete = 0;  // Resetto il flag della finestra
	}
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON; // ✅ CORREZIONE: Impostato su ON per usare il cristallo
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  */
static void MX_I2C1_Init(void) { /* Generata da CubeMX */ }
/**
  * @brief SPI1 Initialization Function
  */
static void MX_SPI1_Init(void) { /* Generata da CubeMX */ }
/**
  * @brief TIM2 Initialization Function
  */
static void MX_TIM2_Init(void) { /* Generata da CubeMX */ }
/**
  * @brief TIM3 Initialization Function
  */
static void MX_TIM3_Init(void) { /* Generata da CubeMX */ }
/**
  * @brief USART1 Initialization Function
  */
static void MX_USART1_UART_Init(void) { /* Generata da CubeMX */ }
/**
  * @brief USB Initialization Function
  */
static void MX_USB_PCD_Init(void) { /* Generata da CubeMX */ }

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, CS_I2C_SPI_Pin|LD4_Pin|LD3_Pin|LD5_Pin
                          |LD7_Pin|LD9_Pin|LD10_Pin|LD8_Pin
                          |LD6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DRDY_Pin MEMS_INT3_Pin MEMS_INT4_Pin MEMS_INT1_Pin
                           MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = DRDY_Pin|MEMS_INT3_Pin|MEMS_INT4_Pin|MEMS_INT1_Pin
                          |MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_I2C_SPI_Pin LD4_Pin LD3_Pin LD5_Pin
                           LD7_Pin LD9_Pin LD10_Pin LD8_Pin
                           LD6_Pin */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin|LD4_Pin|LD3_Pin|LD5_Pin
                          |LD7_Pin|LD9_Pin|LD10_Pin|LD8_Pin
                          |LD6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : Vibration_Sensor_Pin */
  GPIO_InitStruct.Pin = Vibration_Sensor_Pin_Pin;  // Pin del sensore configurato per generare un interrupt
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;  // Interrupt sul fronte di salita
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(Vibration_Sensor_Pin_GPIO_Port, &GPIO_InitStruct);  // Inizializzazione del pin

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);  // Imposto la priorità di EXTI0 (linea 0)
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);  // Abilito l'interrupt EXTI nel NVIC
}

/* USER CODE BEGIN 4 */

/**
  * @brief  GPIO External Interrupt callback
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)  // Chiamata in automatico dalla HAL; ISR dedicata per EXTI0
{
    if(GPIO_Pin == Vibration_Sensor_Pin_Pin)  // Verifica che l'interrupt provenga dal pin corretto
    {
        pulse_count++;  // Incremento contatore per contare un impulso di vibrazione
    }
}

/**
  * @brief  Timer period elapsed callback
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)  // Chiamata ogni volta che TIM2 fa overflow
{
    if(htim->Instance == TIM2)  // Controllo per verificare che l'interrupt sia di TIM2
    {
        measurement_complete = 1;  // Indica la finestra della finestra di misurazione
    }
}

/**
  * @brief  Processa i dati di vibrazione e li invia
  */
void Process_Vibration_Measurement(void)  // Elaborazione dei dati raccolti
{
    __disable_irq();  // Disabilito le interruzioni per proteggere il valore di pulse_count
    uint32_t pulses = pulse_count;  // Copio il valore del contatore
    pulse_count = 0;  // Resetto il contatore per la prossima finestra di misurazione
    __enable_irq();  // Riabilito le interruzioni

    pulse_frequency = pulses * 10;  // Calcolo della frequenza
    uint8_t new_level = Map_Frequency_To_Level(pulse_frequency);

    if(new_level != current_level)  // Controllo che mi serve per aggiornare il livello solamente se è cambiato rispetto alla precedente misurazione
    {
        current_level = new_level;
        Update_LED_Display(current_level);  // Se il livello è cambiato allora accendo i LED
        HAL_UART_Transmit(&huart1, &current_level, 1, 100);  // Trasmissione del byte via UART del livello misurato
    }
}

/**
  * @brief  Mappa la frequenza al livello corrispondente.
  */
uint8_t Map_Frequency_To_Level(uint32_t frequency)  // Mappatura della frequenza al livello
{
    for(int8_t i = 7; i >= 0; i--)
    {
        if(frequency >= frequency_thresholds[i])
        {
            return i + 1;
        }
    }
    return 0;
}

/**
  * @brief  Aggiorna i LED in base al livello
  */
void Update_LED_Display(uint8_t level)  // Accensione dei LED in base al livello calcolato
{
    for(uint8_t i = 0; i < 8; i++)
    {
    	// Spengo tutti i LED
        HAL_GPIO_WritePin(GPIOE, led_pins[i], GPIO_PIN_RESET);
    }

    // Accendo tutti i LED
    if(level > 0)
    {
        for(uint8_t i = 0; i < level; i++)
        {
            HAL_GPIO_WritePin(GPIOE, led_pins[i], GPIO_PIN_SET);
        }
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
      HAL_GPIO_TogglePin(GPIOE, LD3_Pin);
      HAL_Delay(100);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

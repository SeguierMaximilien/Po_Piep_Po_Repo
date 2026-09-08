#ifndef UART_TERMINAL_H
#define UART_TERMINAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f3xx_hal.h"  // Adaptez selon votre série STM32

/* Définitions ---------------------------------------------------------------*/
#define UART_TERMINAL_RX_BUFFER_SIZE  256
#define UART_TERMINAL_CMD_BUFFER_SIZE  64

#define UART_TERMINAL_WELCOME "\r\nUART Terminal ready\r\n"
#define UART_TERMINAL_VERSION "\r\nUART Terminal version 1.0\r\n"

/* Types ---------------------------------------------------------------------*/
typedef struct {
  uint8_t rx_buffer[UART_TERMINAL_RX_BUFFER_SIZE];
  uint16_t write_index;
  uint16_t read_index;
  uint8_t cmd_buffer[UART_TERMINAL_CMD_BUFFER_SIZE];
  uint8_t cmd_ready;
  uint8_t last_char_was_cr;
  UART_HandleTypeDef *huart;
} uart_terminal_t;

typedef void (*command_handler_t)(int argc,char* arg[]);



/* Fonctions publiques -------------------------------------------------------*/

/**
  * @brief  Initialise le terminal UART
  * @param  huart: Handle de l'UART à utiliser
  * @retval HAL status
  */
HAL_StatusTypeDef UART_Terminal_Init(UART_HandleTypeDef *huart);

/**
  * @brief  Démarre la réception DMA
  * @retval HAL status
  */
HAL_StatusTypeDef UART_Terminal_Start(void);

/**
  * @brief  Traite les commandes reçues
  * @note   À appeler régulièrement dans la boucle principale
  */
void UART_Terminal_Process(void);

/**
  * @brief  Callback pour l'interruption IDLE UART
  * @note   À appeler depuis USARTx_IRQHandler
  */
void UART_Terminal_Callback(void);

/**
  * @brief  Envoie un message via le terminal
  * @param  message: Chaîne à envoyer
  * @retval HAL status
  */
HAL_StatusTypeDef UART_Terminal_Send(const char* message);

/**
  * @brief  Envoie un message formaté via le terminal
  * @param  format: Format printf
  * @param  ...: Arguments variables
  * @retval HAL status
  */
HAL_StatusTypeDef UART_Terminal_Printf(const char* format, ...);

/**
  * @brief  Affiche le prompt
  */
void UART_Terminal_Show_Prompt(void);

/**
  * @brief  Enregistre un handler de commandes personnalisé
  * @param  handler: Fonction de callback pour traiter les commandes
  */
void UART_Terminal_Register_Command_Handler(command_handler_t handler);

/**
  * @brief  Handler de commandes par défaut
  * @param  command: Commande à traiter
  */
void UART_Terminal_Default_Command_Handler(int argc,char* arg[]);

void UART_Terminal_Timeout_Callback(void);

void UART_Terminal_Check_Timeout(void);

void UART_Terminal_Parse_Command(const char* command, int* argc, char* argv[10]);

#ifdef __cplusplus
}
#endif

#endif /* UART_TERMINAL_H */

#include "uart_terminal.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
/* Variables privées ---------------------------------------------------------*/
static uart_terminal_t terminal;
static command_handler_t custom_command_handler = NULL;
static uint32_t last_receive_time = 0;
static uint8_t reception_active = 0;

/* Fonctions privées ---------------------------------------------------------*/
static void Process_Received_Data(uint16_t dma_pos);
static uint8_t Is_EndOfLine(uint8_t current_char, uint8_t *last_char_was_cr);
static void Process_Command_Internal(const char *command);

/* Fonctions publiques -------------------------------------------------------*/

/**
 * @brief  Initialise le terminal UART
 */
HAL_StatusTypeDef UART_Terminal_Init(UART_HandleTypeDef *huart) {
	// Vérification des paramètres
	if (huart == NULL) {
		return HAL_ERROR;
	}

	// Initialisation de la structure
	memset(&terminal, 0, sizeof(terminal));
	terminal.last_char_was_cr = 0;
	terminal.huart = huart;

	// Démarrage de la réception DMA
	return UART_Terminal_Start();
}

/**
 * @brief  Démarre la réception DMA
 */
HAL_StatusTypeDef UART_Terminal_Start(void) {
	if (terminal.huart == NULL) {
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status = HAL_UARTEx_ReceiveToIdle_DMA(terminal.huart,
			terminal.rx_buffer,
			UART_TERMINAL_RX_BUFFER_SIZE);

	if (status == HAL_OK) {

		// Message de bienvenue
		UART_Terminal_Send(UART_TERMINAL_WELCOME);
		UART_Terminal_Show_Prompt();
	}

	return status;
}

/**
 * @brief  Callback pour l'interruption IDLE UART
 */
void UART_Terminal_Callback(void) {

	// Calcul de la position actuelle dans le buffer DMA
	uint16_t dma_pos = UART_TERMINAL_RX_BUFFER_SIZE
			- __HAL_DMA_GET_COUNTER(terminal.huart->hdmarx);

	// Traitement des données reçues
	Process_Received_Data(dma_pos);

}

/**
 * @brief  Traite les commandes reçues
 */
void UART_Terminal_Process(void) {

	if (terminal.cmd_ready) {
		Process_Command_Internal((char*) terminal.cmd_buffer);
		terminal.cmd_ready = 0;
	}
}

/**
 * @brief  Envoie un message via le terminal
 */
HAL_StatusTypeDef UART_Terminal_Send(const char *message) {
	if (terminal.huart == NULL || message == NULL) {
		return HAL_ERROR;
	}

	return HAL_UART_Transmit(terminal.huart, (uint8_t*) message,
			strlen(message), 100);
}

/**
 * @brief  Envoie un message formaté via le terminal
 */
HAL_StatusTypeDef UART_Terminal_Printf(const char *format, ...) {
	if (terminal.huart == NULL || format == NULL) {
		return HAL_ERROR;
	}

	char buffer[128];
	va_list args;
	va_start(args, format);
	int len = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (len > 0 && len < (int) sizeof(buffer)) {
		return HAL_UART_Transmit(terminal.huart, (uint8_t*) buffer, len, 100);
	}

	return HAL_ERROR;
}

/**
 * @brief  Affiche le prompt
 */
void UART_Terminal_Show_Prompt(void) {
	UART_Terminal_Send("\r\n> ");
}

/**
 * @brief  Enregistre un handler de commandes personnalisé
 */
void UART_Terminal_Register_Command_Handler(command_handler_t handler) {
	custom_command_handler = handler;
}

/**
 * @brief  Handler de commandes par défaut
 * argc : nombre d'arguments sur la ligne de commande
 * argv : tableau de argc chaînes de caractères
 *
 * exemple : "cmd A 123" -> argc=3 , argv[0]="cmd", argv[1]="A", argv[2]="123"
 */
void UART_Terminal_Default_Command_Handler(int argc, char *argv[]) {
	static int debug = 0;
	if (debug) {
		UART_Terminal_Printf("\r\ndebug : argc = %d\r\n", argc);
		for (int i = 0; i < argc; i++) {
			UART_Terminal_Printf("debug : arg%d : %s\r\n", i, argv[i]);
		}
	}
	if (argc == 0)
		return;

	if (strcmp(argv[0], "help") == 0) {
		UART_Terminal_Send("Available commands:\r\n");
		UART_Terminal_Send("  help    - Show this help\r\n");
		UART_Terminal_Send("  version - Show firmware version\r\n");
		UART_Terminal_Send("  clear   - Clear screen\r\n");
		UART_Terminal_Send(
				"  debug <state> - state=on : print command parsing , state=off no print\r\n");
		UART_Terminal_Send("  example <n> <p> - Parameter example command\r\n");
		return;
	}
	if (strcmp(argv[0], "version") == 0) {
		UART_Terminal_Send(UART_TERMINAL_VERSION);
		return;
	}
	if (strcmp(argv[0], "clear") == 0) {
		UART_Terminal_Send("\033[2J\033[H");  // Codes ANSI pour effacer l'écran
		return;
	}
	if (strcmp(argv[0], "debug") == 0) {
		if (argc != 2) {
			UART_Terminal_Printf(
					"\r\ndebug command must have exactly 1 parameter (%d are given)\r\n",
					argc - 1);
			return;
		}
		if (strcmp(argv[1], "on") == 0) {
			debug = 1;
			UART_Terminal_Printf("\r\ndebug mode on\r\n");
		} else if (strcmp(argv[1], "off") == 0) {
			debug = 0;
			UART_Terminal_Printf("\r\ndebug mode off\r\n");
		} else {
			UART_Terminal_Printf("\r\ndebug parameter must be on or off\r\n");

		}
		return;
	}
	if (strcmp(argv[0], "example") == 0) {
		if (argc != 3) {
			UART_Terminal_Printf(
					"\r\nexample command must have exactly 2 parameters  (%d are given)\r\n",
					argc - 1);
			return;
		}
		int n = strtoul(argv[1], NULL, 10); // conversion chaîne en entier
		int p = strtoul(argv[2], NULL, 10);
		UART_Terminal_Printf("\r\nexample parameters are n=%d p=%d\r\n", n, p);
		return;
	}

	UART_Terminal_Printf("\r\nUnknown command: ");
	for (int i = 0; i < argc; i++) {
		UART_Terminal_Printf("%s ", argv[i]);
	}
	UART_Terminal_Printf("\r\n");
	UART_Terminal_Send("Type 'help' for available commands\r\n");

}

/* Fonctions privées ---------------------------------------------------------*/

/**
 * @brief  Traitement des données reçues
 */
static void Process_Received_Data(uint16_t dma_pos) {
	uint16_t bytes_to_process = 0;
	// Calcul du nombre d'octets à traiter
	if (dma_pos >= terminal.write_index) {
		bytes_to_process = dma_pos - terminal.write_index;
	} else {
		bytes_to_process = UART_TERMINAL_RX_BUFFER_SIZE - terminal.write_index
				+ dma_pos;
	}

	// Traitement octet par octet
	for (uint16_t i = 0; i < bytes_to_process; i++) {
		uint8_t received_char = terminal.rx_buffer[terminal.write_index];

		// Détection de fin de ligne
		if (Is_EndOfLine(received_char, &terminal.last_char_was_cr)) {
			if (terminal.read_index > 0)  // Commande non vide
					{
				terminal.cmd_buffer[terminal.read_index] = '\0';
				terminal.cmd_ready = 1;
				terminal.read_index = 0;
			}
		} else if (received_char == '\b' || received_char == 0x7F)  // Backspace
				{
			if (terminal.read_index > 0) {
				terminal.read_index--;
				// Écho du backspace
				HAL_UART_Transmit(terminal.huart, (uint8_t*) "\b \b", 3, 10);
			}
		} else if (received_char >= 32 && received_char <= 126) // Caractères imprimables
				{
			if (terminal.read_index < (UART_TERMINAL_CMD_BUFFER_SIZE - 1)) {
				terminal.cmd_buffer[terminal.read_index++] = received_char;
				// Écho du caractère
				HAL_UART_Transmit(terminal.huart, &received_char, 1, 10);
			} else {
				// Buffer de commande plein
				UART_Terminal_Send("\r\nCommand buffer full! Press ENTER\r\n");
				terminal.read_index = 0;
			}
		}

		// Mise à jour de l'index d'écriture
		terminal.write_index = (terminal.write_index + 1)
				% UART_TERMINAL_RX_BUFFER_SIZE;
	}
}

/**
 * @brief  Détecte les fins de ligne
 */
static uint8_t Is_EndOfLine(uint8_t current_char, uint8_t *last_char_was_cr) {
	if (current_char == '\r') {
		*last_char_was_cr = 1;
		return 1;
	} else if (current_char == '\n') {
		if (*last_char_was_cr) {
			*last_char_was_cr = 0;
			return 0;  // \n fait partie de \r\n, on l'ignore
		}
		*last_char_was_cr = 0;
		return 1;
	} else {
		*last_char_was_cr = 0;
		return 0;
	}
}

/**
 * @brief  Traitement interne des commandes
 */
static void Process_Command_Internal(const char *command) {
  char * argv[10];
  int argc;

	UART_Terminal_Parse_Command(command, &argc, argv);
	if (custom_command_handler != NULL) {
		custom_command_handler(argc, argv);
	} else {
		UART_Terminal_Default_Command_Handler(argc, argv);
	}

	UART_Terminal_Show_Prompt();
}
/**
 * @brief  Callback de réception UART
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == terminal.huart) {
		last_receive_time = HAL_GetTick();
		reception_active = 1;
	}
}

/**
 * @brief  Vérification du timeout de réception
 */
void UART_Terminal_Check_Timeout(void) {
	if (reception_active && (HAL_GetTick() - last_receive_time > 10)) // 10ms timeout
			{
		reception_active = 0;
		UART_Terminal_Timeout_Callback();
	}
}

/**
 * @brief  Callback de timeout
 */
void UART_Terminal_Timeout_Callback(void) {
	uint16_t dma_pos = UART_TERMINAL_RX_BUFFER_SIZE
			- __HAL_DMA_GET_COUNTER(terminal.huart->hdmarx);

	Process_Received_Data(dma_pos);
}

/**
 * @brief  Parse une commande avec strtok
 * @param  command: Commande à parser
 * @param  argc: Pointeur pour stocker le nombre d'arguments
 * @param  argv: Tableau pour stocker les arguments (max 10)
 */
void UART_Terminal_Parse_Command(const char *command, int *argc, char *argv[10]) {
	if (command == NULL) {
		*argc = 0;
		return;
	}

	char *token;
	int i = 0;

	// Copie de la commande pour éviter de modifier l'original
	// la copie doit être faite dans une adresse statique car
	// les adresses sotckées dans argv doivent pouvoir être utilisées dans d'autres fonctions.
	static char cmd_copy[UART_TERMINAL_CMD_BUFFER_SIZE];
	strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
	cmd_copy[sizeof(cmd_copy) - 1] = '\0';

	// Premier token (commande)
	token = strtok(cmd_copy, " \t");

	while (token != NULL && i < 10) {
		argv[i++] = token;
		token = strtok(NULL, " \t");  // Tokens suivants
	}

	*argc = i;
}

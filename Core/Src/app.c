#include "app.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "main.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_dma.h"
#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_uart.h"
#include "stm32g4xx_hal_uart_ex.h"
#include "usart.h"

#define UART_RX_BUFFER_SIZE 128
#define UART_TX_BUFFER_SIZE 128

static uint8_t uartRXBuffer[UART_RX_BUFFER_SIZE] = {0};
static uint8_t* uartRXBufferLastMessagePtr = uartRXBuffer;
static uint8_t* const uartRXBufferEnd = uartRXBuffer + UART_RX_BUFFER_SIZE;
static uint8_t uartTXBuffer[UART_TX_BUFFER_SIZE] = {0};

static uint8_t uartMessageBuffer[UART_RX_BUFFER_SIZE + 1] = {0};
static size_t uartMessageLength = 0;

char const* helloMessage = "Hello, world!\n";
char const* helloDMAMessage = "DMA is working!\n";

#define NEWLINE_TESTS_COUNT 4
char const* newlineTests[NEWLINE_TESTS_COUNT] = {"This is split \nusing newline",
                                                 "This is split \rusing carriage return",
                                                 "This is split \n\rusing newline and carriage return",
                                                 "This is split \r\nusing carriage return and newline"};

size_t copyUARTMessage(uint8_t* destinationBuffer, bool terminate) {
    uint8_t* const messageEndPtr = uartRXBuffer + UART_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(hlpuart1.hdmarx);

    if (uartRXBufferLastMessagePtr <= messageEndPtr) {
        // no overflow
        size_t const messageLength = messageEndPtr - uartRXBufferLastMessagePtr;

        memcpy(destinationBuffer, uartRXBufferLastMessagePtr, messageLength);
        if (terminate) {
            destinationBuffer[messageLength] = '\0';
        }

        uartRXBufferLastMessagePtr = messageEndPtr;
        return messageLength;
    } else {
        // yes overflow
        size_t const messagePrefixLength = uartRXBufferEnd - uartRXBufferLastMessagePtr;
        size_t const messageSuffixLength = messageEndPtr - uartRXBuffer;
        size_t const messageLength = messagePrefixLength + messageSuffixLength;

        memcpy(destinationBuffer, uartRXBufferLastMessagePtr, messagePrefixLength);
        memcpy(destinationBuffer + messagePrefixLength, uartRXBuffer, messageSuffixLength);

        if (terminate) {
            destinationBuffer[messageLength] = '\0';
        }

        uartRXBufferLastMessagePtr = messageEndPtr;
        return messageLength;
    }
}

void handleUARTMessage() {
    if (uartMessageLength > 0) {
        if (strcmp((char const*)uartMessageBuffer, "newline_test") == 0) {
            for (size_t i = 0; i < NEWLINE_TESTS_COUNT; i++) {
                HAL_UART_Transmit(&hlpuart1, (uint8_t*)newlineTests[i], strlen(newlineTests[i]), HAL_MAX_DELAY);
            }
        } else {
            HAL_UART_Transmit(&hlpuart1, uartMessageBuffer, uartMessageLength, HAL_MAX_DELAY);
        }

        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        uartMessageLength = 0;
    }
}

void setup() {
    HAL_UART_Transmit(&hlpuart1, (uint8_t*)helloMessage, strlen(helloMessage), HAL_MAX_DELAY);
    HAL_UART_Transmit_DMA(&hlpuart1, (uint8_t*)helloDMAMessage, strlen(helloDMAMessage));

    HAL_UARTEx_ReceiveToIdle_DMA(&hlpuart1, uartRXBuffer, UART_RX_BUFFER_SIZE);
}

void loop() { handleUARTMessage(); }

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size) {
    uartMessageLength = copyUARTMessage(uartMessageBuffer, true);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {}
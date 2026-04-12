#include "uart.h"

void USART2_Init(void) {
	// Enable GPIOA clock (You may already have this in main, but it's safe to call again)
	RCC->AHB1ENR |= (1U << 0);

	// Enable USART2 clock (USART2 is on APB1)
	RCC->APB1ENR |= (1U << 17);

	// Configure PA2 (TX) for Alternate Function mode
	GPIOA->MODER &= ~(3U << 4);  // Clear bits 4 and 5
	GPIOA->MODER |= (2U << 4);   // Set to Alternate Function (10)

	// Set Alternate Function type to AF7 (USART2) for Pin PA2
	GPIOA->AFR[0] &= ~(15U << 8); // Clear AF bits for PA2
	GPIOA->AFR[0] |= (7U << 8);   // Set AF7 (0111)

	// Configure Baudrate (Target: 115200 baud)
	// Assuming default 16MHz APB1 clock: 16,000,000 / 115200 = ~138.89 (0x8A in hex)
	USART2->BRR = 0x008A;

	// Enable Transmitter (TE) and enable USART (UE)
	USART2->CR1 |= (1U << 3);  // TE
	USART2->CR1 |= (1U << 13); // UE
}

// Redirect printf() to USART2
int __io_putchar(int ch) {
	// Wait for the TXE (Transmit Data Register Empty) bit to be set
	while (!(USART2->SR & (1U << 7)));

	// Load the character into the Data Register
	USART2->DR = (ch & 0xFF);

	return ch;
}

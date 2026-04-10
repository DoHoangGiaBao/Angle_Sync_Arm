#include "mpu6050.h"

void I2C1_config(void) {
	// Enable Port B Clock
	RCC->AHB1ENR |= (1U << 1);

	// Set PB8 (SLC) and PB9 (SDA) to Alternate Function Mode
	GPIOB->MODER &= ~((3U << 16) | (3U << 18));
	GPIOB->MODER |= (2U << 16) | (2U << 18);

	// Set PB8 and PB9 to Open-Drain
	GPIOB->OTYPER &= ~((1U << 8) | (1U << 9));
	GPIOB->OTYPER |= (1U << 8) | (1U << 9);

	// Set PB8 and PB9 to High Speed
	GPIOB->OSPEEDR |= (2U << 16) | (2U << 18);

	// Unable internal Pull-up resistors for PB8 and PB9
	GPIOB->PUPDR &= ~((3U << 16) | (3U << 18));
	GPIOB->PUPDR |= (1U << 16) | (1U << 18);

	// Connect PB8 and PB9 to Alternate Function 4 (AF4 is I2C1 for these pins)
	GPIOB->AFR[1] &= ~((15U << 0) | (15U << 4));
	GPIOB->AFR[1] |= (4U << 0) | (4U << 4);

	// Enable I2C1 clock
	RCC->APB1ENR |= (1U << 21);

	// Software reset I2C1
	I2C1->CR1 |= (1U << 15);
	I2C1->CR1 &= ~(1U << 15);

	// Inform IC21 of APB1 6 frequency (ABP1 current clock speed is 16MHz)
	I2C1->CR2 |= (16U << 0);

	// Set Clock Control Register for Standard Mode (100kHz)
	// Formula: CCR = (T_{r(SCL)} + T_{w(SCLH)}) * F_{PCLKx} (T_{r(SCL)}, T_{w(SCLH)} can be found in datasheet)
	I2C1->CCR = (1 + 4) * 16;

	// Set Maximum Rise Time
	// Formula: TRISE = T_{r(SCL)} * F_{PCLKx} + 1
	I2C1->TRISE = 1 * 16 + 1;

	// Enable IC21 Peripheral
	I2C1->CR1 |= (1U << 0);
}

void I2C1_start(void) {
	// Generate START
	I2C1->CR1 |= (1U << 8);

	// Wait for SB bit to set
	while(!(I2C1->SR1 & (1U << 0)));
}

void I2C1_stop(void) {
	// Stop I2C1
	I2C1->CR1 |= (1U << 9);
}

void I2C1_write(uint8_t data) {
	// Wait for TXE bit to set
	while(!(I2C1->SR1 & (1U << 7)));

	// Write data to Data Register
	I2C1->DR = data;

	// Wait for BTF to set
	while(!(I2C1->SR1 & (1U << 2)));
}

void I2C1_send_address(uint8_t address) {
	// Set address to Data Register
	I2C1->DR = address;

	// Wait for ADDR bit to set
	while(!(I2C1->SR1 & (1U << 1)));

	// Read SR1 and SR2 to clear ADDR bit
	(void)(I2C1->SR1 | I2C1->SR2);
}

void I2C1_read_multi_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t size) {
	I2C1_start(); // Start I2C1
	I2C1_send_address(dev_addr << 1); // Send device salve address: 7-bit address + Write bit (0)
	I2C1_write(reg_addr); // Write address

	// Restart for Read
	I2C1_start();
	I2C1_send_address((dev_addr << 1) | 1); // Send device salve address: 7-bit address + Read bit (1)

	// Enable Acknowledge
	I2C1->CR1 |= (1U << 10);

	// Read loop
	while(size) {
		if (size == 1) {
			// Last byte to read: Disable ACK and generate STOP
			I2C1->CR1 &= ~(1U << 10);
			I2C1_stop();
		}

		while(!(I2C1->SR1 & (1U << 6))); // Wait for RxNE bit to set (Data register not empty)

		*data++ = I2C1->DR; // Read data
		size--;
	}
}

void MPU6050_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
	I2C1_start();
	I2C1_send_address(dev_addr << 1);
	I2C1_write(reg_addr);
	I2C1_write(data);
	I2C1_stop();
}

void MPU6050_Init(void) {
	// Wake up the sensor (write 0 to Power Management 1)
	MPU6050_WriteReg(MPU_ADDR, WAKE_REG_ADDR, 0);

	// Set Digital Low Pass Filter (DLPF) to ~42Hz
	MPU6050_WriteReg(MPU_ADDR, DLPF_REG_ADDR, 3);

	// Set Gyroscope Full Scale Range to ±500 deg/s
	MPU6050_WriteReg(MPU_ADDR, GYRO_REG_ADDR, 1);

	// Set Accelerometer Full Scale Range to ±2g
	MPU6050_WriteReg(MPU_ADDR, ACCEL_REG_ADDR, 0);
}

uint8_t MPU6050_Test_Connection(void) {
	uint8_t who_am_i = 0;

	// Read 1 byte from the WHO_AM_I register
	I2C1_read_multi_byte(MPU_ADDR, WHO_AM_I_REG_ADDR, &who_am_i, 1);

	// The MPU6050 should always return 0x68
	if (who_am_i == 0x68) {
		return 1; // Success: MCU is communicating with the sensor
	}

	return 0; // Failure: Sensor not found or I2C bus error
}

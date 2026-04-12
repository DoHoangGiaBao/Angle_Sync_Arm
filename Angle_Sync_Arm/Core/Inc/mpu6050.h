#ifndef INC_MPU6050_H_
#define INC_MPU6050_H_

#include <stm32f4xx_hal.h>

#define MPU_ADDR 0x68
#define WAKE_REG_ADDR 0x6B
#define DLPF_REG_ADDR 0x1A
#define GYRO_REG_ADDR 0x1B
#define ACCEL_REG_ADDR 0x1C
#define WHO_AM_I_REG_ADDR 0x75
#define DATA_REG_ADDR 0x3B

void I2C1_config();

// I2C Configuration and Primitives
void I2C1_config(void);
void I2C1_start(void);
void I2C1_stop(void);
void I2C1_write(uint8_t data);
void I2C1_send_address(uint8_t address);
void I2C1_read_multi_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t size);

// MPU-6050 Functions
void MPU6050_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
void MPU6050_Init(void);
uint8_t MPU6050_Test_Connection(void);

#endif /* INC_MPU6050_H_ */

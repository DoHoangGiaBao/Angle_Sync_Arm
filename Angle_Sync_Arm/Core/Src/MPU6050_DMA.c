#include "stm32f4xx.h"
#include <math.h>
#include "MPU6050_DMA.h"

uint8_t mpu_data_buffer[14];
volatile float pitch = 0;
volatile float roll = 0;
volatile float yaw = 0;
volatile int obstacle_detected = 0; // DÙNG CHUNG BIẾN NÀY CHO TOÀN HỆ THỐNG
volatile uint8_t dma_transfer_complete = 0; // Cờ báo hiệu DMA đã xong

const float alpha = 0.98f;
const float dt = 0.005f;

/* PB8 (SCL), PB9 (SDA) */
void I2C1_Init() {
    RCC->AHB1ENR |= (1U << 1);
    RCC->APB1ENR |= (1U << 21);

    GPIOB->MODER &= ~((3U << 16) | (3U << 18));
    GPIOB->MODER |= (2U << 16) | (2U << 18);
    GPIOB->OTYPER |= (1U << 8) | (1U << 9);
    GPIOB->OSPEEDR |= (3U << 16) | (3U << 18);
    GPIOB->PUPDR &= ~((3U << 16) | (3U << 18));
    GPIOB->PUPDR |= (1U << 16) | (1U << 18);
    GPIOB->AFR[1] |= (4U << 0) | (4U << 4);

    I2C1->CR1 |= (1U << 15);
    I2C1->CR1 &= ~(1U << 15);

    I2C1->CR2 = 16;
    I2C1->CCR &= ~0xFFF;
    I2C1->CCR |= (1U << 15) | (1U << 14);
    I2C1->CCR |= 13;
    I2C1->TRISE = 6;

    I2C1->CR1 |= (1U << 0);
}

void DMA1_Init() {
    RCC->AHB1ENR |= (1U << 21);

    DMA1_Stream0->CR &= ~(1U << 0);
    while (DMA1_Stream0->CR & (1U << 0));

    DMA1_Stream0->PAR = (uint32_t)&(I2C1->DR);
    DMA1_Stream0->M0AR = (uint32_t)mpu_data_buffer;

    DMA1_Stream0->CR = 0;
    DMA1_Stream0->CR |= (1U << 25);
    DMA1_Stream0->CR |= (1U << 10);
    // ĐÃ XÓA BIT CIRCULAR Ở ĐÂY ĐỂ TRÁNH XUNG ĐỘT I2C
    DMA1_Stream0->CR |= (1U << 4);

    NVIC_SetPriority(DMA1_Stream0_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

void MPU6050_Write_Register(uint8_t reg, uint8_t data) {
    I2C1->CR1 |= (1U << 8);
    while (!(I2C1->SR1 & (1U << 0)));

    I2C1->DR = (0x68 << 1); // địa chỉ
    while (!(I2C1->SR1 & (1U << 1)));
    (void)I2C1->SR2;

    I2C1->DR = reg;
    while (!(I2C1->SR1 & (1U << 7)));

    I2C1->DR = data;
    while (!(I2C1->SR1 & (1U << 7)));

    I2C1->CR1 |= (1U << 9);
}

void MPU6050_Init() {
    MPU6050_Write_Register(0x6B, 0x00); // PWR_MGMT_1
    MPU6050_Write_Register(0x1A, 0x03); // CONFIG
    MPU6050_Write_Register(0x1C, 0x00); // ACCEL_CONFIG
    MPU6050_Write_Register(0x1B, 0x00); // GYRO_CONFIG
    MPU6050_Write_Register(0x19, 0x04); // SMPRT_DIV
}

void MPU6050_Start_DMA_Read() {
    dma_transfer_complete = 0; // Xóa cờ hoàn thành

    /* Reset DMA stream cho nhịp đọc mới */
    DMA1_Stream0->CR &= ~(1U << 0);
    while (DMA1_Stream0->CR & (1U << 0));
    DMA1->LIFCR = 0x3DUL; // Xóa mọi cờ lỗi DMA
    DMA1_Stream0->NDTR = 14;
    DMA1_Stream0->CR |= (1U << 0);

    /* Cấp quyền DMA cho I2C */
    I2C1->CR2 |= (1U << 11) | (1U << 12); // Bật DMAEN và LAST

    /* Tạo tín hiệu START và gửi lệnh đọc */
    I2C1->CR1 |= (1U << 10);
    I2C1->CR1 |= (1U << 8);
    while (!(I2C1->SR1 & (1U << 0)));

    I2C1->DR = (0x68 << 1);
    while (!(I2C1->SR1 & (1U << 1)));
    (void)I2C1->SR2;

    I2C1->DR = 0x3B; // ACCEL_XOUT_H
    while (!(I2C1->SR1 & (1U << 7)));

    I2C1->CR1 |= (1U << 8);
    while (!(I2C1->SR1 & (1U << 0)));

    I2C1->DR = (0x68 << 1) | 0x01;
    while (!(I2C1->SR1 & (1U << 1)));
    (void)I2C1->SR2;
}

void DMA1_Stream0_IRQHandler(void) {
    if (DMA1->LISR & (1U << 5)) {
        DMA1->LIFCR |= (1U << 5);

        // 1. Tắt DMA Stream và ngắt I2C DMA để nhường bus cho LCD
        DMA1_Stream0->CR &= ~(1U << 0);
        I2C1->CR2 &= ~((1U << 11) | (1U << 12));

        // 2. TẠO TÍN HIỆU STOP BẮT BUỘC
        I2C1->CR1 |= (1U << 9);

        // 3. Tính toán math nếu không có vật cản
        if (!obstacle_detected) {
            // SỬA LỖI 1: Sử dụng int16_t (có dấu) để giữ được số âm
            int16_t raw_ax = (int16_t)(mpu_data_buffer[0] << 8 | mpu_data_buffer[1]);
            int16_t raw_ay = (int16_t)(mpu_data_buffer[2] << 8 | mpu_data_buffer[3]);
            int16_t raw_az = (int16_t)(mpu_data_buffer[4] << 8 | mpu_data_buffer[5]);

            // SỬA LỖI 2 & 3: Áp dụng lại chính xác công thức nguyên bản của bạn
            // Bỏ qua Gyroscope để tránh hiện tượng lag do sai lệch dt vòng lặp
            roll  = atan2f((float)raw_ay, (float)raw_az) * 57.29578f;
            pitch = atan2f(-(float)raw_ax, (float)raw_az) * 57.29578f;
        }

        // Báo hiệu cho hàm main() biết đã xong
        dma_transfer_complete = 1;
    }
}

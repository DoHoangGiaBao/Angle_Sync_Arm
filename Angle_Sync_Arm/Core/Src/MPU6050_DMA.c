#include "stm32f4xx.h"
#include <math.h>
#include "MPU6050_DMA.h"
// Khai báo buffẻr gồm 14 bytes dữ liệu
uint8_t mpu_data_buffer[14];
// Giá trị roll, pitch, yaw
volatile float pitch = 0;
volatile float roll = 0;
volatile float yaw = 0;
volatile int obstacle_detected = 0; // DÙNG CHUNG BIẾN NÀY CHO TOÀN HỆ THỐNG
volatile uint8_t dma_transfer_complete = 0; // Cờ báo hiệu DMA đã xong

const float alpha = 0.98f;
const float dt = 0.005f;

/* PB8 (SCL), PB9 (SDA) */
void I2C1_Init() {
	// Cấp xung clock cho Port B
    RCC->AHB1ENR |= (1 << 1);
    // I2C1 EN
    RCC->APB1ENR |= (1 << 21);
    // Reset cấu hình cũ PB8, PB9
    GPIOB->MODER &= ~((3 << 16) | (3 << 18));
    // Cấu hình Alternate function
    GPIOB->MODER |= (2 << 16) | (2 << 18);
    // Chế độ Open drain (cực máng hở)
    GPIOB->OTYPER |= (1 << 8) | (1 << 9);
    // Tốc độ truyền Very High Speed
    GPIOB->OSPEEDR |= (3 << 16) | (3 << 18);
    // Cấu hình chế độ pull up
    GPIOB->PUPDR &= ~((3 << 16) | (3 << 18));
    GPIOB->PUPDR |= (1 << 16) | (1 << 18);
    // AFR[1] quản lý từ 8 đến 15
    // 4 => I2C1_SCL, I2C_SDA
    GPIOB->AFR[1] |= (4 << 0) | (4 << 4);

    // Reset trạng thái bus I2C1
    I2C1->CR1 |= (1 << 15);
    // Bit 15 không tự quay về 0, phải chèn 0 vào bit 15 trở lại
    I2C1->CR1 &= ~(1 << 15);

    // Cho I2C1 biết clock của vdk là 16MHz
    I2C1->CR2 = 16;
    // Reset thanh ghi, dữ 4 bit cao
    I2C1->CCR &= ~0xFFF;
    // Chọn Fast mode (Max 400kHz + DUTY (Low/High = 16/9)
    I2C1->CCR |= (1U << 15) | (1U << 14);
    // tần số xung SCL = 16M/(25 * CCR) = 50kHz
    I2C1->CCR |= 13;
    // Cấu hình để I2C1 biêt thời gian sườn lên
    // Công thức: TRISE = (Max_Rise_Time / T PCLK 1) + 1
    // 350/62.5 + 1 = 6 (lấy phần nguyên)
    I2C1->TRISE = 6;
    // EN Pheripheral
    I2C1->CR1 |= (1 << 0);
}

void DMA1_Init() {
	// EN DMA1
    RCC->AHB1ENR |= (1 << 21);
    // Tắt bộ DMA1 để cấu hình
    DMA1_Stream0->CR &= ~(1 << 0);
    // CPU phải chờ cho đến khi đã tắt bit về 0
    // Để đợi cho DMA1 truyền nốt gói dữ liệu dang dở
    while (DMA1_Stream0->CR & (1 << 0));

    // Nạp địa chỉ là địa chỉ của ngoại vi I2C
    // Khi MPU gửi 1 byte về, byte đó nằm trong thanh ghi DR này
    DMA1_Stream0->PAR = (uint32_t)&(I2C1->DR);
    // Địa chỉ bộ nhớ RAM (đích): là biến ta khao báo trong RAM
    // Mục đích: Lấy được dữ liệu từ I2C1 => cho vào trong buffer của RAM
    DMA1_Stream0->M0AR = (uint32_t)mpu_data_buffer;

    // Reset control
    DMA1_Stream0->CR = 0;
    // Chọn DMA kênh 1
    // Quy ước: dữ liệu I2C1 phải đi qua DMA1 Stream0
    DMA1_Stream0->CR |= (1 << 25);
    // MÍNC: tăng địa chỉ bộ nhớ
    // DMA tự động tằng con trỏ bộ nhớ khi có dữ liệu mới
    DMA1_Stream0->CR |= (1 << 10);
    // Bit TCIE: ngắt khi truyền xong 14 bytes dữ liệu
    DMA1_Stream0->CR |= (1 << 4);
    // Đặt ưu tiên
    NVIC_SetPriority(DMA1_Stream0_IRQn, 1);
    // EN ngắt ngoại vi
    NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

// Hàm giúp i2c cấu hình MPU
void MPU6050_Write_Register(uint8_t reg, uint8_t data) {
	// Tín hiệu START
    I2C1->CR1 |= (1 << 8);
    // Chờ đên khi SB bật lên 1 => đã có START
    while (!(I2C1->SR1 & (1 << 0)));
    // 7 bit địa chỉ MPU, bit cuói = 0 I2C muốn ghi vào thanh ghi của MPU
    I2C1->DR = (0x68 << 1); // địa chỉ
    // Chờ ADDR lên 1, sau đó set ADDR = 0 bằng cách đoc 2 thanh ghi SR1, SR2
    while (!(I2C1->SR1 & (1 << 1)));
    (void)I2C1->SR2;
    // nạp địa chỉ thanh ghi cần ghi
    I2C1->DR = reg;
    // Chờ cờ TXE = 1, khi đó dữ liệu từ DR đã đẩy sang Shift reg
    while (!(I2C1->SR1 & (1 << 7)));
    // Lúc này DR trống, nạp dữ liệu mới vào
    I2C1->DR = data;
    while (!(I2C1->SR1 & (1 << 7)));
    // Tín hiệu STOP dừng truyền
    I2C1->CR1 |= (1U << 9);
}

void MPU6050_Init() {
	// Đánh thức MPU và dùng xung 8MHz nội bộ
    MPU6050_Write_Register(0x6B, 0x00); // PWR_MGMT_1
    // Kích hoạt bộ lọc thông thấp
    MPU6050_Write_Register(0x1A, 0x03); // CONFIG
    // full scale
    MPU6050_Write_Register(0x1C, 0x00); // ACCEL_CONFIG
    // full scale
    MPU6050_Write_Register(0x1B, 0x00); // GYRO_CONFIG
    // Sample rate = (Gyro output rate)/(1 + SMPRT_DIV) = 200Hz
    MPU6050_Write_Register(0x19, 0x04); // SMPRT_DIV
}

void MPU6050_Start_DMA_Read() {
    dma_transfer_complete = 0; // Xóa cờ hoàn thành

    // Reset DMA stream cho nhịp đọc mới
    DMA1_Stream0->CR &= ~(1 << 0);
    // Đợi cho tắt hẳn
    while (DMA1_Stream0->CR & (1 << 0));
    DMA1->LIFCR = 0x3DUL; // Xóa mọi cờ lỗi DMA
    DMA1_Stream0->NDTR = 14; // Thiết lập số bytes truyền là 14
    // Bật DMA1
    DMA1_Stream0->CR |= (1 << 0);

    /* Cấp quyền DMA cho I2C */
    // DMAEN: 1 byte lấy vè, DMA hãy lấy byte đó
    // LAST: truyền xong 14 bytes phải đi kèm 1 lệnh NACK
    I2C1->CR2 |= (1 << 11) | (1 << 12); // Bật DMAEN và LAST

    /* Tạo tín hiệu START và gửi lệnh đọc */
    I2C1->CR1 |= (1 << 10); // Khi tryền xong trả lại 1 ACK
    I2C1->CR1 |= (1 << 8); // Tín hiệu START
    while (!(I2C1->SR1 & (1 << 0))); // Chờ cờ SB bật lên 1

    // i2c muốn ghi vào slave có địa chỉ này
    I2C1->DR = (0x68 << 1);
    // Kiểm tra ADDR và xóa ADDR
    while (!(I2C1->SR1 & (1U << 1)));
    (void)I2C1->SR2;

    I2C1->DR = 0x3B; // gửi địa chỉ thanh ghi bắt đầu ACCEL_XOUT_H
    while (!(I2C1->SR1 & (1 << 7))); // Chờ TXE = 1, dữ liệu từ DR đã xuóng thanh ghi dich

    // Repeat START
    I2C1->CR1 |= (1 << 8);
    // Chờ cờ SB = 1
    while (!(I2C1->SR1 & (1 << 0)));
    // I2C1 muốn đọc từ MPU ( bit cuối = 1)
    I2C1->DR = (0x68 << 1) | 0x01;
    // kiểm tra và xóa cờ ADDR
    while (!(I2C1->SR1 & (1U << 1)));
    (void)I2C1->SR2;
}

// Hàm ngắt
void DMA1_Stream0_IRQHandler(void) {
    if (DMA1->LISR & (1 << 5)) { // Kiểm tra TCIF0 : trànfer complete
        DMA1->LIFCR |= (1 << 5); // Xóa cờ chuẩn bị lần ngắt sau

        // 1. Tắt DMA Stream và ngắt I2C DMA để nhường bus cho LCD
        DMA1_Stream0->CR &= ~(1U << 0);
        I2C1->CR2 &= ~((1 << 11) | (1 << 12)); // Tắt EN LAST và DMA

        // 2. TẠO TÍN HIỆU STOP BẮT BUỘC
        I2C1->CR1 |= (1 << 9);

        // 3. Tính toán math nếu không có vật cản
        if (!obstacle_detected) {
            // Sử dụng int16_t (có dấu) để giữ được số âm
        	// Cấu trúc 14 bytes: 6 2 6 | accel temp gyro
            int16_t raw_ax = (int16_t)(mpu_data_buffer[0] << 8 | mpu_data_buffer[1]);
            int16_t raw_ay = (int16_t)(mpu_data_buffer[2] << 8 | mpu_data_buffer[3]);
            int16_t raw_az = (int16_t)(mpu_data_buffer[4] << 8 | mpu_data_buffer[5]);

            // Bỏ qua Gyroscope để tránh hiện tượng lag do sai lệch dt vòng lặp
            roll  = atan2f((float)raw_ay, (float)raw_az) * 57.29578f;
            pitch = atan2f(-(float)raw_ax, (float)raw_az) * 57.29578f;
        }

        // Báo hiệu cho hàm main() biết đã xong
        dma_transfer_complete = 1;
    }
}

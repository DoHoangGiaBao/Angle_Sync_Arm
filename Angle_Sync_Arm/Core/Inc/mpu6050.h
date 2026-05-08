#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f4xx.h" // Hoặc thư viện dòng chip tương ứng (ví dụ stm32f401xe.h)

/* --- Cấu hình I2C & DMA --- */
// Địa chỉ I2C 8-bit của MPU6050 khi chân AD0 nối GND (0x68 << 1)
#define MPU6050_ADDR         0xD0

// Đọc 14 bytes: 6 byte Accel + 2 byte Temp + 6 byte Gyro
#define DMA_BUFFER_SIZE      14

/* --- Địa chỉ các thanh ghi MPU6050 (Registers) --- */
#define REG_SMPLRT_DIV       0x19 // Thanh ghi chia tần số lấy mẫu
#define REG_GYRO_CONFIG      0x1B // Thanh ghi cấu hình Gyro (con quay hồi chuyển)
#define REG_ACCEL_CONFIG     0x1C // Thanh ghi cấu hình Accel (gia tốc kế)
#define REG_ACCEL_XOUT_H     0x3B // Thanh ghi bắt đầu chứa dữ liệu đo (Accel X High byte)
#define REG_PWR_MGMT_1       0x6B // Thanh ghi quản lý nguồn 1 (Dùng để đánh thức cảm biến)

/* --- Khai báo các biến toàn cục (extern để file khác có thể dùng) --- */
extern uint8_t          dma_rx_buffer[DMA_BUFFER_SIZE];
extern int16_t          Accel_X_Raw, Accel_Y_Raw, Accel_Z_Raw;
extern int16_t          Gyro_X_Raw,  Gyro_Y_Raw,  Gyro_Z_Raw;
extern volatile uint8_t dma_transfer_complete;

/* --- Khai báo hàm (Function Prototypes) --- */
void MPU6050_Init(void);
void MPU6050_Read_All_DMA(void);
void MPU6050_Parse_Data(void);

#endif /* MPU6050_H */

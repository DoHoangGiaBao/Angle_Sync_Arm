#include "main.h"

/* Biến lưu trữ góc sau khi tính toán */
float Roll, Pitch;

//void delay_simple(uint32_t delay) {
//    while(delay--) {
//        __NOP();
//    }
//}

int main(void) {
    /* 1. Khởi tạo ngoại vi */
    // Hàm này đã bao gồm khởi tạo I2C, DMA và đánh thức MPU6050
    MPU6050_Init();

    // Khởi tạo PWM cho Servo
    Servo_F401_Init();

    /* 2. Cấu hình mặc định cho Servo về vị trí cân bằng (90 độ) */
    Set_Servo1(90);
    Set_Servo2(90);

    while (1) {
        /* 3. Bắt đầu đọc dữ liệu từ cảm biến qua DMA (Không chặn) */
        MPU6050_Read_All_DMA();

        /* 4. Chờ cho đến khi DMA hoàn tất việc chuyển dữ liệu */
        // Biến dma_transfer_complete được cập nhật trong DMA1_Stream0_IRQHandler
        while (dma_transfer_complete == 0);
        dma_transfer_complete = 0; // Reset cờ

        /* 5. Tính toán góc nghiêng từ dữ liệu thô (Accel_X_Raw...) */
        // Các biến Raw này được driver cập nhật tự động sau khi DMA xong
        Roll  = atan2((float)Accel_Y_Raw, (float)Accel_Z_Raw) * 57.295f;
        Pitch = atan2(-(float)Accel_X_Raw, (float)Accel_Z_Raw) * 57.295f;

        /* 6. Quy đổi góc sang vị trí Servo (Map 0-180) */
        int s1_pos = (int)(Roll + 90);
        int s2_pos = (int)(90 - Pitch);

        /* 7. Giới hạn an toàn (Constrain) */
        if (s1_pos < 0)   s1_pos = 0;
        if (s1_pos > 180) s1_pos = 180;
        if (s2_pos < 0)   s2_pos = 0;
        if (s2_pos > 180) s2_pos = 180;

        /* 8. Điều khiển Servo di chuyển theo góc nghiêng */
        Set_Servo1(s1_pos);
        Set_Servo2(s2_pos);

        /* Delay nhỏ để tránh quá tải bus I2C và giúp Servo kịp phản hồi */
        delay_simple(50000);
    }
}

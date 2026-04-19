#include "stm32f4xx.h"
#include "mpu6050.h"
#include "servo.h"   // LỖI 1: Phải có dòng này để máy hiểu các hàm Servo
#include <math.h>

// Khai báo lại các biến chứa dữ liệu thô
int16_t Accel_X, Accel_Y, Accel_Z;
float Roll, Pitch;

int main(void) {
    // 1. Khởi tạo các ngoại vi
    I2C1_config();
    MPU6050_Init();
    Servo_F401_Init();

    // Kiểm tra kết nối MPU6050
    if (!MPU6050_Test_Connection()) {
        while(1);
    }

    uint8_t buffer[6];

    while (1) {
    	I2C1_read_multi_byte(0x68, 0x3B, buffer, 6);
        // 3. Chuyển đổi dữ liệu thô
        Accel_X = (int16_t)(buffer[0] << 8 | buffer[1]);
        Accel_Y = (int16_t)(buffer[2] << 8 | buffer[3]);
        Accel_Z = (int16_t)(buffer[4] << 8 | buffer[5]);

        // 4. Tính toán góc nghiêng (Đơn vị: Độ)
        Roll  = atan2((float)Accel_Y, (float)Accel_Z) * 57.295f;
        Pitch = atan2(-(float)Accel_X, (float)Accel_Z) * 57.295f;

        // 5. Chuyển đổi (Map) và ĐẶT TÊN BIẾN ĐỒNG NHẤT
        int s1_pos = (int)(Roll + 90);
        int s2_pos = (int)(90 - Pitch);

        // 6. THIẾT LẬP GIỚI HẠN AN TOÀN (LỖI 2: Đã sửa tên biến khớp với trên)
        if (s1_pos < 0)   s1_pos = 0;
        if (s1_pos > 180) s1_pos = 180;

        if (s2_pos < 0)   s2_pos = 0;
        if (s2_pos > 180) s2_pos = 180;

        // 7. Điều khiển Servo
        Set_Servo1(s1_pos);
        Set_Servo2(s2_pos);

        // Delay nhỏ để hệ thống ổn định
        delay_simple(100000);
    }
}

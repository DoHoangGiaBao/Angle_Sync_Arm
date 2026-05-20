#include "main.h"
#include <stdio.h>
/* Biến lưu trữ góc sau khi tính toán */
float Roll, Pitch;
char lcd_buffer[16];
/* Cờ trạng thái từ cảm biến IR */
volatile uint8_t ir_triggered = 0;

void IR_Interrupt_Init(void) {
    // 1. Cấp xung nhịp cho GPIOA và SYSCFG (Bộ chuyển mạch ngắt)
    RCC->AHB1ENR |= (1 << 0);
    RCC->APB2ENR |= (1 << 14);

    // 2. Cấu hình chân PA8 là Input, có điện trở kéo lên (Pull-up)
    // Cảm biến IR thường xuất mức Thấp (0) khi phát hiện vật
    GPIOA->MODER &= ~(3U << 16); // Input
    GPIOA->PUPDR &= ~(3U << 16);
    GPIOA->PUPDR |=  (1U << 16); // Pull-up

    // 3. Kết nối EXTI Line 8 với Port A thông qua SYSCFG
    // PA8 dùng EXTICR3 (tương đương index 2 trong mảng), quản lý bit 3:0
    SYSCFG->EXTICR[2] &= ~(0xFU << 0); // Ghi 0000 để chọn Port A

    // 4. Thiết lập ngắt khi có cạnh xuống (Falling Edge)
    EXTI->FTSR |= (1 << 8);

    // 5. Mở mặt nạ cho phép ngắt trên Line 8
    EXTI->IMR |= (1 << 8);

    // 6. Cấu hình độ ưu tiên và bật ngắt trên NVIC
    NVIC_SetPriority(EXTI9_5_IRQn, 1); // Ưu tiên cao hơn vòng lặp main
    NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void EXTI9_5_IRQHandler(void) {
    // Luôn kiểm tra cờ PR để xác nhận đúng Line 8 gây ra ngắt
    if (EXTI->PR & (1 << 8)) {

        // Đảo trạng thái cờ: Đang chạy -> Dừng, Đang dừng -> Chạy
        ir_triggered = !ir_triggered;

        // BẮT BUỘC: Xóa cờ ngắt bằng cách ghi 1 để không bị kẹt vòng lặp
        EXTI->PR |= (1 << 8);
    }
}


int main(void) {
    /* 1. Khởi tạo ngoại vi */
    // Hàm này đã bao gồm khởi tạo I2C, DMA và đánh thức MPU6050
    MPU6050_Init();

    // Khởi tạo PWM cho Servo
    Servo_F401_Init();
    LCD_Init();        // Khởi tạo LCD thông qua bộ bus I2C1 đã bật ở trên
    IR_Interrupt_Init(); // Khởi tạo ngắt PA8
    /* 2. Cấu hình mặc định cho Servo về vị trí cân bằng (90 độ) */
    Set_Servo1(90);
    Set_Servo2(90);

    /* In tiêu đề tĩnh một lần duy nhất */
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_String("IMU 2DOF Monitor");

    while (1) {
        /* 3. Bắt đầu đọc dữ liệu từ cảm biến qua DMA (Không chặn) */
        MPU6050_Read_All_DMA();

        /* 4. Chờ cho đến khi DMA hoàn tất việc chuyển dữ liệu */
        // Biến dma_transfer_complete được cập nhật trong DMA1_Stream0_IRQHandler
        while (dma_transfer_complete == 0);
        dma_transfer_complete = 0; // Reset cờ

        if (ir_triggered) {
                    // Nếu bị trigger, hiển thị cảnh báo và KHÔNG cập nhật góc Servo
                    LCD_SetCursor(1, 0);
                    LCD_String("! IR DETECTED ! ");

                    // Có thể thêm lệnh đưa tay robot về vị trí an toàn ở đây nếu cần
                    // Set_Servo1(90);
                    // Set_Servo2(90);
        }

        else {
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

			// Định dạng chuỗi hiển thị góc Roll và Pitch ngắn gọn, căn lề 3 ký tự
			sprintf(lcd_buffer, "R:%3d   P:%3d   ", (int)Roll, (int)Pitch);

			LCD_SetCursor(1, 0);
			LCD_String(lcd_buffer);
        }
			/* Delay nhỏ để tránh quá tải bus I2C và giúp Servo kịp phản hồi */
			delay_simple(50000);

    }
}

#include "main.h"
#include <stdio.h>
#include "lcd_i2c.h"
#include "MPU6050_DMA.h"
#include <math.h>

/* Kết nối tới các biến trong file MPU6050_DMA.c */
extern volatile float roll;
extern volatile float pitch;
extern volatile int obstacle_detected;
extern volatile uint8_t dma_transfer_complete;

char lcd_buffer[32];


void IR_Interrupt_Init(void) {
	RCC->AHB1ENR |= (1 << 0) | (1 << 1); // Bật Clock cho Port A và Port B
    RCC->APB2ENR |= (1 << 14); // Bật Clock cho SYSCFG

    GPIOA->MODER &= ~(3 << 16);
    GPIOA->PUPDR &= ~(3 << 16);
    GPIOA->PUPDR |=  (1 << 16);

    // 3. Cấu hình chân PB10 (IR 2 - NÚT MỞ KHÓA)
    GPIOB->MODER &= ~(3 << 20);         // Input (Chân 10 nằm ở bit 21:20)
    GPIOB->PUPDR &= ~(3 << 20);
    GPIOB->PUPDR |=  (1 << 20);

    SYSCFG->EXTICR[2] &= ~(0xF << 0);

    SYSCFG->EXTICR[2] &= ~(0xF << 8);   // Xóa cấu hình cũ
    SYSCFG->EXTICR[2] |=  (1 << 8);

    // 5. Thiết lập ngắt cạnh xuống (Falling Edge) cho cả Line 8 và Line 10
    EXTI->FTSR |= (1 << 8) | (1 << 10);

    EXTI->IMR |= (1 << 8) | (1 << 10);

    NVIC_SetPriority(EXTI9_5_IRQn, 1);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    NVIC_SetPriority(EXTI15_10_IRQn, 1);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI9_5_IRQHandler(void) {
	if (EXTI->PR & (1 << 8)) {
	        // Chủ động set cứng cờ thành 1 (Dừng hệ thống)
	        obstacle_detected = 1;

	        EXTI->PR |= (1 << 8); // Xóa cờ ngắt
	    }
}

void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR & (1 << 10)) {
        // Chủ động xóa cờ về 0 (Hệ thống chạy lại bình thường)
        obstacle_detected = 0;

        EXTI->PR |= (1 << 10); // Xóa cờ ngắt
    }
}

int main(void) {
    I2C1_Init();
    DMA1_Init();
    MPU6050_Init();

    Servo_F401_Init();
    LCD_Init();
    IR_Interrupt_Init();

    Set_Servo1(90);
    Set_Servo2(90);

    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_String("IMU 2DOF Monitor");

    while (1) {
        /* 1. Gửi lệnh yêu cầu DMA đọc MPU6050 */
        MPU6050_Start_DMA_Read();

        while (dma_transfer_complete == 0);

        if (obstacle_detected) {
            LCD_SetCursor(1, 0);
            LCD_String("! IR DETECTED ! ");
        }
        else {
            int s1_pos = (int)(roll + 90);
            int s2_pos = (int)(90 - pitch);

            if (s1_pos < 0)   s1_pos = 0;
            if (s1_pos > 180) s1_pos = 180;
            if (s2_pos < 0)   s2_pos = 0;
            if (s2_pos > 180) s2_pos = 180;

            Set_Servo1(s1_pos);
            Set_Servo2(s2_pos);

            // Để chuỗi dài 16 ký tự để nó ghi đè hoàn toàn chữ "IMU RECONNECTING"
            sprintf(lcd_buffer, "R:%3d P:%3d     ", (int)roll, (int)pitch);
            LCD_SetCursor(1, 0);
            LCD_String(lcd_buffer);
        }

        delay_simple(50000);
    }
}

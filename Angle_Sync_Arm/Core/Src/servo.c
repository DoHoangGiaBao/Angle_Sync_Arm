#include "servo.h"

void Servo_F401_Init(void) {
    // 1. Cấp Clock cho GPIOA và Timer 2
    RCC->AHB1ENR |= (1 << 0);
    RCC->APB1ENR |= (1 << 0);

    // 2. Cấu hình PA0 và PA1 là Alternate Function (AF)
    // PA0: MODER[1:0] = 10, PA1: MODER[3:2] = 10
    GPIOA->MODER &= ~((3 << 0) | (3 << 2)); // Xóa bit cũ
    GPIOA->MODER |=  ((2 << 0) | (2 << 2)); // Thiết lập AF

    // 3. Chọn AF1 (TIM2) cho PA0 và PA1 trong thanh ghi AFRL
    GPIOA->AFR[0] &= ~((0xF << 0) | (0xF << 4));
    GPIOA->AFR[0] |=  ((1 << 0) | (1 << 4));

    // 4. Cấu hình Timer 2 tạo xung 50Hz (Chu kỳ 20ms)
    // Giả sử Clock hệ thống là 16MHz -> PSC = 15 để đếm 1MHz (1us)
    TIM2->PSC = 16 - 1;
    TIM2->ARR = 20000 - 1;

    // 5. Cấu hình Channel 1 và 2 ở chế độ PWM Mode 1
    TIM2->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC2M);
    TIM2->CCMR1 |= (6 << 4) | (6 << 12);
    TIM2->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE;

    // 6. Cho phép xuất xung ra chân (Output Enable)
    TIM2->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;

    // 7. Bật Timer 2
    TIM2->CR1 |= TIM_CR1_CEN;
}

void Set_Servo1(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    // Map từ 0-180 sang 500-2500 (ứng với 0.5ms - 2.5ms)
    TIM2->CCR1 = 500 + (angle * 2000 / 180);
}

void Set_Servo2(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    TIM2->CCR2 = 500 + (angle * 2000 / 180);
}


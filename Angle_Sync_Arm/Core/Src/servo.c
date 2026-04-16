#include "stm32f4xx.h"
void Servo_F401_Init(void) {
   // 1. Cấp Clock cho GPIOA và Timer 2
   RCC->AHB1ENR |= (1 << 0);
   RCC->APB1ENR |= (1 << 0);
   // 2. Cấu hình PA0 và PA1 là Alternate Function (AF)
   // Thiết lập AF (10)
   GPIOA->MODER |= (1 << 1);
   GPIOA->MODER &= ~(1 << 0);
   GPIOA->MODER |= (1 << 3);
   GPIOA->MODER &= ~(1 << 2);
   // 3. Chọn AF1 (TIM2) cho PA0 và PA1 trong thanh ghi AFRL
   GPIOA->AFR[0] &= ~((0xFU << (0 * 4)) | (0xFU << (1 * 4)));
   GPIOA->AFR[0] |=  ((1U << (0 * 4)) | (1U << (1 * 4))); // AF1 là TIM2
   // 4. Cấu hình Timer 2 tạo xung 50Hz (Chu kỳ 20ms)
   // Tần số Clock của TIM2 là 16MHz
   // Chọn Prescaler = 16 - 1 = 83 => Tần số đếm = 1MHz (1us mỗi tick)
   TIM2->PSC = 16 - 1;
   // ARR = 20000 => 20000 * 1us = 20ms (50Hz)
   TIM2->ARR = 20000 - 1;
   // 5. Cấu hình Channel 1 và 2 ở chế độ PWM Mode 1
   TIM2->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC2M);
   TIM2->CCMR1 |= (6U << 4) | (6U << 12); // Mode 1: 0110
   TIM2->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE; // Preload Enable
   // 6. Cho phép xuất xung ra chân (Output Enable)
   TIM2->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;
   // 7. Bật Timer 2
   TIM2->CR1 |= TIM_CR1_CEN;
}
// Hàm đặt góc quay (0 - 180)
// 0 độ   -> 0.5ms (CCR = 500)
// 180 độ -> 2.5ms (CCR = 2500)
void Set_Servo1(int angle) {
   if (angle < 0) angle = 0;
   if (angle > 180) angle = 180;
   TIM2->CCR1 = 500 + (angle * 2000 / 180);
}
void Set_Servo2(int angle) {
   if (angle < 0) angle = 0; if (angle > 180) angle = 180;
   TIM2->CCR2 = 500 + (angle * 2000 / 180);
}
void delay_simple(uint32_t count) {
   while(count--) { __NOP(); }
}
// ... (Phần Init giữ nguyên)
int main(void) {
   Servo_F401_Init();
   while (1) {
       // Test vị trí 0 độ
//        Set_Servo1(0);
//        Set_Servo2(0);
//        delay_simple(2000000); // Tăng delay lên rất lớn (khoảng 1-2 giây)
//
//        // Test vị trí 90 độ
//        Set_Servo1(90);
//        Set_Servo2(90);
//        delay_simple(2000000);
//
//        // Test vị trí 180 độ
//        Set_Servo1(180);
//        Set_Servo2(180);
//        delay_simple(2000000);
   		Set_Servo1(0);
   		delay_simple(2000000);
   		Set_Servo1(90);
   		delay_simple(2000000);
   		Set_Servo1(180);
   		delay_simple(2000000);
   }
}

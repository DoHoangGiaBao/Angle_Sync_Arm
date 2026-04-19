#ifndef SERVO_H_
#define SERVO_H_

#include "stm32f4xx.h"

// Khởi tạo Timer 2 và GPIO cho 2 Servo (PA0, PA1)
void Servo_F401_Init(void);

// Điều khiển Servo 1 (Chân PA0) - Góc từ 0 đến 180
void Set_Servo1(int angle);

// Điều khiển Servo 2 (Chân PA1) - Góc từ 0 đến 180
void Set_Servo2(int angle);

// Hàm delay đơn giản (dùng để test)
void delay_simple(uint32_t count);

#endif /* SERVO_H_ */
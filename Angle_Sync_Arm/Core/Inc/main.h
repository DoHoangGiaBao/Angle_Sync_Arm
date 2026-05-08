#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx.h"
#include "mpu6050.h"
#include "servo.h"
#include <math.h>

/* Hàm delay đơn giản dùng cho các tác vụ khởi tạo */
void delay_simple(uint32_t delay);

#endif /* __MAIN_H */

#ifndef INC_COMPLEMENTARY_FILTER_H_
#define INC_COMPLEMENTARY_FILTER_H_

#include <stm32f4xx_hal.h>

extern float angle_filtered;
extern float angular_velocity;

float get_filtered_angle(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t size);

#endif /* INC_COMPLEMENTARY_FILTER_H_ */

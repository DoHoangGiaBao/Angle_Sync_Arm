#include <math.h>
#include "complementary_filter.h"
#include "mpu6050.h"

#define PI 3.14159265359f

const float dt = 0.01f;			// Sampling time in seconds
const float alpha = 0.98f;		// Complementary filter weight
float angle_filtered = 0.0f;	// Filtered angle
float angular_velocity = 0.0f;	// Angular velocity (rad/s)

float get_filtered_angle(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t size) {
	// Get accelerometer and gyroscope data
	I2C1_read_multi_byte(dev_addr, reg_addr, data, size);
	int16_t accel_x = (data[0] << 8) | data[1];
	int16_t accel_z = (data[4] << 8) | data[5];

	int16_t gyro_y = (data[10] << 8) | data[11];

	// Calculate accelerometer angle in degrees
	float accel_angle = atan2((float)accel_x, (float)accel_z) * 180 / PI;

	// Convert gyroscope rate to degrees/s
	float gyro_rate = (float)gyro_y / 65.5f;

	angular_velocity = gyro_rate * (PI / 180.0f);

	// Apply complementary filter
	angle_filtered = alpha * (angle_filtered + gyro_rate * dt) + (1 - alpha) * accel_angle;

	return angle_filtered * (PI / 180.0f);
}

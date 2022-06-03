/*
 * Microcurrent
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#include <stdint.h>
#include <pthread.h>
#include "ldo.h"


static struct i2c_device mcp4725;
static pthread_mutex_t *i2c_lock = NULL;
static float ldou_cached = 2.5;


int ldo_init(struct i2c_master *i2c, pthread_mutex_t *lock)
{
	IF_R(mcp4725_open(&mcp4725, i2c, MCP4725_ADDR_A2), -1);
	i2c_lock = lock;
	ldo_voltage(NAN);
	return 0;
}

float ldo_voltage(float voltage)
{
	if (!i2c_lock) {
		return NAN;
	}

	pthread_mutex_lock(i2c_lock);
	if (voltage >= 0.25 && voltage < 4.5) {
		/* set new voltage */
		int32_t v = (int32_t)(voltage * 4096.0L / 5.0L);
		mcp4725_wr(&mcp4725, 0, v);
		ldou_cached = voltage;
	} else {
		/* read current voltage */
		int32_t v = mcp4725_rd(&mcp4725, 0);
		voltage = 5.0L * (float)v / 4096.0L;
		ldou_cached = voltage;
	}
	pthread_mutex_unlock(i2c_lock);

	return voltage;
}

float ldo_voltage_cached(void)
{
	float v = 0.0;
	pthread_mutex_lock(i2c_lock);
	v = ldou_cached;
	pthread_mutex_unlock(i2c_lock);
	return v;
}

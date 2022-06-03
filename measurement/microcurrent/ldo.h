/*
 * Microcurrent
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#ifndef _LDO_H_
#define _LDO_H_

#include <libe/libe.h>

int ldo_init(struct i2c_master *i2c, pthread_mutex_t *lock);
float ldo_voltage(float voltage);
float ldo_voltage_cached(void);


#endif /* _LDO_H_ */

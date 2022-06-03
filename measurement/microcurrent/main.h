/*
 * Microcurrent
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdint.h>
#include <libe/libe.h>

uint8_t osr_get(void);
int osr_set(uint8_t value);

#endif /* _MAIN_H_ */

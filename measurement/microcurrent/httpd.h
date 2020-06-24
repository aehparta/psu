/*
 * Microcurrent httpd.
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#ifndef _HTTPD_H_
#define _HTTPD_H_

#include <stdint.h>

int httpd_init(void);
void httpd_quit(void);
int httpd_start(char *address, uint16_t port, char *root);

#endif /* _HTTPD_H_ */

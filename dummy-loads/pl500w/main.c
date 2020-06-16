/*
 * Scan i2c bus for devices.
 */

#include <libe/libe.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include "config.h"


static pthread_t t_i2c;
static struct display display;
static uint8_t buffer[SSD1306_BUFFER_SIZE];
static struct i2c_master i2c;
struct rot_enc re;


void *i2c_thread(void *data)
{
	os_time_t t = os_timef();

	while (1) {
		char str[32];


		float x;
		optctl(&re, ROT_ENC_OPT_GET_VALUE, &x);
		sprintf(str, "%4.1f", x);
		draw_string(&display, &g_sFontCmtt18, str, -1, 0, 0, true);




		sprintf(str, "%3.3f", (float)os_timef());
		draw_string(&display, &g_sFontCmtt18, str, -1, 0, 20, true);




		sprintf(str, "%4.0f", (float)(os_timef() - t) * 1000);
		draw_string(&display, &g_sFontCmtt18, str, -1, 0, 40, true);
		t = os_timef();
		display_update(&display);
	}

	return NULL;
}

#ifdef TARGET_ESP32
int app_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	void *context = NULL;

	/* base init */
	os_init();
	log_init();

	/* open display */
#ifdef TARGET_LINUX
	ERROR_IF_R(argc < 2, 1, "Give i2c device as and argument\nExample: ./display-x86.elf /dev/i2c-3");
	context = argv[1];
#endif
	ERROR_IF_R(i2c_master_open(&i2c, context, 400000, 0, 0), 1, "unable to open i2c device");
	ERROR_IF_R(ssd1306_i2c_open(&display, &i2c, 0, 0, 0), 1, "unable to open ssd1306 display");
	optctl(&display, DISPLAY_OPT_SET_BUFFER, buffer);
	draw_fill(&display, 0, 0, 128, 64, 0x000000);

	/* ftdi init */
#ifdef USE_FTDI
	ERROR_IF_R(os_ftdi_use(OS_FTDI_GPIO_0_TO_63, CFG_FTDI_VID, CFG_FTDI_PID, CFG_FTDI_DESC, CFG_FTDI_SERIAL), 1, "unable to open ftdi device for gpio 0-63");
#endif

	/* open rotary encoder */
	rot_enc_open(&re, CFG_ROT_ENC_A, CFG_ROT_ENC_B);
	optwr_float(&re, ROT_ENC_OPT_SET_MIN, 0);
	optwr_float(&re, ROT_ENC_OPT_SET_MAX, 10000);
	// optwr_float(&re, ROT_ENC_OPT_SET_STEP, 0.1);
	// optwr_u8(&re, ROT_ENC_OPT_SET_FLAGS, ROT_ENC_FLAG_ROLL);

	/* create i2c handler thread */
	pthread_create(&t_i2c, NULL, i2c_thread, NULL);

	while (1) {
		rot_enc_rd(&re);
	}

	// os_sleepi(5);

	display_close(&display);

	/* close i2c */
	// i2c_master_close(&i2c);

	return 0;
}

/*
 * Lead-acid battery charger using ACT4751
 */

#include <libe/libe.h>

#define R_ILIM          20000 /* ohms */
#define DUMMY_LOAD_EN   GPIOB0
#define OUT_EN          GPIOB1
#define CONVERSIONS     50

struct i2c_master i2c;
struct i2c_device dev;
struct adc adc;


int p_init(void)
{
	/* change clock from default 1MHZ (CKDIV8) to 8MHZ (no CKDIV8) */
	CLKPR = 0x80; /* enable clock divisor change */
	CLKPR = 0x00; /* change clock divisor to one */

	/* library initializations */
	os_init();
	log_init();
	adci_open(&adc, NULL);
	// DEBUG_MSG("adc frequency: %lu", adci_frequency(&adc, 0));
	DEBUG_MSG("adc frequency: %lu", adci_frequency(&adc, 10000));

	/* gpio initialization */
	gpio_output(DUMMY_LOAD_EN);
	gpio_low(DUMMY_LOAD_EN);
	gpio_output(OUT_EN);
	gpio_low(OUT_EN);

	/* act4751 initialization */
	ERROR_IF_R(i2c_master_open(&i2c, NULL, 0, 0, 0), -1, "unable to open i2c master");
	ERROR_IF_R(act4751_open(&dev, &i2c, ACT4751_ADDR, R_ILIM), -1, "unable to open act4751");
	i2c_write_reg_byte(&dev, 0x09, i2c_read_reg_byte(&dev, 0x09) | 0x08);
	act4751_set_main_voltage(&dev, 14.4);
	act4751_set_main_current(&dev, 3.00);

	return 0;
}

int main(void)
{
	ERROR_IF_R(p_init(), -1, "base initialization failed");

	while (1) {
		float Uin = 0.0, Uout = 0.0, Iout = 0.0;
		os_wdt_reset();

		for (int8_t i = 0; i < CONVERSIONS; i++) {
			Uin += adci_read_raw(&adc, 6);
			Uout += adci_read_raw(&adc, 3);
			Iout += adci_read_raw(&adc, 7);
		}
		Uin /= (29.86 * CONVERSIONS);
		Uout /= (29.86 * CONVERSIONS);
		Iout /= (190.0 * CONVERSIONS);


		// i2c_write_byte(&dev, 0x00);
		// i2c_read(&dev, b, 16);
		// HEX_DUMP(b, 16);
		DEBUG_MSG("adc: %2.2f %2.2f %1.2f", Uin, Uout, Iout);
		os_delay_ms(1000);
	}

	return 0;
}

/*
 * Lead-acid battery charger using ACT4751
 */

#include <libe/libe.h>

#define R_ILIM          8200 /* ohms */
#define CONVERSIONS     500

#define ADC_CH_UIN      6
#define ADC_CH_UOUT     3
#define ADC_CH_IOUT     7
#define ADC_CH_TBAT     2

#define DUMMY_LOAD_EN   GPIOB0
#define OUTPUT_EN       GPIOB1

#define NVM_A_UOUT_TOP      14.4
#define NVM_A_UOUT_FLOAT    13.8
#define NVM_A_BAT_C         1.2


struct i2c_master i2c;
struct i2c_device dev;
struct adc adc;

void check(void)
{
	float Uin = 0.0, Uout = 0.0, Iout = 0.0, Tbat = 0.0;

	/* read values */
	for (int i = 0; i < CONVERSIONS; i++) {
		Uin += adci_read_raw(&adc, ADC_CH_UIN);
		Uout += adci_read_raw(&adc, ADC_CH_UOUT);
		Iout += adci_read_raw(&adc, ADC_CH_IOUT);
		Tbat += (adci_read_raw(&adc, ADC_CH_TBAT) - 508.0);
		/* using 500 us delay with 500 conversions uses around 740 ms */
		os_delay_us(500);
	}

	/* do averaging */
	Uin /= (29.86 * CONVERSIONS);
	Uout /= (29.86 * CONVERSIONS);
	Iout /= (377.0 * CONVERSIONS);
	Tbat /= (7.2 * CONVERSIONS);

	/* minimum accuracy around 20 mA */
	if (Iout < 0.020) {
		Iout = 0.0;
	}

	DEBUG_MSG("adc: %2.2fV %2.2fV %1.3fA %2.1fC°", Uin, Uout, Iout, Tbat);
}

void output(bool enable)
{
	if (enable) {
		gpio_output(OUTPUT_EN);
		gpio_low(OUTPUT_EN);
	} else {
		gpio_input(OUTPUT_EN);
	}
}

void dummy_load(bool enable)
{
	if (enable) {
		gpio_high(DUMMY_LOAD_EN);
	} else {
		gpio_low(DUMMY_LOAD_EN);
	}
}

int p_init(void)
{
	/* change clock from default 1MHZ (CKDIV8) to 8MHZ (no CKDIV8) */
	CLKPR = 0x80; /* enable clock divisor change */
	CLKPR = 0x00; /* change clock divisor to one */

	/* library initializations */
	os_init();
	log_init();
	nvm_init(NULL, 0);
	adci_open(&adc, NULL); /* samples-per-second will be around 4807 */
	// DEBUG_MSG("ADC SPS: %lu", adci_sps(&adc, 0));

	/* dummy load off */
	gpio_output(DUMMY_LOAD_EN);
	gpio_low(DUMMY_LOAD_EN);
	/* output off */
	gpio_input(OUTPUT_EN);

	/* act4751 initialization */
	ERROR_IF_R(i2c_master_open(&i2c, NULL, 0, 0, 0), -1, "unable to open i2c master");
	ERROR_IF_R(act4751_open(&dev, &i2c, ACT4751_ADDR, R_ILIM), -1, "unable to open act4751");
	i2c_write_reg_byte(&dev, 0x09, i2c_read_reg_byte(&dev, 0x09) | 0x08);
	act4751_set_main_voltage(&dev, 14.4);
	// act4751_set_main_voltage(&dev, 0.0);
	act4751_set_main_current(&dev, 2.000);

	return 0;
}

int main(void)
{
	ERROR_IF_R(p_init(), -1, "base initialization failed");

	sei();
	os_time_t t = os_timef() + 1.0;
	while (1) {
		os_wdt_reset();
		check();

		while (t > os_timef());
		t++;
	}

	return 0;
}

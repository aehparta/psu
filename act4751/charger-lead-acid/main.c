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

#define INPUT_TOO_LOW_DIF   0.5 /* volts */
#define MAX_CHARGE_I        2.0 /* amperes */

#define NVM_A_UOUT_TOP              (float*)0 /* float, 4 bytes */
#define NVM_A_UOUT_FLOAT            (float*)4 /* float, 4 bytes */
#define NVM_A_BAT_C                 (float*)8 /* float, 4 bytes */
#define NVM_A_BAT_LOW               (float*)12 /* float, 4 bytes */
#define NVM_A_BAT_FAULTY            (float*)16 /* float, 4 bytes */
#define NVM_A_BAT_NEED_TO_TOP       (float*)20 /* float, 4 bytes */
#define NVM_A_BAT_MAX_CHG_C         (float*)24 /* float, 4 bytes */
#define NVM_A_BAT_CHG_TOP_FIN_C     (float*)28 /* float, 4 bytes */
#define NVM_A_SETTLE_DELAY          (uint8_t *)32 /* seconds, 1 byte */

#define UOUT_TOP_DEFAULT            14.4
#define UOUT_FLOAT_DEFAULT          13.8
#define BAT_C_DEFAULT               1.2
#define BAT_LOW_DEFAULT             11.5
#define BAT_FAULTY_DEFAULT          10.5
#define BAT_NEED_TO_TOP_DEFAULT     12.6 /* 2.1 volts per cell is around 90% of capacity left */
#define BAT_MAX_CHG_C_DEFAULT       0.3
#define BAT_CHG_TOP_FIN_C_DEFAULT   0.05
#define SETTLE_DELAY_DEFAULT        5 /* seconds */


enum {
	MODE_BOOTUP = 0,
	MODE_SETTLE,
	MODE_CHARGE_TOP,
	MODE_CHARGE_FLOAT,
	MODE_BAT_INUSE,
	MODE_BAT_EMPTY,
	MODE_BAT_DISCONNECTED,
	MODE_BAT_MAYBE_FAULTY,
	MODE_BAT_FAULTY,
	MODE_BAT_MONITOR,
};

struct i2c_master i2c;
struct i2c_device dev;
struct adc adc;
uint8_t mode = MODE_BOOTUP;
uint8_t settle_delay = 0;
uint32_t charge_top_time = 0;
uint32_t charge_float_time = 0;


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

void set_settle_wait(void)
{
	settle_delay = nvm_read_byte(NVM_A_SETTLE_DELAY, SETTLE_DELAY_DEFAULT);
}

bool check_faulty(float voltage)
{
	/* check if battery disconnected */
	if (voltage < 1.0) {
		ERROR_IF(mode != MODE_BAT_DISCONNECTED, "battery not connected");
		output(false);
		mode = MODE_BAT_DISCONNECTED;
		return true;
	}

	/* check faulty battery voltage */
	if (voltage < nvm_read_float(NVM_A_BAT_FAULTY, BAT_FAULTY_DEFAULT)) {
		output(false);
		/* wait to check if battery was disconnected instead of being faulty */
		if (mode != MODE_BAT_FAULTY && mode != MODE_BAT_MAYBE_FAULTY) {
			set_settle_wait();
			mode = MODE_BAT_MAYBE_FAULTY;
			return true;
		}
		/* after settle delay, bat still faulty and not disconnected etc */
		CRIT_IF(mode != MODE_BAT_FAULTY, "battery is faulty");
		mode = MODE_BAT_FAULTY;
		return true;
	}

	return false;
}

void check(void)
{
	float Uin = 0.0, Uout = 0.0, Iout = 0.0, Tbat = 0.0;

	/* read values */
	for (int i = 0; i < CONVERSIONS; i++) {
		Uin += adci_read_raw(&adc, ADC_CH_UIN);
		Uout += adci_read_raw(&adc, ADC_CH_UOUT);
		Iout += adci_read_raw(&adc, ADC_CH_IOUT);
		Tbat += (adci_read_raw(&adc, ADC_CH_TBAT) - 508.0);
		/* using 500 us delay with 500 conversions makes a total of around 740 ms */
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

	// printf("Uin: %0.2fV, Uout: %0.2fV, Iout: %0.3fA, Tbat: %0.1fC°\r\n", Uin, Uout, Iout, Tbat);
	static int once_per_minute = 0;
	if (once_per_minute < time(NULL)) {
		printf("Uin: %0.2fV, Uout: %0.2fV, Iout: %0.3fA, Tbat: %0.1fC°\r\n", Uin, Uout, Iout, Tbat);
		if (once_per_minute == 0) {
			once_per_minute = time(NULL);
		}
		once_per_minute += 60;
	}

	/* wait for settle delay first (set on boot and some other situations) */
	if (settle_delay > 0) {
		settle_delay--;
		return;
	}

	/* if input is too low or disconnected */
	if ((Uin - INPUT_TOO_LOW_DIF) < Uout) {
		/* output disable,
		 * forgot to connect physical EN-pin to GND so cannot actually power off main buck,
		 * instead set voltage to near zero
		 */
		act4751_set_main_voltage(&dev, 0.1);
		/* check if battery is faulty */
		if (check_faulty(Uout)) {
			return;
		}
		/* check if battery voltage is too low = battery is empty or was already declared empty (and has just been "recovered" a little) */
		if (Uout < nvm_read_float(NVM_A_BAT_LOW, BAT_LOW_DEFAULT) || mode == MODE_BAT_EMPTY) {
			WARN_IF(mode != MODE_BAT_EMPTY, "battery is empty");
			output(false);
			mode = MODE_BAT_EMPTY;
			return;
		}
		/* keep output connected until battery voltage is too low */
		NOTICE_IF(mode != MODE_BAT_INUSE, "running on battery");
		output(true);
		mode = MODE_BAT_INUSE;
		return;
	}

	if (mode == MODE_BAT_MONITOR) {
		/* check if battery is faulty */
		if (check_faulty(Uout)) {
			return;
		}
		/* check if voltage is above topping level */
		if (Uout >= nvm_read_float(NVM_A_BAT_NEED_TO_TOP, BAT_NEED_TO_TOP_DEFAULT)) {
			return;
		}
		/* ok, need to charge now again */
	}

	/* if we are not in any charging state, determine which state we should change into */
	if (mode != MODE_CHARGE_TOP && mode != MODE_CHARGE_FLOAT) {
		/* main buck is disabled in other modes, check battery first */
		if (check_faulty(Uout)) {
			return;
		}
		/* when coming here after using battery or faulty battery/disconnection, wait a little for things to settle */
		if (mode != MODE_BOOTUP && mode != MODE_SETTLE) {
			if (mode == MODE_BAT_INUSE || mode == MODE_BAT_EMPTY) {
				INFO_MSG("main input connected");
			}
			set_settle_wait();
			mode = MODE_SETTLE;
			return;
		}

		charge_top_time = 0;
		charge_float_time = 0;

		if (Uout < nvm_read_float(NVM_A_BAT_NEED_TO_TOP, BAT_NEED_TO_TOP_DEFAULT)) {
			mode = MODE_CHARGE_TOP;
		} else {
			mode = MODE_CHARGE_FLOAT;
		}
	}

	/* get charging options */
	float bat_c = nvm_read_float(NVM_A_BAT_C, BAT_C_DEFAULT);
	float bat_chg_c = nvm_read_float(NVM_A_BAT_MAX_CHG_C, BAT_MAX_CHG_C_DEFAULT);
	float bat_u_top = nvm_read_float(NVM_A_UOUT_TOP, UOUT_TOP_DEFAULT);
	float bat_u_float = nvm_read_float(NVM_A_UOUT_FLOAT, UOUT_FLOAT_DEFAULT);
	float bat_chg_top_fin_c = nvm_read_float(NVM_A_BAT_CHG_TOP_FIN_C, BAT_CHG_TOP_FIN_C_DEFAULT);
	float chg_i = bat_c * bat_chg_c;

	/* set charging current */
	if (chg_i > MAX_CHARGE_I) {
		chg_i = MAX_CHARGE_I;
	}
	act4751_set_main_current(&dev, chg_i);

	/* check mode and switch if needed */
	if (mode == MODE_CHARGE_TOP) {
		act4751_set_main_voltage(&dev, bat_u_top);
		charge_top_time++;
		if (charge_top_time == 1) {
			INFO_MSG("started topping charge, voltage: %0.2fV, max current: %0.2fA", bat_u_top, chg_i);
			set_settle_wait();
			return;
		}
		float fin_i = bat_c * bat_chg_top_fin_c;
		if (fin_i > Iout) {
			INFO_MSG("ended topping charge, time taken: %us", charge_top_time);
			mode = MODE_CHARGE_FLOAT;
		}
	} else if (mode == MODE_CHARGE_FLOAT) {
		act4751_set_main_voltage(&dev, bat_u_float);
		charge_float_time++;
		if (charge_float_time == 1) {
			INFO_MSG("started float charge, voltage: %0.2fV", bat_u_float);
			set_settle_wait();
			return;
		}
	}

	/* if no power is drawn, then battery is either full or disconnected */
	if (Iout <= 0.0) {
		INFO_MSG("battery full or disconnected, switching to monitoring mode");
		act4751_set_main_voltage(&dev, 0.1);
		mode = MODE_BAT_MONITOR;
		set_settle_wait();
	}

	/* always turn on output when input is connected and battery is ok
	 * this is to make the switch to battery instant if main power is lost
	 */
	output(true);
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
	/* disable under-voltage protection, not working in a charger thingie like this */
	act4751_set_uvp(&dev, false);
	/* output disable,
	 * forgot to connect physical EN-pin to GND so cannot actually power off main buck,
	 * instead set voltage to near zero
	 */
	act4751_set_main_voltage(&dev, 0.1);
	/* 100 mA max current initially */
	act4751_set_main_current(&dev, 0.1);
	// char b[16];
	// for (int i = 0; ; i++) {
	// 	b[0] = 0x00;
	// 	i2c_write(&dev, b, 1);
	// 	i2c_read(&dev, b, 16);

	// 	// printf("%02x %02x %02x %02x\r\n", b[0], b[1], b[2], b[3]);
	// 	HEX_DUMP(b, 16);
	// 	// printf("%d %02x\r\n", i, i2c_read_reg_byte(&dev, ACT4751_REG_STATE));

	// 	os_delay_ms(500);
	// };

	return 0;
}

int main(void)
{
	ERROR_IF_R(p_init(), -1, "base initialization failed");

	sei();
	os_time_t t = os_timef() + 1.0;
	set_settle_wait();
	INFO_MSG("charger started");
	while (1) {
		os_wdt_reset();
		check();
		/* wait until a full second has passed */
		while (t > os_timef());
		t++;
	}

	return 0;
}

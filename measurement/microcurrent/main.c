/*
 * Random testing area for development.
 */

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libe/libe.h>


// #define VREF_VCC

#ifdef VREF_VCC
#define VREF                        3.3L
#define OFFSET                      885.0L
#else
#define VREF                        2.555L
#define OFFSET                      1010.0L
#endif

#define PWM_PERIOD                  "100"
#define PWM_DUTY_CYCLE              "50"

#define PWM_EXPORT_FILE             "/sys/class/pwm/pwmchip0/export"
#define PWM_PERIOD_FILE             "/sys/class/pwm/pwmchip0/pwm0/period"
#define PWM_DUTY_CYCLE_FILE         "/sys/class/pwm/pwmchip0/pwm0/duty_cycle"
#define PWM_ENABLE_FILE             "/sys/class/pwm/pwmchip0/pwm0/enable"


int pwm_write(const char *file, const char *str)
{
	int fd = open(file, O_WRONLY);
	ERROR_IF_R(fd < 0, -1, "Failed to open: %s, data to be written: %s, reason: %s", file, str, strerror(errno));
	write(fd, str, strlen(str));
	close(fd);
	return 0;
}

int pwm_init(void)
{
	struct stat st;

	ERROR_IF_R(stat(PWM_EXPORT_FILE, &st), -1, "PWM not found, tried to stat(): %s", PWM_EXPORT_FILE);
	if (stat(PWM_ENABLE_FILE, &st)) {
		IF_R(pwm_write(PWM_EXPORT_FILE, "0"), -1);
	} else {
		IF_R(pwm_write(PWM_ENABLE_FILE, "0"), -1);
	}

	/* duty cycle to zero first */
	IF_R(pwm_write(PWM_DUTY_CYCLE_FILE, "0"), -1);

	/* set period and then duty cycle, then enable */
	IF_R(pwm_write(PWM_PERIOD_FILE, PWM_PERIOD), -1);
	IF_R(pwm_write(PWM_DUTY_CYCLE_FILE, PWM_DUTY_CYCLE), -1);
	IF_R(pwm_write(PWM_ENABLE_FILE, "1"), -1);

	return 0;
}

int main(void)
{
	struct spi_master master;
	struct spi_device device;

	os_init();
	log_init();

	/* setup pwm */
	IF_R(pwm_init(), 1);

	/* irq gpio */
	gpio_input(19);

	/* open ft232h type device and try to see if it has a nrf24l01+ connected to it through mpsse-spi */
	ERROR_IF_R(spi_master_open(
	               &master, /* must give pre-allocated spi master as pointer */
	               NULL, /* context depends on platform */
	               10e6,
	               2,
	               1,
	               0
	           ), -1, "failed to open spi master");
	mcp356x_open(&device, &master, 3);

	/* reset */
	mcp356x_fast_command(&device, MCP356X_CMD_RESET);
	os_sleepf(0.1);

	/* configure */
	optwr_u8(&device, MCP356X_OPT_CLK_SEL, 0x0);
	// optwr_u8(&device, MCP356X_OPT_CLK_SEL, 0x0);
	optwr_u8(&device, MCP356X_OPT_ADC_MODE, 0x3);
	optwr_u8(&device, MCP356X_OPT_PRE, 0x3);

	/* OSR */
	// optwr_u8(&device, MCP356X_OPT_OSR, MCP356X_OSR_16384);
	optwr_u8(&device, MCP356X_OPT_OSR, MCP356X_OSR_1024);

	optwr_u8(&device, MCP356X_OPT_BOOST, 0x3);
	optwr_u8(&device, MCP356X_OPT_GAIN, MCP356X_GAIN_X16);
	optwr_u8(&device, MCP356X_OPT_CONV_MODE, 0x3);
	optwr_u8(&device, MCP356X_OPT_IRQ_MODE, 0x1);

	/* wait for the device to settle */
	os_sleepf(0.1);

	/* first read few rounds and wait a bit */
	mcp356x_ch(&device, MCP356X_CH_01);
	for (int i = 0; i < 10; i++) {
		while (gpio_read(19));
		mcp356x_rd(&device);
	}
	os_sleepf(1.0);

	/* read */
	float min = -1e6, max = -1e6;
	double sum = 0;
	int count = 0;
	FILE *cap_fd = NULL;
	os_time_t cap_t = os_timef() + 1.0;
	while (1) {
		while (gpio_read(19));

		int32_t x = mcp356x_rd(&device) / 256.0L + OFFSET;
 		float U = (float)((double)x / (double)0x7fffff * VREF / 16.0L * 1000000.0L);

		min = min <= -1e6 ? x : min;
		max = max <= -1e6 ? x : max;
		min = x < min ? x : min;
		max = x > max ? x : max;


		sum += U;
		count++;
		if (count >= 10) {
			sum /= count;
			printf("%+12.1f %+12d %+12.0f %+12.0f %+12.0f\n", sum, x, min, max, max - min);
			sum = 0;
			count = 0;
		}


		if (!cap_fd) {
			struct timespec tp;
			char fs[256];
			clock_gettime(CLOCK_REALTIME, &tp);
			sprintf(fs, "capture-%012lu.%09lu", tp.tv_sec, tp.tv_nsec);
			cap_fd = fopen(fs, "w");
		}
		fwrite(&U, sizeof(U), 1, cap_fd);
		if (cap_t < os_timef()) {
			fclose(cap_fd);
			cap_fd = NULL;
			cap_t += 1.0;
		}
	}

	/* free */
	spi_master_close(&master);
	log_quit();
	os_quit();
	return 0;
}

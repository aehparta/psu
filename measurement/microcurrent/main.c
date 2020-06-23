/*
 * Random testing area for development.
 */

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <libe/libe.h>


// #define VREF_VCC

#ifdef VREF_VCC
#define VREF                        3.3L
#define OFFSET                      885.0L
#else
#define VREF                        (2.555L / 3.298L)
#define OFFSET                      60.0L
#endif

#define INTERVAL                    0.5
#define SAMPLE_COUNT                1

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
	optwr_u8(&device, MCP356X_OPT_OSR, MCP356X_OSR_256);

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
	double sum = 0;
	int count = 0;
	os_time_t t = os_timef() + INTERVAL;
	float samples[SAMPLE_COUNT];
	int sample_c = 0;

	// int cap_fd = -1;
	// os_time_t cap_t = os_timef() + 0.5;

	while (1) {
		while (gpio_read(19));

		int32_t x = mcp356x_rd(&device) / 256.0L + OFFSET;
		sum += ((double)x / (double)0x7fffff * VREF / 16.0L);
		count++;

		if (t < os_timef()) {
			sum /= count;
			samples[sample_c++] = sum;
			if (sample_c >= SAMPLE_COUNT) {
				struct timespec tp;
				char fs[256];
				clock_gettime(CLOCK_REALTIME, &tp);
				sprintf(fs, "%012lu%03lu.capture", tp.tv_sec, tp.tv_nsec / 1000000);
				int fd = open(fs, O_CREAT | O_WRONLY, 0644);
				write(fd, samples, sizeof(samples));
				close(fd);
				sample_c = 0;
			}

			printf("%+12.1f uA\n", sum * 1000000.0);

			sum = 0.0;
			count = 0;
			t += INTERVAL;
		}


		// if (cap_fd < 0) {
		// 	struct timespec tp;
		// 	char fs[256];
		// 	clock_gettime(CLOCK_REALTIME, &tp);
		// 	sprintf(fs, "%012lu%03lu.capture", tp.tv_sec, tp.tv_nsec / 1000000);
		// 	cap_fd = open(fs, O_CREAT | O_WRONLY, 0644);
		// 	flock(cap_fd, LOCK_EX);
		// }
		// write(cap_fd, &U, sizeof(U));
		// if (cap_t < os_timef()) {
		// 	flock(cap_fd, LOCK_UN);
		// 	close(cap_fd);
		// 	cap_fd = -1;
		// 	cap_t += 0.5;
		// }
	}

	/* free */
	spi_master_close(&master);
	log_quit();
	os_quit();
	return 0;
}

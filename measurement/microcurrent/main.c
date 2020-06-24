/*
 * Microcurrent software for Orange Pi Zero.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libe/libe.h>


#define MULTIPLIER                  (1.0785L / 16.0)
#define OFFSET                      1350.0L

#define INTERVAL                    0.5
#define SAMPLE_COUNT                100000

#define PWM_PERIOD                  200
#define PWM_DUTY_CYCLE              (PWM_PERIOD / 2)

#define PWM_EXPORT_FILE             "/sys/class/pwm/pwmchip0/export"
#define PWM_UNEXPORT_FILE           "/sys/class/pwm/pwmchip0/unexport"
#define PWM_PERIOD_FILE             "/sys/class/pwm/pwmchip0/pwm0/period"
#define PWM_DUTY_CYCLE_FILE         "/sys/class/pwm/pwmchip0/pwm0/duty_cycle"
#define PWM_ENABLE_FILE             "/sys/class/pwm/pwmchip0/pwm0/enable"

#define I2C_DEVICE                  "/dev/i2c-0"
#define I2C_FREQUENCY               100000

#define IRQ_PIN                     19

#define INFLUXDB_IP                 "127.0.0.1"
#define INFLUXDB_UDP_PORT           8089


/* globals */
struct spi_master master;
struct spi_device device;
pthread_t display_thread;
int influxdb_udp_socket = -1;
struct sockaddr_in influxdb_addr;


/* written from main and read from display update */
static volatile float I = 0, Iavg = 0, Ipeak = 0;
static volatile time_t sampling_started = 0;
static volatile bool exec = true;


int sys_class_write(const char *file, char *str)
{
	int fd = open(file, O_WRONLY);
	ERROR_IF_R(fd < 0, -1, "Failed to open: %s, value to be written: %s, reason: %s", file, str, strerror(errno));
	write(fd, str, strlen(str));
	close(fd);
	return 0;
}

int sys_class_write_int(const char *file, int n)
{
	char str[256];
	snprintf(str, sizeof(str), "%d", n);
	return sys_class_write(file, str);
}

/* pwm initialization */
int pwm_init(void)
{
	struct stat st;

	/* check that pwm is available */
	ERROR_IF_R(stat(PWM_EXPORT_FILE, &st), -1, "PWM not found, tried to stat(): %s", PWM_EXPORT_FILE);

	/* check if pwm is already exported and disable just in case if it is */
	if (stat(PWM_ENABLE_FILE, &st)) {
		IF_R(sys_class_write_int(PWM_EXPORT_FILE, 0), -1);
	} else {
		IF_R(sys_class_write_int(PWM_ENABLE_FILE, 0), -1);
	}

	/* duty cycle to zero first */
	IF_R(sys_class_write_int(PWM_DUTY_CYCLE_FILE, 0), -1);

	/* set period and then duty cycle, then enable */
	IF_R(sys_class_write_int(PWM_PERIOD_FILE, PWM_PERIOD), -1);
	IF_R(sys_class_write_int(PWM_DUTY_CYCLE_FILE, PWM_DUTY_CYCLE), -1);
	IF_R(sys_class_write_int(PWM_ENABLE_FILE, 1), -1);

	return 0;
}

/* pwm shutdown */
void pwm_quit(void)
{
	sys_class_write_int(PWM_ENABLE_FILE, 0);
	sys_class_write_int(PWM_UNEXPORT_FILE, 0);
}

/* display update is threaded since it is low priority */
void *display_thread_func(void *p)
{
	struct display display;
	uint8_t buffer[SSD1306_BUFFER_SIZE];
	struct i2c_master i2c;
	char str[32];

	/* open and initialize display, exit this thread with a warning if this fails */
	WARN_IF_R(i2c_master_open(&i2c, I2C_DEVICE, I2C_FREQUENCY, 0, 0), NULL, "unable to open i2c device");
	WARN_IF_R(ssd1306_i2c_open(&display, &i2c, 0, 0, 0), NULL, "unable to open ssd1306 display");
	optctl(&display, DISPLAY_OPT_SET_BUFFER, buffer);

	/* start filling display with values */
	DEBUG_MSG("lcd display update thread started");
	while (exec) {
		draw_fill(&display, 0, 0, 128, 64, 0x000000);

		/* full scale in uA */
		snprintf(str, sizeof(str), "%8.1fuA", I * 1000000.0);
		draw_string(&display, &g_sFontCmtt24, str, -1, 8, 0, true);

		/* mA/uA scale depending on value */
		if (I > 0.000999) {
			snprintf(str, sizeof(str), "%6.2f", I * 1000.0);
		} else {
			snprintf(str, sizeof(str), "%6.1f", I * 1000000.0);
		}
		draw_string(&display, &g_sFontCmtt30, str, -1, 2, 20, true);
		draw_string(&display, &g_sFontCmtt24, "uA", -1, 104, 20, true);

		/* average and peak */
		draw_string(&display, &g_sFontFixed6x8, "Iavg 9uA Ipeak 1.0mA", -1, 0, 48, true);

		/* sampling time */
		time_t st = time(NULL) - sampling_started;
		snprintf(str, sizeof(str), "%02lu:%02lu:%02lu", st / 3600, (st / 60) % 60, st % 60);
		draw_string(&display, &g_sFontFixed6x8, str, -1, 0, 56, true);

		/* not really a need to update display very often */
		os_sleepf(0.3);
	}

	DEBUG_MSG("lcd display thread exiting");
	display_close(&display);
	i2c_master_close(&i2c);
	return NULL;
}

int p_init(char argc, char *argv[])
{
	os_init();
	log_init();

	/* setup pwm */
	IF_R(pwm_init(), 1);

	/* display */
	pthread_create(&display_thread, NULL, display_thread_func, NULL);

	/* irq gpio */
	gpio_input(IRQ_PIN);

	/* udp socket and address for influxdb data writing */
	influxdb_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	ERROR_IF_R(influxdb_udp_socket < 0, -1, "failed to create influxdb udp socket");
	influxdb_addr.sin_family = AF_INET;
	influxdb_addr.sin_addr.s_addr = inet_addr(INFLUXDB_IP);
	influxdb_addr.sin_port = htons(INFLUXDB_UDP_PORT);

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
	optwr_u8(&device, MCP356X_OPT_CLK_SEL, 0x0); /* external clock */
	optwr_u8(&device, MCP356X_OPT_ADC_MODE, 0x3); /* continuous conversion */
	optwr_u8(&device, MCP356X_OPT_PRE, 0x0); /* no pre-scaling */
	optwr_u8(&device, MCP356X_OPT_OSR, MCP356X_OSR_4096); /* 4096 OSR */
	optwr_u8(&device, MCP356X_OPT_BOOST, 0x3); /* full boost */
	optwr_u8(&device, MCP356X_OPT_GAIN, MCP356X_GAIN_X16); /* 16 GAIn */
	optwr_u8(&device, MCP356X_OPT_AZ_MUX, 1); /* AZ_MUX offset calibration ON */
	optwr_u8(&device, MCP356X_OPT_CONV_MODE, 0x3); /* continous conversion */
	optwr_u8(&device, MCP356X_OPT_IRQ_MODE, 0x1); /* interrupt only on sample ready */

	/* wait for the device to settle */
	os_sleepf(0.1);

	return 0;
}

void p_exit(int code)
{
	exec = false;
	pthread_join(display_thread, NULL);
	mcp356x_close(&device);
	spi_master_close(&master);
	pwm_quit();
	log_quit();
	os_quit();
	exit(code);
}

int main(int argc, char *argv[])
{
	/* init */
	if (p_init(argc, argv)) {
		CRIT_MSG("initialization failed");
		p_exit(EXIT_FAILURE);
	}

	/* signal handlers */
	signal(SIGINT, p_exit);
	signal(SIGTERM, p_exit);

	/* variables for reading */
	double sum = 0;
	int count = 0;
	os_time_t t = os_timef() + INTERVAL;
	struct timespec tp;
	int32_t x;
	double xd;
	char line[1024];
	int n;

	/* start main application loop */
	sampling_started = time(NULL);
	while (1) {
		/* wait for sample to be ready */
		while (gpio_read(IRQ_PIN)) {
			os_sleepf(0.001);
		}

		/* get timestamp as soon as possible */
		clock_gettime(CLOCK_REALTIME, &tp);

		/* get sample */
		x = mcp356x_rd(&device) / 256.0L + OFFSET;
		xd = ((double)x / (double)0x7fffff * MULTIPLIER);

		/* average calculation */
		sum += xd;
		count++;

		/* send to influxdb */
		n = snprintf(line, sizeof(line), "measurements I=%.9lf %lu%09lu\n", xd, tp.tv_sec, tp.tv_nsec);
		sendto(influxdb_udp_socket, line, n, 0, (const struct sockaddr *)&influxdb_addr, sizeof(influxdb_addr));

		/* update value later shown on display with interval so we get an average */
		if (t < os_timef()) {
			/* calculate average */
			sum /= count;
			I = (float)sum;
			printf("%+12.1f uA, samples: %d\n", sum * 1000000.0, count);

			/* reset all */
			sum = 0.0;
			count = 0;
			t += INTERVAL;
		}
	}

	p_exit(EXIT_SUCCESS);
	return 0;
}
/* NAME:   Zachary Berger
 * EMAIL:  zackeberger@gmail.com
 * ID:     705082271 
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define B 4275
#define R0 100000.0

/* Implement DUMMY flag */
#ifdef DUMMY

#define MRAA_SUCCESS 0
#define MRAA_GPIO_IN 0
#define MRAA_GPIO_EDGE_RISING 2

typedef int mraa_aio_context;
typedef int mraa_gpio_context;
typedef int mraa_result_t;
typedef int mraa_gpio_dir_t;
typedef int mraa_gpio_edge_t;

mraa_aio_context mraa_aio_init(unsigned int pin)
{
	(void) pin;
	return 1;
}

mraa_gpio_context mraa_gpio_init(int pin)
{
	(void) pin;
	return 1;
}

void mraa_deinit(void)
{
	return;
}

mraa_result_t mraa_aio_close(mraa_aio_context c)
{
	(void) c;
	return MRAA_SUCCESS;
}

mraa_result_t mraa_gpio_close(mraa_gpio_context c)
{
	(void) c;
	return MRAA_SUCCESS;
}

mraa_result_t mraa_gpio_dir(mraa_gpio_context dev, mraa_gpio_dir_t dir)
{
	(void) dev;
	(void) dir;
	return MRAA_SUCCESS;
}

int mraa_aio_read(mraa_aio_context c)
{
	(void) c;
	return 650;
}

mraa_result_t mraa_gpio_isr(mraa_gpio_context dev, mraa_gpio_edge_t edge, void (*handler), void * args)
{
	(void) dev;
	(void) edge;
	(void) handler;
	(void) args;
	return MRAA_SUCCESS;
}

#else

#include <mraa.h>

#endif



/* Function prototypes */
char * get_current_time(void);
float convert_temp_reading(int reading, char format);
void do_when_pushed(void * arg);
void run_command(char * command);
void close_hardware(void);
void must_write(int fd, char * buffer, ssize_t nbytes);

/* Define optarg struct */
struct option args[] = {
	{"period", required_argument, NULL, 'p'},	/* sampling interval, in seconds */
	{"scale", required_argument, NULL, 's'},	/* C (celsius) or F (fahrenheit) */
	{"log", required_argument, NULL, 'l'},		/* specify a logfile */
	{0,0,0,0}
};

/* Relevant global variables */
char time_ret[16];			/* build timestamp in this array */

mraa_aio_context temp_sensor;		/* mraa aio type */
mraa_gpio_context button;		/* mraa gpio type */

int sample_rate;			/* rate at which temperature is sampled */
char temp_format;			/* either F(ahrenheit) or C(elsius) */
int generate_reports;			/* flag -- generate temp reports */

char * logfile;				/* hold name of logfile */
int logfile_fd;				/* hold file descriptor of logfile */

char raw_command[BUFFER_SIZE];		/* hold the raw user command */
char processed_command[BUFFER_SIZE];	/* hold the processed user command */


int main(int argc, char* argv[])
{
	int i, k;				/* counter variables */

	struct timespec current_time;		/* store the current time */
	time_t take_temp_now;			/* if the time is take_temp_now, take a temperature reading */

	int temp_reading;			/* direct temp reading from temperature sensor */
	float readable_temp;			/* the human readable temperature */

	struct pollfd pollfds[1];		/* polling struct for input */


	/* Set default parameter values */
	sample_rate = 1;
	temp_format = 'F';
	generate_reports = 1;
	logfile = NULL;

	/* Parse program arguments with getopt_long */
	while ( (i = getopt_long(argc, argv, "", args, NULL)) != -1 )
	{
		switch (i)
		{
			case 'p':
				sample_rate = atoi(optarg);		/* Note that atoi lacks error checking */
				break;

			case 's':
				if (strlen(optarg) < 1)
				{
					fprintf(stderr, "Error: bad argument to --scale\n");
					fprintf(stderr, "usage: %s [--period number] [--scale 'C' or 'F'] [--log filename]\n", argv[0]);
					exit(1);
				}

				if (optarg[0] != 'F' && optarg[0] != 'C')
				{
					fprintf(stderr, "Error: bad argument to --scale\n");
					fprintf(stderr, "usage: %s [--period number] [--scale 'C' or 'F'] [--log filename]\n", argv[0]);
					exit(1);
				}

				/* Set the temp format to 'F' or 'C' */
				temp_format = optarg[0];

				break;

			case 'l':
				logfile = optarg;
				break;

			default:
				fprintf(stderr, "usage: %s [--period number] [--scale 'C' or 'F'] [--log filename]\n", argv[0]);
				exit(1);
		}
	}

	/* If log option was specified, maintain a log of temperature readings */
	if (logfile != NULL)
	{
		if ( (logfile_fd = creat(logfile, 0666)) < 0 )
		{
			fprintf(stderr, "Error creating logfile: %s\n", strerror(errno));
			exit(1);
		}
	}

	/* Initialize the sensor */
	temp_sensor = mraa_aio_init(0);
	if ( ((void *) (long) temp_sensor) == NULL)
	{
		fprintf(stderr, "Failed to initialize AIO\n");
		mraa_deinit();
		exit(1);
	}

	/* Initialize the button */
	button = mraa_gpio_init(73);
	if ( ((void*) (long) button) == NULL)
	{
		fprintf(stderr, "Failed to initialize GPIO\n");
		mraa_deinit();
		exit(1);
	}

	/* Set direction of communication for button */
	if (mraa_gpio_dir(button, MRAA_GPIO_IN) != MRAA_SUCCESS)
	{
		fprintf(stderr, "Failed to set GPIO direction\n");
		exit(1);
	}

	/* If the button is pressed, call do_when_pushed */
	mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, do_when_pushed, NULL);

	/* Set up the polling information */
	pollfds[0].fd = STDIN_FILENO;
	pollfds[0].events = POLLIN + POLLHUP + POLLERR;

	/* Set the times */
	clock_gettime(CLOCK_REALTIME, &current_time);
	take_temp_now = current_time.tv_sec; 

	while (1)
	{
		/* Get current time */
		clock_gettime(CLOCK_REALTIME, &current_time);

		if (current_time.tv_sec >= take_temp_now && generate_reports)
		{	
			char to_write[256];
			char reading[16];

			temp_reading = mraa_aio_read(temp_sensor);
			readable_temp = convert_temp_reading(temp_reading, temp_format);
			sprintf(reading, "%.1f", readable_temp);

			/* Build log entry */
			strcpy(to_write, get_current_time());
			strcat(to_write, " ");
			strcat(to_write, reading);
			strcat(to_write, "\n");

			if (logfile != NULL)
			{
				must_write(logfile_fd, to_write, strlen(to_write));
			}

			must_write(STDOUT_FILENO, to_write, strlen(to_write));
			
			/* Get the next time to take the temp */
			take_temp_now = current_time.tv_sec + sample_rate;	
		}

		/* Use poll syscall with no timeout */
		if (poll(pollfds, 1, 0) < 0)
		{
			fprintf(stderr, "poll() error: %s\n", strerror(errno));
			exit(1);
		}

		/* There is data to read */
		if (pollfds[0].revents & POLLIN)
		{
			int r;
			if ( (r = read(STDIN_FILENO, raw_command, sizeof(raw_command))) < 0 )
			{
				fprintf(stderr, "read() failure: %s\n", strerror(errno));
				exit(1);
			}

			/* Cannot assume that one command is in one read.
			 * Use an auxiliary array to hold the processed command that we build up.
			 *
			 * Check read char by char.
			 * 	If a char is '\n', we've reached the end of a command.
			 * 	Else, the command is still building up.
			 */

			i = 0;
			for (k = 0; k < r && i < BUFFER_SIZE; k++)
			{
				if (raw_command[k]  == '\n')
				{
					/* Add a null byte to the command */
					processed_command[i] = '\0';

					/* processed_command holds a full command */
					run_command(processed_command);
					
					/* Prep processed_command for future commands */
					/* Best way to clear an array in C
					 * https://www.quora.com/What-is-the-best-way-to-clear-an-array-using-C-language */	
					i = 0;
					memset(processed_command, 0, sizeof(processed_command));
				}
				else
				{
					/* Build up the processed command */
					processed_command[i] = raw_command[k];
					i++;
				}
			}
		}

		/* If there is a polling error or holdup, we are done, so stop taking input */
		if (pollfds[0].revents & (POLLHUP | POLLERR))
		{
			/* Output SHUTDOWN and exit */
			do_when_pushed(NULL);
		}
	}

	close_hardware();
}

char * get_current_time(void)
{
	struct timespec ts;
	struct tm * tm;

	/* Get the time */
	clock_gettime(CLOCK_REALTIME, &ts);

	/* Get tm with human readable time */
	tm = localtime(&(ts.tv_sec));

	/* Format the output */	
	sprintf(time_ret, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

	return time_ret;
}

/* format is either 'F' or 'C' */
float convert_temp_reading(int reading, char format)
{
	float R = (1023.0 / ((float) reading) ) - 1.0;
	R = R0 * R;

	/* Get temperature in Celsius (C) */
	float C = (1.0 / ((log(R/R0) / B) + (1.0/298.15))) - 273.15;
	
	/* Get temperature in Fahrenheit (F) */
	float F = ((C * 9) / 5) + 32;
	
	if (format == 'C')
	{
		return C;
	}
	else
	{	
		return F;
	}
}


void run_command(char * command)
{
	/* Set up logging info */
	if (logfile != NULL)
	{
		char to_write[256];
		
		strcpy(to_write, command);
		strcat(to_write, "\n");
		
		must_write(logfile_fd, to_write, strlen(to_write));
	}

	/* Process the command */
	if (strcmp(command, "SCALE=F") == 0)
	{
		temp_format = 'F';
	}
	else if (strcmp(command, "SCALE=C") == 0)
	{
		temp_format = 'C';
	}
	else if (strcmp(command, "STOP") == 0)
	{
		generate_reports = 0;	
	}
	else if (strcmp(command, "START") == 0)
	{
		generate_reports = 1;
	}
	else if (strcmp(command, "OFF") == 0)
	{
		/* Output SHUTDOWN and exit */
		do_when_pushed(NULL);
	}
	else
	{
		/* Check if command is PERIOD=# */	
		if (strncmp(command, "PERIOD=", 7) == 0)
		{
			size_t q;
			int num_is_valid = 1;

			/* The command is PERIOD= */
			/* Obtain the number of seconds from argument */
			for (q = 7; q < strlen(command); q++)
			{
				if ( !isdigit(command[q]) )
				{
					num_is_valid = 0;
				}
			}

			if (num_is_valid)
			{
				sample_rate = atoi(command + 7);
			}
			/* else, do nothing -- invalid command */
		}

		/* Don't do anything if command is prefixed with LOG */
		/* Don't do anything if bad command */
	}
}

void do_when_pushed(void * arg)
{
	char to_write[256];

	/* Cast unused argument to void to prevent compiler warning */
	(void) arg;

	strcpy(to_write, get_current_time());
	strcat(to_write, " SHUTDOWN\n");
	
	if (logfile != NULL)
	{
		must_write(logfile_fd, to_write, strlen(to_write));
	}

	must_write(STDOUT_FILENO, to_write, strlen(to_write));
	
	close_hardware();
	exit(0);	
}

void close_hardware(void)
{
	/* Close AIO */
	if (mraa_aio_close(temp_sensor) != MRAA_SUCCESS)
	{
		fprintf(stderr, "Could not close AIO\n");
		exit(1);
	}

	/* Close GPIO */
	if (mraa_gpio_close(button) != MRAA_SUCCESS)
	{
		fprintf(stderr, "Could not close GPIO\n");
		exit(1);
	}
}

/* Utility function to write EXACTLY the specified number of nbytes */
void must_write(int fd, char * buffer, ssize_t nbytes)
{
	int ret;
	int index;

	index = 0;

	while (nbytes > 0)
	{
		if ( (ret = write(fd, buffer + index, nbytes)) < 0 )
		{
			fprintf(stderr, "write() failure: %s\n", strerror(errno));
			exit(1);
		}

		index += ret;
		nbytes -= ret;
	}
}

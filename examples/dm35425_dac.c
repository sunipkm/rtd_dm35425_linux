/**
    @file

    @brief
        Example program which demonstrates the use of the DAC.

    @verbatim

        This example program sends data to the DAC for instant conversion.
        To see the output data, connect an oscilloscope to the AOUT0
        pin (CN3 Pin 17) and AGND (CN3 Pin 18).

        The user can control what value goes out the DAC by using keys to
        increase or decrease the desired voltage.

        Follow the on-screen instructions for adjusting the voltage.

        Press 'q' to quit the program.

    @endverbatim

    @verbatim
    --------------------------------------------------------------------------
    This file and its contents are copyright (C) RTD Embedded Technologies,
    Inc.  All Rights Reserved.

    This software is licensed as described in the RTD End-User Software License
    Agreement.  For a copy of this agreement, refer to the file LICENSE.TXT
    (which should be included with this software) or contact RTD Embedded
    Technologies, Inc.
    --------------------------------------------------------------------------
    @endverbatim

    $Id: dm35425_dac.c 108025 2017-04-14 15:09:34Z rgroner $
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include "dm35425_gbc_library.h"
#include "dm35425_dac_library.h"
#include "dm35425_ioctl.h"
#include "dm35425_examples.h"
#include "dm35425.h"
#include "dm35425_util_library.h"

/**
 * Define a default range to use, if the user does not
 * provide one.
 */
#define DEFAULT_RANGE		DM35425_DAC_RNG_BIPOLAR_5V

/**
 * Define a channel to use, if the user does not
 * provide one.
 */
#define DEFAULT_CHANNEL		0

/**
 * Name of the program as invoked on the command line
 */
static char *program_name;

/**
*******************************************************************************
@brief
    Print information on stderr about how the program is to be used.  After
    doing so, the program is exited.
 *******************************************************************************
*/
static void usage(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "NAME\n\n\t%s\n\n", program_name);
	fprintf(stderr, "USAGE\n\n\t%s [OPTIONS]\n\n", program_name);

	fprintf(stderr, "OPTIONS\n\n");

	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tShow this help screen and exit.\n");

	fprintf(stderr, "\t--minor NUM\n");
	fprintf(stderr,
		"\t\tSpecify the minor number (>= 0) of the board to open.  When not specified,\n");
	fprintf(stderr, "\t\tthe device file with minor 0 is opened.\n");

	fprintf(stderr, "\t--channel CHAN\n");
	fprintf(stderr,
		"\t\tUse the specified DAC input channel.  Defaults to channel 0.\n");

	fprintf(stderr, "\t--range RNG\n");
	fprintf(stderr, "\t\tUse the specified range and mode of the DAC.\n");
	fprintf(stderr, "\t\t\t10B = 10V, Bipolar\n");
	fprintf(stderr, "\t\t\t5B = 5V, Bipolar (Default)\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

/**
*******************************************************************************
@brief
    The main program.

@param
    argument_count

    Number of args passed on the command line, including the executable name

@param
    arguments

    Pointer to array of character strings, which are the args themselves.

@retval
    0

    Success

@retval
    Non-Zero

    Failure.

 *******************************************************************************
*/
int main(int argument_count, char **arguments)
{

	struct DM35425_Board_Descriptor *board;
	struct DM35425_Function_Block my_dac;
	unsigned long int minor = 0;
	int result;

	unsigned int range = DEFAULT_RANGE;
	unsigned int channel = DEFAULT_CHANNEL;

	int help_option_given = 0;
	struct termios old_tio, new_tio;

	int status;
	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"range", 1, 0, RANGE_OPTION},
		{"channel", 1, 0, CHANNELS_OPTION},
		{0, 0, 0, 0}
	};

	float voltage = 0.0;
	int16_t conv_value = 0;
	int increment = 1;
	unsigned char keypress = '0';

	uint8_t marker;
	struct timeval start_pressing;
	struct timeval next_press;
	struct timeval temp_clock;
	float time_difference = 0.0;

	program_name = arguments[0];

	// Show usage, parse arguments
	while (1) {
		/*
		 * Parse the next command line option and any arguments it may require
		 */
		status = getopt_long(argument_count,
				     arguments, "", options, NULL);

		/*
		 * If getopt_long() returned -1, then all options have been processed
		 */
		if (status == -1) {
			break;
		}

		/*
		 * Figure out what getopt_long() found
		 */
		switch (status) {

			/*#################################################################
			   User entered '--help'
			   ################################################################# */
		case HELP_OPTION:
			help_option_given = 0xFF;
			break;

			/*#################################################################
			   User entered '--minor'
			   ################################################################# */
		case MINOR_OPTION:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			minor = strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if ((minor == ULONG_MAX)
			    && (errno == ERANGE)) {
				error(0, 0,
				      "ERROR: Device minor number caused numeric overflow");
				usage();
			}

			/*
			 * Catch argument strings with valid decimal prefixes, for
			 * example "1q", and argument strings which cannot be converted,
			 * for example "abc1"
			 */
			if ((*invalid_char_p != '\0')
			    || (invalid_char_p == optarg)) {
				error(0, 0,
				      "ERROR: Non-decimal device minor number");
				usage();
			}

			break;

			/*#################################################################
			   User entered '--channel'
			   ################################################################# */
		case CHANNELS_OPTION:
			/*
			 * Convert channel argument string to unsigned long integer
			 */
			errno = 0;
			channel = strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if ((channel == ULONG_MAX)
			    && (errno == ERANGE)) {
				error(0, 0,
				      "ERROR: Channel number caused numeric overflow");
				usage();
			}

			/*
			 * Catch argument strings with valid decimal prefixes, for
			 * example "1q", and argument strings which cannot be converted,
			 * for example "abc1"
			 */
			if ((*invalid_char_p != '\0')
			    || (invalid_char_p == optarg)) {
				error(0, 0,
				      "ERROR: Non-decimal device minor number");
				usage();
			}

			if (channel >= DM35425_NUM_DAC_DMA_CHANNELS) {
				error(0, 0,
				      "ERROR: Channel number must be from 0 to %u.",
				      DM35425_NUM_DAC_DMA_CHANNELS - 1);
				usage();
			}
			break;

			/*#################################################################
			   User entered --range option
			   ################################################################# */
		case RANGE_OPTION:
			if (strcmp(optarg, "10B") == 0) {
				range = DM35425_DAC_RNG_BIPOLAR_10V;
			} else if (strcmp(optarg, "5B") == 0) {
				range = DM35425_DAC_RNG_BIPOLAR_5V;
			} else {
				error(0, 0,
				      "ERROR: Range and mode entered did not match available options.");
				usage();
			}

			break;

			/*#################################################################
			   User entered unsupported option
			   ################################################################# */
		case '?':
			usage();
			break;

			/*#################################################################
			   getopt_long() returned unexpected value
			   ################################################################# */
		default:
			error(EXIT_FAILURE,
			      0,
			      "ERROR: getopt_long() returned unexpected value %#x",
			      status);
			break;
		}
	}

	/*
	 * Recognize '--help' option before any others
	 */

	if (help_option_given) {
		usage();
	}

	printf("Opening board.....");
	result = DM35425_Board_Open(minor, &board);

	check_result(result, "Could not open board");
	printf("success.\nResetting board.....");
	result = DM35425_Gbc_Board_Reset(board);

	check_result(result, "Could not reset board");
	printf("success.\nOpening DAC......\n");

	result = DM35425_Dac_Open(board, DAC_0, &my_dac);

	check_result(result, "Could not open DAC");

	printf("Found DAC0, with %d DMA channels (%d buffers each)\n",
	       my_dac.num_dma_channels, my_dac.num_dma_buffers);

	printf("Using Channel %u\n", channel);

	result = DM35425_Dac_Channel_Setup(board, &my_dac, channel, range);

	check_result(result, "Error setting output range.");

	result = DM35425_Dac_Reset(board, &my_dac);

	check_result(result, "Error stopping DAC");

	tcgetattr(STDIN_FILENO, &old_tio);

	new_tio = old_tio;

	new_tio.c_lflag &= ~ICANON;
	new_tio.c_lflag &= ~ECHO;

	tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

	result = DM35425_Dac_Volts_To_Conv(range, 0.0f, &conv_value);

	check_result(result, "Error converting voltage to conversion");

	result = DM35425_Dac_Set_Last_Conversion(board,
						 &my_dac,
						 channel, 0, conv_value);
	check_result(result, "Error setting last conversion");

	printf
	    ("\n\nPress 'i' to increase the voltage, and 'd' to decrease it.\n");
	printf("Hold down the key to change the voltage more rapidly.\n");
	printf("Press '1' for 1.0 V, '2' for 2.0 V, etc.\n");
	printf
	    ("For negative numbers, hold down the Shift key ('Shift-1', 'Shift-2', etc)\n");
	printf("Press 'q' to quit.\n\n");
	printf("Sample: %6d\tVoltage: %3.6f       \r", conv_value, voltage);
	gettimeofday(&start_pressing, NULL);

	while (keypress != 'q') {
		gettimeofday(&next_press, NULL);
		keypress = getchar();

		gettimeofday(&temp_clock, NULL);
		time_difference = DM35425_Get_Time_Diff(temp_clock, next_press);
		if (time_difference < 38000.0f) {
			/* They're holding down the key.  The longer it is held down,
			 * the larger the increment in the value to output the DAC,
			 * thus the faster the change in voltage.
			 */
			next_press = temp_clock;
			increment = (((int)
				      DM35425_Get_Time_Diff(next_press,
							    start_pressing)) +
				     1) / 50000;

		} else {
			start_pressing = temp_clock;
			next_press = temp_clock;
			increment = 1;

		}

		if (keypress == 'i') {
			if (conv_value <= (DM35425_DAC_MAX - increment)) {
				conv_value += increment;
			} else {
				conv_value = DM35425_DAC_MAX;
			}

		}

		if (keypress == 'd') {

			if (conv_value >= (DM35425_DAC_MIN + increment)) {
				conv_value -= increment;
			} else {
				conv_value = DM35425_DAC_MIN;
			}
		}

		if (keypress == '0') {
			result = DM35425_Dac_Volts_To_Conv(range,
							   0, &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		if (keypress == '1') {
			result = DM35425_Dac_Volts_To_Conv(range,
							   1, &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		if (keypress == '2') {
			result = DM35425_Dac_Volts_To_Conv(range,
							   2, &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		if (keypress == '3') {
			result = DM35425_Dac_Volts_To_Conv(range,
							   3, &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		if (keypress == '4') {
			result = DM35425_Dac_Volts_To_Conv(range,
							   4, &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		if (keypress == '5') {

			/**
			 * The DAC cannot achieve +5.0 volts, but for purposes of
			 * the example, we allow them to select 5 as a value.
			 * However, we'll have to request the actual max
			 * value from the library function.
			 */
			result = DM35425_Dac_Volts_To_Conv(range,
							   4.999847412f,
							   &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		if (range == DM35425_DAC_RNG_BIPOLAR_10V) {

			if (keypress == '5') {

				result = DM35425_Dac_Volts_To_Conv(range,
								   5,
								   &conv_value);

				check_result(result,
					     "Error converting voltage to conversion");
			}

			if (keypress == '6') {
				result = DM35425_Dac_Volts_To_Conv(range,
								   6,
								   &conv_value);

				check_result(result,
					     "Error converting voltage to conversion");
			}

			if (keypress == '7') {

				result = DM35425_Dac_Volts_To_Conv(range,
								   7,
								   &conv_value);

				check_result(result,
					     "Error converting voltage to conversion");
			}

			if (keypress == '8') {
				result = DM35425_Dac_Volts_To_Conv(range,
								   8,
								   &conv_value);

				check_result(result,
					     "Error converting voltage to conversion");
			}

			if (keypress == '9') {

				result = DM35425_Dac_Volts_To_Conv(range,
								   9,
								   &conv_value);

				check_result(result,
					     "Error converting voltage to conversion");
			}

			if (keypress == '^') {

				result = DM35425_Dac_Volts_To_Conv(range,
								   -6,
								   &conv_value);

				check_result(result,
					     "Error converting voltage to conversion");
			}

			if (keypress == '&') {
				result = DM35425_Dac_Volts_To_Conv(range,
								   -7,
								   &conv_value);

				check_result(result,
					     "Error converting voltage to conversion");
			}

			if (keypress == '*') {

				result = DM35425_Dac_Volts_To_Conv(range,
								   -8,
								   &conv_value);

				check_result(result,
					     "Error converting voltage to conversion");
			}

			if (keypress == '(') {
				result = DM35425_Dac_Volts_To_Conv(range,
								   -9,
								   &conv_value);

				check_result(result,
					     "Error converting voltage to conversion");
			}

		}

		if (keypress == '!') {
			result = DM35425_Dac_Volts_To_Conv(range,
							   -1, &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		if (keypress == '@') {
			result = DM35425_Dac_Volts_To_Conv(range,
							   -2, &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		if (keypress == '#') {
			result = DM35425_Dac_Volts_To_Conv(range,
							   -3, &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		if (keypress == '$') {
			result = DM35425_Dac_Volts_To_Conv(range,
							   -4, &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		if (keypress == '%') {
			result = DM35425_Dac_Volts_To_Conv(range,
							   -5, &conv_value);

			check_result(result,
				     "Error converting voltage to conversion");
		}

		result = DM35425_Dac_Set_Last_Conversion(board,
							 &my_dac,
							 channel,
							 0, conv_value);
		check_result(result, "Error setting last conversion");

		if (result != 0) {
			printf("ERROR setting last conversion.  Errno = %d.\n",
			       errno);
			keypress = 'q';
		}

		result = DM35425_Dac_Get_Last_Conversion(board,
							 &my_dac,
							 channel,
							 &marker, &conv_value);

		if (result != 0) {
			printf("ERROR getting last conversion.  Errno = %d.\n",
			       errno);
			keypress = 'q';
		}

		result = DM35425_Dac_Conv_To_Volts(range, conv_value, &voltage);

		if (result != 0) {
			printf
			    ("ERROR converting conversion to voltage.  Errno = %d.\n",
			     errno);
			keypress = 'q';
		}

		printf("Sample: %6d \tVoltage: %3.6f    \r", conv_value,
		       voltage);

	}

	tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

	printf("\n\nClosing Board\n");
	result = DM35425_Board_Close(board);

	check_result(result, "Error closing board.");

	printf("Example program successfully completed.\n");
	return 0;

}

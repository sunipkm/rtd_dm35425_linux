/**
    @file

    @brief
        Example program which demonstrates the use of the DAC and DMA.

    @verbatim

        This example program generates wave form data and "plays" it out
        the specified DAC channel.  To see the output data, connect an
        oscilloscope to the AOUT0 pin.

        After the program is running, you can alter the rate of DAC output
        by entering a new frequency and hitting Enter.  Note that the frequency
        of the waveform seen on an oscilloscope will be different than the frequency
        of the DAC, depending on the number of samples used in creating the wave.

	Use the --help command line option to see all possible input values.

        Hit Ctrl-C to exit the example.

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

    $Id: dm35425_dac_dma.c 108025 2017-04-14 15:09:34Z rgroner $
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>

#include "dm35425_gbc_library.h"
#include "dm35425_dac_library.h"
#include "dm35425_ioctl.h"
#include "dm35425_examples.h"
#include "dm35425_dma_library.h"
#include "dm35425_util_library.h"
#include "dm35425.h"

/**
 * We will only use one buffer in this example, and loop it
 */
#define NUM_BUFFERS_TO_USE	1

/**
 * Rate to use if user does not enter one on the command line (Hz)
 */
#define DEFAULT_RATE		100

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
 * Number of samples to create.  Increase this number for a "finer"
 * waveform.
 */
#define BUFFER_SIZE_SAMPLES	100

/**
 * Buffer size to allocate in bytes
 */
#define BUFFER_SIZE_BYTES	(BUFFER_SIZE_SAMPLES * sizeof(int))

/**
 * Name of the program as invoked on the command line
 */

static char *program_name;

/**
 * Boolean indicating the program should be exited.
 */
volatile int exit_program = 0;

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
	fprintf(stderr, "USAGE\n\n\t%s WAVE [OPTIONS]\n\n", program_name);

	fprintf(stderr, "WAVE (Required)\n\n");
	fprintf(stderr, "\t--wave WAVEFORM\n");
	fprintf(stderr,
		"\t\tSpecify the waveform to be output.  Possible values are square,\n");
	fprintf(stderr, "\t\tsine, and sawtooth.\n");
	fprintf(stderr, "\n");

	fprintf(stderr, "OPTIONS\n\n");

	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tShow this help screen and exit.\n");

	fprintf(stderr, "\t--minor NUM\n");
	fprintf(stderr,
		"\t\tSpecify the minor number (>= 0) of the board to open.  When not specified,\n");
	fprintf(stderr, "\t\tthe device file with minor 0 is opened.\n");

	fprintf(stderr, "\t--rate RATE\n");
	fprintf(stderr,
		"\t\tUse the specified rate (Hz).  The default is %d.\n",
		DEFAULT_RATE);

	fprintf(stderr, "\t--channel CHAN\n");
	fprintf(stderr,
		"\t\tUse the specified DAC input channel.  Defaults to channel 0.\n");

	fprintf(stderr, "\t--range RNG\n");
	fprintf(stderr, "\t\tUse the specified range of the DAC.\n");
	fprintf(stderr, "\t\t\t10B = 10V, Bipolar\n");
	fprintf(stderr, "\t\t\t5B = 5V, Bipolar (Default)\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

/**
*******************************************************************************
@brief
    Signal handler for SIGINT Control-C keyboard interrupt.

@param
    signal_number

    Signal number passed in from the kernel.

@warning
    One must be extremely careful about what functions are called from a signal
    handler.
 *******************************************************************************
*/

static void sigint_handler(int signal_number)
{
	exit_program = 0xff;
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
	char input_str[200];

	struct sigaction signal_action;

	unsigned int current_buffer;
	uint32_t current_count;
	int current_action;
	int status_overflow;
	int status_underflow;
	int status_used;
	int status_invalid;
	int status_complete;

	uint8_t buff_status;
	uint8_t buff_control;
	uint32_t buff_size;

	uint32_t rate = DEFAULT_RATE, actual_rate;
	unsigned int range = DEFAULT_RANGE;
	unsigned int channel = DEFAULT_CHANNEL;

	int help_option_given = 0;
	int wave_option_given = 0;

	int status;
	char waveform_str[100];
	enum DM35425_Waveforms waveform = DM35425_SINE_WAVE;

	int32_t *buffer;
	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"rate", 1, 0, RATE_OPTION},
		{"wave", 1, 0, WAVE_OPTION},
		{"range", 1, 0, RANGE_OPTION},
		{"channel", 1, 0, CHANNELS_OPTION},
		{0, 0, 0, 0}
	};

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
			   User entered rate
			   ################################################################# */
		case RATE_OPTION:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			rate = strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if ((rate == ULONG_MAX)
			    && (errno == ERANGE)) {
				error(0, 0,
				      "ERROR: Rate number caused numeric overflow");
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
				      "ERROR: Non-decimal rate value entered");
				usage();
			}
			break;

			/*#################################################################
			   User entered a waveform choice
			   ################################################################# */
		case WAVE_OPTION:
			strcpy(waveform_str, optarg);
			wave_option_given = 1;
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

	if (!wave_option_given) {
		error(0, 0, "ERROR: Please specify a waveform to display.");
		usage();
	}

	if (strcmp(waveform_str, "sine") == 0) {
		waveform = DM35425_SINE_WAVE;
	} else if (strcmp(waveform_str, "square") == 0) {
		waveform = DM35425_SQUARE_WAVE;
	} else if (strcmp(waveform_str, "sawtooth") == 0) {
		waveform = DM35425_SAWTOOTH_WAVE;
	} else {
		error(0, 0,
		      "ERROR: Invalid waveform specified.  Please use either sine, square, or sawtooth.");
		usage();
	}

	signal_action.sa_handler = sigint_handler;
	sigfillset(&(signal_action.sa_mask));
	signal_action.sa_flags = 0;

	if (sigaction(SIGINT, &signal_action, NULL) < 0) {
		error(EXIT_FAILURE, errno, "ERROR: sigaction() FAILED");
	}

	printf("Opening board.....");
	result = DM35425_Board_Open(minor, &board);

	check_result(result, "Could not open board");
	printf("success.\nResetting board.....");
	result = DM35425_Gbc_Board_Reset(board);

	check_result(result, "Could not reset board");
	printf("success.\nOpening DAC......");

	result = DM35425_Dac_Open(board, DAC_0, &my_dac);

	check_result(result, "Could not open DAC");

	printf("Found DAC 0, with %d DMA channels (%d buffers each)\n",
	       my_dac.num_dma_channels, my_dac.num_dma_buffers);
	printf("Using Channel %u\n", channel);

	result = DM35425_Dac_Set_Clock_Src(board,
					   &my_dac, DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting DAC clock");

	result = DM35425_Dac_Set_Conversion_Rate(board,
						 &my_dac, rate, &actual_rate);

	fprintf(stdout, "Rate requested: %d  Actual Rate Achieved: %d\n", rate,
		actual_rate);
	check_result(result, "Error setting sample rate");

	buffer = (int *)malloc(BUFFER_SIZE_BYTES);

	check_result(buffer == NULL, "Error allocating space for buffer.");

	result = DM35425_Generate_Signal_Data(waveform,
					      buffer,
					      BUFFER_SIZE_SAMPLES,
					      DM35425_DAC_MAX,
					      DM35425_DAC_MIN, 0, 0x00000FFF);

	check_result(result, "Error trying to generate data for the DAC.");

	fprintf(stdout, "Initializing and configuring DMA Channel %d....",
		CHANNEL_0);
	result =
	    DM35425_Dma_Initialize(board, &my_dac, channel,
				   NUM_BUFFERS_TO_USE, BUFFER_SIZE_BYTES);

	check_result(result, "Error initializing DMA");

	result = DM35425_Dma_Setup(board,
				   &my_dac,
				   channel,
				   DM35425_DMA_SETUP_DIRECTION_WRITE,
				   IGNORE_USED);

	check_result(result, "Error configuring DMA");

	fprintf(stdout, "success!\n");

	result = DM35425_Dma_Status(board,
				    &my_dac,
				    channel,
				    &current_buffer,
				    &current_count,
				    &current_action,
				    &status_overflow,
				    &status_underflow,
				    &status_used,
				    &status_invalid, &status_complete);

	check_result(result, "Error getting DMA status");

	printf
	    ("DMA Status: Current Buffer: %d  Count: %d  Action: 0x%x  Status: "
	     "Ov: %d  Un: %d  Used: %d  Inv: %d  Comp: %d\n", current_buffer,
	     current_count, current_action, status_overflow, status_underflow,
	     status_used, status_invalid, status_complete);

	result = DM35425_Dma_Buffer_Setup(board,
					  &my_dac,
					  channel,
					  BUFFER_0,
					  DM35425_DMA_BUFFER_CTRL_VALID |
					  DM35425_DMA_BUFFER_CTRL_LOOP);

	check_result(result, "Error setting up buffer control.");

	result = DM35425_Dma_Buffer_Status(board,
					   &my_dac,
					   channel,
					   BUFFER_0,
					   &buff_status,
					   &buff_control, &buff_size);
	fprintf(stdout, "    Buffer 0: Stat: 0x%x  Ctrl: 0x%x  Size: %u\n",
		buff_status, buff_control, buff_size);

	check_result(result, "Error getting buffer status.");

	result = DM35425_Dma_Write(board,
				   &my_dac,
				   channel,
				   BUFFER_0, BUFFER_SIZE_BYTES, buffer);

	check_result(result, "Writing to DMA buffer failed");

	free(buffer);

	fprintf(stdout, "Starting DMA Channel %d......", channel);
	result = DM35425_Dma_Start(board, &my_dac, channel);

	check_result(result, "Error starting DMA");

	printf("success.\n");

	fprintf(stdout, "Starting DAC.\n");

	result = DM35425_Dac_Set_Start_Trigger(board,
					       &my_dac,
					       DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting start trigger for DAC.");

	result = DM35425_Dac_Set_Stop_Trigger(board,
					      &my_dac, DM35425_CLK_SRC_NEVER);

	check_result(result, "Error setting stop trigger for DAC.");

	result = DM35425_Dac_Channel_Setup(board, &my_dac, channel, range);

	check_result(result, "Error setting output range for DAC.");

	result = DM35425_Dac_Start(board, &my_dac);

	check_result(result, "Error starting DAC");

	printf("\nPress Ctrl-C to exit.\n\n");
	while (!exit_program) {

		printf("Current Rate: %d    Enter new rate: ", actual_rate);
		if (scanf("%s", input_str) > 0) {
			rate = atoi(input_str);

			if (rate > 0) {
				result = DM35425_Dac_Set_Conversion_Rate(board,
									 &my_dac,
									 rate,
									 &actual_rate);

				check_result(result,
					     "Error setting sample rate");
			}
		}

	}

	printf("success.\nClosing Board....");
	result = DM35425_Board_Close(board);

	check_result(result, "Error closing board.");
	printf("success.\nExample program successfully completed.\n\n");

	return 0;

}

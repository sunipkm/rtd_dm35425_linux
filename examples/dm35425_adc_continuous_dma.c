/**
    @file

    @brief
        Example program which demonstrates the use of the ADC and
        DMA.

    @verbatim

        This example program will collect data from the ADC(s)
        specified by the user, at the rate specified by the user, and will
        write the data to a file.  It will do this continuously until the
        user hits CTRL-C (or the filesystem becomes full).

        Connect the signal of interest to AIN0 (pin 1 of CN3) and AGND
        (pin 21 of CN3).

        This is a very intensive operation for the PC, working CPU, memory,
        and file I/O fairly hard.  Thus, there is no way to determine
        for sure what the highest sustainable rate of collecting data is.

	Maximum sustainable throughput is HIGHLY system dependent. Higher
	sample rates might be achievable through better buffer size
	selection or use of an operating system with realtime features.

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

    $Id: dm35425_adc_continuous_dma.c 108578 2017-05-04 20:38:56Z rgroner $
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <getopt.h>
#include <string.h>

#include "dm35425_gbc_library.h"
#include "dm35425_adc_library.h"
#include "dm35425_dac_library.h"
#include "dm35425_ioctl.h"
#include "dm35425_examples.h"
#include "dm35425_dma_library.h"
#include "dm35425.h"
#include "dm35425_util_library.h"
#include "dm35425_os.h"

/**
 * Default rate to use, if user does not enter one. (Hz)
 */
#define DEFAULT_RATE		1000

/**
 * Default rate to use for the DAC
 */
#define DAC_RATE		10000

/**
 * Define a channel to use, if the user does not
 * provide one.
 */
#define DEFAULT_CHANNEL		0

/**
 * Define a default range to use, if the user does not
 * provide one.
 */
#define DEFAULT_RANGE		DM35425_ADC_RNG_BIPOLAR_5V

/**
 * Size of the DMA buffer to be used by the DAC
 */
#define DAC_BUFFER_SIZE_SAMPLES		10000

/**
 * Size of the DMA buffer for the DAC, in bytes
 */
#define DAC_BUFFER_SIZE_BYTES		(DAC_BUFFER_SIZE_SAMPLES * sizeof(int))

/**
 * Name of file when saving as ASCII
 */
#define ASCII_FILE_NAME "./adc_dma.txt"

/**
 * Name of file when saving as binary
 */
#define BIN_FILE_NAME "./adc_dma.bin"

/**
 * Name of the program as invoked on the command line
 */
static char *program_name;

/**
 * Boolean flag indicating if there was a DMA error.
 */
static int dma_has_error = 0;

/**
 * Pointer to board descriptor
 */
static struct DM35425_Board_Descriptor *board;

/**
 * Pointer to array of function blocks that will hold the ADC descriptors
 */
static struct DM35425_Function_Block my_adc;

/**
 * Array of buffer counts, used to track progress of each ADC
 * as data is copied.
 */
static unsigned long buffer_count;

/**
 * Pointer to local memory buffer where data is copied from the kernel buffers
 * when a DMA buffer becomes full.
 */
static int **local_buffer;

/**
 * Boolean indicating the program should exit.
 */
static volatile int exit_program = 0;

/**
 * Size of the buffer allocated, in bytes.
 */
static unsigned long buffer_size_bytes = 0;

/**
 * ADC Channel to use
 */
static unsigned int channel = DEFAULT_CHANNEL;

/**
 * Which buffer is next to be copied from DMA
 */
static unsigned int next_buffer;

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

	fprintf(stderr, "\t--rate RATE\n");
	fprintf(stderr,
		"\t\tUse the specified rate (Hz).  The default is %d.\n",
		DEFAULT_RATE);

	fprintf(stderr, "\t--samples NUM\n");
	fprintf(stderr,
		"\t\tStop the example after NUM samples have been collected.  Note that\n");
	fprintf(stderr,
		"\t\tthe actual number of samples taken might be larger due to buffer sizes.\n");

	fprintf(stderr, "\t--binary\n");
	fprintf(stderr,
		"\t\tWrite data to file in binary format, instead of default ASCII.\n");

	fprintf(stderr, "\t--bin2txt\n");
	fprintf(stderr, "\t\tThe program will convert the %s file to\n",
		BIN_FILE_NAME);
	fprintf(stderr, "\t\t%s and exit.\n\n", ASCII_FILE_NAME);
	fprintf(stderr,
		"\t\tNote: Because the rate affects the buffer size, and the\n");
	fprintf(stderr,
		"\t\tnumber of ADC affects data layout, you must include the\n");
	fprintf(stderr,
		"\t\t--rate and --num_adc arguments as well, IF they were used\n");
	fprintf(stderr, "\t\tto create the binary file in the first place.\n");

	fprintf(stderr, "\t--channel CHAN\n");
	fprintf(stderr,
		"\t\tUse the specified ADC input channel.  Defaults to channel 0.\n");

	fprintf(stderr, "\t--range RNG\n");
	fprintf(stderr,
		"\t\tUse the specified range and mode of the ADC. Default is 5V Bipolar (5B).\n");
	fprintf(stderr, "\t\t\t10B = 10V, Bipolar\n");
	fprintf(stderr, "\t\t\t10U = 10V, Unipolar\n");
	fprintf(stderr, "\t\t\t5B = 5V, Bipolar\n");
	fprintf(stderr, "\t\t\t5U = 5V, Unipolar\n");
	fprintf(stderr, "\t\t\t2.5B = 2.5V, Bipolar\n");
	fprintf(stderr, "\t\t\t2.5U = 2.5V, Unipolar\n");
	fprintf(stderr, "\t\t\t1.25B = 1.25V, Bipolar\n");
	fprintf(stderr, "\t\t\t1.25U = 1.25V, Unipolar\n");
	fprintf(stderr, "\t\t\t.625B = 0.625V, Bipolar\n");
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
    Output the status of a DMA channel.  This is a helper function to determine
    the cause of an error when it occurs.

@param
    handle

    Pointer to the board handle.

@param
    func_block

    Pointer to the function block containing the DMA channel

@param
    channel

    The DMA channel we want the status of.

 @retval
    None
 *******************************************************************************
*/

void output_channel_status(struct DM35425_Board_Descriptor *handle,
			   const struct DM35425_Function_Block *func_block,
			   unsigned int channel)
{
	int result;
	unsigned int current_buffer;
	uint32_t current_count;
	int current_action;
	int status_overflow;
	int status_underflow;
	int status_used;
	int status_invalid;
	int status_complete;

	result = DM35425_Dma_Status(handle,
				    func_block,
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
	    ("FB%d Ch%d DMA Status: Current Buffer: %u  Count: %ul  Action: 0x%x  Status: "
	     "Ov: %d  Un: %d  Used: %d  Inv: %d  Comp: %d\n",
	     func_block->fb_num, channel, current_buffer, current_count,
	     current_action, status_overflow, status_underflow, status_used,
	     status_invalid, status_complete);
}

/**
*******************************************************************************
@brief
    The interrupt subroutine that will execute when a DMA interrupt occurs.
    This function will read from the DMA, copying data from the kernel buffers
    to the user buffers so that we can access the data.

@param
    int_info

    A structure containing information about the interrupt.

 @retval
    None.
 *******************************************************************************
*/
static void ISR(struct dm35425_ioctl_interrupt_info_request int_info)
{

	int result = 0;
	int buffer_full = 0;
	int dma_error = 0;
	int chan_complete, chan_error;
	unsigned int chan_with_int;

	if (int_info.valid_interrupt) {

		// It's a DMA interrupt
		if (int_info.interrupt_fb < 0) {

			result = DM35425_Dma_Find_Interrupt(board,
							    &my_adc,
							    &chan_with_int,
							    &chan_complete,
							    &chan_error);

			check_result(result, "Error finding DMA interrupt.");

			if (!chan_complete && !chan_error) {
				printf("** ISR called with no interrupt set.");
				return;
			}

			result = DM35425_Dma_Check_For_Error(board,
							     &my_adc,
							     channel,
							     &dma_error);

			check_result(result, "Error checking for DMA error.");

			if (dma_error) {
				dma_has_error = 1;
				exit_program = 1;
				return;
			}
			result = DM35425_Dma_Check_Buffer_Used(board,
							       &my_adc,
							       channel,
							       next_buffer,
							       &buffer_full);

			check_result(result, "Error finding used buffer.");

			while (buffer_full) {
				result = DM35425_Dma_Read(board,
							  &my_adc,
							  channel,
							  next_buffer,
							  buffer_size_bytes,
							  local_buffer
							  [next_buffer]);

				buffer_count++;
				check_result(result,
					     "Error getting DMA buffer");

				result = DM35425_Dma_Reset_Buffer(board,
								  &my_adc,
								  channel,
								  next_buffer);

				check_result(result, "Error resetting buffer");

				next_buffer =
				    (next_buffer + 1) % my_adc.num_dma_buffers;

				result = DM35425_Dma_Check_Buffer_Used(board,
								       &my_adc,
								       channel,
								       next_buffer,
								       &buffer_full);

				check_result(result,
					     "Error finding used buffer.");

			}

			result = DM35425_Dma_Clear_Interrupt(board,
							     &my_adc,
							     channel,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     CLEAR_INTERRUPT);

		} else {
			printf("*** Process non-DMA interrupt for FB 0x%x.\n",
			       int_info.interrupt_fb);
		}

		result = DM35425_Gbc_Ack_Interrupt(board);

		check_result(result, "Error calling ACK interrupt.");

	}

}

/**
*******************************************************************************
@brief
    This function will setup the DAC to provide an output sine wave that
    can be used for the testing of ADC DMA.

    It will setup DAC Channel 0 only.

 @retval
    None.
 *******************************************************************************
*/
static void setup_dacs()
{

	struct DM35425_Function_Block my_dac;
	unsigned int actual_rate;
	int32_t *dac_buffer;
	int result;

	result = DM35425_Dac_Open(board, DAC_0, &my_dac);

	check_result(result, "Could not open DAC");

	result = DM35425_Dac_Set_Clock_Src(board,
					   &my_dac, DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting DAC clock");

	result = DM35425_Dac_Set_Conversion_Rate(board,
						 &my_dac,
						 DAC_RATE, &actual_rate);

	check_result(result, "Error setting sample rate");

	result = DM35425_Dac_Channel_Setup(board,
					   &my_dac,
					   CHANNEL_0,
					   DM35425_DAC_RNG_BIPOLAR_5V);

	check_result(result, "Error setting DAC output range");

	dac_buffer = (int *)malloc(DAC_BUFFER_SIZE_BYTES);

	check_result(dac_buffer == NULL, "Error allocating space for buffer.");

	result = DM35425_Generate_Signal_Data(DM35425_SINE_WAVE,
					      dac_buffer,
					      DAC_BUFFER_SIZE_SAMPLES,
					      DM35425_DAC_MAX,
					      DM35425_DAC_MIN, 0, 0x00000FFF);

	check_result(result, "Error trying to generate data for the DAC.");

	fprintf(stdout, "Initializing and configuring DAC DMA.");

	result =
	    DM35425_Dma_Initialize(board, &my_dac,
				   CHANNEL_0, 1, DAC_BUFFER_SIZE_BYTES);

	check_result(result, "Error initializing DMA");

	result = DM35425_Dma_Setup(board,
				   &my_dac,
				   CHANNEL_0,
				   DM35425_DMA_SETUP_DIRECTION_WRITE,
				   IGNORE_USED);

	check_result(result, "Error configuring DMA");

	fprintf(stdout, "success!\n");

	result = DM35425_Dma_Buffer_Setup(board,
					  &my_dac,
					  CHANNEL_0,
					  BUFFER_0,
					  DM35425_DMA_BUFFER_CTRL_VALID |
					  DM35425_DMA_BUFFER_CTRL_LOOP);

	check_result(result, "Error setting up buffer control.");

	result = DM35425_Dma_Write(board,
				   &my_dac,
				   CHANNEL_0,
				   BUFFER_0, DAC_BUFFER_SIZE_BYTES, dac_buffer);

	check_result(result, "Writing to DMA buffer failed");

	fprintf(stdout, "Starting DMA Channel %d......", CHANNEL_0);
	result = DM35425_Dma_Start(board, &my_dac, CHANNEL_0);

	check_result(result, "Error starting DMA");

	printf("success.\n");

	free(dac_buffer);

	fprintf(stdout, "Starting DAC.\n");

	result = DM35425_Dac_Set_Start_Trigger(board,
					       &my_dac,
					       DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting start trigger for DAC.");

	result = DM35425_Dac_Set_Stop_Trigger(board,
					      &my_dac, DM35425_CLK_SRC_NEVER);

	check_result(result, "Error setting stop trigger for DAC.");

	result = DM35425_Dac_Start(board, &my_dac);

	check_result(result, "Error starting DAC");

}

/**
*******************************************************************************
@brief
    Convert a binary data file to ASCII values.  The format will be the same
    as the data file produced without the --binary argument.  The example
    program will exit after finishing.

 @retval
    None.
 *******************************************************************************
*/
void convert_bin_to_txt(unsigned int samples_in_buffer)
{

	FILE *fp_in, *fp_out;
	unsigned long sample_num, output_index = 0;
	int num_read = 0;
	long total_read = 0;
	int *buff;

	buff = (int *)malloc(samples_in_buffer * sizeof(int));
	if (buff == NULL) {
		error(EXIT_FAILURE, errno,
		      "Error allocating memory to read binary file contents.\n");
	}

	fp_in = fopen(BIN_FILE_NAME, "rb");

	if (fp_in == NULL) {
		error(EXIT_FAILURE, errno,
		      "open() FAILED to open binary input file %s.\n",
		      BIN_FILE_NAME);
	}

	fp_out = fopen(ASCII_FILE_NAME, "w");

	if (fp_out == NULL) {
		error(EXIT_FAILURE, errno,
		      "open() FAILED to open ASCII output file %s.\n",
		      ASCII_FILE_NAME);
	}

	num_read = fread(buff, sizeof(int), samples_in_buffer, fp_in);
	total_read += num_read;

	while (num_read > 0) {

		for (sample_num = 0; sample_num < samples_in_buffer;
		     sample_num++) {
			fprintf(fp_out, "%lu\t%d\n", output_index,
				buff[sample_num]);
			output_index++;
		}
		num_read = fread(buff, sizeof(int), samples_in_buffer, fp_in);
		total_read += num_read;

	}
	printf("Total bytes converted to ASCII: %ld\n",
	       total_read * sizeof(int));

	free(buff);

	fclose(fp_in);
	fclose(fp_out);

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
	unsigned long int minor = 0;
	int result;
	int index;
	FILE *fp;

	int buff;
	uint8_t buff_status;
	uint8_t buff_control;
	uint32_t buff_size;
	unsigned int buffer_to_get;
	unsigned int buffers_copied;
	unsigned long local_buffer_count;
	unsigned long bytes_written = 0;
	unsigned long output_index = 0;

	// Initialize this to its largest possible value.
	unsigned long samples_to_collect = -1, samples_in_buffer = 0;
	int store_in_binary = 0;
	unsigned int timeout_count = 0;

	struct sigaction signal_action;
	uint32_t rate = DEFAULT_RATE, actual_rate = 0;

	unsigned int range = DEFAULT_RANGE;

	int help_option_given = 0, convert_bin_file = 0;
	int status;
	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"rate", 1, 0, RATE_OPTION},
		{"samples", 1, 0, SAMPLES_OPTION},
		{"binary", 0, 0, BINARY_OPTION},
		{"channel", 1, 0, CHANNELS_OPTION},
		{"range", 1, 0, RANGE_OPTION},
		{"bin2txt", 0, 0, BIN2TXT_OPTION},
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
			   User entered number of samples
			   ################################################################# */
		case SAMPLES_OPTION:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			samples_to_collect =
			    strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if ((samples_to_collect == ULONG_MAX)
			    && (errno == ERANGE)) {
				error(0, 0,
				      "ERROR: Samples number caused numeric overflow");
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
				      "ERROR: Non-decimal samples value entered");
				usage();
			}
			break;

			/*#################################################################
			   User entered '--binary'
			   ################################################################# */
		case BINARY_OPTION:
			store_in_binary = 0xFF;
			break;

			/*#################################################################
			   User entered '--bin2txt'
			   ################################################################# */
		case BIN2TXT_OPTION:
			convert_bin_file = 1;
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

			if (channel >= DM35425_NUM_ADC_DMA_CHANNELS) {
				error(0, 0,
				      "ERROR: Channel number must be from 0 to %u.",
				      DM35425_NUM_ADC_DMA_CHANNELS - 1);
				usage();
			}
			break;

			/*#################################################################
			   User entered --range option
			   ################################################################# */
		case RANGE_OPTION:
			if (strcmp(optarg, "10B") == 0) {
				range = DM35425_ADC_RNG_BIPOLAR_10V;
			} else if (strcmp(optarg, "10U") == 0) {
				range = DM35425_ADC_RNG_UNIPOLAR_10V;
			} else if (strcmp(optarg, "5B") == 0) {
				range = DM35425_ADC_RNG_BIPOLAR_5V;
			} else if (strcmp(optarg, "5U") == 0) {
				range = DM35425_ADC_RNG_UNIPOLAR_5V;
			} else if (strcmp(optarg, "2.5B") == 0) {
				range = DM35425_ADC_RNG_BIPOLAR_2_5V;
			} else if (strcmp(optarg, "2.5U") == 0) {
				range = DM35425_ADC_RNG_UNIPOLAR_2_5V;
			} else if (strcmp(optarg, "1.25U") == 0) {
				range = DM35425_ADC_RNG_UNIPOLAR_1_25V;
			} else if (strcmp(optarg, "1.25B") == 0) {
				range = DM35425_ADC_RNG_BIPOLAR_1_25V;
			} else if (strcmp(optarg, ".625B") == 0) {
				range = DM35425_ADC_RNG_BIPOLAR_625mV;
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

	if (rate < 1 || rate > DM35425_ADC_MAX_RATE) {
		error(0, 0, "Error: Rate given not within range of board.");
		usage();
	}

	/*
	 * Trying to come up with a reasonable size for buffers that doesn't
	 * take forever to fill up with slower rates, but also makes it possible
	 * to run at higher rates.
	 */
	buffer_size_bytes = rate / 50 & ~0x3;
	if (buffer_size_bytes < (20 * sizeof(int))) {
		buffer_size_bytes = 20 * sizeof(int);
	}

	samples_in_buffer = buffer_size_bytes / sizeof(int);

	if (convert_bin_file) {
		convert_bin_to_txt(samples_in_buffer);
		return 0;
	}

	signal_action.sa_handler = sigint_handler;
	sigfillset(&(signal_action.sa_mask));
	signal_action.sa_flags = 0;

	if (sigaction(SIGINT, &signal_action, NULL) < 0) {
		error(EXIT_FAILURE, errno, "ERROR: sigaction() FAILED");
	}

	if (store_in_binary) {
		fp = fopen(BIN_FILE_NAME, "wb");
	} else {
		fp = fopen(ASCII_FILE_NAME, "w");
	}

	if (fp == NULL) {
		error(EXIT_FAILURE, errno,
		      "open() FAILED on output data file.\n");
	}

	printf("Opening board.....");
	result = DM35425_Board_Open(minor, &board);

	check_result(result, "Could not open board");
	printf("success.\nResetting board.....");
	result = DM35425_Gbc_Board_Reset(board);

	check_result(result, "Could not reset board");
	printf("success.\n");

	setup_dacs();

	result = DM35425_Adc_Open(board, ADC_0, &my_adc);

	check_result(result, "Could not open ADC");

	printf("Found ADC, with %d DMA channels (%d buffers each)\n",
	       my_adc.num_dma_channels, my_adc.num_dma_buffers);

	result = DM35425_Adc_Set_Clock_Src(board,
					   &my_adc, DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting ADC clock");

	buffer_count = 0;
	local_buffer_count = 0;

	fprintf(stdout, "Initializing DMA Channel %d...", channel);
	result = DM35425_Dma_Initialize(board,
					&my_adc,
					channel,
					my_adc.num_dma_buffers,
					buffer_size_bytes);

	check_result(result, "Error initializing DMA");

	result = DM35425_Dma_Setup(board,
				   &my_adc,
				   channel,
				   DM35425_DMA_SETUP_DIRECTION_READ,
				   NOT_IGNORE_USED);

	check_result(result, "Error configuring DMA");

	fprintf(stdout, "Setting DMA Interrupts......");
	result = DM35425_Dma_Configure_Interrupts(board,
						  &my_adc,
						  channel,
						  INTERRUPT_ENABLE,
						  ERROR_INTR_ENABLE);

	check_result(result, "Error setting DMA Interrupts");
	fprintf(stdout, "success!\n");

	for (buff = 0; buff < my_adc.num_dma_buffers; buff++) {

		buff_control = DM35425_DMA_BUFFER_CTRL_VALID |
		    DM35425_DMA_BUFFER_CTRL_INTR;

		if (buff == (DM35425_NUM_ADC_DMA_BUFFERS - 1)) {
			buff_control |= DM35425_DMA_BUFFER_CTRL_LOOP;

		}

		result = DM35425_Dma_Buffer_Setup(board,
						  &my_adc,
						  channel, buff, buff_control);

		check_result(result, "Error setting buffer control.");

		result = DM35425_Dma_Buffer_Status(board,
						   &my_adc,
						   channel,
						   buff,
						   &buff_status,
						   &buff_control, &buff_size);

		check_result(result, "Error getting buffer status.");

		fprintf(stdout,
			"    Buffer %d: Stat: 0x%x  Ctrl: 0x%x  Size: %d\n",
			buff, buff_status, buff_control, buff_size);
	}

	result = DM35425_Adc_Channel_Setup(board,
					   &my_adc,
					   channel,
					   DM35425_ADC_NO_DELAY,
					   range,
					   DM35425_ADC_INPUT_SINGLE_ENDED);

	check_result(result, "Error setting up channel.");

	// Allocate local memory for data.
	local_buffer = malloc(sizeof(int **) * my_adc.num_dma_buffers);
	check_result(local_buffer == NULL,
		     "Could not allocate for local buffer");

	for (buff = 0; buff < my_adc.num_dma_buffers; buff++) {
		local_buffer[buff] = (int *)malloc(buffer_size_bytes);

		check_result(local_buffer[buff] == NULL,
			     "Could not allocate for local buffer");

	}

	fprintf(stdout, "success.\nInstalling user ISR .....");
	result = DM35425_General_InstallISR(board, ISR);
	check_result(result, "DM35425_General_InstallISR()");
	fprintf(stdout, "success.\n");

	fprintf(stdout, "Starting ADC %d DMA ......", ADC_0);
	result = DM35425_Dma_Start(board, &my_adc, channel);

	check_result(result, "Error starting DMA");

	printf("success.\nInitializing ADC......");
	result = DM35425_Adc_Set_Start_Trigger(board,
					       &my_adc,
					       DM35425_CLK_SRC_IMMEDIATE);
	check_result(result, "Error setting start trigger.");

	result = DM35425_Adc_Set_Stop_Trigger(board,
					      &my_adc, DM35425_CLK_SRC_NEVER);
	check_result(result, "Error setting stop trigger.");

	result = DM35425_Adc_Set_Sample_Rate(board,
					     &(my_adc), rate, &actual_rate);

	check_result(result, "Failed to set sample rate for ADC.");
	fprintf(stdout,
		"success.\nRate requested: %d  Actual Rate Achieved: %d\n",
		rate, actual_rate);

	result = DM35425_Adc_Initialize(board, &my_adc);

	check_result(result, "Failed or timed out initializing ADC.");

	printf("Starting ADC\n");

	result = DM35425_Adc_Start(board, &my_adc);

	check_result(result, "Error starting ADC");

	buffers_copied = 0;

	printf("\nPress Ctrl-C to exit.\n\n");
	/*
	 * Loop here until an error occurs, or the user hits CTRL-C, or we've
	 * collected the requested number of samples.  Loop through the ADCs
	 * and see if a buffer has been copied from kernel space.  If so, then
	 * write it out to disk.
	 */
	while (!exit_program && (output_index < samples_to_collect)) {

		if ((buffer_count - local_buffer_count) >
		    my_adc.num_dma_buffers) {
			fprintf(stdout, "Local buffer for ADC was overrun.\n");
			exit_program = 1;
		} else {

			timeout_count = 0;
			while ((local_buffer_count == buffer_count)
			       && !exit_program && (timeout_count < 5000)) {
				DM35425_Micro_Sleep(1000);
				timeout_count++;
			}

			if (timeout_count == 5000) {
				exit_program = 1;
			}

			if (exit_program) {
				break;
			}
		}

		// 1 buffer from each ADC is now complete.
		buffer_to_get = local_buffer_count % my_adc.num_dma_buffers;

		if (store_in_binary) {

			fwrite(local_buffer[buffer_to_get], sizeof(int),
			       samples_in_buffer, fp);

			bytes_written += buffer_size_bytes;
			output_index += samples_in_buffer;

		} else {
			for (index = 0;
			     index < (buffer_size_bytes / sizeof(int));
			     index++) {

				fprintf(fp, "%lu\t%d\t", output_index,
					local_buffer[buffer_to_get][index]);

				output_index++;
				fprintf(fp, "\n");
			}
		}

		buffers_copied++;

		fprintf(stdout, "Copied %5d buffers.        \r",
			buffers_copied);

		local_buffer_count++;

	}

	if (dma_has_error) {
		output_channel_status(board, &my_adc, channel);
	}

	result = DM35425_Dma_Configure_Interrupts(board,
						  &my_adc,
						  channel,
						  INTERRUPT_DISABLE,
						  ERROR_INTR_DISABLE);

	check_result(result, "Error setting DMA Interrupts");

	if (output_index >= samples_to_collect) {
		fprintf(stdout, "Reached number of samples (%lu)\n",
			samples_to_collect);
	}

	if (store_in_binary) {
		fprintf(stdout, "Wrote %lu bytes to file.\n", bytes_written);
	}

	fclose(fp);

	for (buff = 0; buff < my_adc.num_dma_buffers; buff++) {

		free(local_buffer[buff]);

	}

	free(local_buffer);

	printf("Removing ISR\n");
	result = DM35425_General_RemoveISR(board);

	check_result(result, "Error removing ISR.");

	printf("Closing Board\n");
	result = DM35425_Board_Close(board);

	check_result(result, "Error closing board.");

	printf("Example program successfully completed.\n");
	return 0;

}

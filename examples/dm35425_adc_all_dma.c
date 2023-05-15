/**
	@file

	@brief
		Example program which demonstrates the use of the ADC and
		DMA, using all ADC channels at the same time.

	@verbatim

		This example program will collect data from the ADC(s)
		specified by the user, at the rate specified by the user, and will
		write the data to files.  It will do this continuously until the
		user hits CTRL-C (or the filesystem becomes full).

		Connect the signals of interest to the appropriate ADC Input pins.

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

	$Id: dm35425_adc_all_dma.c 125638 2020-05-07 20:41:01Z lfrankenfield $
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
#include "dm35425_ioctl.h"
#include "dm35425_examples.h"
#include "dm35425_dma_library.h"
#include "dm35425.h"
#include "dm35425_util_library.h"
#include "dm35425_os.h"

/**
 * Default rate to use, if user does not enter one. (Hz)
 */
#define DEFAULT_RATE 10

/**
 * Define a default range to use, if the user does not
 * provide one.
 */
#define DEFAULT_RANGE DM35425_ADC_RNG_BIPOLAR_5V

/**
 * Prefix for files that will be output during example
 */
#define DAT_FILE_NAME_PREFIX "./adc_dma_data_ch"

/**
 * Prefix for files that will be output during example
 */
#define DAT_FILE_NAME_SUFFIX ".dat"

/**
 * Name of the program as invoked on the command line
 */
static char *program_name;

/**
 * Boolean flag indicating if there was a DMA error.
 */
static int dma_has_error[DM35425_NUM_ADC_DMA_CHANNELS];

/**
 * Pointer to board descriptor
 */
static struct DM35425_Board_Descriptor *board;

/**
 * Pointer to array of function blocks that will hold the ADC descriptors
 */
static struct DM35425_Function_Block my_adc;

/**
 * Buffer count, used to tell the main loop that a buffer has been copied.
 */
static unsigned long buffer_count;

/**
 * Pointer to local memory buffer where data is copied from the kernel buffers
 * when a DMA buffer becomes full.
 */
static int **local_buffer[DM35425_NUM_ADC_DMA_CHANNELS];

/**
 * Boolean indicating the program should exit.
 */
static volatile int exit_program = 0;

/**
 * Size of the buffer allocated, in bytes.
 */
static unsigned long buffer_size_bytes = 0;

/**
 * What buffer is next to be copied from DMA
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

	fprintf(stderr, "\t--range RNG\n");
	fprintf(stderr, "\t\tUse the specified range of the ADC. \n");
	fprintf(stderr, "\t\t\t10B = 10V, Bipolar\n");
	fprintf(stderr, "\t\t\t10U = 10V, Unipolar\n");
	fprintf(stderr, "\t\t\t5B = 5V, Bipolar (Default)\n");
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

	printf("FB%d Ch%d DMA Status: Current Buffer: %u  Count: %ul  Action: 0x%x  Status: "
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
	unsigned int channel = 0;

	if (int_info.valid_interrupt)
	{

		// It's a DMA interrupt
		if (int_info.interrupt_fb < 0)
		{

			result = DM35425_Dma_Check_Buffer_Used(board,
												   &my_adc,
												   channel,
												   next_buffer,
												   &buffer_full);

			check_result(result, "Error finding used buffer.");
			check_result(buffer_full == 0,
						 "DMA Interrupt occurred, but buffer was not full.");

			// Read all DMA channels
			for (channel = 0;
				 channel < DM35425_NUM_ADC_DMA_CHANNELS;
				 channel++)
			{

				result = DM35425_Dma_Check_For_Error(board,
													 &my_adc,
													 channel,
													 &dma_error);

				check_result(result,
							 "Error checking for DMA error.");

				if (dma_error)
				{
					dma_has_error[channel] = 1;
					exit_program = 1;
					return;
				}

				result = DM35425_Dma_Read(board,
										  &my_adc,
										  channel,
										  next_buffer,
										  buffer_size_bytes,
										  local_buffer[channel]
													  [next_buffer]);

				check_result(result,
							 "Error getting DMA buffer");

				result = DM35425_Dma_Reset_Buffer(board,
												  &my_adc,
												  channel,
												  next_buffer);

				check_result(result, "Error resetting buffer");

				result = DM35425_Dma_Clear_Interrupt(board,
													 &my_adc,
													 channel,
													 NO_CLEAR_INTERRUPT,
													 NO_CLEAR_INTERRUPT,
													 NO_CLEAR_INTERRUPT,
													 NO_CLEAR_INTERRUPT,
													 CLEAR_INTERRUPT);

				check_result(result,
							 "Error clearing DMA interrupt.");
			}

			next_buffer =
				(next_buffer + 1) % my_adc.num_dma_buffers;
			buffer_count++;
		}
		else
		{
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
	char fileName[200];
	FILE *fp[DM35425_NUM_ADC_DMA_CHANNELS];

	int buff;
	uint8_t buff_status;
	uint8_t buff_control;
	uint32_t buff_size;
	unsigned int buffer_to_get;
	unsigned int buffers_copied;
	unsigned long local_buffer_count;
	unsigned long num_samples_taken[DM35425_NUM_ADC_DMA_CHANNELS];
	unsigned int channel = 0, samples_per_buffer;

	// Initialize this to its largest possible value.
	unsigned long samples_to_collect = -1;
	struct sigaction signal_action;
	uint32_t rate = DEFAULT_RATE, actual_rate = 0;

	unsigned int range = DEFAULT_RANGE;
	float volt;

	int help_option_given = 0;
	int status;
	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"rate", 1, 0, RATE_OPTION},
		{"samples", 1, 0, SAMPLES_OPTION},
		{"channel", 1, 0, CHANNELS_OPTION},
		{"range", 1, 0, RANGE_OPTION},
		{0, 0, 0, 0}};

	program_name = arguments[0];

	// Show usage, parse arguments
	while (1)
	{
		/*
		 * Parse the next command line option and any arguments it may require
		 */
		status = getopt_long(argument_count,
							 arguments, "", options, NULL);

		/*
		 * If getopt_long() returned -1, then all options have been processed
		 */
		if (status == -1)
		{
			break;
		}

		/*
		 * Figure out what getopt_long() found
		 */
		switch (status)
		{

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
			if ((minor == ULONG_MAX) && (errno == ERANGE))
			{
				error(0, 0,
					  "ERROR: Device minor number caused numeric overflow");
				usage();
			}

			/*
			 * Catch argument strings with valid decimal prefixes, for
			 * example "1q", and argument strings which cannot be converted,
			 * for example "abc1"
			 */
			if ((*invalid_char_p != '\0') || (invalid_char_p == optarg))
			{
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

			if ((rate == ULONG_MAX) && (errno == ERANGE))
			{
				error(0, 0,
					  "ERROR: Rate number caused numeric overflow");
				usage();
			}

			/*
			 * Catch argument strings with valid decimal prefixes, for
			 * example "1q", and argument strings which cannot be converted,
			 * for example "abc1"
			 */

			if ((*invalid_char_p != '\0') || (invalid_char_p == optarg))
			{
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
			if ((samples_to_collect == ULONG_MAX) && (errno == ERANGE))
			{
				error(0, 0,
					  "ERROR: Samples number caused numeric overflow");
				usage();
			}

			/*
			 * Catch argument strings with valid decimal prefixes, for
			 * example "1q", and argument strings which cannot be converted,
			 * for example "abc1"
			 */
			if ((*invalid_char_p != '\0') || (invalid_char_p == optarg))
			{
				error(0, 0,
					  "ERROR: Non-decimal samples value entered");
				usage();
			}
			break;

			/*#################################################################
			   User entered --range option
			   ################################################################# */
		case RANGE_OPTION:
			if (strcmp(optarg, "10B") == 0)
			{
				range = DM35425_ADC_RNG_BIPOLAR_10V;
			}
			else if (strcmp(optarg, "10U") == 0)
			{
				range = DM35425_ADC_RNG_UNIPOLAR_10V;
			}
			else if (strcmp(optarg, "5B") == 0)
			{
				range = DM35425_ADC_RNG_BIPOLAR_5V;
			}
			else if (strcmp(optarg, "5U") == 0)
			{
				range = DM35425_ADC_RNG_UNIPOLAR_5V;
			}
			else if (strcmp(optarg, "2.5B") == 0)
			{
				range = DM35425_ADC_RNG_BIPOLAR_2_5V;
			}
			else if (strcmp(optarg, "2.5U") == 0)
			{
				range = DM35425_ADC_RNG_UNIPOLAR_2_5V;
			}
			else if (strcmp(optarg, "1.25U") == 0)
			{
				range = DM35425_ADC_RNG_UNIPOLAR_1_25V;
			}
			else if (strcmp(optarg, "1.25B") == 0)
			{
				range = DM35425_ADC_RNG_BIPOLAR_1_25V;
			}
			else if (strcmp(optarg, ".625B") == 0)
			{
				range = DM35425_ADC_RNG_BIPOLAR_625mV;
			}
			else
			{
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

	if (help_option_given)
	{
		usage();
	}

	if (rate < 1 || rate > DM35425_ADC_MAX_RATE)
	{
		error(0, 0, "Error: Rate given not within range of board.");
		usage();
	}

	/*
	 * Trying to come up with a reasonable size for buffers that doesn't
	 * take forever to fill up with slower rates, but also makes it possible
	 * to run at higher rates.
	 */
	samples_per_buffer = (rate / 40);
	if (samples_per_buffer < 20)
	{
		samples_per_buffer = 20;
	}

	buffer_size_bytes = samples_per_buffer * sizeof(int);

	signal_action.sa_handler = sigint_handler;
	sigfillset(&(signal_action.sa_mask));
	signal_action.sa_flags = 0;

	if (sigaction(SIGINT, &signal_action, NULL) < 0)
	{
		error(EXIT_FAILURE, errno, "ERROR: sigaction() FAILED");
	}

	for (channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++)
	{
		sprintf(fileName, "%s%u%s", DAT_FILE_NAME_PREFIX, channel,
				DAT_FILE_NAME_SUFFIX);
		remove(fileName);

		fp[channel] = fopen(fileName, "w");

		if (fp[channel] == NULL)
		{
			error(EXIT_FAILURE, errno,
				  "open() FAILED on output data file.\n");
		}
	}

	printf("Opening board.....");
	result = DM35425_Board_Open(minor, &board);

	check_result(result, "Could not open board");
	printf("success.\nResetting board.....");
	result = DM35425_Gbc_Board_Reset(board);

	check_result(result, "Could not reset board");
	printf("success.\n");

	result = DM35425_Adc_Open(board, ADC_0, &my_adc); // Initialize my_adc

	check_result(result, "Could not open ADC");

	printf("Found ADC, with %d DMA channels (%d buffers each)\n",
		   my_adc.num_dma_channels, my_adc.num_dma_buffers);

	result = DM35425_Adc_Set_Clock_Src(board,
									   &my_adc, DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting ADC clock");

	buffer_count = 0;
	local_buffer_count = 0;
	for (channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++)
	{

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
												  INTERRUPT_DISABLE,
												  ERROR_INTR_DISABLE);

		check_result(result, "Error setting DMA Interrupts");
		fprintf(stdout, "success!\n");

		/* Although the interrupt bit is going to be set for every buffer,
		   we're going to enable DMA interrupts only for channel 0
		 */
		for (buff = 0; buff < my_adc.num_dma_buffers; buff++)
		{

			buff_control = DM35425_DMA_BUFFER_CTRL_VALID |
						   DM35425_DMA_BUFFER_CTRL_INTR;

			if (buff == (DM35425_NUM_ADC_DMA_BUFFERS - 1)) // for the last one enable this to loop back to 0
			{
				buff_control |= DM35425_DMA_BUFFER_CTRL_LOOP;
			}

			result = DM35425_Dma_Buffer_Setup(board,
											  &my_adc,
											  channel,
											  buff, buff_control);

			check_result(result, "Error setting buffer control.");

			result = DM35425_Dma_Buffer_Status(board,
											   &my_adc,
											   channel,
											   buff,
											   &buff_status,
											   &buff_control,
											   &buff_size);

			check_result(result, "Error getting buffer status.");

			fprintf(stdout,
					"    Buffer %d: Stat: 0x%x  Ctrl: 0x%x  Size: %d\n",
					buff, buff_status, buff_control, buff_size);
		}

		result = DM35425_Adc_Channel_Setup(board,
										   &my_adc,
										   channel,
										   DM35425_ADC_2_FULL_SAMPLE_DELAY,
										   range,
										   DM35425_ADC_INPUT_SINGLE_ENDED);

		check_result(result, "Error setting up channel.");
	}

	fprintf(stdout, "Enabling DMA Channel 0 Interrupts......");
	result = DM35425_Dma_Configure_Interrupts(board,
											  &my_adc,
											  CHANNEL_0,
											  INTERRUPT_ENABLE,
											  ERROR_INTR_ENABLE);

	check_result(result, "Error setting DMA Interrupts");

	for (channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++)
	{
		// Allocate local memory for data.
		local_buffer[channel] = (int **)
			malloc(sizeof(int *) * my_adc.num_dma_buffers);
		check_result(local_buffer[channel] == NULL,
					 "Could not allocate for local buffer");

		for (buff = 0; buff < my_adc.num_dma_buffers; buff++)
		{
			local_buffer[channel][buff] =
				(int *)malloc(buffer_size_bytes);

			check_result(local_buffer[channel][buff] == NULL,
						 "Could not allocate for local buffer");
		}
	}

	fprintf(stdout, "success.\nInstalling user ISR .....");
	result = DM35425_General_InstallISR(board, ISR);
	check_result(result, "DM35425_General_InstallISR()");
	fprintf(stdout, "success.\n");

	for (channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++)
	{
		fprintf(stdout, "Starting ADC DMA Channel %d......", channel);
		result = DM35425_Dma_Start(board, &my_adc, channel);

		check_result(result, "Error starting DMA");

		printf("success.\n");

		num_samples_taken[channel] = 0;
	}

	printf("Initializing ADC......");
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
	/* Loop here until an error occurs, or the user hits CTRL-C, or we've
	 * collected the requested number of samples.  Check the buffer count
	 * and see if a buffer has been copied from kernel space.  If so, then
	 * write it out to disk for all channels.
	 */
	while (!exit_program && (num_samples_taken[0] < samples_to_collect))
	{

		if (buffer_count > local_buffer_count)
		{

			// Buffer for Channel 0 is now complete.
			// Assume other channels will also be complete.
			buffer_to_get =
				local_buffer_count % my_adc.num_dma_buffers;

			for (channel = 0;
				 channel < DM35425_NUM_ADC_DMA_CHANNELS;
				 channel++)
			{

				for (index = 0; index < samples_per_buffer;
					 index++)
				{
					DM35425_Adc_Sample_To_Volts(range,
												local_buffer
													[channel]
													[buffer_to_get][index],
												&volt);

					fprintf(fp[channel], "%lu\t%.2f\t",
							num_samples_taken[channel],
							volt);

					num_samples_taken[channel]++;
					fprintf(fp[channel], "\n");
				}

				buffers_copied++;

				fprintf(stdout, "Copied %5d buffers.        \r",
						buffers_copied);
			}
			local_buffer_count++;
		}
		else
		{
			DM35425_Micro_Sleep(100);
		}
	}

	for (channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++)
	{

		if (dma_has_error[channel])
		{
			output_channel_status(board, &my_adc, channel);
		}

		result = DM35425_Dma_Configure_Interrupts(board,
												  &my_adc,
												  channel,
												  INTERRUPT_DISABLE,
												  ERROR_INTR_DISABLE);

		check_result(result, "Error setting DMA Interrupts");

		for (buff = 0; buff < my_adc.num_dma_buffers; buff++)
		{

			free(local_buffer[channel][buff]);
		}

		free(local_buffer[channel]);

		fclose(fp[channel]);
	}

	if (num_samples_taken[0] >= samples_to_collect)
	{
		fprintf(stdout, "Took %lu samples (%lu expected)\n",
				num_samples_taken[0], samples_to_collect);
	}

	printf("Removing ISR\n");
	result = DM35425_General_RemoveISR(board);

	check_result(result, "Error removing ISR.");

	printf("Closing Board\n");
	result = DM35425_Board_Close(board);

	check_result(result, "Error closing board.");

	printf("Example program successfully completed.\n");
	return 0;
}

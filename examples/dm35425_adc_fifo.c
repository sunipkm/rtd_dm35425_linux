/**
    @file

    @brief
        Example program which demonstrates the use of the ADC FIFO and
        its interrupts.

    @verbatim

        This example program uses an ADC to collect data.  The main point
        of the example is to demonstrate copying data out of the FIFO and
        showing the correct interrupts occurring throughout the process.

	Connect the signal of interest to AIN0 (CN3 Pin 1) and AGND
        (CN3 Pin 21), or pins corresponding to desired channel.

        For convenience in testing the ADC, especially differential voltages,
        the DAC is setup to output these specific voltages:

        AOUT0: -6V
        AOUT1: -3V
        AOUT2:  4V
        AOUT3:  8V

        The program will continue to run until CTRL-C is pressed.

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

    $Id: dm35425_adc_fifo.c 108025 2017-04-14 15:09:34Z rgroner $
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>

#include "dm35425_gbc_library.h"
#include "dm35425_adc_library.h"
#include "dm35425_dac_library.h"
#include "dm35425_dma_library.h"
#include "dm35425_ioctl.h"
#include "dm35425_examples.h"
#include "dm35425_util_library.h"
#include "dm35425_board_access.h"
#include "dm35425_types.h"
#include "dm35425.h"
#include "dm35425_os.h"

/**
 * Rate to run at, if the user does not provide one. (Hz)
 */
#define DEFAULT_RATE	500

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
 * Define a default mode to use, if the user does not provide one
 */
#define DEFAULT_MODE		DM35425_ADC_INPUT_SINGLE_ENDED

/**
 * Name of the program as invoked on the command line
 */
static char *program_name;

/**
 * Count of non-DMA interrupts.
 */
volatile int non_dma_interrupt_count = 0;

/**
 * Count of DMA interrupts.
 */
volatile int dma_interrupt_count = 0;

/**
 * Count of unexpected interrupts.
 */
volatile int unexpected_interrupt_count = 0;

/**
 * Count of expected interrupts.
 */
volatile int interrupt_count = 0;

/**
 * Boolean indicating whether or not to exit the program.
 */
volatile int exit_program = 0;

/**
 * Function block descriptor
 */
struct DM35425_Function_Block my_adc;

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

	fprintf(stderr, "\t--mode MODE\n");
	fprintf(stderr, "\t\tChange the mode of the ADC.\n");
	fprintf(stderr, "\t\t\tse = single-ended (Default)\n");
	fprintf(stderr, "\t\t\tdiff = differential\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

/**
*******************************************************************************
@brief
    The interrupt subroutine that will execute when an interrupt occurs.  It will
    simply increment a count, which the main program will act on.
 *******************************************************************************
*/

void ISR(struct dm35425_ioctl_interrupt_info_request int_info)
{

	interrupt_count++;

	if (int_info.error_occurred) {

		printf("ISR: Error received.\n");
		return;

	}

	if (int_info.valid_interrupt) {

		// Check that it's from the function block we expect
		if ((int_info.interrupt_fb & 0x7FFFFFFF) == my_adc.fb_num) {
			// Check for DMA or non-DMA interrupt
			if (int_info.interrupt_fb < 0) {
				dma_interrupt_count++;
			} else {
				non_dma_interrupt_count++;
			}
		} else {
			unexpected_interrupt_count++;
		}

	}

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

static void DM35425_Setup_Dacs(struct DM35425_Board_Descriptor *board)
{

	struct DM35425_Function_Block my_dac;
	int result;
	int16_t conv_value = 0;

	/****************************
	 * Set DAC 0 Chan 0 to -6.0 volts
	 ****************************/
	result = DM35425_Dac_Open(board, DAC_0, &my_dac);

	check_result(result, "Could not open DAC");

	result = DM35425_Dac_Channel_Setup(board,
					   &my_dac,
					   CHANNEL_0,
					   DM35425_DAC_RNG_BIPOLAR_10V);

	check_result(result, "Error setting output range.");

	result = DM35425_Dac_Reset(board, &my_dac);

	check_result(result, "Error stopping DAC");

	result = DM35425_Dac_Volts_To_Conv(DM35425_DAC_RNG_BIPOLAR_10V,
					   -6.0f, &conv_value);

	check_result(result, "Error converting voltage to conversion");

	result = DM35425_Dac_Set_Last_Conversion(board,
						 &my_dac,
						 CHANNEL_0, 0, conv_value);
	check_result(result, "Error setting last conversion");

	/****************************
	 * Set DAC 0 Chan 1 to -3.0 volts
	 ****************************/
	result = DM35425_Dac_Channel_Setup(board,
					   &my_dac,
					   CHANNEL_1,
					   DM35425_DAC_RNG_BIPOLAR_10V);

	check_result(result, "Error setting output range.");

	result = DM35425_Dac_Reset(board, &my_dac);

	check_result(result, "Error stopping DAC");

	result = DM35425_Dac_Volts_To_Conv(DM35425_DAC_RNG_BIPOLAR_10V,
					   -3.0f, &conv_value);

	check_result(result, "Error converting voltage to conversion");

	result = DM35425_Dac_Set_Last_Conversion(board,
						 &my_dac,
						 CHANNEL_1, 0, conv_value);
	check_result(result, "Error setting last conversion");

	/****************************
	 * Set DAC 0 Chan 2 to 4.0 volts
	 ****************************/
	result = DM35425_Dac_Channel_Setup(board,
					   &my_dac,
					   CHANNEL_2,
					   DM35425_DAC_RNG_BIPOLAR_10V);

	check_result(result, "Error setting output range.");

	result = DM35425_Dac_Reset(board, &my_dac);

	check_result(result, "Error stopping DAC");

	result = DM35425_Dac_Volts_To_Conv(DM35425_DAC_RNG_BIPOLAR_10V,
					   4.0f, &conv_value);

	check_result(result, "Error converting voltage to conversion");

	result = DM35425_Dac_Set_Last_Conversion(board,
						 &my_dac,
						 CHANNEL_2, 0, conv_value);
	check_result(result, "Error setting last conversion");

	/****************************
	 * Set DAC 0 Chan 3 to 8.0 volts
	 ****************************/
	result = DM35425_Dac_Open(board, DAC_0, &my_dac);

	check_result(result, "Could not open DAC");

	result = DM35425_Dac_Channel_Setup(board,
					   &my_dac,
					   CHANNEL_3,
					   DM35425_DAC_RNG_BIPOLAR_10V);

	check_result(result, "Error setting output range.");

	result = DM35425_Dac_Reset(board, &my_dac);

	check_result(result, "Error stopping DAC");

	result = DM35425_Dac_Volts_To_Conv(DM35425_DAC_RNG_BIPOLAR_10V,
					   8.0f, &conv_value);

	check_result(result, "Error converting voltage to conversion");

	result = DM35425_Dac_Set_Last_Conversion(board,
						 &my_dac,
						 CHANNEL_3, 0, conv_value);
	check_result(result, "Error setting last conversion");

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

	unsigned long int minor = 0;
	int result;
	int my_value;

	int fifo_count = 1;

	int status_overflow;
	int status_underflow;
	int status_used;
	int status_invalid;
	uint32_t sample_count;

	unsigned int actual_rate;
	unsigned int channel = DEFAULT_CHANNEL;
	unsigned int range = DEFAULT_RANGE;
	enum DM35425_Input_Mode mode = DEFAULT_MODE;

	int help_option_given = 0;
	int status;
	struct sigaction signal_action;
	uint16_t interrupt_status;

	float volts = 0;

	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"channel", 1, 0, CHANNELS_OPTION},
		{"range", 1, 0, RANGE_OPTION},
		{"mode", 1, 0, MODE_OPTION},
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
			   User entered --mode option
			   ################################################################# */
		case MODE_OPTION:
			if (strcmp(optarg, "se") == 0) {
				mode = DM35425_ADC_INPUT_SINGLE_ENDED;
			} else if (strcmp(optarg, "diff") == 0) {
				mode = DM35425_ADC_INPUT_DIFFERENTIAL;
			} else {
				error(0, 0,
				      "ERROR: Mode must be either se or diff.");
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
	printf("success.\n");

	DM35425_Setup_Dacs(board);

	printf("Opening ADC......\n");

	result = DM35425_Adc_Open(board, ADC_0, &my_adc);

	check_result(result, "Could not open ADC");

	printf("Found ADC, with %d DMA channels (%d buffers each)\n",
	       my_adc.num_dma_channels, my_adc.num_dma_buffers);

	printf("Using Channel %u\n", channel);

	result = DM35425_Adc_Set_Clock_Src(board,
					   &my_adc, DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting ADC clock");

	result = DM35425_Adc_Set_Pre_Trigger_Samples(board, &my_adc, 0);

	check_result(result, "Error setting pre-capture samples.");

	result = DM35425_Adc_Set_Post_Stop_Samples(board, &my_adc, 0);

	check_result(result, "Error setting post-capture samples.");

	// Make sure the DMA is in a Clear state.
	printf("\n\nSetting DMA to Clear....");
	result = DM35425_Dma_Clear(board, &my_adc, channel);
	check_result(result, "Error Clearing DMA");

	printf("Disabling A/D interrupts...");
	result = DM35425_Adc_Interrupt_Set_Config(board,
						  &my_adc,
						  DM35425_ADC_INT_POST_BUFF_FULL_MASK,
						  INTERRUPT_DISABLE);
	check_result(result, "Error disabling interrupts");

	fprintf(stdout, "Disabling DMA Interrupts......");
	result = DM35425_Dma_Configure_Interrupts(board,
						  &my_adc,
						  channel,
						  INTERRUPT_DISABLE,
						  ERROR_INTR_DISABLE);

	check_result(result, "Error disabling interrupts");

	fprintf(stdout, "Installing user ISR ...\n");
	result = DM35425_General_InstallISR(board, ISR);
	check_result(result, "DM35425_General_InstallISR()");

	result = DM35425_Adc_Interrupt_Set_Config(board,
						  &my_adc,
						  DM35425_ADC_INT_SAMP_COMPL_MASK,
						  INTERRUPT_ENABLE);

	check_result(result, "Error setting interrupt.");

	result = DM35425_Adc_Channel_Setup(board,
					   &my_adc,
					   channel,
					   DM35425_ADC_NO_DELAY, range, mode);

	check_result(result, "Error setting up channel.");

	printf("Initializing ADC......\n");
	result = DM35425_Adc_Set_Start_Trigger(board,
					       &my_adc,
					       DM35425_CLK_SRC_IMMEDIATE);
	check_result(result, "Error setting start trigger.");

	result = DM35425_Adc_Set_Stop_Trigger(board,
					      &my_adc, DM35425_CLK_SRC_NEVER);
	check_result(result, "Error setting stop trigger.");

	result = DM35425_Adc_Set_Sample_Rate(board,
					     &my_adc,
					     DEFAULT_RATE, &actual_rate);

	check_result(result, "Failed to set sample rate for ADC.");
	fprintf(stdout,
		"ADC0: Rate requested: %d  Actual Rate Achieved: %d\n",
		DEFAULT_RATE, actual_rate);

	result = DM35425_Adc_Initialize(board, &my_adc);

	check_result(result, "Failed or timed out initializing ADC.");

	fprintf(stdout,
		"Enabling DMA Interrupts (for overrun and underrun)......");
	result =
	    DM35425_Dma_Configure_Interrupts(board, &my_adc, channel,
					     INTERRUPT_DISABLE,
					     ERROR_INTR_ENABLE);

	check_result(result, "Error enabling interrupts");

	// Set the DMA engine to the Pause state.
	printf("Setting DMA engine to PAUSE...");
	result = DM35425_Dma_Pause(board, &my_adc, channel);
	check_result(result, "Failed setting DMA to Pause");
	printf("success\n");

	check_result(interrupt_count != 0,
		     "Interrupt has already happened, and ADC has not been started.");

	result = DM35425_Adc_Start(board, &my_adc);

	check_result(result, "Error starting ADC");

	printf("\n\nPress Ctrl-C to exit.\n\n");

	printf("Waiting for FIFO overflow.....\n");

	while (!exit_program && !interrupt_count) {
		DM35425_Micro_Sleep(100);
	}

	if (exit_program) {
		check_result(1, "User elected to exit with Ctrl-C");
	}
	// Immediately get the ADC sample count
	result = DM35425_Adc_Get_Sample_Count(board, &my_adc, &sample_count);

	check_result(result, "Error getting ADC Sample count.");

	// Check that the interrupt we received was the overrun error interrupt.
	check_result(unexpected_interrupt_count,
		     "Interrupt received, but was not from our function block.");
	check_result(non_dma_interrupt_count,
		     "Non-DMA Interrupt received, expected DMA interrupt.");

	result = DM35425_Dma_Get_Errors(board,
					&my_adc,
					channel,
					&status_overflow,
					&status_underflow,
					&status_used, &status_invalid);

	check_result(result, "Error getting DMA status");
	check_result(status_underflow || status_used
		     || status_invalid,
		     "Was expecting Overflow error, but got other DMA error.");
	check_result(status_overflow == 0,
		     "Was expecting Overflow error, but did not receive it.");

	printf("  *** DMA Interrupt: FIFO Overflow ***\n");
	check_result(sample_count <= DM35425_FIFO_SAMPLE_SIZE,
		     "Sample count should have been larger than FIFO size, but wasn't.");

	printf
	    ("Overflow occurred after %d samples were taken, with a FIFO size of %d\n",
	     sample_count, DM35425_FIFO_SAMPLE_SIZE);

	result = DM35425_Adc_Set_Stop_Trigger(board,
					      &my_adc,
					      DM35425_CLK_SRC_IMMEDIATE);
	check_result(result, "Error setting stop trigger.");

	result = DM35425_Dma_Clear_Interrupt(board,
					     &my_adc,
					     channel,
					     CLEAR_INTERRUPT,
					     NO_CLEAR_INTERRUPT,
					     NO_CLEAR_INTERRUPT,
					     NO_CLEAR_INTERRUPT,
					     NO_CLEAR_INTERRUPT);

	check_result(result, "Error clearing DMA interrupts.");

	interrupt_count = 0;
	dma_interrupt_count = 0;

	printf("Emptying FIFO and checking for error flags\n");

	result = DM35425_Dma_Get_Errors(board,
					&my_adc,
					channel,
					&status_overflow,
					&status_underflow,
					&status_used, &status_invalid);

	check_result(result, "Error getting DMA status");

	/* Now read the values out of the FIFO one at a time.  We will read out exactly
	   the number of samples in the FIFO, which should then trigger a "Sampling Complete"
	   interrupt.
	 */
	fifo_count = 1;
	while (fifo_count <= DM35425_FIFO_SAMPLE_SIZE && !status_underflow
	       && (interrupt_count == 0)) {

		result =
		    DM35425_Adc_Fifo_Channel_Read(board,
						  &my_adc, channel, &my_value);

		check_result(result, "Error getting ADC value.");

		result = DM35425_Adc_Sample_To_Volts(range, my_value, &volts);

		check_result(result, "Error converting ADC sample to volts.");

		result = DM35425_Dma_Get_Errors(board,
						&my_adc,
						channel,
						&status_overflow,
						&status_underflow,
						&status_used, &status_invalid);

		check_result(result, "Error getting DMA status");

		printf("%d\t%+2.5f\t%d\tUn: %d   Ov: %d\n", fifo_count, volts,
		       my_value, status_underflow, status_overflow);

		fifo_count++;

		DM35425_Micro_Sleep(1000);
	}

	check_result(interrupt_count == 0,
		     "Expected an interrupt, but none occurred.");

	check_result(status_underflow,
		     "Error:  Underflow indicated during FIFO reading.");
	check_result(fifo_count <= DM35425_FIFO_SAMPLE_SIZE,
		     "Error: An interrupt occurred before we were done emptying the FIFO.");

	//Now that all of the FIFO has been read, there should be a "sampling complete" interrupt
	check_result(non_dma_interrupt_count == 0,
		     "Expected a Sampling Complete Interrupt, but found none.");
	check_result(dma_interrupt_count > 0
		     || unexpected_interrupt_count > 0,
		     "Expected a Sampling Complete Interrupt, but found DMA/Unknown.");

	result = DM35425_Adc_Interrupt_Get_Status(board,
						  &my_adc, &interrupt_status);
	check_result(result, "Error getting ADC interrupt status.");

	check_result((interrupt_status & DM35425_ADC_INT_SAMP_COMPL_MASK) == 0,
		     "Expected a Sampling Complete Interrupt, but found other also.");

	printf("  *** Interrupt: Sampling Complete ***\n");

	result = DM35425_Adc_Interrupt_Clear_Status(board,
						    &my_adc, interrupt_status);
	check_result(result, "Error clearing interrupts.");

	interrupt_count = 0;
	dma_interrupt_count = 0;
	non_dma_interrupt_count = 0;
	printf
	    ("\nAll FIFO values read.\n\nReading FIFO one more time to check for underflow error.\n");

	result = DM35425_Adc_Fifo_Channel_Read(board,
					       &my_adc, channel, &my_value);

	check_result(result, "Error getting ADC value.");

	printf("\nWaiting for underflow error interrupt....\n");

	while (!exit_program && !interrupt_count) {
		DM35425_Micro_Sleep(100);
	}

	if (exit_program) {
		check_result(1, "User elected to exit with Ctrl-C");
	}

	check_result(dma_interrupt_count == 0,
		     "Expected DMA Interrupt (Underflow), but did not receive one.");
	check_result(non_dma_interrupt_count
		     || unexpected_interrupt_count,
		     "Expected DMA Interrupt (Underflow), but received non-DMA/other.");

	result = DM35425_Dma_Get_Errors(board,
					&my_adc,
					channel,
					&status_overflow,
					&status_underflow,
					&status_used, &status_invalid);

	check_result(result, "Error getting DMA status");
	check_result(status_overflow || status_used
		     || status_invalid,
		     "Expected underflow only, but received other interrupts as well.");

	check_result(status_underflow == 0,
		     "Expected underflow, but did not get it.");

	printf("  *** Interrupt: DMA Underflow ***\n");

	printf("\n\nStopping Adc............");

	result = DM35425_Adc_Reset(board, &my_adc);

	check_result(result, "Error starting ADC");

	printf("success!\nDisabling interrupt.....");

	result = DM35425_Adc_Interrupt_Set_Config(board,
						  &my_adc,
						  DM35425_ADC_INT_SAMPLE_TAKEN_MASK,
						  INTERRUPT_DISABLE);

	check_result(result, "Error removing interrupt.");

	printf("success!\nRemoving ISR......");
	result = DM35425_General_RemoveISR(board);

	check_result(result, "Error removing ISR.");

	printf("success.\n");

	printf("Closing Board\n");
	result = DM35425_Board_Close(board);

	check_result(result, "Error closing board.");
	printf("Example program successfully completed.\n");
	return 0;

}

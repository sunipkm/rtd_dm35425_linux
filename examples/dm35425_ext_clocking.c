/**
    @file

    @brief
        Example program which demonstrates the use of the external
        clocking function block.

    @verbatim

        This example program uses function blocks to create signals which
        are looped back into external clock inputs.  Each signal generated
        produces the equivalent of a square wave.

        Make connections as follows:

        CN3: Pin 17 to Pin 39
        CN3: Pin 37 to Pin 41

        The DAC uses DMA data to output a square wave (all 0's then all 1's).
        One of the DAC pins is then looped to the first external clock
        input pin.

        The ADIO will use the external clock signal to clock its own data
        out, data which consists of a square wave (all 0's then all 1's).
        One of the ADIO pins is then looped to the second external clock
        input pin.

        The ADC will use that external clock signal to control sampling
        of data.

        The sample/clock counter of each function block can then be polled
        to verify the correct functioning.  The ADIO should run at half the
        rate of the DAC, and the ADC should run at half the rate of the ADIO.

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

    $Id: dm35425_ext_clocking.c 108025 2017-04-14 15:09:34Z rgroner $
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
#include "dm35425_adc_library.h"
#include "dm35425_adio_library.h"
#include "dm35425_ext_clocking_library.h"
#include "dm35425_ioctl.h"
#include "dm35425_examples.h"
#include "dm35425_dma_library.h"
#include "dm35425_util_library.h"
#include "dm35425.h"
#include "dm35425_os.h"

/**
 * Set the direction of the ADIO to output
 */
#define DM35425_ADIO_DIR_OUTPUT	0xFFFFFFFF

/**
 * Set the direction of the external clocking to input
 */
#define DM35425_EXT_CLK_DIR_INPUT	0x00

/**
 * Set the edge detect to be on the rising edge for all clocks
 */
#define DM35425_EXT_CLK_EDGE_RISING		0x00

/**
 * We will only use one buffer in this example, and loop it
 */
#define NUM_BUFFERS_TO_USE	1

/**
 * Rate to use if user does not enter one on the command line (Hz)
 */
#define DEFAULT_RATE		20

/**
 * Number of samples to create.
 */
#define BUFFER_SIZE_SAMPLES	2

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
	fprintf(stderr, "USAGE\n\n\t%s [OPTIONS]\n\n", program_name);

	fprintf(stderr, "OPTIONS\n\n");

	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tShow this help screen and exit.\n");

	fprintf(stderr, "\t--minor NUM\n");
	fprintf(stderr,
		"\t\tSpecify the minor number (>= 0) of the board to open.  When not specified,\n");
	fprintf(stderr, "\t\tthe device file with minor 0 is opened.\n");
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
	struct DM35425_Function_Block my_adio;
	struct DM35425_Function_Block my_adc;
	struct DM35425_Function_Block my_ext_clk;

	unsigned long int minor = 0;
	int result;

	struct sigaction signal_action;

	unsigned int current_buffer;

	uint32_t current_count;
	int current_action;
	int status_overflow;
	int status_underflow;
	int status_used;
	int status_invalid;
	int status_complete;

	unsigned int conversions = 0;
	uint8_t buff_status;
	uint8_t buff_control;
	uint32_t buff_size;

	uint32_t rate = DEFAULT_RATE, actual_rate;

	int help_option_given = 0;
	int status;

	int32_t buffer[2];
	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"rate", 1, 0, RATE_OPTION},
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

	/******************************************************
	 * Setup DAC to output a square wave using just two
	 * values, 0 and a max
	 ******************************************************/
	printf("success.\nOpening DAC......");

	result = DM35425_Dac_Open(board, DAC_0, &my_dac);

	check_result(result, "Could not open DAC");

	printf("Found DAC_0, with %d DMA channels (%d buffers each)\n",
	       my_dac.num_dma_channels, my_dac.num_dma_buffers);

	result = DM35425_Dac_Set_Clock_Src(board,
					   &my_dac, DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting DAC clock");

	result = DM35425_Dac_Set_Conversion_Rate(board,
						 &my_dac, rate, &actual_rate);

	fprintf(stdout, "Rate requested: %d  Actual Rate Achieved: %d\n", rate,
		actual_rate);
	check_result(result, "Error setting sample rate");

	buffer[0] = DM35425_DAC_MIN;
	buffer[1] = DM35425_DAC_MAX;

	fprintf(stdout, "Initializing and configuring DMA Channel 0....");
	result = DM35425_Dma_Initialize(board,
					&my_dac,
					CHANNEL_0, 1, BUFFER_SIZE_BYTES);

	check_result(result, "Error initializing DMA");

	result = DM35425_Dma_Setup(board,
				   &my_dac,
				   CHANNEL_0,
				   DM35425_DMA_SETUP_DIRECTION_WRITE,
				   IGNORE_USED);

	check_result(result, "Error configuring DMA");

	fprintf(stdout, "success!\n");

	result = DM35425_Dma_Status(board,
				    &my_dac,
				    CHANNEL_0,
				    &current_buffer,
				    &current_count,
				    &current_action,
				    &status_overflow,
				    &status_underflow,
				    &status_used,
				    &status_invalid, &status_complete);

	check_result(result, "Error getting DMA status");

	printf
	    ("DAC DMA Status: Current Buffer: %d  Count: %d  Action: 0x%x  Status: "
	     "Ov: %d  Un: %d  Used: %d  Inv: %d  Comp: %d\n", current_buffer,
	     current_count, current_action, status_overflow, status_underflow,
	     status_used, status_invalid, status_complete);

	result = DM35425_Dma_Buffer_Setup(board,
					  &my_dac,
					  CHANNEL_0,
					  BUFFER_0,
					  DM35425_DMA_BUFFER_CTRL_VALID |
					  DM35425_DMA_BUFFER_CTRL_LOOP);

	check_result(result, "Error setting up buffer control.");

	result = DM35425_Dma_Buffer_Status(board,
					   &my_dac,
					   CHANNEL_0,
					   BUFFER_0,
					   &buff_status,
					   &buff_control, &buff_size);
	fprintf(stdout, "    Buffer 0: Stat: 0x%x  Ctrl: 0x%x  Size: %u\n",
		buff_status, buff_control, buff_size);

	check_result(result, "Error getting buffer status.");

	result = DM35425_Dma_Write(board,
				   &my_dac,
				   CHANNEL_0,
				   BUFFER_0, BUFFER_SIZE_BYTES, buffer);

	check_result(result, "Writing to DMA buffer failed");

	fprintf(stdout, "Starting DMA Channel %d......", CHANNEL_0);
	result = DM35425_Dma_Start(board, &my_dac, CHANNEL_0);

	check_result(result, "Error starting DMA");

	printf("success.\n");

	result = DM35425_Dac_Set_Start_Trigger(board,
					       &my_dac,
					       DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting start trigger for DAC.");

	result = DM35425_Dac_Set_Stop_Trigger(board,
					      &my_dac, DM35425_CLK_SRC_NEVER);

	check_result(result, "Error setting stop trigger for DAC.");

	result = DM35425_Dac_Channel_Setup(board,
					   &my_dac,
					   CHANNEL_0,
					   DM35425_DAC_RNG_UNIPOLAR_5V);

	check_result(result, "Error setting output range for DAC.");

	result = DM35425_Dac_Set_Clock_Src(board,
					   &my_dac, DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting clock source for DAC.");

	/***************************************************
	* Setting up ADIO, which will trigger off of external
	* global clock coming through BUS2, and will
	* output all bits via DMA, an alternating pattern
	* of 0's and then 1's
	****************************************************/
	result = DM35425_Adio_Open(board, 0, &my_adio);

	check_result(result, "Could not open ADIO");

	printf("\nOpened ADI0\n");

	result = DM35425_Adio_Set_Clock_Src(board, &my_adio, DM35425_CLK_BUS2);

	check_result(result, "Error setting ADIO clock");

	result = DM35425_Adio_Set_Clk_Divider(board, &my_adio, 0);

	check_result(result, "Error setting ADIO clock divider");

	result = DM35425_Adio_Set_Direction(board,
					    &my_adio, DM35425_ADIO_DIR_OUTPUT);

	check_result(result, "Error setting ADIO direction");

	buffer[0] = 0;
	buffer[1] = 0xFFFFFFFF;

	fprintf(stdout,
		"Initializing and configuring ADIO OUT DMA Channel....\n");
	result =
	    DM35425_Dma_Initialize(board, &my_adio,
				   DM35425_ADIO_OUT_DMA_CHANNEL, 1,
				   BUFFER_SIZE_BYTES);

	check_result(result, "Error initializing DMA");

	result = DM35425_Dma_Setup(board,
				   &my_adio,
				   DM35425_ADIO_OUT_DMA_CHANNEL,
				   DM35425_DMA_SETUP_DIRECTION_WRITE,
				   IGNORE_USED);

	check_result(result, "Error configuring DMA");

	result = DM35425_Dma_Write(board,
				   &my_adio,
				   DM35425_ADIO_OUT_DMA_CHANNEL,
				   BUFFER_0, BUFFER_SIZE_BYTES, buffer);

	check_result(result, "Writing to DMA output buffer failed");

	result = DM35425_Dma_Buffer_Setup(board,
					  &my_adio,
					  DM35425_ADIO_OUT_DMA_CHANNEL,
					  BUFFER_0,
					  DM35425_DMA_BUFFER_CTRL_VALID |
					  DM35425_DMA_BUFFER_CTRL_LOOP);

	check_result(result, "Error setting up buffer control.");

	result = DM35425_Dma_Status(board,
				    &my_adio,
				    DM35425_ADIO_OUT_DMA_CHANNEL,
				    &current_buffer,
				    &current_count,
				    &current_action,
				    &status_overflow,
				    &status_underflow,
				    &status_used,
				    &status_invalid, &status_complete);

	check_result(result, "Error getting DMA status");

	printf
	    ("ADIO DMA Status: Current Buffer: %d  Count: %d  Action: 0x%x  Status: "
	     "Ov: %d  Un: %d  Used: %d  Inv: %d  Comp: %d\n", current_buffer,
	     current_count, current_action, status_overflow, status_underflow,
	     status_used, status_invalid, status_complete);

	result = DM35425_Dma_Buffer_Status(board,
					   &my_adio,
					   DM35425_ADIO_OUT_DMA_CHANNEL,
					   BUFFER_0,
					   &buff_status,
					   &buff_control, &buff_size);
	fprintf(stdout, "    Buffer 0: Stat: 0x%x  Ctrl: 0x%x  Size: %u\n",
		buff_status, buff_control, buff_size);

	check_result(result, "Error getting buffer status.");

	fprintf(stdout, "Starting ADIO DMA ......");
	result =
	    DM35425_Dma_Start(board, &my_adio, DM35425_ADIO_OUT_DMA_CHANNEL);

	check_result(result, "Error starting DMA");
	fprintf(stdout, "success\n");
	result = DM35425_Adio_Set_Start_Trigger(board,
						&my_adio,
						DM35425_CLK_SRC_IMMEDIATE);
	check_result(result, "Error setting start trigger.");

	result = DM35425_Adio_Set_Stop_Trigger(board,
					       &my_adio, DM35425_CLK_SRC_NEVER);
	check_result(result, "Error setting stop trigger.");

	fprintf(stdout, "ADIO setup successfully\n");

	/***************************************************
	* Setting up ADC, which will trigger off of external
	* global clock coming through BUS3, and will
	* simply record data into DMA
	****************************************************/

	result = DM35425_Adc_Open(board, ADC_0, &my_adc);

	check_result(result, "Could not open ADC");

	printf("Found ADC, with %d DMA channels (%d buffers each)\n",
	       my_adc.num_dma_channels, my_adc.num_dma_buffers);

	result = DM35425_Adc_Set_Clock_Src(board, &my_adc, DM35425_CLK_BUS3);

	check_result(result, "Error setting ADC clock");

	result = DM35425_Adc_Set_Start_Trigger(board,
					       &my_adc,
					       DM35425_CLK_SRC_IMMEDIATE);
	check_result(result, "Error setting start trigger.");

	result = DM35425_Adc_Set_Stop_Trigger(board,
					      &my_adc, DM35425_CLK_SRC_NEVER);
	check_result(result, "Error setting stop trigger.");

	fprintf(stdout, "Initializing DMA Channel 0....");
	result = DM35425_Dma_Initialize(board,
					&my_adc,
					CHANNEL_0, 1, BUFFER_SIZE_BYTES);

	check_result(result, "Error initializing DMA");

	result = DM35425_Dma_Setup(board,
				   &my_adc,
				   CHANNEL_0,
				   DM35425_DMA_SETUP_DIRECTION_READ,
				   NOT_IGNORE_USED);

	check_result(result, "Error configuring DMA");

	fprintf(stdout, "Disabling DMA Interrupts......");
	result = DM35425_Dma_Configure_Interrupts(board,
						  &my_adc,
						  CHANNEL_0,
						  INTERRUPT_DISABLE,
						  ERROR_INTR_DISABLE);

	check_result(result, "Error setting DMA Interrupts");
	fprintf(stdout, "success!\n");

	result = DM35425_Dma_Buffer_Setup(board,
					  &my_adc,
					  CHANNEL_0,
					  BUFFER_0,
					  DM35425_DMA_BUFFER_CTRL_VALID |
					  DM35425_DMA_BUFFER_CTRL_LOOP);

	check_result(result, "Error setting buffer control.");

	result = DM35425_Dma_Buffer_Status(board,
					   &my_adc,
					   CHANNEL_0,
					   BUFFER_0,
					   &buff_status,
					   &buff_control, &buff_size);

	check_result(result, "Error getting buffer status.");

	fprintf(stdout,
		"    Buffer 0: Stat: 0x%x  Ctrl: 0x%x  Size: %d\n",
		buff_status, buff_control, buff_size);

	fprintf(stdout, "Starting ADC0 DMA ......");
	result = DM35425_Dma_Start(board, &my_adc, CHANNEL_0);

	check_result(result, "Error starting DMA");
	fprintf(stdout, "success\n");

	result = DM35425_Adc_Channel_Setup(board,
					   &my_adc,
					   CHANNEL_0,
					   DM35425_ADC_NO_DELAY,
					   DM35425_ADC_RNG_BIPOLAR_5V,
					   DM35425_ADC_INPUT_SINGLE_ENDED);

	check_result(result, "Error setting up channel.");

	result = DM35425_Adc_Set_Clk_Divider(board, &my_adc, 0);

	check_result(result, "Error setting clock divider");

	result = DM35425_Adc_Initialize(board, &my_adc);

	check_result(result, "Failed or timed out initializing ADC.");

	fprintf(stdout, "ADC setup successfullly\n");

	/*************************************************
	* Finally, setup the global clocking function block
	**************************************************/
	result = DM35425_Ext_Clocking_Open(board, 0, &my_ext_clk);

	check_result(result, "Error opening global clocking FB");

	result = DM35425_Ext_Clocking_Set_Dir(board,
					      &my_ext_clk,
					      DM35425_EXT_CLK_DIR_INPUT);

	check_result(result, "Error setting gbl clocking direction");

	result = DM35425_Ext_Clocking_Set_Edge(board,
					       &my_ext_clk,
					       DM35425_EXT_CLK_EDGE_RISING);

	check_result(result, "Error setting clocking edge");

	result = DM35425_Ext_Clocking_Set_Pulse_Width(board,
						      &my_ext_clk,
						      DM35425_CLK_BUS2, 1);

	check_result(result, "Error setting pulse width");

	result = DM35425_Ext_Clocking_Set_Method(board,
						 &my_ext_clk,
						 DM35425_CLK_BUS2,
						 DM35425_EXT_CLOCKING_NOT_GATED);

	check_result(result, "Error setting gating method for BUS2");

	result = DM35425_Ext_Clocking_Set_Method(board,
						 &my_ext_clk,
						 DM35425_CLK_BUS3,
						 DM35425_EXT_CLOCKING_NOT_GATED);

	check_result(result, "Error setting gating method for BUS2");

	/***************************************************
	* Now, start all of them from last to first
	***************************************************/

	printf("Starting ADC\n");

	result = DM35425_Adc_Start(board, &my_adc);

	check_result(result, "Error starting ADC");

	result = DM35425_Adio_Start(board, &my_adio);

	check_result(result, "Error starting ADIO");

	result = DM35425_Dac_Start(board, &my_dac);

	check_result(result, "Error starting DAC");

	fprintf(stdout, "\nPress Ctrl-C to exit.\n\n");
	fprintf(stdout,
		"Clock Counts\n=======================================\n");
	while (!exit_program) {

		result = DM35425_Dac_Get_Conversion_Count(board,
							  &my_dac,
							  &conversions);

		check_result(result, "Error getting DAC conversion count.");

		fprintf(stdout, "DAC: %4u    ", conversions);

		result = DM35425_Adio_Get_Sample_Count(board,
						       &my_adio, &conversions);

		check_result(result, "Error getting ADIO sample count.");

		fprintf(stdout, "ADIO: %4u    ", conversions);

		result = DM35425_Adc_Get_Sample_Count(board,
						      &my_adc, &conversions);

		check_result(result, "Error getting ADC sample count.");

		fprintf(stdout, "ADC: %4u    \r", conversions);

		DM35425_Micro_Sleep(100);

	}

	printf("success.\nClosing Board....");
	result = DM35425_Board_Close(board);

	check_result(result, "Error closing board.");
	printf("success.\nExample program successfully completed.\n\n");

	return 0;

}

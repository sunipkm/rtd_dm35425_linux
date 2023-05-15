/**
    @file

    @brief
        Example program which demonstrates the use of the ADIO
        and DMA.

    @verbatim

        The example will make use of 3 DMA buffers for each
        of the three DMA channels (ADIO In, ADIO Out, and ADIO
        Direction).  Data will "play out" of the ADIO Out and
        Direction channels, and be stored in the ADIO In DMA
        buffer.  Doing this, we'll receive a pattern in the DMA
        In buffers that is the result of the output and changing
        bit direction values.

        At the end, we'll compare what is stored in the ADIO In
        DMA buffers to what should have been the result and make
        sure it is correct.

        This example requires a loopback of DIO0-DIO7 to DIO8-DIO15
        and DIO16-DIO23 to DIO24-DIO31.  This can most easily be accomplished
        using standard sized jumpers and placing them across the
        following pins:

        CN3 and CN4:
        Pin23 to Pin24
        Pin25 to Pin26
        Pin27 to Pin28
        Pin29 to Pin30
        Pin31 to Pin32
        Pin33 to Pin34
        Pin35 to Pin36
        Pin37 to Pin38

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

    $Id: dm35425_adio_dma.c 108025 2017-04-14 15:09:34Z rgroner $
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

#include "dm35425_gbc_library.h"
#include "dm35425_adio_library.h"
#include "dm35425_ioctl.h"
#include "dm35425_util_library.h"
#include "dm35425_examples.h"
#include "dm35425_dma_library.h"

/**
 * Constant defining the input and output direction
 * of the ADIO pins
 */
#define DM35425_ADIO_DIRECTION1		0x00FF00FF

/**
 * Constant defining the input and output direction
 * of the ADIO pins
 */
#define DM35425_ADIO_DIRECTION2		0xFF00FF00

/**
 * Size of the DMA buffer, in samples
 */
#define BUFFER_SIZE_SAMPLES		10000

/**
 * Size of the DMA buffer, in bytes.
 */
#define BUFFER_SIZE_BYTES		(BUFFER_SIZE_SAMPLES * 4)

/**
 * Rate the ADIO is running
 */
#define ADIO_RATE			10000

/**
 * Name of the program as invoked on the command line
 */
static char *program_name;

/**
 * A count of buffers copied
 */
volatile unsigned int buffer_copied = 0;

/**
 * Flag indicating a DMA error occured
 */
int dma_has_error = 0;

/**
 * Boolean indicating the program should be exited.
 */
volatile int exit_program = 0;

/**
 * Pointer to board descriptor
 */
struct DM35425_Board_Descriptor *board;

/**
 * ADIO function block descriptor
 */
struct DM35425_Function_Block my_adio;

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
void ISR(struct dm35425_ioctl_interrupt_info_request int_info)
{
	char message[200];

	int result = 0;

	check_result(int_info.error_occurred, message);

	if (int_info.valid_interrupt) {

		// It's a DMA interrupt
		if (int_info.interrupt_fb < 0) {

			buffer_copied++;

			result = DM35425_Dma_Clear_Interrupt(board,
							     &my_adio,
							     DM35425_ADIO_IN_DMA_CHANNEL,
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
	    ("FB%d Ch%d DMA Status: Current Buffer: %u  Count: %u  Action: 0x%x  Status: "
	     "Ov: %d  Un: %d  Used: %d  Inv: %d  Comp: %d\n",
	     func_block->fb_num, channel, current_buffer, current_count,
	     current_action, status_overflow, status_underflow, status_used,
	     status_invalid, status_complete);
}

/**
*******************************************************************************
@brief
    Output the status of a DMA buffer.  This is a helper function to determine
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

@param
    buffer

    The DMA channel buffer we want the status of.

 @retval
    None
 *******************************************************************************
*/
void output_dma_buffer_status(struct DM35425_Board_Descriptor *handle,
			      const struct DM35425_Function_Block *func_block,
			      unsigned int channel, unsigned int buffer)
{
	uint8_t buff_status, buff_control;
	uint32_t buff_size;
	int result = 0;

	result = DM35425_Dma_Buffer_Status(handle,
					   func_block,
					   channel,
					   buffer,
					   &buff_status,
					   &buff_control, &buff_size);

	check_result(result, "Error getting buffer status");

	fprintf(stdout, "    Buffer 0: Stat: 0x%x  Ctrl: 0x%x  Size: %u\n",
		buff_status, buff_control, buff_size);

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
	int result, index;

	int adio_num = 0;

	uint32_t rate;

	int help_option_given = 0;

	int status;
	struct sigaction signal_action;

	uint32_t *output_buffer[3], *input_buffer[3], *dir_buffer[3];

	int buffer = 0;
	unsigned int buffer_read = 0;
	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, 1},
		{"minor", 1, 0, 2},
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
	printf("success.\nOpening ADIO......");

	result = DM35425_Adio_Open(board, 0, &my_adio);

	check_result(result, "Could not open ADIO");

	printf("Found ADIO%d\n", adio_num);

	result = DM35425_Adio_Set_Clock_Src(board,
					    &my_adio,
					    DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting ADIO clock");

	result = DM35425_Adio_Set_Pacer_Clk_Rate(board,
						 &my_adio, ADIO_RATE, &rate);
	check_result(result, "Error setting conversion rate.");

	fprintf(stdout, "Requested rate %u, got %u.\n", ADIO_RATE, rate);

	/*************************************************
	*   Setup IN DMA Channel
	**************************************************/
	fprintf(stdout,
		"Initializing and configuring ADIO IN DMA Channel....\n");
	result =
	    DM35425_Dma_Initialize(board, &my_adio, DM35425_ADIO_IN_DMA_CHANNEL,
				   3, BUFFER_SIZE_BYTES);

	check_result(result, "Error initializing DMA");

	result = DM35425_Dma_Setup(board,
				   &my_adio,
				   DM35425_ADIO_IN_DMA_CHANNEL,
				   DM35425_DMA_SETUP_DIRECTION_READ,
				   IGNORE_USED);

	check_result(result, "Error configuring DMA");

	fprintf(stdout, "Setting DMA Interrupts......");
	result = DM35425_Dma_Configure_Interrupts(board,
						  &my_adio,
						  DM35425_ADIO_IN_DMA_CHANNEL,
						  INTERRUPT_ENABLE,
						  ERROR_INTR_ENABLE);

	check_result(result, "Error setting DMA Interrupts");
	fprintf(stdout, "success!\n");

	output_channel_status(board, &my_adio, DM35425_ADIO_IN_DMA_CHANNEL);

	/**
	 * Set the DMA to halt once the 3rd buffer is finished
	 */
	for (index = 0; index < 3; index++) {

		if (index < 2) {
			result = DM35425_Dma_Buffer_Setup(board,
							  &my_adio,
							  DM35425_ADIO_IN_DMA_CHANNEL,
							  index,
							  DM35425_DMA_BUFFER_CTRL_VALID
							  |
							  DM35425_DMA_BUFFER_CTRL_INTR);

			check_result(result,
				     "Error setting up buffer control.");
		} else {
			result = DM35425_Dma_Buffer_Setup(board,
							  &my_adio,
							  DM35425_ADIO_IN_DMA_CHANNEL,
							  index,
							  DM35425_DMA_BUFFER_CTRL_VALID
							  |
							  DM35425_DMA_BUFFER_CTRL_HALT
							  |
							  DM35425_DMA_BUFFER_CTRL_INTR);

			check_result(result,
				     "Error setting up buffer control.");

		}

		output_dma_buffer_status(board,
					 &my_adio,
					 DM35425_ADIO_IN_DMA_CHANNEL, index);

	}
	fprintf(stdout, "\n");

	/*************************************************
	*   Setup OUT DMA Channel
	**************************************************/
	fprintf(stdout,
		"Initializing and configuring ADIO OUT DMA Channel....\n");
	result =
	    DM35425_Dma_Initialize(board, &my_adio,
				   DM35425_ADIO_OUT_DMA_CHANNEL, 3,
				   BUFFER_SIZE_BYTES);

	check_result(result, "Error initializing DMA");

	result = DM35425_Dma_Setup(board,
				   &my_adio,
				   DM35425_ADIO_OUT_DMA_CHANNEL,
				   DM35425_DMA_SETUP_DIRECTION_WRITE,
				   IGNORE_USED);

	check_result(result, "Error configuring DMA");

	output_channel_status(board, &my_adio, DM35425_ADIO_OUT_DMA_CHANNEL);

	for (index = 0; index < 3; index++) {

		if (index < 2) {
			result = DM35425_Dma_Buffer_Setup(board,
							  &my_adio,
							  DM35425_ADIO_OUT_DMA_CHANNEL,
							  index,
							  DM35425_DMA_BUFFER_CTRL_VALID);

			check_result(result,
				     "Error setting up buffer control.");
		} else {
			result = DM35425_Dma_Buffer_Setup(board,
							  &my_adio,
							  DM35425_ADIO_OUT_DMA_CHANNEL,
							  index,
							  DM35425_DMA_BUFFER_CTRL_VALID
							  |
							  DM35425_DMA_BUFFER_CTRL_HALT);

			check_result(result,
				     "Error setting up buffer control.");
		}
		output_dma_buffer_status(board,
					 &my_adio,
					 DM35425_ADIO_OUT_DMA_CHANNEL, index);
	}
	fprintf(stdout, "\n");

	/*************************************************
	*   Setup DIR DMA Channel
	**************************************************/
	fprintf(stdout,
		"Initializing and configuring ADIO DIR DMA Channel....\n");
	result =
	    DM35425_Dma_Initialize(board, &my_adio,
				   DM35425_ADIO_DIR_DMA_CHANNEL, 3,
				   BUFFER_SIZE_BYTES);

	check_result(result, "Error initializing DMA");

	result = DM35425_Dma_Setup(board,
				   &my_adio,
				   DM35425_ADIO_DIR_DMA_CHANNEL,
				   DM35425_DMA_SETUP_DIRECTION_WRITE,
				   IGNORE_USED);

	check_result(result, "Error configuring DMA");

	output_channel_status(board, &my_adio, DM35425_ADIO_DIR_DMA_CHANNEL);

	for (index = 0; index < 3; index++) {

		result = DM35425_Dma_Buffer_Setup(board,
						  &my_adio,
						  DM35425_ADIO_DIR_DMA_CHANNEL,
						  index,
						  DM35425_DMA_BUFFER_CTRL_VALID);

		check_result(result, "Error setting up buffer control.");

		output_dma_buffer_status(board,
					 &my_adio,
					 DM35425_ADIO_DIR_DMA_CHANNEL, index);

	}

	fprintf(stdout, "\n");

	for (index = 0; index < 3; index++) {
		output_buffer[index] = (uint32_t *) malloc(BUFFER_SIZE_BYTES);
		input_buffer[index] = (uint32_t *) malloc(BUFFER_SIZE_BYTES);
		dir_buffer[index] = (uint32_t *) malloc(BUFFER_SIZE_BYTES);

		check_result((output_buffer[index] == NULL) ||
			     (input_buffer[index] == NULL) ||
			     (dir_buffer[index] == NULL),
			     "Error allocating space for DMA buffer.");

	}

	/*******************************************************************
	* In the first set of buffers, the output will be a series of
	* bit values, and the direction will have ADIO0-ADIO7 as output,
	* ADIO8-15 as input, etc.
	********************************************************************/
	for (index = 0; index < BUFFER_SIZE_SAMPLES; index++) {
		output_buffer[0][index] = (index * 2000) % 0xffffffff;
		dir_buffer[0][index] = DM35425_ADIO_DIRECTION1;
	}

	/*******************************************************************
	* In the second set of buffers, the output will be a series of
	* bit values, and the direction will have ADIO0-ADIO7 as input,
	* ADIO8-15 as output, etc.
	********************************************************************/
	for (index = 0; index < BUFFER_SIZE_SAMPLES; index++) {
		output_buffer[1][index] = (index * 2000) % 0xABABABAB;
		dir_buffer[1][index] = DM35425_ADIO_DIRECTION2;
	}

	/*******************************************************************
	* In the third set of buffers, the output will be a constant value
	* (0xFFFFFFFF), and the direction will be an alternating pattern
	********************************************************************/
	for (index = 0; index < BUFFER_SIZE_SAMPLES; index++) {
		output_buffer[2][index] = 0xFFFFFFFF;
		dir_buffer[2][index] = 3 << (index % 32);
	}

	/*******************************************************************
	*  Now write the buffers to the DMA buffers
	*******************************************************************/
	for (buffer = 0; buffer < 3; buffer++) {
		result = DM35425_Dma_Write(board,
					   &my_adio,
					   DM35425_ADIO_OUT_DMA_CHANNEL,
					   buffer,
					   BUFFER_SIZE_BYTES,
					   output_buffer[buffer]);

		check_result(result, "Writing to DMA output buffer failed");

		result = DM35425_Dma_Write(board,
					   &my_adio,
					   DM35425_ADIO_DIR_DMA_CHANNEL,
					   buffer,
					   BUFFER_SIZE_BYTES,
					   dir_buffer[buffer]);

		check_result(result, "Writing to DMA direction buffer failed");
	}

	fprintf(stdout, "success.\nInstalling user ISR .....");
	result = DM35425_General_InstallISR(board, ISR);
	check_result(result, "DM35425_General_InstallISR()");
	fprintf(stdout, "success.\n");

	fprintf(stdout, "Starting ADIO DMA ......");
	result = DM35425_Dma_Start(board, &my_adio,
				   DM35425_ADIO_OUT_DMA_CHANNEL);

	check_result(result, "Error starting DMA");

	result = DM35425_Dma_Start(board, &my_adio,
				   DM35425_ADIO_IN_DMA_CHANNEL);

	check_result(result, "Error starting DMA");

	result = DM35425_Dma_Start(board, &my_adio,
				   DM35425_ADIO_DIR_DMA_CHANNEL);

	check_result(result, "Error starting DMA");

	printf("success.\n");
	result = DM35425_Adio_Set_Start_Trigger(board,
						&my_adio,
						DM35425_CLK_SRC_IMMEDIATE);
	check_result(result, "Error setting start trigger.");

	result = DM35425_Adio_Set_Stop_Trigger(board,
					       &my_adio, DM35425_CLK_SRC_NEVER);
	check_result(result, "Error setting stop trigger.");

	buffer_copied = 0;

	printf("Starting ADIO\n");

	result = DM35425_Adio_Start(board, &my_adio);

	check_result(result, "Error starting ADIO");

	buffer_read = 0;

	while (!exit_program) {

		if (buffer_read < buffer_copied) {
			printf("Input Buffer copied!\n");

			result = DM35425_Dma_Read(board,
						  &my_adio,
						  DM35425_ADIO_IN_DMA_CHANNEL,
						  buffer_read,
						  BUFFER_SIZE_BYTES,
						  input_buffer[buffer_read]);

			check_result(result, "Error getting DMA buffer");

			result = DM35425_Dma_Reset_Buffer(board,
							  &my_adio,
							  DM35425_ADIO_IN_DMA_CHANNEL,
							  buffer_read);

			check_result(result, "Error resetting buffer");

			buffer_read++;

		} else {

			DM35425_Micro_Sleep(100000);
		}

		if (buffer_read == 3) {
			exit_program = 1;
		}

	}

	check_result(dma_has_error, "****DMA Error was detected.\n");

	/*************************
	* Check first buffer
	**************************/

	printf("\nComparing output buffer to expected value...\n\n");
	for (index = 1; index < BUFFER_SIZE_SAMPLES; index++) {
		if ((output_buffer[1][index - 1] & dir_buffer[0][index - 1]) !=
		    ((input_buffer[0][index] & ~dir_buffer[0][index - 1]) >> 8))
		{

			printf
			    ("Mismatch:  Out: 0x%8x    In: 0x%8x   Dir: 0x%8x\n",
			     output_buffer[0][index - 1],
			     input_buffer[0][index], dir_buffer[0][index - 1]);
			check_result(1,
				     "Input buffer[0] did not match expected result");
		}
	}

	for (index = 1; index < BUFFER_SIZE_SAMPLES; index++) {
		if ((input_buffer[1][index] & ~dir_buffer[1][index - 1]) !=
		    ((output_buffer[1][index - 1] & dir_buffer[1][index - 1]) >>
		     8)) {

			printf
			    ("Mismatch:  Out: 0x%8x    In: 0x%8x   Dir: 0x%8x\n",
			     output_buffer[1][index - 1],
			     input_buffer[1][index], dir_buffer[1][index - 1]);
			check_result(1,
				     "Input buffer[1] did not match expected result");
		}
	}

	result = DM35425_Board_Close(board);
	printf("No errors.\n");
	check_result(result, "Error closing board.");
	printf("Example program successfully completed.\n");

	for (buffer = 0; buffer < 3; buffer++) {
		free(output_buffer[buffer]);
		free(input_buffer[buffer]);
		free(dir_buffer[buffer]);
	}

	return 0;

}

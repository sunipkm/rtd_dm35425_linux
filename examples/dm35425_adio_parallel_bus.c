/**
    @file

    @brief
        Example program which demonstrates the use of the ADIO acting
        as a parallel bus.

    @verbatim

        The ADIO may be used as a parallel bus to transfer data from 1 board
        to another.  In this mode, 3 ADIO signals are used for control, and
        the remaining 29 bits are used for passing data. This board uses DMA
        and the parallel bus mode to transfer data from 1 board to another.

        Two DM35425 boards are required for this example. Both boards will run
        the same example program, but one will be designated as the "sender",
        and one the "receiver".  Both examples should be executed and allowed
        to complete their setup before the data transfer is begun.

	In this example, only the ADIO bits on CN3 will be used for passing
	data.  All ADIO pins on CN3 (Pins 23-38) must be connected from 1 board
	to CN3 (Pins 23-38) of the 2nd board.

	The three control lines on CN4 must also be connected between the
	boards:

	CN4 Pin 24
	CN4 Pin 26
	CN4 Pin 28

	Use the --help command line option to see all possible options.

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

    $Id: dm35425_adio_parallel_bus.c 108025 2017-04-14 15:09:34Z rgroner $
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
#include "dm35425_adio_library.h"
#include "dm35425.h"

/**
 * Constant giving direction for sender board
 */
#define DM35425_ADIO_OUT_DIRECTION		0xBFFFFFFF

/**
 * Constant giving direction for receiver board
 */
#define DM35425_ADIO_IN_DIRECTION		0x40000000

/**
 * Rate of passing data (Hz)
 */
#define DEFAULT_RATE	100000

/**
 * Number of samples to create.
 */
#define BUFFER_SIZE_SAMPLES	0x400

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
 * Keep a count of how many interrupts have happened
 */
unsigned interrupt_count = 0;

/**
 * Boolean indicating that this example is being run by the sender
 */
int is_sender = 0;

/**
 * Boolean indicating that this example is being run by the receiver.
 */
int is_receiver = 0;

/**
 * Function block for ADIO
 */
struct DM35425_Function_Block my_adio;

/**
 * Board descriptor
 */
struct DM35425_Board_Descriptor *board;

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
	fprintf(stderr, "USAGE\n\n\t%s [OPTIONS] ROLE\n\n", program_name);

	fprintf(stderr, "ROLE (Required)\n\n");
	fprintf(stderr,
		"\tThis example requires a sender board and receiver board.  The user must\n");
	fprintf(stderr, "\tspecify which role this example will use.\n");
	fprintf(stderr, "\t\t--sender\n");
	fprintf(stderr, "\t\t\tThis example is sending the data.\n");
	fprintf(stderr, "\t\t--receiver\n");
	fprintf(stderr, "\t\t\tThis example is receiving the data.\n");

	fprintf(stderr, "OPTIONS\n\n");

	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tShow this help screen and exit.\n");

	fprintf(stderr, "\t--minor NUM\n");
	fprintf(stderr,
		"\t\tSpecify the minor number (>= 0) of the board to open.  When not specified,\n");
	fprintf(stderr, "\t\tthe device file with minor 0 is opened.\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
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

	// It's a DMA interrupt
	if (int_info.interrupt_fb < 0) {

		interrupt_count++;

		if (is_sender) {
			result = DM35425_Dma_Clear_Interrupt(board,
							     &my_adio,
							     DM35425_ADIO_OUT_DMA_CHANNEL,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     CLEAR_INTERRUPT);
		} else {
			result = DM35425_Dma_Clear_Interrupt(board,
							     &my_adio,
							     DM35425_ADIO_IN_DMA_CHANNEL,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     CLEAR_INTERRUPT);
		}

	} else {
		printf("*** Process non-DMA interrupt for FB 0x%x.\n",
		       int_info.interrupt_fb);
	}

	result = DM35425_Gbc_Ack_Interrupt(board);

	check_result(result, "Error calling ACK interrupt.");

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

	unsigned long int minor = 0;
	int result;

	struct sigaction signal_action;

	uint32_t current_buffer = 0, current_count = 0;

	int current_action;
	int status_overflow;
	int status_underflow;
	int status_used;
	int status_invalid;
	int status_complete;
	unsigned int buffer_num = 0, index = 0;
	unsigned int local_interrupt_count = 0;
	unsigned int data_transferred_count = 0;
	unsigned int matched_count = 0;
	uint8_t buff_status;
	uint8_t buff_control;
	uint32_t buff_size;

	uint32_t actual_rate;

	int help_option_given = 0;
	int status;

	int32_t *send_buffer = NULL, *receive_buffer = NULL;
	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"sender", 0, 0, SENDER_OPTION},
		{"receiver", 0, 0, RECEIVER_OPTION},
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
			   User entered --sender
			   ################################################################# */
		case SENDER_OPTION:

			is_sender = 1;

			break;

			/*#################################################################
			   User entered a waveform choice
			   ################################################################# */
		case RECEIVER_OPTION:

			is_receiver = 1;
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

	if ((is_sender ^ is_receiver) != 1) {
		error(0, 0,
		      "ERROR: You must specify this board as either the sender "
		      "or the receiver, but not both.\n");
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

	result = DM35425_Adio_Open(board, ADIO_0, &my_adio);

	check_result(result, "Could not open ADIO");

	printf("Found ADIO%u, with %d DMA channels (%d buffers each)\n",
	       ADIO_0, my_adio.num_dma_channels, my_adio.num_dma_buffers);

	result = DM35425_Adio_Set_Clock_Src(board,
					    &my_adio,
					    DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting DAC clock");

	result = DM35425_Adio_Set_Pacer_Clk_Rate(board,
						 &my_adio, DEFAULT_RATE,
						 &actual_rate);

	fprintf(stdout, "Rate requested: %d  Actual Rate Achieved: %d\n",
		DEFAULT_RATE, actual_rate);
	check_result(result, "Error setting sample rate");

	result = DM35425_Adio_Set_P_Bus_Enable(board, &my_adio, ENABLED);

	check_result(result, "Error enabling parallel bus.");

	send_buffer =
	    (int *)malloc(my_adio.num_dma_buffers * BUFFER_SIZE_SAMPLES *
			  sizeof(int));

	check_result(send_buffer == NULL,
		     "Error allocating space for buffers.");

	for (index = 0; index < my_adio.num_dma_buffers * BUFFER_SIZE_SAMPLES;
	     index++) {
		send_buffer[index] = index;

	}

	if (is_sender) {
		fprintf(stdout,
			"Initializing and configuring ADIO DMA OUTPUT Channel "
			"as data SENDER.....\n");
		result =
		    DM35425_Dma_Initialize(board, &my_adio,
					   DM35425_ADIO_OUT_DMA_CHANNEL,
					   my_adio.num_dma_buffers,
					   BUFFER_SIZE_BYTES);

		check_result(result, "Error initializing DMA");
		fprintf(stdout, "Setting DMA Interrupts......");
		result = DM35425_Dma_Configure_Interrupts(board,
							  &my_adio,
							  DM35425_ADIO_OUT_DMA_CHANNEL,
							  INTERRUPT_ENABLE,
							  ERROR_INTR_ENABLE);

		check_result(result, "Error setting DMA Interrupts");
		fprintf(stdout, "success!\n");

		result = DM35425_Dma_Setup(board,
					   &my_adio,
					   DM35425_ADIO_OUT_DMA_CHANNEL,
					   DM35425_DMA_SETUP_DIRECTION_WRITE,
					   IGNORE_USED);

		check_result(result, "Error configuring DMA");

		fprintf(stdout, "success!\n");

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
		    ("DMA Status: Current Buffer: %d  Count: %d  Action: 0x%x  Status: "
		     "Ov: %d  Un: %d  Used: %d  Inv: %d  Comp: %d\n",
		     current_buffer, current_count, current_action,
		     status_overflow, status_underflow, status_used,
		     status_invalid, status_complete);

		for (buffer_num = 0; buffer_num < my_adio.num_dma_buffers;
		     buffer_num++) {

			if (buffer_num < (my_adio.num_dma_buffers - 1)) {
				result = DM35425_Dma_Buffer_Setup(board,
								  &my_adio,
								  DM35425_ADIO_OUT_DMA_CHANNEL,
								  buffer_num,
								  DM35425_DMA_BUFFER_CTRL_VALID
								  |
								  DM35425_DMA_BUFFER_CTRL_INTR);

				check_result(result,
					     "Error setting up buffer control.");
			} else {

				result = DM35425_Dma_Buffer_Setup(board,
								  &my_adio,
								  DM35425_ADIO_OUT_DMA_CHANNEL,
								  buffer_num,
								  DM35425_DMA_BUFFER_CTRL_VALID
								  |
								  DM35425_DMA_BUFFER_CTRL_HALT
								  |
								  DM35425_DMA_BUFFER_CTRL_INTR);

				check_result(result,
					     "Error setting up buffer control.");

			}
			result = DM35425_Dma_Buffer_Status(board,
							   &my_adio,
							   DM35425_ADIO_OUT_DMA_CHANNEL,
							   buffer_num,
							   &buff_status,
							   &buff_control,
							   &buff_size);
			fprintf(stdout,
				"    Buffer %u: Stat: 0x%x  Ctrl: 0x%x  Size: %u\n",
				buffer_num, buff_status, buff_control,
				buff_size);

			check_result(result, "Error getting buffer status.");

			result = DM35425_Dma_Write(board,
						   &my_adio,
						   DM35425_ADIO_OUT_DMA_CHANNEL,
						   buffer_num,
						   BUFFER_SIZE_BYTES,
						   send_buffer +
						   (buffer_num *
						    BUFFER_SIZE_SAMPLES));

			check_result(result, "Writing to DMA buffer failed");
		}

		fprintf(stdout, "Starting DMA Output Channel......");
		result =
		    DM35425_Dma_Start(board, &my_adio,
				      DM35425_ADIO_OUT_DMA_CHANNEL);

		check_result(result, "Error starting DMA");

		printf("success.\n");

		result = DM35425_Adio_Set_Direction(board,
						    &my_adio,
						    DM35425_ADIO_OUT_DIRECTION);

		check_result(result, "Could not set direction of ADIO pins.");

		result = DM35425_Adio_Set_P_Bus_Ready_Enable(board,
							     &my_adio, ENABLED);

		check_result(result, "Error enabling parallel bus ready.");

	} else {

		result = DM35425_Adio_Set_Direction(board,
						    &my_adio,
						    DM35425_ADIO_IN_DIRECTION);

		check_result(result, "Could not set direction of ADIO pins.");

		receive_buffer =
		    (int *)malloc(my_adio.num_dma_buffers *
				  BUFFER_SIZE_SAMPLES * sizeof(int));

		check_result(receive_buffer == NULL,
			     "Error allocating space for buffers.");

		fprintf(stdout,
			"Initializing and configuring ADIO DMA INPUT Channel "
			"as data RECEIVER.....\n");
		result =
		    DM35425_Dma_Initialize(board, &my_adio,
					   DM35425_ADIO_IN_DMA_CHANNEL,
					   my_adio.num_dma_buffers,
					   BUFFER_SIZE_BYTES);

		check_result(result, "Error initializing DMA");

		fprintf(stdout, "Setting DMA Interrupts......");
		result = DM35425_Dma_Configure_Interrupts(board,
							  &my_adio,
							  DM35425_ADIO_IN_DMA_CHANNEL,
							  INTERRUPT_ENABLE,
							  ERROR_INTR_ENABLE);

		check_result(result, "Error setting DMA Interrupts");
		fprintf(stdout, "success!\n");

		result = DM35425_Dma_Setup(board,
					   &my_adio,
					   DM35425_ADIO_IN_DMA_CHANNEL,
					   DM35425_DMA_SETUP_DIRECTION_READ,
					   IGNORE_USED);

		check_result(result, "Error configuring DMA");

		fprintf(stdout, "success!\n");

		result = DM35425_Dma_Status(board,
					    &my_adio,
					    DM35425_ADIO_IN_DMA_CHANNEL,
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
		     "Ov: %d  Un: %d  Used: %d  Inv: %d  Comp: %d\n",
		     current_buffer, current_count, current_action,
		     status_overflow, status_underflow, status_used,
		     status_invalid, status_complete);

		for (buffer_num = 0; buffer_num < my_adio.num_dma_buffers;
		     buffer_num++) {

			if (buffer_num < (my_adio.num_dma_buffers - 1)) {
				result = DM35425_Dma_Buffer_Setup(board,
								  &my_adio,
								  DM35425_ADIO_IN_DMA_CHANNEL,
								  buffer_num,
								  DM35425_DMA_BUFFER_CTRL_VALID
								  |
								  DM35425_DMA_BUFFER_CTRL_INTR);

				check_result(result,
					     "Error setting up buffer control.");
			} else {

				result = DM35425_Dma_Buffer_Setup(board,
								  &my_adio,
								  DM35425_ADIO_IN_DMA_CHANNEL,
								  buffer_num,
								  DM35425_DMA_BUFFER_CTRL_VALID
								  |
								  DM35425_DMA_BUFFER_CTRL_HALT
								  |
								  DM35425_DMA_BUFFER_CTRL_INTR);

				check_result(result,
					     "Error setting up buffer control.");

			}
			result = DM35425_Dma_Buffer_Status(board,
							   &my_adio,
							   DM35425_ADIO_IN_DMA_CHANNEL,
							   buffer_num,
							   &buff_status,
							   &buff_control,
							   &buff_size);
			fprintf(stdout,
				"    Buffer %u: Stat: 0x%x  Ctrl: 0x%x  Size: %u\n",
				buffer_num, buff_status, buff_control,
				buff_size);

			check_result(result, "Error getting buffer status.");

		}

		fprintf(stdout, "Starting DMA Input Channel......");
		result =
		    DM35425_Dma_Start(board, &my_adio,
				      DM35425_ADIO_IN_DMA_CHANNEL);

		check_result(result, "Error starting DMA");

		printf("success.\n");

	}

	fprintf(stdout, "Installing user ISR .....");
	result = DM35425_General_InstallISR(board, ISR);
	check_result(result, "DM35425_General_InstallISR()");
	fprintf(stdout, "success.\n");

	fprintf(stdout, "Starting ADIO.\n");

	result = DM35425_Adio_Set_Start_Trigger(board,
						&my_adio,
						DM35425_CLK_SRC_IMMEDIATE);

	check_result(result, "Error setting start trigger for ADIO.");

	result = DM35425_Adio_Set_Stop_Trigger(board,
					       &my_adio, DM35425_CLK_SRC_NEVER);

	check_result(result, "Error setting stop trigger for ADIO.");

	if (is_sender) {
		printf
		    ("Setup complete!\n\nPress Ctrl-C to continue when the receiver has completed its setup.\n");
	} else {
		printf
		    ("Setup complete!\n\nPress Ctrl-C to continue when the sender has completed its setup.\n");
	}

	while (!exit_program) {
		DM35425_Micro_Sleep(20000);
	}

	exit_program = 0;

	result = DM35425_Adio_Start(board, &my_adio);

	check_result(result, "Error starting ADIO");

	printf("\nPress Ctrl-C to exit.\n\n");
	local_interrupt_count = 0;

	printf("Waiting for transfer to begin.....\n");

	while (!exit_program &&
	       current_count <= (DM35425_FIFO_SAMPLE_SIZE * sizeof(int)) &&
	       current_buffer == 0) {
		if (is_sender) {
			result = DM35425_Dma_Get_Current_Buffer_Count(board,
								      &my_adio,
								      DM35425_ADIO_OUT_DMA_CHANNEL,
								      &current_buffer,
								      &current_count);
		} else {
			result = DM35425_Dma_Get_Current_Buffer_Count(board,
								      &my_adio,
								      DM35425_ADIO_IN_DMA_CHANNEL,
								      &current_buffer,
								      &current_count);

		}

		check_result(result, "Error getting current buffer count");

		DM35425_Micro_Sleep(20);
	}

	while (!exit_program) {

		if (local_interrupt_count < interrupt_count) {
			if (is_sender) {
				printf("Buffer sent across parallel bus.\n");
			} else {

				result = DM35425_Dma_Read(board,
							  &my_adio,
							  DM35425_ADIO_IN_DMA_CHANNEL,
							  local_interrupt_count,
							  BUFFER_SIZE_BYTES,
							  receive_buffer +
							  (local_interrupt_count
							   *
							   BUFFER_SIZE_SAMPLES));

				check_result(result,
					     "Error getting DMA buffer");

				result = DM35425_Dma_Reset_Buffer(board,
								  &my_adio,
								  DM35425_ADIO_IN_DMA_CHANNEL,
								  local_interrupt_count);

				check_result(result, "Error resetting buffer");

				printf("Buffer %d copied from parallel bus\n",
				       local_interrupt_count);

			}
			data_transferred_count += BUFFER_SIZE_SAMPLES;
			local_interrupt_count++;
		} else {

			if (is_sender) {

				printf("DMA OUT:\n");
				output_channel_status(board,
						      &my_adio,
						      DM35425_ADIO_OUT_DMA_CHANNEL);
			} else {
				printf("\nDMA IN:\n");
				output_channel_status(board,
						      &my_adio,
						      DM35425_ADIO_IN_DMA_CHANNEL);

			}
			DM35425_Micro_Sleep(2000000);
		}

		if (local_interrupt_count == my_adio.num_dma_buffers) {
			exit_program = 1;
		}

	}

	if (is_receiver) {

		fprintf(stdout,
			"\nComparing received data to sent data....\n\n");
		if (data_transferred_count > 0) {
			for (index = 0;
			     index <
			     my_adio.num_dma_buffers * BUFFER_SIZE_SAMPLES;
			     index++) {
				if (send_buffer[index] !=
				    (receive_buffer[index] & 0xFFFF)) {
					fprintf(stdout, "%d      %d   ",
						send_buffer[index],
						(receive_buffer[index] &
						 0xFFFF));
					fprintf(stdout,
						"   <----- Mismatch\n\n");
					break;
				} else {
					matched_count++;
				}

			}
		}
		free(receive_buffer);
		check_result(matched_count != data_transferred_count,
			     "Data received does not match data sent.");

	}
	free(send_buffer);

	printf("Closing Board....");
	result = DM35425_Board_Close(board);

	check_result(result, "Error closing board.");

	check_result(data_transferred_count == 0, "No data was received.");

	printf("success.\nExample program successfully completed.\n\n");

	return 0;

}

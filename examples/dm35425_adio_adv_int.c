/**
    @file

    @brief
        Example program which demonstrates the use of the ADIO
        advanced interrupts.

    @verbatim

        This example requires a loopback of DIO0-DIO7 to DIO8-DIO15
        and DIO16-DIO23 to DIO24-DIO31.  This can most easily be accomplished
        using standard sized jumpers and placing them across the
        following pins of CN3 and CN4:

        Pin23 to Pin24
        Pin25 to Pin26
        Pin27 to Pin28
        Pin29 to Pin30
        Pin31 to Pin32
        Pin33 to Pin34
        Pin35 to Pin36
        Pin37 to Pin38

        The program demonstrates the DIO match and event interrupts.
        It will match on the value 0xAA, and when that value passes
        through the input pins, it will throw an interrupt.  It matches
        0xAA on the upper and lower 16-bit input values.

        The example will then wait for an event interrupt, which will occur
        if any input bit changes to zero.  This is accomplished by changing the
        digital outputs from 1s to 0s upon hitting Enter.  Because the outputs
        are tied to the inputs, the input will sense the bit change and
        generate an interrupt.  In this way, event interrupts can be
        tested without changing the loopback configuration.

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

    $Id: dm35425_adio_adv_int.c 108025 2017-04-14 15:09:34Z rgroner $
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
#include "dm35425_os.h"

/**
 * Constant defining the input and output direction
 * of the ADIO pins
 */
#define DM35425_ADIO_DIRECTION		0x00FF00FF

/**
 * Value to match on for the lower 8-bits.
 */
#define DM35425_ADIO_MATCH1		0x0000AA00

/**
 * Value to match on for the upper 8-bits
 */
#define DM35425_ADIO_MATCH2		0xAA000000

/**
 * Name of the program as invoked on the command line
 */
static char *program_name;

/**
 * Pointer to the board descriptor
 */
struct DM35425_Board_Descriptor *board;

/**
 * Function block for the ADIO
 */
struct DM35425_Function_Block my_adio;

/**
 * Total count of interrupts
 */
static volatile unsigned int total_interrupt_count = 0;

/**
 * Boolean indicating whether or not to exit the program.
 */
volatile int exit_program = 0;

/**
*******************************************************************************
@brief
    The interrupt subroutine that will execute when an ADIO interrupt occurs.
    This function will increment the count and clear the interrupt

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

		total_interrupt_count++;
		result = DM35425_Adio_Interrupt_Clear_Status(board,
							     &my_adio,
							     DM35425_ADIO_INT_ADV_INT_MASK);

		result = DM35425_Gbc_Ack_Interrupt(board);

		check_result(result, "Error calling ACK interrupt.");

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

	int help_option_given = 0;

	char keypress;
	int status;
	unsigned int interrupt_count = 0;
	uint32_t input_value;
	uint32_t output_value;
	struct sigaction signal_action;
	char message[200];
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
	printf("success.\nOpening DIO......");

	result = DM35425_Adio_Open(board, 0, &my_adio);

	check_result(result, "Could not open ADIO");

	printf("Found ADIO0\n");

	result = DM35425_Adio_Set_Direction(board,
					    &my_adio, DM35425_ADIO_DIRECTION);

	check_result(result, "Could not set direction of ADIO pins.");

	fprintf(stdout, "Installing user ISR .....");
	result = DM35425_General_InstallISR(board, ISR);
	check_result(result, "DM35425_General_InstallISR()");
	fprintf(stdout, "success.\n");

	result = DM35425_Adio_Interrupt_Set_Config(board,
						   &my_adio,
						   DM35425_ADIO_INT_ADV_INT_MASK,
						   0xFF);

	check_result(result, "Could not set interrupt enable.");

	/* Use the mask to hide the Output bits from being evaluated in the
	 * input value register
	 */
	result = DM35425_Adio_Set_Adv_Int_Mask(board,
					       &my_adio,
					       DM35425_ADIO_DIRECTION);

	check_result(result, "Error setting Advanced Interrupt Mask");

	result = DM35425_Adio_Set_Adv_Int_Comp(board,
					       &my_adio, DM35425_ADIO_MATCH1);

	check_result(result, "Error setting Advanced Interrupt Compare");

	result = DM35425_Adio_Set_Adv_Int_Mode(board,
					       &my_adio, DM35425_ADV_INT_MATCH);

	check_result(result, "Error setting Advanced Interrupt Mode to Match");

	interrupt_count = 0;

	printf
	    ("\n============== Testing MATCH Advanced Interrupt ============\n");
	printf
	    (" Testing values 0x00 to 0xFF, looking for match on 0xAA in lower word..\n");
	for (output_value = 0; output_value <= 0xFF; output_value++) {

		result = DM35425_Adio_Set_Output_Value(board,
						       &my_adio, output_value);

		check_result(result, "Could not set output value.");

		result = DM35425_Adio_Get_Input_Value(board,
						      &my_adio, &input_value);

		check_result(result, "Error getting input value");

		if ((input_value & ~DM35425_ADIO_DIRECTION) ==
		    DM35425_ADIO_MATCH1) {

			printf("\nOutput: 0x%8x\t\tInput: 0x%8x ***MATCH***\n",
			       output_value,
			       input_value & ~DM35425_ADIO_DIRECTION);
		}

		DM35425_Micro_Sleep(100);

		if (interrupt_count < total_interrupt_count) {
			printf("*** Interrupt received ***\n");
			interrupt_count++;
		}

	}

	result = DM35425_Adio_Set_Adv_Int_Comp(board,
					       &my_adio, DM35425_ADIO_MATCH2);

	check_result(result, "Error setting Advanced Interrupt Compare");

	printf
	    ("\n\nTesting values 0x00 to 0xFF, looking for match on 0xAA in upper word..\n\n");
	for (output_value = 0x00000; output_value <= 0xFF0000;
	     output_value += 0x10000) {

		result = DM35425_Adio_Set_Output_Value(board,
						       &my_adio, output_value);

		check_result(result, "Could not set output value.");

		result = DM35425_Adio_Get_Input_Value(board,
						      &my_adio, &input_value);

		check_result(result, "Error getting input value");

		if ((input_value & ~DM35425_ADIO_DIRECTION) ==
		    DM35425_ADIO_MATCH2) {

			printf("\nOutput: 0x%8x\t\tInput: 0x%8x ***MATCH***\n",
			       output_value,
			       input_value & ~DM35425_ADIO_DIRECTION);
		}

		DM35425_Micro_Sleep(100);

		if (interrupt_count < total_interrupt_count) {
			printf("*** Interrupt received ***\n");
			interrupt_count++;
		}
	}

	sprintf(message, "Expected 2 interrupts, but received %u.",
		total_interrupt_count);
	check_result(total_interrupt_count != 2, message);

	printf("\nMatching Advanced Interrupt test passed.\n\n");
	printf
	    ("\n============== Testing EVENT Advanced Interrupt ============\n");

	result = DM35425_Adio_Set_Adv_Int_Mode(board,
					       &my_adio,
					       DM35425_ADV_INT_DISABLED);

	check_result(result,
		     "Error setting Advanced Interrupt Mode to Disabled");

	result = DM35425_Adio_Set_Adv_Int_Capt(board, &my_adio, 0xFFFFFFFF);

	check_result(result,
		     "Error setting Advanced Interrupt Capture register");

	result = DM35425_Adio_Set_Adv_Int_Mask(board, &my_adio, 0x0);

	check_result(result, "Error setting Advanced Interrupt Mask register");

	result = DM35425_Adio_Set_Output_Value(board, &my_adio, 0xFFFFFFFF);

	check_result(result, "Could not set output value.");

	result = DM35425_Adio_Set_Adv_Int_Mode(board,
					       &my_adio, DM35425_ADV_INT_EVENT);

	check_result(result, "Error setting Advanced Interrupt Mode to Event");

	total_interrupt_count = 0;
	interrupt_count = 0;
	output_value = 0xFFFFFFFF;

	printf
	    ("Hit Enter to toggle an output bit, changing the input bit, and\n");
	printf("triggering an interrupt.\n\n");
	printf
	    ("Waiting for event....(Hit CTRL-C to stop, Enter to trigger event)\n");
	while (!exit_program && (interrupt_count < 5)) {

		keypress = getchar();

		/* This is mainly just to avoid the compiler warning */
		if (keypress == 'x') {
			exit_program = 1;
		} else {
			output_value ^= 0xAAAAAAAA;

			result = DM35425_Adio_Set_Output_Value(board,
							       &my_adio,
							       output_value);

			check_result(result, "Could not set output value.");

			DM35425_Micro_Sleep(10000);

			if (interrupt_count < total_interrupt_count) {

				result = DM35425_Adio_Get_Adv_Int_Capt(board,
								       &my_adio,
								       &input_value);

				check_result(result,
					     "Error getting Advanced Interrupt Capture register");

				printf
				    ("*** Event interrupt received *** (Capture: 0x%8x)\n",
				     input_value);
				interrupt_count++;
			}
			DM35425_Micro_Sleep(100);
		}

	}

	result = DM35425_Adio_Set_Adv_Int_Mode(board,
					       &my_adio,
					       DM35425_ADV_INT_DISABLED);

	check_result(result,
		     "Error setting Advanced Interrupt Mode to Disabled");

	result = DM35425_Adio_Interrupt_Set_Config(board,
						   &my_adio,
						   DM35425_ADIO_INT_ADV_INT_MASK,
						   0x00);

	check_result(result, "Could not set interrupt to disabled.");

	check_result(total_interrupt_count < 5,
		     "Was expecting 5 event interrupts (at least), but got less");

	printf("\nClosing Board\n");
	result = DM35425_Board_Close(board);

	check_result(result, "Error closing board.");
	printf("Example program successfully completed.\n");
	return 0;

}

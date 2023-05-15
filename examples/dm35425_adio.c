/**
    @file

    @brief
        Example program which demonstrates the use of the ADIO.

    @verbatim

        This example program sets 16-bits of DIO to output, and
        16-bits to input.  We'll connect the output to the input and
        then write every possible 16-bit value to the output
        and verify the same value on the input pins.

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

    $Id: dm35425_adio.c 108025 2017-04-14 15:09:34Z rgroner $
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>

#include "dm35425_gbc_library.h"
#include "dm35425_adio_library.h"
#include "dm35425_ioctl.h"
#include "dm35425_util_library.h"
#include "dm35425_examples.h"

/**
 * Constant defining the input and output direction
 * of the ADIO pins
 */
#define DM35425_ADIO_DIRECTION		0x00FF00FF

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

	int adio_num = 0;

	int help_option_given = 0;

	int status;

	uint32_t input_value;
	uint32_t output_value;

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

	printf("Opening board.....");
	result = DM35425_Board_Open(minor, &board);

	check_result(result, "Could not open board");
	printf("success.\nResetting board.....");
	result = DM35425_Gbc_Board_Reset(board);

	check_result(result, "Could not reset board");
	printf("success.\nOpening DIO......");

	result = DM35425_Adio_Open(board, 0, &my_adio);

	check_result(result, "Could not open ADIO");

	printf("Found ADIO%d\n", adio_num);

	/*
	 * Set the direction of bits 0-15 to output and bits 16-31 to input
	 */
	result = DM35425_Adio_Set_Direction(board,
					    &my_adio, DM35425_ADIO_DIRECTION);

	check_result(result, "Could not set direction of DIO pins.");

	output_value = 0;
	input_value = 0;

	for (output_value = 0; output_value <= 0xFF; output_value++) {

		result = DM35425_Adio_Set_Output_Value(board,
						       &my_adio, output_value);

		check_result(result, "Could not set output value.");

		result = DM35425_Adio_Get_Input_Value(board,
						      &my_adio, &input_value);

		input_value &= ~DM35425_ADIO_DIRECTION;
		input_value >>= 8;

		printf("Output: 0x%x\t\tInput: 0x%x\n", output_value,
		       input_value);

		check_result(!(output_value == input_value),
			     "Values do not match!");

	}

	for (output_value = 0x00000; output_value <= 0xFF0000;
	     output_value += 0x10000) {

		result = DM35425_Adio_Set_Output_Value(board,
						       &my_adio, output_value);

		check_result(result, "Could not set output value.");

		result = DM35425_Adio_Get_Input_Value(board,
						      &my_adio, &input_value);

		input_value &= ~DM35425_ADIO_DIRECTION;
		input_value >>= 8;

		printf("Output: 0x%x\t\tInput: 0x%x\n", output_value,
		       input_value);

		check_result(!(output_value == input_value),
			     "Values do not match!");

	}

	printf("\nOutput values matched input values.\n\n");
	printf("Closing Board\n");
	result = DM35425_Board_Close(board);

	check_result(result, "Error closing board.");
	printf("Example program successfully completed.\n");
	return 0;

}

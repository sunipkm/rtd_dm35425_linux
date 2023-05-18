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

#include "dm35425_adc_multiboard.h"

void ISR(int num_boards, float *** voltages, void *user_data)
{

}

int main()
{
    struct DM35425_ADCDMA_Descriptor *first_brd = NULL, *second_brd = NULL;
    DM35425_ADCDMA_Open(0, &first_brd);
    DM35425_ADCDMA_Open(1, &second_brd);
    DM35425_ADCDMA_Configure_ADC(first_brd, 10, 10, DM35425_ADC_2_FULL_SAMPLE_DELAY, DM35425_ADC_INPUT_SINGLE_ENDED, DM35425_ADC_RNG_BIPOLAR_5V);
    DM35425_ADCDMA_Configure_ADC(second_brd, 10, 10, DM35425_ADC_2_FULL_SAMPLE_DELAY, DM35425_ADC_INPUT_SINGLE_ENDED, DM35425_ADC_RNG_BIPOLAR_5V);
    DM35425_ADCDMA_Close(first_brd);
    DM35425_ADCDMA_Close(second_brd);
    return 0;
}
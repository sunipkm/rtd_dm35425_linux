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

/**
 * @brief Interrupt service routine for the ADCs. This function is called every time the ADCs have a full buffer of data.
 *
 * @param num_boards Number of boards
 * @param readouts Readout struct, containing number of channels, number of samples per channel and the float voltages
 * @param user_data Any user data
 */
void ISR(int num_boards, struct DM35425_ADCDMA_Readout *readouts, void *user_data)
{
    static int call_ct = 0;
    FILE **fp = (FILE **)user_data;
    if (num_boards <= 0)
    {
        printf("Error: ISR called with error %d\n", num_boards);
        return;
    }
    for (int i = 0; i < num_boards; i++)
    {
        for (int j = 0; j < readouts[i].num_channels; j++)
        {
            for (int k = 0; k < readouts[i].num_samples; k++)
            {
                fprintf(fp[i * DM35425_NUM_ADC_DMA_CHANNELS + j], "%u %f\n", (unsigned int)(call_ct * readouts[i].num_samples + k), readouts[i].voltages[j][k]);
            }
        }
    }
    call_ct++;
    printf("Callback called %d times\n", call_ct);
}

#define NUM_BOARDS 3

int main()
{
    // Open files for readout data
    FILE **fp = NULL;
    fp = malloc(sizeof(FILE *) * NUM_BOARDS * DM35425_NUM_ADC_DMA_CHANNELS);
    for (int i = 0; i < NUM_BOARDS; i++)
    {
        for (int j = 0; j < DM35425_NUM_ADC_DMA_CHANNELS; j++)
        {
            const char *fmt = "adc_%d.%d.dat";
            char fname[128];
            snprintf(fname, 128, fmt, i, j);
            fp[i * DM35425_NUM_ADC_DMA_CHANNELS + j] = fopen(fname, "w");
            if (fp[i * DM35425_NUM_ADC_DMA_CHANNELS + j] == NULL)
            {
                goto clean_files;
            }
        }
    }
    // Open the ADCs
    struct DM35425_ADCDMA_Descriptor *first_brd = NULL, *second_brd = NULL, *third_brd = NULL;
    DM35425_ADCDMA_Open(0, &first_brd);
    DM35425_ADCDMA_Open(1, &second_brd);
    DM35425_ADCDMA_Open(2, &third_brd);
    // Configure the ADCs
    DM35425_ADCDMA_Configure_ADC(first_brd, 10, 10,
                                 DM35425_ADC_2_FULL_SAMPLE_DELAY, DM35425_ADC_INPUT_SINGLE_ENDED, DM35425_ADC_RNG_BIPOLAR_5V);
    DM35425_ADCDMA_Configure_ADC(second_brd, 10, 10,
                                 DM35425_ADC_2_FULL_SAMPLE_DELAY, DM35425_ADC_INPUT_DIFFERENTIAL, DM35425_ADC_RNG_BIPOLAR_5V);
    DM35425_ADCDMA_Configure_ADC(third_brd, 10, 10,
                                 DM35425_ADC_2_FULL_SAMPLE_DELAY, DM35425_ADC_INPUT_SINGLE_ENDED, DM35425_ADC_RNG_BIPOLAR_5V);
    // Combine the boards
    struct DM35425_Multiboard_Descriptor *mbd = NULL;
    DM35425_ADC_Multiboard_Init(&mbd, NUM_BOARDS, first_brd, second_brd, third_brd);
    // Install the interrupt service routine and do not block
    DM35425_ADC_Multiboard_InstallISR(mbd, ISR, (void *)fp, false);
    // Wait for things to happen
    sleep(5);
    // Remove the ISR
    DM35425_ADC_Multiboard_RemoveISR(mbd);
    // Destroy the combined boards
    DM35425_ADC_Multiboard_Destroy(mbd);
    // Close the individual boards
    DM35425_ADCDMA_Close(first_brd);
    DM35425_ADCDMA_Close(second_brd);
    DM35425_ADCDMA_Close(third_brd);

    // Close data files
    for (int i = 0; i < NUM_BOARDS; i++)
    {
        for (int j = 0; j < DM35425_NUM_ADC_DMA_CHANNELS; j++)
        {
            fclose(fp[i * DM35425_NUM_ADC_DMA_CHANNELS + j]);
        }
    }

clean_files:
    free(fp);
    return 0;
}
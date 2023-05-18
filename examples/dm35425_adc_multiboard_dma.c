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

void ISR(int num_boards, struct DM35425_ADCDMA_Readout *readouts, void *user_data)
{
    static int call_ct = 0;
    FILE **fp = (FILE **) user_data;
    for (int i = 0; i < num_boards; i++)
    {
        for (int j = 0; j < readouts[i].num_channels; j++)
        {
            for (int k = 0; k < readouts[i].num_samples; k++)
            {
                fprintf(fp[i * DM35425_NUM_ADC_DMA_CHANNELS + j], "%u %f\n", (unsigned int) (call_ct * readouts[i].num_samples + k), readouts[i].voltages[j][k]);
            }
        }
    }
    call_ct++;
}

int main()
{
    FILE **fp = NULL;
    fp = malloc(sizeof(FILE *) * 2 * DM35425_NUM_ADC_DMA_CHANNELS);
    for (int i = 0; i < 2; i++)
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
    struct DM35425_ADCDMA_Descriptor *first_brd = NULL, *second_brd = NULL;
    DM35425_ADCDMA_Open(0, &first_brd);
    DM35425_ADCDMA_Open(1, &second_brd);
    DM35425_ADCDMA_Configure_ADC(first_brd, 10, 10, DM35425_ADC_2_FULL_SAMPLE_DELAY, DM35425_ADC_INPUT_SINGLE_ENDED, DM35425_ADC_RNG_BIPOLAR_5V);
    DM35425_ADCDMA_Configure_ADC(second_brd, 10, 10, DM35425_ADC_2_FULL_SAMPLE_DELAY, DM35425_ADC_INPUT_DIFFERENTIAL, DM35425_ADC_RNG_BIPOLAR_5V);
    struct DM35425_Multiboard_Descriptor *mbd = NULL;
    DM35425_ADC_Multiboard_Init(&mbd, 2, first_brd, second_brd);
    DM35425_ADC_Multiboard_InstallISR(mbd, ISR, (void *) fp, false);
    sleep(5);
    DM35425_ADC_Multiboard_RemoveISR(mbd);
    DM35425_ADC_Multiboard_Destroy(mbd);
    DM35425_ADCDMA_Close(first_brd);
    DM35425_ADCDMA_Close(second_brd);

    for (int i = 0; i < 2; i++)
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
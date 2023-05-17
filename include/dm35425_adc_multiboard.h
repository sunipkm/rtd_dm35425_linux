/**
 * @file dm35425_multiboard.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Function declarations for the DM35425 that are Linux specific and
 * provides single ISR functionality for ADCs on multiple boards.
 * @version 0.1
 * @date 2023-05-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _DM35425_BOARD_ADC_MULTIBOARD__H_
#define _DM35425_BOARD_ADC_MULTIBOARD__H_

#include <stdarg.h>
#include <signal.h>
#include <pthread.h>
#include "dm35425.h"
#include "dm35425_board_access.h"
#include "dm35425_adc_library.h"

#ifndef _Nullable
#define _Nullable
#endif

#ifndef _Nonnull
#define _Nonnull
#endif

#ifndef MULTIBRD_DBG_LVL
#define MULTI_BRD_DBG_LVL 3
#endif

typedef void (*DM35425_Multiboard_ISR)(int, struct DM35425_Board_Descriptor **, void *);

/**
 * @brief ADC DMA Descriptor (combines all necessary structures and fields to interact with ADC channels in one structure.)
 * 
 */
struct DM35425_ADCDMA_Descriptor 
{
    struct DM35425_Board_Descriptor *board; // board descriptor
    struct DM35425_Function_Block *fb; // ADC function block
    size_t buf_sz; // buffer size in bytes
    size_t buf_ct; // buffer count
    int next_buf; // next buffer index
    int **local_buf[DM35425_NUM_ADC_DMA_CHANNELS]; // local buffer
    enum DM35425_Channel_Delay delay;
    enum DM35425_Input_Mode input_mode;
};

int DM35425_ADCDMA_Open(int minor, struct DM35425_ADCDMA_Descriptor ** _Nonnull handle);

int DM35425_ADCDMA_Close(struct DM35425_ADCDMA_Descriptor * _Nonnull handle);

int DM35425_ADCDMA_Configure_ADC(struct DM35425_ADCDMA_Descriptor * _Nonnull handle, uint32_t rate, size_t samples_per_buf, enum DM35425_Channel_Delay delay, enum DM35425_Input_Mode input_mode, enum DM35425_Input_Ranges range);

struct DM35425_Multiboard_Descriptor {
    struct DM35425_ADCDMA_Descriptor **boards;
    int num_boards;
    pthread_t pid;
    DM35425_Multiboard_ISR isr;
    volatile sig_atomic_t done;
    unsigned int timeout_sec;
};

int DM35425_ADC_Multiboard_Init(struct DM35425_Multiboard_Descriptor ** _Nonnull mbd, int num_boards, struct DM35425_ADCDMA_Descriptor * _Nonnull first_board, ...);

int DM35425_ADC_Multiboard_Destroy(struct DM35425_Multiboard_Descriptor * _Nonnull mbd);

#endif // _DM35425_BOARD_ADC_MULTIBOARD__H_
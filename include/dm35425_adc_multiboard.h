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
#include <stdint.h>
#include <signal.h>
#include <stdbool.h>
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
#define MULTIBRD_DBG_LVL 3
#endif

typedef void (*DM35425_Multiboard_ISR)(int, float *** _Nullable, void * _Nullable);

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
    int num_samples_taken[DM35425_NUM_ADC_DMA_CHANNELS]; // number of samples taken
    uint32_t rate; // sampling rate
    uint32_t actual_rate; // set sampling rate
    bool started; 
    enum DM35425_Channel_Delay delay;
    enum DM35425_Input_Mode input_mode;
    enum DM35425_Input_Ranges range;
};

int DM35425_ADCDMA_Open(int minor, struct DM35425_ADCDMA_Descriptor ** _Nonnull handle);

int DM35425_ADCDMA_Close(struct DM35425_ADCDMA_Descriptor * _Nonnull handle);

int DM35425_ADCDMA_Configure_ADC(struct DM35425_ADCDMA_Descriptor * _Nonnull handle, uint32_t rate, size_t samples_per_buf, enum DM35425_Channel_Delay delay, enum DM35425_Input_Mode input_mode, enum DM35425_Input_Ranges range);

struct DM35425_Multiboard_Descriptor {
    volatile sig_atomic_t done;
    int num_boards;
    DM35425_Multiboard_ISR isr;
    void *user_data;
    struct DM35425_ADCDMA_Descriptor **boards;
    pthread_t pid;
};

int DM35425_ADC_Multiboard_Init(struct DM35425_Multiboard_Descriptor ** _Nonnull mbd, int num_boards, struct DM35425_ADCDMA_Descriptor * _Nonnull first_board, ...);

int DM35425_ADC_Multiboard_Destroy(struct DM35425_Multiboard_Descriptor * _Nonnull mbd);

int DM35425_ADC_Multiboard_InstallISR(struct DM35425_Multiboard_Descriptor * _Nonnull mbd, DM35425_Multiboard_ISR isr, void * _Nullable user_data, bool block);

int DM35425_ADC_Multiboard_RemoveISR(struct DM35425_Multiboard_Descriptor * _Nonnull mbd);

enum DM35425_ERROR {
    DM35425_SUCCESS = 0,
    DM35425_ERROR_FIND_USED_BUFFER,
    DM35425_ERROR_BUFFER_NOT_FULL,
    DM35425_ERROR_CHECK_DMA_ERROR,
    DM35425_ERROR_CHANNEL_DMA_ERROR,
    DM35425_ERROR_READ_DMA_BUFFER,
    DM35425_ERROR_RESET_DMA_BUFFER,
    DM35425_ERROR_CLEAR_DMA_INTERRUPT,
    DM35425_ERROR_ACK_INTERRUPT,
    DM35425_ERROR_DMA_READ,
    DM35425_ERROR_IRQ_GET,
    DM35425_INVALID_IRQ_FD_UNREADABLE,
    DM35425_INVALID_IRQ_IO,
    DM35425_INVALID_IRQ_TIMEOUT,
    DM35425_INVALID_IRQ_SELECT,
};

#endif // _DM35425_BOARD_ADC_MULTIBOARD__H_
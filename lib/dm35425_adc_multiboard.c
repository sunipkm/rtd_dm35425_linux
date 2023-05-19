/**
 * @file dm35425_adc_multiboard.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Implementation for the Multi-board ADC driver for the RTD DM35425 boards.
 * @version 1.0
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "dm35425_adc_multiboard.h"
#include "dm35425_board_access.h"
#include "dm35425_ioctl.h"
#include "dm35425_gbc_library.h"
#include "dm35425_adc_library.h"
#include "dm35425_dma_library.h"

#define YELLOW_FG "\033[33m"
#define RED_FG "\033[31m"
#define RESET "\033[0m"

#if (MULTIBRD_DBG_LVL >= 3)
#define MULTIBRD_DBG_INFO(fmt, ...)                                                            \
    {                                                                                          \
        fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                        \
    }
#define MULTIBRD_DBG_INFO_NONL(fmt, ...)                                                  \
    {                                                                                     \
        fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                   \
    }
#define MULTIBRD_DBG_INFO_NONE(fmt, ...)     \
    {                                        \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fflush(stderr);                      \
    }
#else
#define MULTIBRD_DBG_INFO(fmt, ...)
#define MULTIBRD_DBG_INFO_NONL(fmt, ...)
#define MULTIBRD_DBG_INFO_NONE(fmt, ...)
#endif

#if (MULTIBRD_DBG_LVL >= 2)
#define MULTIBRD_DBG_WARN(fmt, ...)                                                                            \
    {                                                                                                          \
        fprintf(stderr, "%s:%d:%s(): " YELLOW_FG fmt "\n" RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                        \
    }
#define MULTIBRD_DBG_WARN_NONL(fmt, ...)                                                  \
    {                                                                                     \
        fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                   \
    }
#define MULTIBRD_DBG_WARN_NONE(fmt, ...)     \
    {                                        \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fflush(stderr);                      \
    }
#else
#define MULTIBRD_DBG_WARN(fmt, ...)
#define MULTIBRD_DBG_WARN_NONL(fmt, ...)
#define MULTIBRD_DBG_WARN_NONE(fmt, ...)
#endif

#if (MULTIBRD_DBG_LVL >= 1)
#define MULTIBRD_DBG_ERR(fmt, ...)                                                                          \
    {                                                                                                       \
        fprintf(stderr, "%s:%d:%s(): " RED_FG fmt "\n" RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                     \
    }
#else
#define MULTIBRD_DBG_ERR(fmt, ...)
#endif

/**
 * @fn timespec_diff(struct timespec *, struct timespec *, struct timespec *)
 * @brief Compute the diff of two timespecs, that is a - b = result.
 * @param a the minuend
 * @param b the subtrahend
 * @param result a - b
 */
static inline void timespec_diff(struct timespec *a, struct timespec *b,
                                 struct timespec *result)
{
    result->tv_sec = a->tv_sec - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0)
    {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}

struct _DM35425_ADCDMA_Descriptor
{
    struct DM35425_Board_Descriptor *board;              // board descriptor
    struct DM35425_Function_Block *fb;                   // ADC function block
    size_t buf_sz;                                       // buffer size in bytes
    size_t buf_ct;                                       // buffer count
    int next_buf;                                        // next buffer index
    int **local_buf[DM35425_NUM_ADC_DMA_CHANNELS];       // local buffer
    int num_samples_taken[DM35425_NUM_ADC_DMA_CHANNELS]; // number of samples taken
    uint32_t rate;                                       // sampling rate
    uint32_t actual_rate;                                // set sampling rate
    bool started;                                        // acquisition started
    enum DM35425_Channel_Delay delay;                    // channel delay
    enum DM35425_Input_Mode input_mode;                  // input mode
    enum DM35425_Input_Ranges range;                     // input range
};

struct _DM35425_Multiboard_Descriptor
{
    volatile sig_atomic_t done;              // flag to indicate thread is done
    int num_boards;                          // number of boards
    DM35425_Multiboard_ISR isr;              // ISR function
    void *user_data;                         // user data
    DM35425_ADCDMA_Descriptor **boards;      // array of board descriptors
    struct DM35425_ADCDMA_Readout *readouts; // array of readouts
    pthread_t pid;                           // thread id
};

/**
 * @brief Convert ADC values to voltages
 *
 * @param handle Handle to ADCDMA device
 * @param voltages Pointer to float array to store voltages
 */
static void DM35425_Convert_ADC(DM35425_ADCDMA_Descriptor *_Nonnull handle, float **_Nonnull voltages);

/**
 * @brief Read out ADC raw values
 *
 * @param handle Handle to ADCDMA device
 * @param int_info Interrupt info from ioctl.
 * @return int 0 on success, negative on error.
 */
static int DM35425_Read_Out_ADC(DM35425_ADCDMA_Descriptor *_Nonnull handle, struct dm35425_ioctl_interrupt_info_request int_info);

/**
 * @brief The thread function for the multiboard ISR
 *
 * @param ptr Pointer to the multiboard descriptor and user data.
 * @return void* 0 on success, 1 on failure.
 */
static void *DM35425_Multiboard_WaitForIRQ(void *ptr);

#define ADC_0 0              /*!< ADC 0 */
#define DAC_0 0              /*!< DAC 0 */
#define NOT_IGNORE_USED 0    /*!< Do not ignore used channels */
#define INTERRUPT_DISABLE 0  /*!< Disable interrupt */
#define ERROR_INTR_DISABLE 0 /*!< Disable error interrupt */
#define INTERRUPT_ENABLE 1   /*!< Enable interrupt */
#define ERROR_INTR_ENABLE 1  /*!< Enable error interrupt */
#define CHANNEL_0 0          /*!< Channel 0 */
#define NO_CLEAR_INTERRUPT 0 /*!< Do not clear interrupt */
#define CLEAR_INTERRUPT 1    /*!< Clear interrupt */

int DM35425_ADCDMA_Open(int minor, DM35425_ADCDMA_Descriptor **handle_)
{
    DM35425_ADCDMA_Descriptor *handle = (DM35425_ADCDMA_Descriptor *)malloc(sizeof(DM35425_ADCDMA_Descriptor));
    if (handle == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    // open board
    int result = DM35425_Board_Open(minor, &(handle->board));
    if (result != 0)
    {
        MULTIBRD_DBG_ERR("Failed to open board");
        goto free_handle;
    }
    // reset board
    result = DM35425_Gbc_Board_Reset(handle->board);
    if (result != 0)
    {
        MULTIBRD_DBG_ERR("Failed to reset board");
        goto free_board;
    }
    // malloc memory for ADC function block
    handle->fb = (struct DM35425_Function_Block *)malloc(sizeof(struct DM35425_Function_Block));
    if (handle->fb == NULL)
    {
        errno = ENOMEM;
        MULTIBRD_DBG_ERR("Failed to malloc memory for ADC function block");
        goto free_board;
    }
    // Open ADC
    result = DM35425_Adc_Open(handle->board, ADC_0, handle->fb);
    if (result != 0)
    {
        MULTIBRD_DBG_ERR("Failed to open ADC");
        goto free_fb;
    }
    // Set ADC Clock Source
    result = DM35425_Adc_Set_Clock_Src(handle->board, handle->fb, DM35425_CLK_SRC_IMMEDIATE);
    if (result != 0)
    {
        MULTIBRD_DBG_ERR("Failed to set ADC clock source");
        goto free_fb;
    }
    // All good, return the handle
    handle->buf_ct = 0;
    *handle_ = handle;
    return 0;
free_fb:
    free(handle->fb);
free_board:
    DM35425_Board_Close(handle->board);
free_handle:
    free(handle);
    return -1;
}

int DM35425_ADCDMA_Close(DM35425_ADCDMA_Descriptor *handle)
{
    if (handle == NULL)
    {
        errno = ENODATA;
        return -1;
    }
    // Close board
    DM35425_Board_Close(handle->board);
    // Free local buffers
    if (handle->local_buf != NULL)
    {
        for (int channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++)
        {
            if (handle->local_buf[channel] != NULL)
            {
                for (int buff = 0; buff < handle->fb->num_dma_buffers; buff++)
                {
                    if (handle->local_buf[channel][buff] != NULL)
                    {
                        free(handle->local_buf[channel][buff]);
                    }
                }
                free(handle->local_buf[channel]);
            }
        }
    }
    // Free ADC function block
    free(handle->fb);
    // Free handle
    free(handle);
    return 0;
}

int DM35425_ADCDMA_Configure_ADC(DM35425_ADCDMA_Descriptor *handle, uint32_t rate, size_t samples_per_buf, enum DM35425_Channel_Delay delay, enum DM35425_Input_Mode input_mode, enum DM35425_Input_Ranges range)
{
    if (handle == NULL)
    {
        MULTIBRD_DBG_ERR("Handle is NULL");
        errno = ENODATA;
        return -1;
    }
    if (rate < 1 || rate > DM35425_ADC_MAX_RATE)
    {
        MULTIBRD_DBG_INFO("Invalid rate %u, acceptable range is 1 to %u", rate, DM35425_ADC_MAX_RATE);
        errno = EINVAL;
        return -1;
    }
    if (samples_per_buf < 1 || samples_per_buf > DM35425_DMA_MAX_BUFFER_SIZE)
    {
        MULTIBRD_DBG_INFO("Invalid samples_per_buf %lu, acceptable range is 1 to %d", samples_per_buf, DM35425_DMA_MAX_BUFFER_SIZE);
        errno = EINVAL;
        return -1;
    }
    size_t buf_sz = samples_per_buf * sizeof(int32_t);
    int channel = 0, result;
    struct DM35425_Board_Descriptor *board = handle->board;
    struct DM35425_Function_Block *fb = handle->fb;
    for (channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++) // for each channel, TODO: Check if channel number depends on input mode
    {
        result = DM35425_Dma_Initialize(board, fb, channel, fb->num_dma_buffers, buf_sz); // initialize DMA for channel
        if (result != 0)
        {
            MULTIBRD_DBG_ERR("Failed to initialize DMA for channel %d", channel);
            return result;
        }
        result = DM35425_Dma_Setup(board, fb, channel, DM35425_DMA_SETUP_DIRECTION_READ, NOT_IGNORE_USED); // setup DMA for channel
        if (result != 0)
        {
            MULTIBRD_DBG_ERR("Failed to setup DMA for channel %d", channel);
            return result;
        }
        result = DM35425_Dma_Configure_Interrupts(board, fb, channel, INTERRUPT_DISABLE, ERROR_INTR_DISABLE); // enable DMA interrupts for channel
        if (result != 0)
        {
            MULTIBRD_DBG_ERR("Failed to disable DMA interrupts for channel %d", channel);
            return result;
        }
        for (int buff = 0; buff < fb->num_dma_buffers; buff++) // for every buffer
        {
            uint8_t buff_stat, buff_ctrl;
            uint32_t buff_sz;
            buff_ctrl = DM35425_DMA_BUFFER_CTRL_VALID | DM35425_DMA_BUFFER_CTRL_INTR;

            if (buff == (DM35425_NUM_ADC_DMA_BUFFERS - 1)) // if last buffer. TODO: Check how this scales with double-ended input
            {
                buff_ctrl |= DM35425_DMA_BUFFER_CTRL_LOOP;
            }

            result = DM35425_Dma_Buffer_Setup(board, fb, channel, buff, buff_ctrl);
            if (result != 0)
            {
                MULTIBRD_DBG_ERR("Failed to setup DMA buffer %d for channel %d", buff, channel);
                return result;
            }

            result = DM35425_Dma_Buffer_Status(board, fb, channel, buff, &buff_stat, &buff_ctrl, &buff_sz);
            if (result != 0)
            {
                MULTIBRD_DBG_ERR("Failed to get DMA buffer %d status for channel %d", buff, channel);
                return result;
            }
            else
            {
                MULTIBRD_DBG_INFO("Board (%p) DMA buffer %d status for channel %d: buff_stat = 0x%x, buff_ctrl = 0x%x, buff_sz = %d", board, buff, channel, buff_stat, buff_ctrl, buff_sz);
            }
        }
        result = DM35425_Adc_Channel_Setup(board, fb, channel, delay, input_mode, range); // setup ADC channel
        if (result != 0)
        {
            MULTIBRD_DBG_ERR("Failed to setup ADC channel %d with delay = %d, input mode = %d, range = %d", channel, delay, input_mode, range);
            return result;
        }
    }

    result = DM35425_Dma_Configure_Interrupts(board, fb, CHANNEL_0, INTERRUPT_ENABLE, ERROR_INTR_ENABLE); // enable DMA interrupts for channel
    if (result != 0)
    {
        MULTIBRD_DBG_ERR("Failed to enable DMA interrupts for channel %d", CHANNEL_0);
        return result;
    }
    // Now we can allocate memory for the local buffers
    for (channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++) // this is a static allocation and will not depend on channel input mode
    {
        int **local_buf = (int **)malloc(sizeof(int *) * fb->num_dma_buffers);
        if (local_buf == NULL)
        {
            MULTIBRD_DBG_ERR("Failed to allocate memory for local buffer for channel %d", channel);
            errno = ENOMEM;
            return -1;
        }
        for (int buff = 0; buff < fb->num_dma_buffers; buff++)
        {
            local_buf[buff] = (int *)malloc(buf_sz);
            if (local_buf[buff] == NULL)
            {
                MULTIBRD_DBG_ERR("Failed to allocate memory for local buffer %d for channel %d", buff, channel);
                errno = ENOMEM;
                return -1;
            }
        }
        handle->local_buf[channel] = local_buf;
    }
    handle->buf_sz = buf_sz;
    handle->delay = delay;
    handle->input_mode = input_mode;
    handle->range = range;
    handle->rate = rate;
    handle->buf_ct = samples_per_buf;
    return 0;
}

int DM35425_ADC_Multiboard_Init(DM35425_Multiboard_Descriptor **_mbd, int num_boards, DM35425_ADCDMA_Descriptor *first_board, ...)
{
    if (first_board == NULL)
    {
        errno = ENODATA;
        return -1;
    }
    if (num_boards < 1)
    {
        errno = ENODATA;
        return -1;
    }

    va_list args;
    va_start(args, first_board);

    DM35425_Multiboard_Descriptor *mbd = (DM35425_Multiboard_Descriptor *)malloc(sizeof(DM35425_Multiboard_Descriptor));
    if (mbd == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    memset(mbd, 0x00, sizeof(DM35425_Multiboard_Descriptor)); // zero out memory

    DM35425_ADCDMA_Descriptor **boards = (DM35425_ADCDMA_Descriptor **)malloc(sizeof(DM35425_ADCDMA_Descriptor *) * num_boards); // allocate num_boards pointers to board descriptors

    if (boards == NULL)
    {
        errno = ENOMEM;
        goto errored;
    }

    memset(boards, 0x00, sizeof(DM35425_ADCDMA_Descriptor *) * num_boards); // zero out memory

    struct DM35425_ADCDMA_Readout *readouts = (struct DM35425_ADCDMA_Readout *)malloc(sizeof(struct DM35425_ADCDMA_Readout) * num_boards); // allocate num_boards readout structs

    if (readouts == NULL)
    {
        errno = ENOMEM;
        goto errored_boards;
    }

    boards[0] = first_board;
    for (int i = 1; i < num_boards; i++) // itreate over and copy ptrs to each board descriptor
    {
        DM35425_ADCDMA_Descriptor *board = va_arg(args, DM35425_ADCDMA_Descriptor *);
        if (board == NULL)
        {
            errno = ENODATA;
            va_end(args);
            goto errored_boards;
        }
        boards[i] = board;
    }

    va_end(args);

    mbd->boards = boards;         // copy over boards
    mbd->num_boards = num_boards; // copy over num_boards
    mbd->readouts = readouts;     // copy over readouts
    *_mbd = mbd;

    return 0;

errored_boards:
    free(boards);
errored:
    free(mbd);
    *_mbd = NULL;
    return -1;
}

int DM35425_ADC_Multiboard_Destroy(DM35425_Multiboard_Descriptor *mbd)
{
    int status = DM35425_ADC_Multiboard_RemoveISR(mbd);

    if (status != 0)
    {
        MULTIBRD_DBG_ERR("Failed to remove ISR");
        return status;
    }

    free(mbd->boards);
    free(mbd);
    return 0;
}

int DM35425_ADC_Multiboard_RemoveISR(DM35425_Multiboard_Descriptor *mbd)
{
    if (mbd == NULL)
    {
        errno = ENODATA;
        return -1;
    }
    mbd->isr = NULL; // set ISR to NULL
    mbd->done = 1;   // set done flag

    for (int i = 0; i < mbd->num_boards; i++) // Disable multiboard ISR
    {
        ioctl(mbd->boards[i]->board->file_descriptor, DM35425_IOCTL_WAKEUP); // wake up ISR
    }

    if (mbd->pid) // pthread was not joined already
    {
        MULTIBRD_DBG_INFO("Joining thread for multiboard ISR: %p", (void *)mbd->pid);
        int rc = pthread_join(mbd->pid, NULL);
        if (rc != 0)
        {
            MULTIBRD_DBG_ERR("Failed to join thread for multiboard ISR: %d", rc);
            errno = rc;
            return -1;
        }
    }
    mbd->pid = 0; // set pid to 0 if called again
    return 0;
}

static void *_start_adc_fcn(void *arg)
{
    DM35425_ADCDMA_Descriptor *adc = (DM35425_ADCDMA_Descriptor *)arg;
    struct DM35425_Board_Descriptor *board = adc->board;
    struct DM35425_Function_Block *fb = adc->fb;
    adc->started = false;

    int result = DM35425_Adc_Initialize(board, fb);
    if (result != 0)
    {
        MULTIBRD_DBG_ERR("Board %p: Failed to initialize ADC.", board);
        return (void *)1;
    }

    result = DM35425_Adc_Start(board, fb);
    if (result != 0)
    {
        MULTIBRD_DBG_ERR("Board %p: Failed to start ADC.", board);
        return (void *)1;
    }
    adc->started = true;
    return NULL;
}

int DM35425_ADC_Multiboard_InstallISR(DM35425_Multiboard_Descriptor *mbd, DM35425_Multiboard_ISR isr, void *user_data, bool block)
{
    if (mbd == NULL)
    {
        errno = ENODATA;
        return -1;
    }
    // Check if ISR is already installed
    if (mbd->isr != NULL)
    {
        MULTIBRD_DBG_ERR("ISR already installed");
        errno = EEXIST;
        return -1;
    }
    mbd->isr = isr;
    mbd->user_data = user_data;

    pthread_t *trig_thr = (pthread_t *)malloc(sizeof(pthread_t) * mbd->num_boards);
    if (trig_thr == NULL)
    {
        MULTIBRD_DBG_ERR("Failed to allocate memory for trigger threads");
        return -1;
    }

    pthread_t pid = 0;
    int rc = pthread_create(&pid, NULL, DM35425_Multiboard_WaitForIRQ, (void *)mbd);
    if (rc != 0)
    {
        MULTIBRD_DBG_ERR("Failed to create thread for multiboard ISR");
        errno = rc;
        goto clean_pthread;
    }
    mbd->pid = pid;
    MULTIBRD_DBG_INFO("Created thread for multiboard ISR: %p", (void *)mbd->pid);
    // Here we need to start the ADCs etc
    for (int idx = 0; idx < mbd->num_boards; idx++)
    {
        DM35425_ADCDMA_Descriptor *handle = mbd->boards[idx];
        struct DM35425_Board_Descriptor *board = mbd->boards[idx]->board;
        struct DM35425_Function_Block *fb = mbd->boards[idx]->fb;
        int result = 0;

        for (int channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++)
        {
            result = DM35425_Dma_Start(board, fb, channel);
            if (result != 0)
            {
                MULTIBRD_DBG_ERR("Failed to start DMA for board %d (%p) channel %d", idx, board, channel);
                goto errored;
            }
            handle->num_samples_taken[channel] = 0;
            MULTIBRD_DBG_INFO("Started DMA for board %d (%p) channel %d", idx, board, channel);
        }

        result = DM35425_Adc_Set_Start_Trigger(board, fb, DM35425_CLK_SRC_IMMEDIATE);
        if (result != 0)
        {
            MULTIBRD_DBG_ERR("Failed to set start trigger for board %d (%p)", idx, board);
            goto errored;
        }

        result = DM35425_Adc_Set_Stop_Trigger(board, fb, DM35425_CLK_SRC_NEVER);
        if (result != 0)
        {
            MULTIBRD_DBG_ERR("Failed to set stop trigger for board %d (%p)", idx, board);
            goto errored;
        }

        result = DM35425_Adc_Set_Sample_Rate(board, fb, handle->rate, &(handle->actual_rate));
        if (result != 0)
        {
            MULTIBRD_DBG_ERR("Failed to set sample rate for board %d (%p)", idx, board);
            goto errored;
        }
        MULTIBRD_DBG_INFO("Board %d (%p): Requested rate %u, achieved %u.", idx, board, handle->rate, handle->actual_rate);

        // start threads to trigger the ADCs
        rc = pthread_create(&(trig_thr[idx]), NULL, _start_adc_fcn, (void *)handle);
        if (rc != 0)
        {
            MULTIBRD_DBG_ERR("Failed to trigger ADC start for board %d (%p)", idx, board);
            errno = rc;
            goto errored;
        }
    }

    // if we are here, ADC start was triggered
    bool started = true;
    for (int idx = 0; idx < mbd->num_boards; idx++)
    {
        void *res = NULL;
        pthread_join(trig_thr[idx], (void **)&res);
        if (res != 0)
        {
            MULTIBRD_DBG_ERR("Failed to start ADC for board %d (%p)", idx, mbd->boards[idx]->board);
            started = false;
        }
    }

    if (!started)
    {
        MULTIBRD_DBG_ERR("Failed to start ADCs");
        goto errored;
    }
    // If we are here, ADCs are started. Clean up memory.
    free(trig_thr);

    if (block)
    {
        rc = pthread_join(mbd->pid, NULL);
        if (rc != 0)
        {
            MULTIBRD_DBG_ERR("Failed to join thread for multiboard ISR");
            errno = rc;
            return -1;
        }
        mbd->pid = 0;
    }
    return 0;

errored:
    for (int idx = 0; idx < mbd->num_boards; idx++)
    {
        struct DM35425_Board_Descriptor *board = mbd->boards[idx]->board;
        ioctl(board->file_descriptor, DM35425_IOCTL_WAKEUP);
    }
    pthread_join(mbd->pid, NULL);
clean_pthread:
    free(trig_thr);
    return -1;
}

static void *DM35425_Multiboard_WaitForIRQ(void *ptr)
{
    /*
     * Set up
     */
    fd_set exception_fds;
    fd_set read_fds;
    int status;
    DM35425_Multiboard_Descriptor *mbd = ptr;
    if (mbd == NULL)
    {
        MULTIBRD_DBG_ERR("Invalid multiboard descriptor");
        errno = EINVAL;
        return NULL;
    }
    void *user_data = mbd->user_data;
    int num_boards = mbd->num_boards;

#if MULTIBRD_DBG_LVL >= 3
    static int isr_call_count = 0;
    static struct timespec start;
    struct timespec lstart = {0, 0};
    struct timespec now;
    struct timespec delta;
#endif // MULTIBRD_DBG_LVL >= 3

    int *irqs = (int *)malloc(sizeof(int) * num_boards);
    if (irqs == NULL)
    {
        MULTIBRD_DBG_ERR("Failed to allocate memory for irqs");
        errno = ENOMEM;
        return NULL;
    }

    for (int i = 0; i < num_boards; i++)
    {
        irqs[i] = 0; // interrupt has not triggered yet
    }

    float ***voltages = (float ***)malloc(sizeof(float *) * num_boards);
    if (voltages == NULL)
    {
        MULTIBRD_DBG_ERR("Failed to allocate memory for voltages");
        errno = ENOMEM;
        return NULL;
    }
    for (int i = 0; i < num_boards; i++)
    {
        voltages[i] = (float **)malloc(sizeof(float *) * DM35425_NUM_ADC_DMA_CHANNELS);
        if (voltages[i] == NULL)
        {
            MULTIBRD_DBG_ERR("Failed to allocate memory for voltages[%d]", i);
            errno = ENOMEM;
            return NULL;
        }
        for (int j = 0; j < DM35425_NUM_ADC_DMA_CHANNELS; j++)
        {
            voltages[i][j] = (float *)malloc(sizeof(float) * (mbd->boards[i]->buf_sz / sizeof(int)));
            if (voltages[i][j] == NULL)
            {
                MULTIBRD_DBG_ERR("Failed to allocate memory for voltages[%d][%d]", i, j);
                errno = ENOMEM;
                return NULL;
            }
            memset(voltages[i][j], 0, sizeof(float) * (mbd->boards[i]->buf_sz / sizeof(int))); // zero out the memory
        }
        mbd->readouts[i].num_channels = DM35425_NUM_ADC_DMA_CHANNELS; // TODO: Change to actual number of channels
        mbd->readouts[i].num_samples = mbd->boards[i]->buf_ct;
        mbd->readouts[i].voltages = voltages[i];
    }

    /* Main event loop */
    while (!mbd->done)
    {
        bool no_error = true; // assume no error
        int avail_irq = 0;    // assume no available IRQs
        union dm35425_ioctl_argument ioctl_arg;

#if MULTIBRD_DBG_LVL >= 3
        clock_gettime(CLOCK_MONOTONIC, &start);
        if (!isr_call_count)
        {
            lstart = start;
        }
        else
        {
            clock_gettime(CLOCK_MONOTONIC, &now);
            timespec_diff(&start, &lstart, &delta);
            MULTIBRD_DBG_INFO_NONE("\n");
            MULTIBRD_DBG_INFO("DMA waiting to select (%d): %ld.%09ld s (since first)\n", isr_call_count, delta.tv_sec, delta.tv_nsec);
        }
        MULTIBRD_DBG_INFO_NONL("IRQ Status:")
        for (int i = 0; i < num_boards; i++)
        {
            MULTIBRD_DBG_INFO_NONE(" %d", irqs[i]);
        }
        MULTIBRD_DBG_INFO_NONE("\n");
#endif // MULTIBRD_DBG_LVL >= 3

        /*
         * Set up the set of file descriptors that will be watched for input
         * activity.  Only the DM35425 device file descriptor is of interest.
         */
        FD_ZERO(&read_fds);
        FD_ZERO(&exception_fds);
        for (int j = 0; j < num_boards; j++) // set up monitoring for boards without IRQ
        {
            if (!irqs[j]) // irq has not triggered yet for this board
            {
                FD_SET(mbd->boards[j]->board->file_descriptor, &read_fds);
                FD_SET(mbd->boards[j]->board->file_descriptor, &exception_fds);
            }
            else // irq has already triggered for this board
            {
                avail_irq++; // count the number of available IRQs
            }
        }

        /*
         * Set up the set of file descriptors that will be watched for exception
         * activity.  Only the DM35425 file descriptor is of interest.
         */

        status = select(FD_SETSIZE,
                        &read_fds, NULL, &exception_fds, NULL);

        if (mbd->done || mbd->isr == NULL) // this is set only when the thread is being closed
        {
            MULTIBRD_DBG_INFO("Out of select: Done = %d, ISR = %p", mbd->done, mbd->isr);
            mbd->done = 1; // ensure main loop breaks
            mbd->isr = NULL;
            break;
        }

        if (status < 0) // select failed
        {
            mbd->done = 1;
            // TODO: Call ISR to indicate error DM35425_INVALID_IRQ_SELECT
            MULTIBRD_DBG_WARN("Exiting ISR thread: select returned negative [%s]", strerror(errno));
            mbd->isr(-DM35425_INVALID_IRQ_SELECT, NULL, user_data);
            no_error = false;
            break;
        }
        else if (status == 0) // select timed out?
        {
            /*
             * No file descriptors have data available.  Something is broken in the
             * driver.
             */
            MULTIBRD_DBG_WARN("Exiting ISR thread: select timed out (returned 0) [%s]", strerror(errno));
            errno = ENODATA;
            // TODO: Call ISR to indicate error DM35425_INVALID_IRQ_TIMEOUT
            mbd->isr(-DM35425_INVALID_IRQ_TIMEOUT, NULL, user_data);
            no_error = false;
            mbd->done = 1;
            break;
        }

        for (int i = 0; i < num_boards && !mbd->done; i++) // for each board
        {
            if (irqs[i]) // if this is set, this board has already been read from
                continue;

            if (FD_ISSET(mbd->boards[i]->board->file_descriptor, &exception_fds)) // if any board returns an exception it is an automatic disqualification
            {
                errno = EIO;
                // TODO: Call ISR to indicate error DM35425_INVALID_IRQ_IO
                MULTIBRD_DBG_WARN("Exiting ISR thread: board returned exception [%s]", strerror(errno));
                mbd->isr(-DM35425_INVALID_IRQ_IO, NULL, user_data);
                no_error = false;
                mbd->done = 1;
                break;
            }

            /*
             * At least one file descriptor has data available and no exception
             * occured.  Check the device file descriptor to see if it is readable.
             */

            if (!FD_ISSET(mbd->boards[i]->board->file_descriptor, &read_fds))
            {

                /*
                 * This board has no data available, skip it.
                 */
                MULTIBRD_DBG_WARN("Board %d has no available data:", i);
                for (int j = 0; j < num_boards; j++)
                {
                    MULTIBRD_DBG_WARN_NONE(" %d", irqs[j]);
                }
                MULTIBRD_DBG_WARN_NONE("\n");
                continue;
            }
            irqs[i] = 1; // if here, interrupt has triggered for this board
#if MULTIBRD_DBG_LVL >= 3
            struct timespec tv_s;
            clock_gettime(CLOCK_MONOTONIC, &tv_s);
#endif // MULTIBRD_DBG_LVL >= 3

            do // exhaust all available IRQs for this board
            {
                status = ioctl(mbd->boards[i]->board->file_descriptor, DM35425_IOCTL_INTERRUPT_GET,
                               &ioctl_arg); // get interrupt info, should have something now
                if (status != 0)
                {
                    no_error = false;
                    mbd->done = 1;
                    // TODO: Call ISR to indicate error DM35425_ERROR_IRQ_GET
                    MULTIBRD_DBG_WARN("Exiting ISR thread: ioctl INTERRUPT_GET returned error [%s]", strerror(errno));
                    mbd->isr(-DM35425_ERROR_IRQ_GET, NULL, user_data);
                    break;
                }
                status = DM35425_Read_Out_ADC(mbd->boards[i], ioctl_arg.interrupt);
                if (status != 0)
                {
                    no_error = false;
                    mbd->done = 1;
                    // TODO: Call ISR to indicate error DM35425_ERROR_DMA_READ
                    MULTIBRD_DBG_WARN("Exiting ISR thread: DM35425_Read_Out_ADC returned error [%s]", strerror(errno));
                    mbd->isr(status, NULL, user_data);
                    break;
                }
            } while (ioctl_arg.interrupt.interrupts_remaining > 0); // exhaust all the pending interrupts
#if MULTIBRD_DBG_LVL >= 3
            struct timespec tv_e;
            clock_gettime(CLOCK_MONOTONIC, &tv_e);
            timespec_diff(&tv_e, &tv_s, &delta);
            MULTIBRD_DBG_INFO("DMA waiting to read (%d) %d: %ld.%09ld s (after select)", i, isr_call_count, delta.tv_sec, delta.tv_nsec);
#endif // MULTIBRD_DBG_LVL >= 3
            if (no_error)
                avail_irq++;
        }

        if (mbd->done || mbd->isr == NULL)
        {
            break;
        }

        if ((avail_irq != num_boards) && no_error) // back to top of loop if we don't have all the devices ready, this way the slowest device triggers the ISR
        {
            MULTIBRD_DBG_INFO("DMA waiting for all devices to trigger ISR (%d/%d)", avail_irq, num_boards);
            continue;
        }
        // if here, we can clear the IRQ monitor to reset the select
        memset(irqs, 0x0, sizeof(int) * num_boards);
        // Now we have interrupts from all devices ISR can be called after voltage conversion
        for (int i = 0; i < num_boards; i++)
        {
            DM35425_Convert_ADC(mbd->boards[i], voltages[i]);
        }
#if MULTIBRD_DBG_LVL >= 3
        if (isr_call_count)
        {
            clock_gettime(CLOCK_MONOTONIC, &now);
            // start = now;
            timespec_diff(&now, &start, &delta);
            MULTIBRD_DBG_INFO_NONE("\n");
            MULTIBRD_DBG_INFO("DMA calling ISR %d: %ld.%09ld s (since loop)", isr_call_count, delta.tv_sec, delta.tv_nsec);
            timespec_diff(&now, &lstart, &delta);
            MULTIBRD_DBG_INFO("DMA calling ISR %d: %ld.%09ld s (since start)\n", isr_call_count, delta.tv_sec, delta.tv_nsec);
        }
        isr_call_count++;
#endif // MULTIBRD_DBG_LVL >= 3
       // TODO: Call ISR
        if (mbd->isr != NULL)
        {
            mbd->isr(num_boards, mbd->readouts, user_data);
        }
        else
        {
            mbd->done = 1;
        }
    }

    for (int i = 0; i < num_boards; i++)
    {
        for (int j = 0; j < DM35425_NUM_ADC_DMA_CHANNELS; j++)
        {
            free(voltages[i][j]);
        }
        free(voltages[i]);
    }
    free(voltages);

    return NULL;
}

static void DM35425_Convert_ADC(DM35425_ADCDMA_Descriptor *handle, float **voltages)
{
    struct DM35425_Function_Block *fb = handle->fb;
    size_t num_samples = handle->buf_sz / sizeof(int);
    int buf_idx = handle->next_buf - 1 < 0 ? fb->num_dma_buffers - 1 : handle->next_buf - 1;
    for (int channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++)
    {
        int dchannel = handle->input_mode == DM35425_ADC_INPUT_DIFFERENTIAL ? channel % 8 + (channel / 16) * 8 : channel;
        for (int i = 0; i < num_samples; i++)
        {
            DM35425_Adc_Sample_To_Volts(handle->range, handle->local_buf[channel][buf_idx][i], &voltages[dchannel][i]);
        }
    }
}

static int DM35425_Read_Out_ADC(DM35425_ADCDMA_Descriptor *handle, struct dm35425_ioctl_interrupt_info_request int_info)
{
    int result = 0;
    int buffer_full = 0;
    int dma_error = 0;
    unsigned int channel = 0;

    struct DM35425_Board_Descriptor *board = handle->board;
    struct DM35425_Function_Block *fb = handle->fb;

    if (int_info.valid_interrupt)
    {

        // It's a DMA interrupt
        if (int_info.interrupt_fb < 0)
        {

            result = DM35425_Dma_Check_Buffer_Used(board,
                                                   fb,
                                                   channel,
                                                   handle->next_buf,
                                                   &buffer_full);

            if (result != 0)
            {
                MULTIBRD_DBG_ERR("Board %p: Error finding used buffer.", board);
                return -DM35425_ERROR_FIND_USED_BUFFER;
            }
            if (buffer_full == 0)
            {
                MULTIBRD_DBG_ERR("Board %p: DMA Interrupt occurred, but buffer was not full.", board);
                return -DM35425_ERROR_BUFFER_NOT_FULL;
            }

            // Read all DMA channels
            for (channel = 0;
                 channel < DM35425_NUM_ADC_DMA_CHANNELS;
                 channel++)
            {

                result = DM35425_Dma_Check_For_Error(board,
                                                     fb,
                                                     channel,
                                                     &dma_error);

                if (result != 0)
                {
                    MULTIBRD_DBG_ERR("Board %p: Checking for DMA error.", board);
                    return -DM35425_ERROR_CHECK_DMA_ERROR;
                }

                if (dma_error)
                {
                    MULTIBRD_DBG_ERR("Board %p: DMA error occurred on channel %d.", board, channel);
                    return -DM35425_ERROR_CHANNEL_DMA_ERROR;
                }

                result = DM35425_Dma_Read(board,
                                          fb,
                                          channel,
                                          handle->next_buf,
                                          handle->buf_sz,
                                          handle->local_buf[channel]
                                                           [handle->next_buf]);

                if (result != 0)
                {
                    MULTIBRD_DBG_ERR("Board %p: Reading DMA buffer on channel %d.", board, channel);
                    return -DM35425_ERROR_READ_DMA_BUFFER;
                }

                result = DM35425_Dma_Reset_Buffer(board,
                                                  fb,
                                                  channel,
                                                  handle->next_buf);

                if (result != 0)
                {
                    MULTIBRD_DBG_ERR("Board %p: Resetting DMA buffer on channel %d.", board, channel);
                    return -DM35425_ERROR_RESET_DMA_BUFFER;
                }

                result = DM35425_Dma_Clear_Interrupt(board,
                                                     fb,
                                                     channel,
                                                     NO_CLEAR_INTERRUPT,
                                                     NO_CLEAR_INTERRUPT,
                                                     NO_CLEAR_INTERRUPT,
                                                     NO_CLEAR_INTERRUPT,
                                                     CLEAR_INTERRUPT);

                if (result != 0)
                {
                    MULTIBRD_DBG_ERR("Board %p: Clearing DMA interrupt on channel %d.", board, channel);
                    return -DM35425_ERROR_CLEAR_DMA_INTERRUPT;
                }
            }

            handle->next_buf = (handle->next_buf + 1) % fb->num_dma_buffers;
        }
        else
        {
            MULTIBRD_DBG_ERR("Board %p: Non-DMA interrupt occurred for FB 0x%x.", board, int_info.interrupt_fb);
        }

        result = DM35425_Gbc_Ack_Interrupt(board);

        if (result != 0)
        {
            MULTIBRD_DBG_ERR("Board %p: Acknowledging interrupt.", board);
            return -DM35425_ERROR_ACK_INTERRUPT;
        }
    }
    return DM35425_SUCCESS;
}

int DM35425_Multiboard_SetISRPriority(DM35425_Multiboard_Descriptor *handle, int priority)
{
    struct sched_param param;

    param.sched_priority = priority;
    if (handle->isr == NULL)
    {
        errno = -EFAULT;
        return -1;
    }

    if (getuid() != 0)
    {
        return 0;
    }

    return pthread_setschedparam(handle->pid, SCHED_FIFO, &param);
}

#if (defined(__linux__) || defined(_POSIX_VERSION)) && defined(_GNU_SOURCE)
int DM35425_Multiboard_SetISRAffinity(DM35425_Multiboard_Descriptor *_Nonnull handle, size_t cpusetsize, const cpu_set_t *cpuset)
{
    return pthread_setaffinity_np(handle->pid, cpusetsize, cpuset);
}

int DM35425_Multiboard_GetISRAffinity(DM35425_Multiboard_Descriptor *_Nonnull handle, size_t cpusetsize, cpu_set_t *cpuset)
{
    return pthread_getaffinity_np(handle->pid, cpusetsize, cpuset);
}
#endif
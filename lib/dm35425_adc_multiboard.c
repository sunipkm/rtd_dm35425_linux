#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "dm35425_adc_multiboard.h"
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
#else
#define MULTIBRD_DBG_INFO(fmt, ...)
#endif

#if (MULTIBRD_DBG_LVL >= 2)
#define MULTIBRD_DBG_WARN(fmt, ...)                                                                            \
    {                                                                                                          \
        fprintf(stderr, "%s:%d:%s(): " YELLOW_FG fmt "\n" RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr);                                                                                        \
    }
#else
#define MULTIBRD_DBG_WARN(fmt, ...)
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
    struct timespec *result) {
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}

static void DM35425_Convert_ADC(struct DM35425_ADCDMA_Descriptor *_Nonnull handle, float **_Nonnull voltages);

static int DM35425_Read_Out_ADC(struct DM35425_ADCDMA_Descriptor *_Nonnull handle, struct dm35425_ioctl_interrupt_info_request int_info);

static void *DM35425_Multiboard_WaitForIRQ(void *ptr);

#define ADC_0 0
#define DAC_0 0
#define NOT_IGNORE_USED 0
#define INTERRUPT_DISABLE 0
#define ERROR_INTR_DISABLE 0
#define INTERRUPT_ENABLE 1
#define ERROR_INTR_ENABLE 1
#define CHANNEL_0 0
#define NO_CLEAR_INTERRUPT 0
#define CLEAR_INTERRUPT 1

int DM35425_ADCDMA_Open(int minor, struct DM35425_ADCDMA_Descriptor **handle_)
{
    struct DM35425_ADCDMA_Descriptor *handle = (struct DM35425_ADCDMA_Descriptor *)malloc(sizeof(struct DM35425_ADCDMA_Descriptor));
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

int DM35425_ADCDMA_Close(struct DM35425_ADCDMA_Descriptor *handle)
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

int DM35425_ADCDMA_Configure_ADC(struct DM35425_ADCDMA_Descriptor *handle, uint32_t rate, size_t samples_per_buf, enum DM35425_Channel_Delay delay, enum DM35425_Input_Mode input_mode, enum DM35425_Input_Ranges range)
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
                MULTIBRD_DBG_INFO("DMA buffer %d status for channel %d: buff_stat = 0x%x, buff_ctrl = 0x%x, buff_sz = %d", buff, channel, buff_stat, buff_ctrl, buff_sz);
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
    return 0;
}

int DM35425_ADC_Multiboard_Init(struct DM35425_Multiboard_Descriptor **_mbd, int num_boards, struct DM35425_ADCDMA_Descriptor *first_board, ...)
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

    struct DM35425_Multiboard_Descriptor *mbd = (struct DM35425_Multiboard_Descriptor *)malloc(sizeof(struct DM35425_Multiboard_Descriptor));
    if (mbd == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    memset(mbd, 0x00, sizeof(struct DM35425_Multiboard_Descriptor)); // zero out memory

    struct DM35425_ADCDMA_Descriptor **boards = (struct DM35425_ADCDMA_Descriptor **)malloc(sizeof(struct DM35425_ADCDMA_Descriptor *) * num_boards); // allocate num_boards pointers to board descriptors

    if (boards == NULL)
    {
        errno = ENOMEM;
        goto errored;
    }

    memset(boards, 0x00, sizeof(struct DM35425_ADCDMA_Descriptor *) * num_boards); // zero out memory

    boards[0] = first_board;
    for (int i = 1; i < num_boards; i++) // itreate over and copy ptrs to each board descriptor
    {
        struct DM35425_ADCDMA_Descriptor *board = va_arg(args, struct DM35425_ADCDMA_Descriptor *);
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
    *_mbd = mbd;

    return 0;

errored_boards:
    free(boards);
errored:
    free(mbd);
    *_mbd = NULL;
    return -1;
}

int DM35425_ADC_Multiboard_Destroy(struct DM35425_Multiboard_Descriptor *mbd)
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

int DM35425_ADC_Multiboard_RemoveISR(struct DM35425_Multiboard_Descriptor * _Nonnull mbd)
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
        int rc = pthread_join(mbd->pid, NULL);
        if (rc != 0)
        {
            MULTIBRD_DBG_ERR("Failed to join thread for multiboard ISR");
            errno = rc;
            return -1;
        }
    }
    return 0;
}

int DM35425_ADC_Multiboard_InstallISR(struct DM35425_Multiboard_Descriptor * _Nonnull mbd, DM35425_Multiboard_ISR isr, void *user_data, bool block)
{
    if (mbd == NULL)
    {
        errno = ENODATA;
        return -1;
    }
    mbd->isr = isr;
    mbd->user_data = user_data;

    int rc = pthread_create(&(mbd->pid), NULL, DM35425_Multiboard_WaitForIRQ, (void *) mbd);
    if (rc != 0)
    {
        MULTIBRD_DBG_ERR("Failed to create thread for multiboard ISR");
        errno = rc;
        return -1;
    }

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
}

static void *DM35425_Multiboard_WaitForIRQ(void *ptr)
{
    fd_set exception_fds;
    fd_set read_fds;
    int status;
    struct DM35425_Multiboard_Descriptor *mbd = ptr;
    void *user_data = mbd->user_data;

    float ***voltages = (float ***)malloc(sizeof(float *) * mbd->num_boards);
    if (voltages == NULL)
    {
        MULTIBRD_DBG_ERR("Failed to allocate memory for voltages");
        errno = ENOMEM;
        return NULL;
    }
    for (int i = 0; i < mbd->num_boards; i++)
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
        }
    }

    while (!mbd->done)
    {
        bool no_error = true;
        int avail_irq = 0;

        for (int i = 0; i < mbd->num_boards && !mbd->done; i++)
        {
            union dm35425_ioctl_argument ioctl_arg;
            /*
             * Set up the set of file descriptors that will be watched for input
             * activity.  Only the DM35425 device file descriptor is of interest.
             */

            FD_ZERO(&read_fds);
            FD_ZERO(&exception_fds);
            for (int i = 0; i < mbd->num_boards; i++)
            {
                FD_SET(mbd->boards[i]->board->file_descriptor, &read_fds);
                FD_SET(mbd->boards[i]->board->file_descriptor, &exception_fds);
            }

            /*
             * Set up the set of file descriptors that will be watched for exception
             * activity.  Only the DM35425 file descriptor is of interest.
             */

            status = select(FD_SETSIZE,
                            &read_fds, NULL, &exception_fds, NULL);

            if (mbd->done || mbd->isr == NULL) // this is set only when the thread is being closed
            {
                mbd->done = 1; // ensure main loop breaks
                mbd->isr = NULL;
                break;
            }

            if (status < 0) // select failed
            {
                mbd->done = 1;
                // TODO: Call ISR to indicate error DM35425_INVALID_IRQ_SELECT
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

                errno = ENODATA;
                // TODO: Call ISR to indicate error DM35425_INVALID_IRQ_TIMEOUT
                mbd->isr(-DM35425_INVALID_IRQ_TIMEOUT, NULL, user_data);
                no_error = false;
                mbd->done = 1;
                break;
            }

            if (FD_ISSET(mbd->boards[i]->board->file_descriptor, &exception_fds))
            {
                errno = EIO;
                // TODO: Call ISR to indicate error DM35425_INVALID_IRQ_IO
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
                 * The device file is not readable.  This means something is broken.
                 */
                errno = ENODATA;
                // TODO: Call ISR to indicate error DM35425_INVALID_IRQ_NODATA
                mbd->isr(-DM35425_INVALID_IRQ_NODATA, NULL, user_data);
                no_error = false;
                mbd->done = 1;
                break;
            }

            do // exhaust all available IRQs
            {
                status = ioctl(mbd->boards[i]->board->file_descriptor, DM35425_IOCTL_INTERRUPT_GET,
                               &ioctl_arg); // get interrupt info, should have something now
                if (status != 0)
                {
                    no_error = false;
                    mbd->done = 1;
                    // TODO: Call ISR to indicate error DM35425_ERROR_IRQ_GET
                    mbd->isr(-DM35425_ERROR_IRQ_GET, NULL, user_data);
                    break;
                }
                status = DM35425_Read_Out_ADC(mbd->boards[i], ioctl_arg.interrupt);
                if (status != 0)
                {
                    no_error = false;
                    mbd->done = 1;
                    // TODO: Call ISR to indicate error DM35425_ERROR_DMA_READ
                    mbd->isr(status, NULL, user_data);
                    break;
                }
            } while (ioctl_arg.interrupt.interrupts_remaining > 0); // exhaust all the pending interrupts
            if (no_error)
                avail_irq++;
        }
        if (avail_irq != mbd->num_boards && !no_error) // back to top of loop if we don't have all the devices ready, this way the slowest device triggers the ISR
        {
            continue;
        }
        // Now we have interrupts from all devices ISR can be called after voltage conversion
        for (int i = 0; i < mbd->num_boards; i++)
        {
            DM35425_Convert_ADC(mbd->boards[i], voltages[i]);
        }
        // TODO: Call ISR
        if (mbd->isr != NULL)
        {
            mbd->isr(mbd->num_boards, voltages, user_data);
        }
        else
        {
            mbd->done = 1;
        }
    }

    for (int i = 0; i < mbd->num_boards; i++)
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

static void DM35425_Convert_ADC(struct DM35425_ADCDMA_Descriptor *handle, float **voltages)
{
    struct DM35425_Function_Block *fb = handle->fb;
    size_t num_samples = handle->buf_sz / sizeof(int);
    int buf_idx = handle->next_buf - 1 < 0 ? fb->num_dma_buffers - 1 : handle->next_buf - 1;
    for (int channel = 0; channel < DM35425_NUM_ADC_DMA_CHANNELS; channel++)
    {
        for (int i = 0; i < num_samples; i++)
        {
            DM35425_Adc_Sample_To_Volts(handle->range, handle->local_buf[channel][buf_idx][i], &voltages[channel][i]);
        }
    }
}

static int DM35425_Read_Out_ADC(struct DM35425_ADCDMA_Descriptor *handle, struct dm35425_ioctl_interrupt_info_request int_info)
{
    int result = 0;
    int buffer_full = 0;
    int dma_error = 0;
    unsigned int channel = 0;

#if MULTIBRD_DBG_LVL >= 3
    static int isr_call_count = 0;
    static struct timespec start;
    struct timespec now;
    struct timespec end;
    struct timespec delta;
#endif // MULTIBRD_DBG_LVL >= 3

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

#if MULTIBRD_DBG_LVL >= 3
            if (!isr_call_count)
            {
                clock_gettime(CLOCK_MONOTONIC, &start);
            }
            else
            {
                clock_gettime(CLOCK_MONOTONIC, &now);
                timespec_diff(&now, &start, &delta);
                MULTIBRD_DBG_INFO("DMA Readout start (%p) %d: %ld.%09ld s (bw/calls)", board, isr_call_count, delta.tv_sec, delta.tv_nsec);
            }
#endif // MULTIBRD_DBG_LVL >= 3

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

#if MULTIBRD_DBG_LVL >= 3
            if (isr_call_count)
            {
                clock_gettime(CLOCK_MONOTONIC, &end);
                timespec_diff(&end, &now, &delta);
                MULTIBRD_DBG_INFO("DMA Readout end (%p) %d: %ld.%09ld s (DMA)\n", board, isr_call_count, delta.tv_sec, delta.tv_nsec);
            }
            isr_call_count++;
#endif // MULTIBRD_DBG_LVL >= 3
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
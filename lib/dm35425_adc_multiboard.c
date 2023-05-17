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
#define MULTIBRD_DBG_INFO(fmt, ...) \
{ \
    
        fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr); \
}
#else
#define MULTIBRD_DBG_INFO(fmt, ...)
#endif

#if (MULTIBRD_DBG_LVL >= 2)
#define MULTIBRD_DBG_WARN(fmt, ...) \
{ \
    
        fprintf(stderr, "%s:%d:%s(): " YELLOW_FG fmt "\n" RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr); \
}
#else
#define MULTIBRD_DBG_WARN(fmt, ...)
#endif

#if (MULTIBRD_DBG_LVL >= 1)
#define MULTIBRD_DBG_ERR(fmt, ...) \
{ \
    
        fprintf(stderr, "%s:%d:%s(): " RED_FG fmt "\n" RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr); \
}
#else
#define MULTIBRD_DBG_ERR(fmt, ...)
#endif

#define ADC_0 0
#define DAC_0 0
#define NOT_IGNORE_USED 0
#define INTERRUPT_DISABLE 0
#define ERROR_INTR_DISABLE 0
#define INTERRUPT_ENABLE 1
#define ERROR_INTR_ENABLE 1
#define CHANNEL_0 0

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

int DM35425_ADCDMA_Configure_ADC(struct DM35425_ADCDMA_Descriptor * handle, uint32_t rate, size_t samples_per_buf, enum DM35425_Channel_Delay delay, enum DM35425_Input_Mode input_mode, enum DM35425_Input_Ranges range)
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
        MULTIBRD_DBG_INFO("Invalid samples_per_buf %u, acceptable range is 1 to %u", samples_per_buf, DM35425_DMA_MAX_BUFFER_SIZE);
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
            int buff_stat, buff_ctrl, buff_sz;
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
    return 0;
}

int DM35425_ADC_Multiboard_Init(struct DM35425_Multiboard_Descriptor **mbd, int num_boards, struct DM35425_ADCDMA_Descriptor * first_board, ...)
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

    mbd = (struct DM35425_Multiboard_Descriptor **)malloc(sizeof(struct DM35425_Multiboard_Descriptor *));
    if (mbd == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    memset(*mbd, 0x00, sizeof(struct DM35425_Multiboard_Descriptor)); // zero out memory

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

    (*mbd)->boards = boards;         // copy over boards
    (*mbd)->num_boards = num_boards; // copy over num_boards

    return 0;

errored_boards:
    free(boards);
errored:
    free(mbd);
    mbd = NULL;
    return -1;
}

int DM35425_ADC_Multiboard_Destroy(struct DM35425_Multiboard_Descriptor *mbd)
{
    if (mbd == NULL)
    {
        errno = ENODATA;
        return -1;
    }

    mbd->done = 1;                                                // set done flag
    for (int i = 0; i < mbd->num_boards; i++) // Disable multiboard ISR
    {
        ioctl(mbd->boards[i]->board->file_descriptor, DM35425_IOCTL_WAKEUP); // wake up ISR
    }

    free(mbd->boards);
    free(mbd);
    return 0;
}

struct DM35425_Multiboard_ThreadArg {
    struct DM35425_Multiboard_Descriptor *mbd;
    void *user_data;
};

static void *DM35425_Multiboard_WaitForIRQ(void *ptr)
{
    fd_set exception_fds;
    fd_set read_fds;
    int status;
    struct DM35425_Multiboard_ThreadArg *arg = (struct DM35425_Multiboard_ThreadArg *)ptr;
    struct DM35425_Multiboard_Descriptor *mbd = arg->mbd;
    void *user_data = arg->user_data;
    struct timeval tv;

    while (!mbd->done)
    {
        bool no_error = true;
        int avail_irq = 0;
        tv.tv_sec = mbd->timeout_sec;
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

            /*
             * Wait for the interrupt to happen.  No timeout is given, which means
             * the process will not be woken up until either an interrupt occurs or
             * a signal is delivered
             */

            status = select(FD_SETSIZE,
                            &read_fds, NULL, &exception_fds, tv.tv_sec > 0 ? &tv : NULL);
            if (status < 0)
            {
                mbd->done = 1;
                // TODO: Call ISR to indicate error
                no_error = false;
                break;
            }
            else if (status == 0)
            {
                /*
                 * No file descriptors have data available.  Something is broken in the
                 * driver.
                 */

                errno = -ENODATA;
                no_error = false;
                mbd->done = 1;
                // TODO: Call ISR to indicate error
                break;
            }

            if (FD_ISSET(mbd->boards[i]->board->file_descriptor, &exception_fds))
            {
                errno = -EIO;
                no_error = false;
                mbd->done = 1;
                // TODO: Call ISR to indicate error
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
                no_error = false;
                errno = -ENODATA;
                mbd->done = 1;
                // TODO: Call ISR to indicate error
                break;
            }

            do
            {
                status = ioctl(mbd->boards[i]->board->file_descriptor, DM35425_IOCTL_INTERRUPT_GET,
                               &ioctl_arg); // get interrupt info, should have something now
                if (status != 0)
                {
                    no_error = false;
                    mbd->done = 1;
                    // TODO: Call ISR to indicate error
                    break;
                }
            } while (ioctl_arg.interrupt.interrupts_remaining > 0); // exhaust all the pending interrupts
            if (no_error)
                avail_irq++;
        }
        if (avail_irq != mbd->num_boards && no_error) // back to top of loop if we don't have all the devices ready, this way the slowest device triggers the ISR
        {
            continue;
        }
        // Now we have interrupts from all devices ISR can be called
        // TODO: Call ISR
        if (mbd->isr != NULL)
        {
            mbd->isr(mbd->num_boards, mbd->boards, user_data);
        }
    }

    return NULL;
}
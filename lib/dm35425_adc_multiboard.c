#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "dm35425_adc_multiboard.h"
#include "dm35425_ioctl.h"

int DM35425_Multiboard_Init(struct DM35425_Multiboard_Descriptor **mbd, int num_boards, ...)
{
    if (num_boards < 1)
    {
        errno = EINVAL;
        return -1;
    }

    va_list args;
    va_start(args, num_boards);

    mbd = (struct DM35425_Multiboard_Descriptor **)malloc(sizeof(struct DM35425_Multiboard_Descriptor *));
    if (mbd == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    memset(*mbd, 0x00, sizeof(struct DM35425_Multiboard_Descriptor)); // zero out memory

    struct DM35425_Board_Descriptor **boards = (struct DM35425_Board_Descriptor **)malloc(sizeof(struct DM35425_Board_Descriptor *) * num_boards); // allocate num_boards pointers to board descriptors

    if (boards == NULL)
    {
        errno = ENOMEM;
        goto errored;
    }

    memset(boards, 0x00, sizeof(struct DM35425_Board_Descriptor *) * num_boards); // zero out memory

    for (int i = 0; i < num_boards; i++) // itreate over and copy ptrs to each board descriptor
    {
        boards[i] = va_arg(args, struct DM35425_Board_Descriptor *);
        if (boards[i] == NULL)
        {
            errno = ENODATA;
            goto errored_boards;
        }
    }

    va_end(args);

    for (int i = 0; i < num_boards; i++) // iterate over and set multiboard_isr flag to 1
    {
        boards[i]->multiboard_isr = 1;
    }

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

int DM35425_Multiboard_Destroy(struct DM35425_Multiboard_Descriptor *mbd)
{
    if (mbd == NULL)
    {
        errno = ENODATA;
        return -1;
    }

    mbd->done = 1;                                                // set done flag
    for (int i = 0; i < mbd->num_boards; i++) // Disable multiboard ISR
    {
        ioctl(mbd->boards[i]->file_descriptor, DM35425_IOCTL_WAKEUP); // wake up ISR
    }

    for (int i = 0; i < mbd->num_boards; i++) // Disable multiboard ISR
    {
        mbd->boards[i]->multiboard_isr = 0;
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
                FD_SET(mbd->boards[i]->file_descriptor, &read_fds);
                FD_SET(mbd->boards[i]->file_descriptor, &exception_fds);
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

            if (FD_ISSET(mbd->boards[i]->file_descriptor, &exception_fds))
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

            if (!FD_ISSET(mbd->boards[i]->file_descriptor, &read_fds))
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
                status = ioctl(mbd->boards[i]->file_descriptor, DM35425_IOCTL_INTERRUPT_GET,
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
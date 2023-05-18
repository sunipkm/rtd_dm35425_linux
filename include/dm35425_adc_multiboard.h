/**
 * @file dm35425_adc_multiboard.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Function declarations for the DM35425 that are Linux specific and
 * provides single ISR functionality for ADCs on multiple boards.
 * @version 1.0
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
#include "dm35425_adc_library.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef _Nullable
/**
 * @brief Indicates whether a pointer can be NULL.
 *
 */
#define _Nullable
#endif

#ifndef _Nonnull
/**
 * @brief The pointer must not be NULL.
 *
 */
#define _Nonnull
#endif

#ifndef MULTIBRD_DBG_LVL
/**
 * @brief Debug level for multiboard functions. 0 = no debug, 1 = errors only, 2 = errors and warnings, 3 = errors, warnings, and info.
 *
 */
#define MULTIBRD_DBG_LVL 1
#endif

/**
 * @brief Defines the possible errors encountered by the ISR ({@link DM35425_Multiboard_ISR}) function.
 *
 */
enum DM35425_ERROR
{
    DM35425_SUCCESS = 0,               /*!< Successful operation. */
    DM35425_ERROR_FIND_USED_BUFFER,    /*!< Could not find the last used buffer. */
    DM35425_ERROR_BUFFER_NOT_FULL,     /*!< Interrupt was triggered but the buffer was not full. */
    DM35425_ERROR_CHECK_DMA_ERROR,     /*!< Could not check the DMA error status. */
    DM35425_ERROR_CHANNEL_DMA_ERROR,   /*!< DMA error on a channel. */
    DM35425_ERROR_READ_DMA_BUFFER,     /*!< Could not read the DMA buffer. */
    DM35425_ERROR_RESET_DMA_BUFFER,    /*!< Could not reset the DMA buffer. */
    DM35425_ERROR_CLEAR_DMA_INTERRUPT, /*!< Could not clear the DMA interrupt. */
    DM35425_ERROR_ACK_INTERRUPT,       /*!< Could not acknowledge the interrupt. */
    DM35425_ERROR_DMA_READ,            /*!< Could not read from the DMA buffer. */
    DM35425_ERROR_IRQ_GET,             /*!< Could not get the IRQ number. */
    DM35425_INVALID_IRQ_FD_UNREADABLE, /*!< The IRQ function descriptor is unreadable. */
    DM35425_INVALID_IRQ_IO,            /*!< Could not perform I/O after receiving interrupt. */
    DM35425_INVALID_IRQ_TIMEOUT,       /*!< Timed out while waiting for interrupt. */
    DM35425_INVALID_IRQ_SELECT,        /*!< Select failed on the ADC board handle. */
};

/**
 * @brief Structure to hold the readout voltages from an ADC board.
 *
 */
struct DM35425_ADCDMA_Readout
{
    int num_channels;   /*!< Number of channels */
    size_t num_samples; /*!< Number of samples per channel */
    float **voltages;   /*!< Array of voltages[num_channels][num_samples] */
};

/**
 * @brief Interrupt service routine for the multi-board ADCs. This function is called every time the ADCs have a full buffer of data.
 *
 * @param num_boards Number of boards when positive. Indicates an error when negative. The error codes are given by the {@link DM35425_Error} enum.
 * @param readouts Pointer to the array of voltage read-outs per device. This is an array of {@link DM35425_ADCDMA_Readout} structures, and NULL in case of error.
 * @param user_data Pointer to user data that was passed to the ISR function.
 */
typedef void (*DM35425_Multiboard_ISR)(int num_boards, struct DM35425_ADCDMA_Readout *_Nullable readouts, void *_Nullable user_data);

/**
 * @brief ADC DMA Descriptor (combines all necessary structures and fields to interact with ADC channels in one structure.)
 *
 */
typedef struct _DM35425_ADCDMA_Descriptor DM35425_ADCDMA_Descriptor;

/**
 * @brief ADC Multiple Board DMA Descriptor (combines all necessary structures and fields to interact with multiple ADC boards in one structure.)
 *
 */
typedef struct _DM35425_Multiboard_Descriptor DM35425_Multiboard_Descriptor;

/**
 * @brief Open a single ADC board.
 *
 * @param minor Minor number of the device (x in /dev/rtd-dm35425-x).
 * @param handle Pointer to the handle to be returned.
 * @return int 0 on success, negative on failure. Errno is set accordingly.
 */
int DM35425_ADCDMA_Open(int minor, DM35425_ADCDMA_Descriptor **_Nonnull handle);

/**
 * @brief Close a single ADC board.
 *
 * @param handle Handle to be closed.
 * @return int 0 on success, negative on failure.
 */
int DM35425_ADCDMA_Close(DM35425_ADCDMA_Descriptor *_Nonnull handle);

/**
 * @brief Configure a single ADC board.
 *
 * @param handle Handle to the ADC board.
 * @param rate Sample rate of the ADC board (in Hz).
 * @param samples_per_buf Number of samples to be collected before the ADC board triggers an interrupt.
 * @param delay Delay between sampling of different channels. See {@link DM35425_Channel_Delay} for possible values.
 * @param input_mode Input mode of the ADC board. Can be {@link DM35425_ADC_INPUT_SINGLE_ENDED} for single ended, 32 channel operation or {@link DM35425_ADC_INPUT_DIFFERENTIAL} for differential, 16 channel operation.
 * @param range Input range of the ADC board. See {@link DM35425_Input_Ranges} for possible values.
 * @return int
 */
int DM35425_ADCDMA_Configure_ADC(DM35425_ADCDMA_Descriptor *_Nonnull handle, uint32_t rate, size_t samples_per_buf, enum DM35425_Channel_Delay delay, enum DM35425_Input_Mode input_mode, enum DM35425_Input_Ranges range);

/**
 * @brief Combine multiple ADC boards into a single structure.
 *
 * @param mbd Handle to the multi-board descriptor to be returned.
 * @param num_boards Number of boards to be combined. Must be greater than 0.
 * @param first_board Handle of the first board. Must be non-null.
 * @param ... Pointers to the handles of the other boards. Must be non-null, and there must be num_boards - 1 of them.
 * @return int 0 on success, -1 on failure. Errno is set accordingly.
 */
int DM35425_ADC_Multiboard_Init(DM35425_Multiboard_Descriptor **_Nonnull mbd, int num_boards, DM35425_ADCDMA_Descriptor *_Nonnull first_board, ...);

/**
 * @brief Destroy a multi-board descriptor.
 *
 * @param mbd Handle to the multi-board descriptor.
 * @return int 0 on success, -1 on failure. Errno is set accordingly.
 */
int DM35425_ADC_Multiboard_Destroy(DM35425_Multiboard_Descriptor *_Nonnull mbd);

/**
 * @brief Install the interrupt service routine for the multi-board ADCs. Installing the ISR starts
 * the data acquisition process on all boards immediately.
 *
 * Note: Ensure that the individual boards are configured before calling this function.
 * Also ensure that the rate * samples_per_buf is the same for all the boards in the multi-board descriptor to avoid interrupt racing between the boards.
 *
 * @param mbd Handle to the multi-board descriptor.
 * @param isr Interrupt service routine. See {@link DM35425_Multiboard_ISR} for details.
 * @param user_data Pointer to user data to be passed to the ISR. Can be NULL.
 * @param block Whether the installer blocks indefinitely. This is implemented using `pthread_join()`, so the ISR or an external thread must call {@link DM35425_ADC_Multiboard_RemoveISR} to unblock the thread calling this function.
 * @return int 0 on success, -1 on failure. Errno is set accordingly.
 */
int DM35425_ADC_Multiboard_InstallISR(DM35425_Multiboard_Descriptor *_Nonnull mbd, DM35425_Multiboard_ISR isr, void *_Nullable user_data, bool block);

/**
 * @brief Remove the interrupt service routine for the multi-board ADCs. This function is called automatically by {@link DM35425_ADC_Multiboard_Destroy}.
 *
 * @param mbd Handle to the multi-board descriptor.
 * @return int 0 on success, -1 on failure. Errno is set accordingly.
 */
int DM35425_ADC_Multiboard_RemoveISR(DM35425_Multiboard_Descriptor *_Nonnull mbd);

/**
 * @brief Set the priority of the interrupt service routine for the multi-board ADCs.
 * 
 * @param handle Handle to the multi-board descriptor.
 * @param priority ISR thread priority. See {@link pthread_setschedparam} for details.
 * @return int 0 on success, non-zero on failure.
 */
int DM35425_Multiboard_SetISRPriority(DM35425_Multiboard_Descriptor *_Nonnull handle, int priority);

#if (defined(__linux__ ) || defined(_POSIX_VERSION)) && defined(_GNU_SOURCE)

/**
 * @brief Set the CPU affinity of the interrupt service routine for the multi-board ADCs.
 * 
 * @param handle 
 * @param cpusetsize 
 * @param cpuset 
 * @return int 
 */
int DM35425_Multiboard_SetISRAffinity(DM35425_Multiboard_Descriptor *_Nonnull handle, size_t cpusetsize,const cpu_set_t *cpuset);

/**
 * @brief Get the CPU affinity of the interrupt service routine for the multi-board ADCs.
 * 
 * @param handle 
 * @param cpusetsize 
 * @param cpuset 
 * @return int 
 */
int DM35425_Multiboard_GetISRAffinity(DM35425_Multiboard_Descriptor *_Nonnull handle, size_t cpusetsize, cpu_set_t *cpuset);
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _DM35425_BOARD_ADC_MULTIBOARD__H_
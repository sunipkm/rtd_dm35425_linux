/**
    @file

    @brief
        Definitions for the DM35425 ADIO Library

    $Id: dm35425_adio_library.h 106898 2017-03-08 13:44:23Z rgroner $
*/

//----------------------------------------------------------------------------
//  COPYRIGHT (C) RTD EMBEDDED TECHNOLOGIES, INC.  ALL RIGHTS RESERVED.
//
//  This software package is dual-licensed.  Source code that is compiled for
//  kernel mode execution is licensed under the GNU General Public License
//  version 2.  For a copy of this license, refer to the file
//  LICENSE_GPLv2.TXT (which should be included with this software) or contact
//  the Free Software Foundation.  Source code that is compiled for user mode
//  execution is licensed under the RTD End-User Software License Agreement.
//  For a copy of this license, refer to LICENSE.TXT or contact RTD Embedded
//  Technologies, Inc.  Using this software indicates agreement with the
//  license terms listed above.
//----------------------------------------------------------------------------


#ifndef _DM35425_ADIO_LIBRARY__H_
#define _DM35425_ADIO_LIBRARY__H_


#include "dm35425_gbc_library.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief
 *      Register value for ADIO Mode Reset
 */
#define DM35425_ADIO_MODE_RESET			0x00
/**
 * @brief
 *      Register value for ADIO Mode Pause
 */
#define DM35425_ADIO_MODE_PAUSE			0x01

/**
 * @brief
 *      Register value for ADIO Mode Go (Single Shot)
 */
#define DM35425_ADIO_MODE_GO_SINGLE_SHOT		0x02

/**
 * @brief
 *      Register value for ADIO Mode Go (Rearm after Stop)
 */
#define DM35425_ADIO_MODE_GO_REARM			0x03
/**
 * @brief
 *      Register value for ADIO Mode Uninitialized
 */
#define DM35425_ADIO_MODE_UNINITIALIZED		0x04


/**
 * @brief
 *      Register value for ADIO Status - Stopped
 */
#define DM35425_ADIO_STAT_STOPPED			0x00

/**
 * @brief
 *      Register value for ADIO Status - Waiting for Start Trigger
 */
#define DM35425_ADIO_STAT_WAITING_START_TRIG	0x02

/**
 * @brief
 *      Register value for ADIO Status - Sampling Data
 */
#define DM35425_ADIO_STAT_SAMPLING			0x03

/**
 * @brief
 *      Register value for ADIO Status - Filling Post-Stop Buffer
 */
#define DM35425_ADIO_STAT_FILLING_POST_TRIG_BUFF	0x04

/**
 * @brief
 *      Register value for ADIO Status - Wait for Rearm
 */
#define DM35425_ADIO_STAT_WAIT_REARM		0x05

/**
 * @brief
 *      Register value for ADIO Status - Done
 */
#define DM35425_ADIO_STAT_DONE			0x07

/**
 * @brief
 *      Register value for ADIO Status - Uninitialized
 */
#define DM35425_ADIO_STAT_UNINITIALIZED		0x08

/**
 * @brief
 *      Register value for ADIO Status - Initializing
 */
#define DM35425_ADIO_STAT_INITIALIZING		0x09


/**
 * @brief
 *      Register value for Interrupt Mask - Sample Taken
 */
#define DM35425_ADIO_INT_SAMPLE_TAKEN_MASK		0x0001

/**
 * @brief
 *      Register value for Interrupt Mask - Advanced Interrupt Occurred
 */
#define DM35425_ADIO_INT_ADV_INT_MASK		0x0002

/**
 * @brief
 *      Register value for Interrupt Mask - Pre-Start Buffer Filled
 */
#define DM35425_ADIO_INT_PRE_BUFF_FULL_MASK		0x0004

/**
 * @brief
 *      Register value for Interrupt Mask - Start Trigger Occurred
 */
#define DM35425_ADIO_INT_START_TRIG_MASK		0x0008

/**
 * @brief
 *      Register value for Interrupt Mask - Stop Trigger Occurred
 */
#define DM35425_ADIO_INT_STOP_TRIG_MASK		0x0010

/**
 * @brief
 *      Register value for Interrupt Mask - Post-Stop Buffer Filled
 */
#define DM35425_ADIO_INT_POST_BUFF_FULL_MASK	0x0020

/**
 * @brief
 *      Register value for Interrupt Mask - Sampling Complete
 */
#define DM35425_ADIO_INT_SAMP_COMPL_MASK		0x0040

/**
 * @brief
 *      Register value for Interrupt Mask - Pacer Clock Tick Occurred
 */
#define DM35425_ADIO_INT_PACER_TICK_MASK		0x0080

/**
 * @brief
 *      Register value for Interrupt Mask - CN3 5V Over-current
 */
#define DM35425_ADIO_INT_CN3_OVER_CURRENT_MASK	0x0100

/**
 * @brief
 *      Register value for Interrupt Mask - CN4 5V Over-current
 */
#define DM35425_ADIO_INT_CN4_OVER_CURRENT_MASK	0x0200

/**
 * @brief
 *      Register value for Interrupt Mask - All Bits
 */
#define DM35425_ADIO_INT_ALL_MASK			0xFFFF


/**
 * @brief
 *      Register value for Clock Event - Disabled
 */
#define DM35425_ADIO_CLK_BUS_SRC_DISABLE			0x00

/**
 * @brief
 *      Register value for Clock Event - Sample Taken
 */
#define DM35425_ADIO_CLK_BUS_SRC_SAMPLE_TAKEN			0x80

/**
 * @brief
 *      Register value for Clock Event - Advanced Interrupt Occurred
 */
#define DM35425_ADIO_CLK_BUS_SRC_ADV_INT			0x81

/**
 * @brief
 *      Register value for Clock Event - Pre-Start Buffer Full
 */
#define DM35425_ADIO_CLK_BUS_SRC_PRE_START_BUFF_FULL		0x82

/**
 * @brief
 *      Register value for Clock Event - Start Trigger Occurred
 */
#define DM35425_ADIO_CLK_BUS_SRC_START_TRIG			0x83

/**
 * @brief
 *      Register value for Clock Event - Stop Trigger Occurred
 */
#define DM35425_ADIO_CLK_BUS_SRC_STOP_TRIG			0x84

/**
 * @brief
 *      Register value for Clock Event - Post-Stop Buffer Full
 */
#define DM35425_ADIO_CLK_BUS_SRC_POST_STOP_BUFF_FULL		0x85

/**
 * @brief
 *      Register value for Clock Event - Sampling Complete
 */
#define DM35425_ADIO_CLK_BUS_SRC_SAMPLING_COMPLETE		0x86

/**
 * @brief
 *      Register value for Parallel Bus Enabled
 */
#define DM35425_ADIO_P_BUS_ENABLED				0x01

/**
 * @brief
 *      Register value for Parallel Bus Disabled
 */
#define DM35425_ADIO_P_BUS_DISABLED				0x00

/**
 * @brief
 *      Register value for Parallel Bus Ready Enabled
 */
#define DM35425_ADIO_P_BUS_READY_ENABLED			0x01

/**
 * @brief
 *      Register value for Parallel Bus Ready Disabled
 */
#define DM35425_ADIO_P_BUS_READY_DISABLED			0x00

/**
 * @brief
 *      DMA Channel number for ADIO IN
 */
#define DM35425_ADIO_IN_DMA_CHANNEL				0

/**
 * @brief
 *      DMA Channel number for ADIO OUT
 */
#define DM35425_ADIO_OUT_DMA_CHANNEL			1

/**
 * @brief
 *      DMA Channel number for ADIO DIR
 */
#define DM35425_ADIO_DIR_DMA_CHANNEL			2

/**
 * @brief
 *      Maximum allowable speed for ADIO
 */
#define DM35425_ADIO_MAX_FREQ				4000000

/*=============================================================================
Enumerations
 =============================================================================*/

 /**
  * @defgroup DM35425_Adio_Library_Enums DM35425 ADIO Public Library Enums
  * @{
 */

/**
  @brief
      Advanced Interrupt Mode of the ADIO.

*/
enum DM35425_Adv_Interrupt_Mode {

	/**
	 * Disabled
	 */
	DM35425_ADV_INT_DISABLED,

	/**
	 * Matching
	 */
	DM35425_ADV_INT_MATCH,

	/**
	 * Event
	 */
	DM35425_ADV_INT_EVENT

};

/**
 * @} DM35425_Adio_Library_Enums
 */

/**
  * @defgroup DM35425_Adio_Library_Functions DM35425 ADIO Public Library Functions
  * @{
 */

/**
*******************************************************************************
@brief
    Open the ADIO indicated, and determine register locations of control
    blocks needed to control it.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    number_of_type

    Which ADIO to open.  The first ADIO on the board will be 0.

@param
    func_block

    Pointer to the function block descriptor.  The descriptor holds the
    information about the function block, including offsets.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Open(struct DM35425_Board_Descriptor *handle,
			unsigned int number_of_type,
			struct DM35425_Function_Block *func_block);


/**
*******************************************************************************
@brief
    Set the ADIO mode to Start

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Start(struct DM35425_Board_Descriptor *handle,
                            		const struct DM35425_Function_Block *func_block);


/**
*******************************************************************************
@brief
    Set the ADIO mode to Start-Rearm

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Start_Rearm(struct DM35425_Board_Descriptor *handle,
                            		const struct DM35425_Function_Block *func_block);


/**
*******************************************************************************
@brief
    Set the ADIO mode to Reset

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Reset(struct DM35425_Board_Descriptor *handle,
                            		const struct DM35425_Function_Block *func_block);


/**
*******************************************************************************
@brief
    Set the ADIO mode to Pause

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Pause(struct DM35425_Board_Descriptor *handle,
                            		const struct DM35425_Function_Block *func_block);


/**
*******************************************************************************
@brief
    Set the ADIO mode to Uninitialized

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Uninitialize(struct DM35425_Board_Descriptor *handle,
                            		const struct DM35425_Function_Block *func_block);


/**
*****************************************************************************
@brief
    Get the ADIO mode-status value.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
	mode_status

    Pointer to the mode_status value to return.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Get_Mode_Status(struct DM35425_Board_Descriptor *handle,
		const struct DM35425_Function_Block *func_block,
		uint8_t *mode_status);


/**
*****************************************************************************
@brief
    Set the clock source for the ADIO.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    source

    Clock source to use for the ADIO.  Consult the user's manual for the list
    of available sources.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL      The clock source selected is not valid.
 */
DM35425LIB_API
int DM35425_Adio_Set_Clock_Src(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					enum DM35425_Clock_Sources source);


/**
*******************************************************************************
@brief
    Set the start trigger for data collection.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    start_trigger

    Trigger to start capturing values.  See the hardware manual for valid
    trigger values.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL      An invalid value was passed for a start trigger
 */

DM35425LIB_API
int DM35425_Adio_Set_Start_Trigger(struct DM35425_Board_Descriptor *handle,
				struct DM35425_Function_Block *func_block,
				uint8_t start_trigger);


/**
*******************************************************************************
@brief
    Set the stop trigger for data collection.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    stop_trigger

    Trigger to stop capturing values.  See the hardware manual for valid
    trigger values.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL      An invalid value was passed for a stop trigger
 */

DM35425LIB_API
int DM35425_Adio_Set_Stop_Trigger(struct DM35425_Board_Descriptor *handle,
				struct DM35425_Function_Block *func_block,
				uint8_t stop_trigger);


/**
*****************************************************************************
@brief
    Set the Clock Divider for the ADIO function block.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    divider

    The requested clock divider.

@retval
    0

    Success.

@retval
    -1

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Set_Clk_Divider(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t divider);


/**
*****************************************************************************
@brief
    Get the Clock Divider Counter for the ADIO function block.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    counter

    Pointer where the counter value will be returned.

@retval
    0

    Success.

@retval
    -1

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Get_Clk_Div_Counter(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t *counter);


/**
*****************************************************************************
@brief
    Set the pacer clock rate, the rate at which conversions happen.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    requested_rate

    The rate being requested (Hz).  It is not always possible to provide the exact
    rate being requested due to the resolution of the divider.

@param
    actual_rate

    Pointer where the returned value of the actual rate achieved (Hz).

@retval
    0

    Success.

@retval
    -1

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Set_Pacer_Clk_Rate(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t requested_rate,
					uint32_t *actual_rate);


/**
*******************************************************************************
@brief
    Set the amount of data to capture prior to start trigger.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    pre_capture_count

    Number of samples to capture prior to the start trigger.

@note
    The amount of data that can be captured prior to the start trigger is
    limited by the size of the FIFO.  Consult the user's manual for this
    information.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL      The size is not within the valid value range.
 */
DM35425LIB_API
int DM35425_Adio_Set_Pre_Trigger_Samples(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t pre_capture_count);


/**
*******************************************************************************
@brief
    Set the amount of data to capture after stop trigger.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    post_capture_count

    Number of samples to capture after the stop trigger.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL      The size is not within the valid value range.
 */
DM35425LIB_API
int DM35425_Adio_Set_Post_Stop_Samples(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t post_capture_count);


/**
*****************************************************************************
@brief
    Get the count of number of samples taken.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    value

    Pointer to returned sample count.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Get_Sample_Count(struct DM35425_Board_Descriptor *handle,
						const struct DM35425_Function_Block *func_block,
						uint32_t *value);


/**
*****************************************************************************
@brief
    Configure the interrupts for the ADIO.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    interrupt_src

    The interrupts to configure.  The bits indicate specific interrupts.  Consult
    the user's manual for a description.

@param
    enable

    Boolean indicating to enable or disable the selected interrupts.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Interrupt_Set_Config(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint16_t interrupt_src,
					int enable);


/**
*****************************************************************************
@brief
    Get the interrupt configuration for the ADIO.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    interrupt_ena

    Pointer to the interrupt configuration register.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Interrupt_Get_Config(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint16_t *interrupt_ena);



/**
*****************************************************************************
@brief
    Get the interrupt status register

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    value

    Pointer to returned interrupt status.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Interrupt_Get_Status(struct DM35425_Board_Descriptor *handle,
						const struct DM35425_Function_Block *func_block,
						uint16_t *value);


/**
*****************************************************************************
@brief
    Clear the interrupt status register

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    value

    Bit mask of which interrupts to clear.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Interrupt_Clear_Status(struct DM35425_Board_Descriptor *handle,
						const struct DM35425_Function_Block *func_block,
						uint16_t value);



/**
*****************************************************************************
@brief
    Set the global clock source for the ADIO.

@param
    handle

    Address of the handle pointer, which will contain the device descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    clock_select

    Which global clock source to set

@param
    clock_source

    Source to set global clock to (what is driving it?)

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL	Invalid clock select or source..

 */
DM35425LIB_API
int DM35425_Adio_Set_Clock_Source_Global(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					enum DM35425_Clock_Sources clock_select,
					int clock_source);


/**
*****************************************************************************
@brief
    Get the global clock source for the selected clock

@param
    handle

    Address of the handle pointer, which will contain the device descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    clock_select

    Which global clock source to get

@param
    clock_source

    Pointer to the returned clock source for the selected global clock

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL	Invalid clock select or source..

 */
DM35425LIB_API
int DM35425_Adio_Get_Clock_Source_Global(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					int clock_select,
					int *clock_source);




/**
*******************************************************************************
@brief
    Get the input value of the ADIO.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    value

    Pointer to returned value.

@note
    The value of pins that are set to output will be zero.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Get_Input_Value(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t *value);



/**
*******************************************************************************
@brief
    Get the current value of the output register.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    value

    Pointer to returned value.

@note
    The value of pins that are set to input will be zero.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Get_Output_Value(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t *value);


/**
*******************************************************************************
@brief
    Set the value to be put on output pins.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    value

    Value to be written to output pins.

@note
    Writing a bit to a pin set to input will have no effect.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Set_Output_Value(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t value);


/**
*******************************************************************************
@brief
    Get the direction of the ADIO pins.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    direction

    Bitmask representing the directions of the pins.  (0 = input, 1 = output)

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Get_Direction(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t *direction);


/**
*******************************************************************************
@brief
    Set the direction of the ADIO pins.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    direction

    Bitmask representing the directions to set the pins.  (0 = input,
    1 = output)

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Set_Direction(struct DM35425_Board_Descriptor *handle,
			const struct DM35425_Function_Block *func_block,
			uint32_t direction);


/**
*******************************************************************************
@brief
    Get the Advanced Interrupt Mode

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    adv_int_mode

    Pointer to the returned value of the advanced interrupt mode register.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Get_Adv_Int_Mode(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint8_t *adv_int_mode);


/**
*******************************************************************************
@brief
    Set the Advanced Interrupt Mode

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    adv_int_mode

    Pointer to the returned value of the advanced interrupt mode register.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Set_Adv_Int_Mode(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint8_t adv_int_mode);

/**
*******************************************************************************
@brief
    Get the Advanced Interrupt Mask

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    adv_int_mask

    Pointer to the returned value of the advanced interrupt mask register.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Get_Adv_Int_Mask(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t *adv_int_mask);


/**
*******************************************************************************
@brief
    Set the Advanced Interrupt Mask

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    adv_int_mask

    Value of the advanced interrupt mask register.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Set_Adv_Int_Mask(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t adv_int_mask);


/**
*******************************************************************************
@brief
    Get the Advanced Interrupt Compare Register

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    adv_int_comp

    Pointer to the returned value of the advanced interrupt compare register.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Get_Adv_Int_Comp(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t *adv_int_comp);


/**
*******************************************************************************
@brief
    Set the Advanced Interrupt Compare Register

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    adv_int_comp

    Value of the advanced interrupt compare register.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Set_Adv_Int_Comp(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t adv_int_comp);


/**
*******************************************************************************
@brief
    Get the Advanced Interrupt Capture Register

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    adv_int_capt

    Pointer to the returned value of the advanced interrupt capture register.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Get_Adv_Int_Capt(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t *adv_int_capt);


/**
*******************************************************************************
@brief
    Set the Advanced Interrupt Capture Register

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    adv_int_capt

    Value of the advanced interrupt compare register.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Set_Adv_Int_Capt(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t adv_int_capt);


/**
*******************************************************************************
@brief
    Get the Parallel Bus Enable Register boolean value

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    p_bus_enabled

    Pointer to the returned value of the parallel bus enable, as a boolean
    (0 = disabled, non-zero = enabled).

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Get_P_Bus_Enable(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					int *p_bus_enabled);

/**
*******************************************************************************
@brief
    Set the Parallel Bus Enable

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    p_bus_enabled

    Boolean value indicating whether to enable the parallel bus or not.
    (0 = disabled, non-zero = enabled).

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Set_P_Bus_Enable(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					int p_bus_enabled);


/**
*******************************************************************************
@brief
    Get the Parallel Bus Ready Enable Register boolean value

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    p_bus_ready_enabled

    Pointer to the returned value of the parallel bus ready enable, as a boolean
    (0 = disabled, non-zero = enabled).

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Get_P_Bus_Ready_Enable(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					int *p_bus_ready_enabled);


/**
*******************************************************************************
@brief
    Set the Parallel Bus Ready Enable

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block representing the ADIO.

@param
    p_bus_ready_enabled

    Boolean value indicating whether to enable the parallel bus ready or not.
    (0 = disabled, non-zero = enabled).

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35425LIB_API
int DM35425_Adio_Set_P_Bus_Ready_Enable(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					int p_bus_ready_enabled);



/**
*******************************************************************************
@brief
    Read an aDIO sample from the FIFO.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    channel

    Channel to get sample from.

@param
    value

    Pointer to returned sample value.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Fifo_Channel_Read(struct DM35425_Board_Descriptor *handle,
                            		const struct DM35425_Function_Block *func_block,
                            		unsigned int channel,
                            		int32_t *value);


/**
*******************************************************************************
@brief
    Write an aDIO sample to the FIFO.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the function block descriptor, which contains the offsets to
    command sections of the board.

@param
    channel

    Channel to write to.

@param
    value

    sample value to write.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n

 */
DM35425LIB_API
int DM35425_Adio_Fifo_Channel_Write(struct DM35425_Board_Descriptor *handle,
                            		const struct DM35425_Function_Block *func_block,
                            		unsigned int channel,
                            		int32_t value);



/**
 * @} DM35425_Adio_Library_Functions
 */

#ifdef __cplusplus
}
#endif

#endif

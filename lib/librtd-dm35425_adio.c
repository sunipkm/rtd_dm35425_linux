/**
	@file

	@brief
		DM35425 ADIO library source code

	$Id: librtd-dm35425_adio.c 103507 2016-10-12 20:58:56Z rgroner $
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

#include <stdint.h>
#include <errno.h>

#include "dm35425_registers.h"
#include "dm35425_gbc_library.h"
#include "dm35425_board_access.h"
#include "dm35425_adio_library.h"
#include "dm35425.h"
#include "dm35425_board_access_structs.h"

/******************************************************************************
 * ADIO Library Functions
 *****************************************************************************/

/*=============================================================================
Private functions
 =============================================================================*/

/**
 * @defgroup DM35425_ADIO_Library_Private_Functions DM35425 ADIO library
 *		private functions source code
 * @{
 */

 /**
  * @internal
  */

/**
*******************************************************************************
@brief
    Validate a clock source.

@param
    clk_src

    The clock source to validate.

@retval
    0

    Success.

@retval
    -1

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL      clock source is not valid.
 *******************************************************************************
 */

static int
DM35425_Adio_Validate_Clock(enum DM35425_Clock_Sources clk_src)
{

	switch (clk_src) {
		/* breaks intentionally omitted */
	case DM35425_CLK_SRC_IMMEDIATE:
	case DM35425_CLK_SRC_NEVER:
	case DM35425_CLK_SRC_BUS2:
	case DM35425_CLK_SRC_BUS3:
	case DM35425_CLK_SRC_BUS4:
	case DM35425_CLK_SRC_BUS5:
	case DM35425_CLK_SRC_BUS6:
	case DM35425_CLK_SRC_BUS7:
	case DM35425_CLK_SRC_CHAN_THRESH:
	case DM35425_CLK_SRC_CHAN_THRESH_INV:
	case DM35425_CLK_SRC_BUS2_INV:
	case DM35425_CLK_SRC_BUS3_INV:
	case DM35425_CLK_SRC_BUS4_INV:
	case DM35425_CLK_SRC_BUS5_INV:
	case DM35425_CLK_SRC_BUS6_INV:
	case DM35425_CLK_SRC_BUS7_INV:
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}

}


/**
*******************************************************************************
@brief
    Validate a clock global select and source.

@param
    select

    The clock global select to validate.

@param
    source

    The clock global source to validate.

@retval
    0

    Success.

@retval
    -1

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL      input is not valid.
 *******************************************************************************
 */

static int
DM35425_Validate_Clock_Global_Source(enum DM35425_Clock_Sources select,
					int source)
{


	switch (source) {
		/* breaks intentionally omitted */
	case DM35425_ADIO_CLK_BUS_SRC_DISABLE:
	case DM35425_ADIO_CLK_BUS_SRC_SAMPLE_TAKEN:
	case DM35425_ADIO_CLK_BUS_SRC_ADV_INT:
	case DM35425_ADIO_CLK_BUS_SRC_PRE_START_BUFF_FULL:
	case DM35425_ADIO_CLK_BUS_SRC_START_TRIG:
	case DM35425_ADIO_CLK_BUS_SRC_STOP_TRIG:
	case DM35425_ADIO_CLK_BUS_SRC_POST_STOP_BUFF_FULL:
	case DM35425_ADIO_CLK_BUS_SRC_SAMPLING_COMPLETE:

		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	switch (select) {
		/* breaks intentionally omitted */
	case DM35425_CLK_SRC_BUS2:
	case DM35425_CLK_SRC_BUS3:
	case DM35425_CLK_SRC_BUS4:
	case DM35425_CLK_SRC_BUS5:
	case DM35425_CLK_SRC_BUS6:
	case DM35425_CLK_SRC_BUS7:
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	return 0;

}


/**
*******************************************************************************
@brief
    Validate an advanced interrupt mode.

@param
    mode

    The mode to validate.

@retval
    0

    Success.

@retval
    -1

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL      input is not valid.
 *******************************************************************************
 */

static int
DM35425_Validate_Adv_Interrupt_Mode(int mode)
{

	switch (mode) {
	case DM35425_ADV_INT_DISABLED:
	/* Breaks intentionally omitted */
	case DM35425_ADV_INT_MATCH:
	case DM35425_ADV_INT_EVENT:
		break;

	default:
		errno = EINVAL;
		return -1;
		break;
	}

	return 0;

}








/**
 * @} DM35425_ADIO_Library_Private_Functions
 */

/*=============================================================================
Public functions
 =============================================================================*/

/**
 * @defgroup DM35425_ADIO_Library_Public_Functions DM35425 ADIO source
 *		code for public library functions
 * @{
 */



DM35425LIB_API
int DM35425_Adio_Open(struct DM35425_Board_Descriptor *handle,
		unsigned int number_of_type,
		struct DM35425_Function_Block *func_block)
{

	return DM35425_Function_Block_Open_Module(handle,
						DM35425_FUNC_BLOCK_ADIO,
						number_of_type,
						func_block);
}




/*******************************************************************************
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
			const struct DM35425_Function_Block *func_block)
{

	union dm35425_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35425_ADIO_MODE_GO_SINGLE_SHOT;


	return DM35425_Write(handle, &ioctl_request);

}


/*******************************************************************************
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
				const struct DM35425_Function_Block *func_block)
{

	union dm35425_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35425_ADIO_MODE_GO_REARM;


	return DM35425_Write(handle, &ioctl_request);

}


/*******************************************************************************
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
			const struct DM35425_Function_Block *func_block)
{

	union dm35425_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35425_ADIO_MODE_RESET;

	return DM35425_Write(handle, &ioctl_request);

}



/*******************************************************************************
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
			const struct DM35425_Function_Block *func_block)
{

	union dm35425_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35425_ADIO_MODE_PAUSE;

	return DM35425_Write(handle, &ioctl_request);

}


/*******************************************************************************
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
		const struct DM35425_Function_Block *func_block)
{

	union dm35425_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35425_ADIO_MODE_UNINITIALIZED;

	return DM35425_Write(handle, &ioctl_request);

}




/*******************************************************************************
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
				uint8_t *mode_status)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_val;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_val = DM35425_Read(handle, &ioctl_request);

	*mode_status = ioctl_request.readwrite.access.data.data8;

	return return_val;
}



/*******************************************************************************
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
				enum DM35425_Clock_Sources source)
{

	union dm35425_ioctl_argument ioctl_request;

	if (DM35425_Adio_Validate_Clock(source)) {
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_CLK_SRC;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = source;

	return DM35425_Write(handle, &ioctl_request);

}


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
				uint8_t trigger)
{


	union dm35425_ioctl_argument ioctl_request;

	if (DM35425_Adio_Validate_Clock(trigger)) {
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_START_TRIG;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = trigger;

	return DM35425_Write(handle, &ioctl_request);

}


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
				uint8_t trigger)
{


	union dm35425_ioctl_argument ioctl_request;

	if (DM35425_Adio_Validate_Clock(trigger)) {
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_STOP_TRIG;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = trigger;

	return DM35425_Write(handle, &ioctl_request);

}


/*******************************************************************************
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
				uint32_t divider)
{
	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_CLK_DIV;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = divider;

	return DM35425_Write(handle, &ioctl_request);
}


/*******************************************************************************
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
				uint32_t *counter)
{
	union dm35425_ioctl_argument ioctl_request;
	int return_result;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_CLK_DIV_COUNTER;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;

	return_result = DM35425_Read(handle, &ioctl_request);
	*counter = ioctl_request.readwrite.access.data.data32;

	return return_result;

}


DM35425LIB_API
int DM35425_Adio_Set_Pacer_Clk_Rate(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t requested_rate,
					uint32_t *actual_rate)
{


	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	uint32_t system_clock_rate;
	int is_std_clk = 0;
	uint32_t divider;


	if ((requested_rate == 0) || (requested_rate > DM35425_ADIO_MAX_FREQ)) {
		errno = EINVAL;
		return -1;
	}

	return_code = DM35425_Gbc_Get_Sys_Clock_Freq(handle,
							&system_clock_rate,
							&is_std_clk);

	if (return_code != 0) {
		return return_code;
	}

	if (!is_std_clk) {
		errno = ENODEV;
		return -1;
	}

	divider = system_clock_rate / requested_rate;
	*actual_rate = system_clock_rate / divider;

	/* The actual formula calls for this minus 1, but we won't do it if
	   divisor would end up less than 1, which is its minimum value.
	*/
	if (divider < 2) {
		divider = 1;
	}
	else {
		divider --;
	}

	if (divider > (uint32_t) system_clock_rate) {
		divider = (uint32_t) system_clock_rate;
	}


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_CLK_DIV;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = divider;
	return DM35425_Write(handle, &ioctl_request);

}



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
    count

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
					uint32_t pre_capture_count)
{

	union dm35425_ioctl_argument ioctl_request;

	if (pre_capture_count > DM35425_FIFO_SAMPLE_SIZE) {
		errno = EINVAL;
		return -1;
	}
	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_PRE_CAPT_COUNT;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = pre_capture_count;

	return DM35425_Write(handle, &ioctl_request);


}


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
    count

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
					uint32_t post_capture_count)
{

	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_POST_CAPT_COUNT;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = post_capture_count;

	return DM35425_Write(handle, &ioctl_request);
}


/*******************************************************************************
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
						uint32_t *value)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_SAMPLE_COUNT;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;


	return_code = DM35425_Read(handle, &ioctl_request);


	*value = ioctl_request.readwrite.access.data.data32;

	return return_code;

}


/*******************************************************************************
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
    int_source

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
					int enable)
{
	union dm35425_ioctl_argument ioctl_request;
    	int return_code;
	uint32_t value = 0;

	if (enable) {
		value = 0xFFFF0000;

		/*
		 * Clear the interrupt status before enabling so we don't
		 * get an interrupt from previous data
		 */
		return_code = DM35425_Adio_Interrupt_Clear_Status(handle, func_block,
		    						interrupt_src);
		if (return_code != 0) {
			return return_code;
		}
	}

	value |= (uint32_t) interrupt_src;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_INT_ENABLE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value;

	return DM35425_Write(handle, &ioctl_request);


}


/*******************************************************************************
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
					uint16_t *interrupt_ena)
{
	union dm35425_ioctl_argument ioctl_request;
	int result = 0;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_INT_ENABLE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;

	result = DM35425_Read(handle, &ioctl_request);

	*interrupt_ena = (ioctl_request.readwrite.access.data.data32 >> 16);

	return result;

}



/*******************************************************************************
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
						uint16_t *value)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_INT_STAT;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_16;


	return_code = DM35425_Read(handle, &ioctl_request);

	*value = ioctl_request.readwrite.access.data.data16;

	return return_code;

}


/*******************************************************************************
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
						uint16_t value)
{

	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_ADIO_INT_STAT;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_16;
	ioctl_request.readwrite.access.data.data16 = value;


	return DM35425_Write(handle, &ioctl_request);


}



/*******************************************************************************
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
					int clock_source)
{

	int return_code = 0;
	union dm35425_ioctl_argument ioctl_request;
	unsigned int offset;

	return_code = DM35425_Validate_Clock_Global_Source(clock_select,
								clock_source);

	if (return_code != 0) {
		return return_code;
	}


	switch (clock_select) {
	case DM35425_CLK_SRC_BUS2:
		offset = DM35425_OFFSET_ADIO_CLK_BUS2;
		break;
	case DM35425_CLK_SRC_BUS3:
		offset = DM35425_OFFSET_ADIO_CLK_BUS3;
		break;
	case DM35425_CLK_SRC_BUS4:
		offset = DM35425_OFFSET_ADIO_CLK_BUS4;
		break;
	case DM35425_CLK_SRC_BUS5:
		offset = DM35425_OFFSET_ADIO_CLK_BUS5;
		break;
	case DM35425_CLK_SRC_BUS6:
		offset = DM35425_OFFSET_ADIO_CLK_BUS6;
		break;
	case DM35425_CLK_SRC_BUS7:
		offset = DM35425_OFFSET_ADIO_CLK_BUS7;
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
						offset;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = (char) clock_source;

	return DM35425_Write(handle, &ioctl_request);
}


/*******************************************************************************
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
					int *clock_source)
{

	int return_code = 0;
	union dm35425_ioctl_argument ioctl_request;
	unsigned int offset;

	switch (clock_select) {
	case DM35425_CLK_SRC_BUS2:
		offset = DM35425_OFFSET_ADIO_CLK_BUS2;
		break;
	case DM35425_CLK_SRC_BUS3:
		offset = DM35425_OFFSET_ADIO_CLK_BUS3;
		break;
	case DM35425_CLK_SRC_BUS4:
		offset = DM35425_OFFSET_ADIO_CLK_BUS4;
		break;
	case DM35425_CLK_SRC_BUS5:
		offset = DM35425_OFFSET_ADIO_CLK_BUS5;
		break;
	case DM35425_CLK_SRC_BUS6:
		offset = DM35425_OFFSET_ADIO_CLK_BUS6;
		break;
	case DM35425_CLK_SRC_BUS7:
		offset = DM35425_OFFSET_ADIO_CLK_BUS7;
		break;
	default:
		return -1;
		break;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
						offset;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_code = DM35425_Read(handle, &ioctl_request);

	*clock_source = (int) ioctl_request.readwrite.access.data.data8;

	return return_code;

}


DM35425LIB_API
int DM35425_Adio_Get_Input_Value(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t *value)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int channel_start_offset = func_block->control_offset +
						DM35425_OFFSET_ADIO_CHAN_START;

	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_INPUT_VAL;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;


	return_code = DM35425_Read(handle, &ioctl_request);

	*value = ioctl_request.readwrite.access.data.data32;

	return return_code;

}


DM35425LIB_API
int DM35425_Adio_Get_Output_Value(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t *value)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int channel_start_offset = func_block->control_offset +
						DM35425_OFFSET_ADIO_CHAN_START;

	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_OUTPUT_VAL;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;


	return_code = DM35425_Read(handle, &ioctl_request);

	*value = ioctl_request.readwrite.access.data.data32;

	return return_code;

}



DM35425LIB_API
int DM35425_Adio_Set_Output_Value(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t value)
{

	union dm35425_ioctl_argument ioctl_request;
	unsigned int channel_start_offset = func_block->control_offset +
						DM35425_OFFSET_ADIO_CHAN_START;

	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_OUTPUT_VAL;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value;

	return DM35425_Write(handle, &ioctl_request);

}



DM35425LIB_API
int DM35425_Adio_Get_Direction(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t *direction)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int channel_start_offset = func_block->control_offset +
					DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_DIRECTION;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;


	return_code = DM35425_Read(handle, &ioctl_request);

	*direction = ioctl_request.readwrite.access.data.data32;

	return return_code;

}



DM35425LIB_API
int DM35425_Adio_Set_Direction(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint32_t direction)
{

	union dm35425_ioctl_argument ioctl_request;
	unsigned int channel_start_offset = func_block->control_offset +
						DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_DIRECTION;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = direction;

	return DM35425_Write(handle, &ioctl_request);

}



DM35425LIB_API
int DM35425_Adio_Get_Adv_Int_Mode(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint8_t *adv_int_mode)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int channel_start_offset = func_block->control_offset +
					DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_ADV_INT_MODE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;


	return_code = DM35425_Read(handle, &ioctl_request);

	*adv_int_mode = ioctl_request.readwrite.access.data.data8;

	return return_code;

}




DM35425LIB_API
int DM35425_Adio_Set_Adv_Int_Mode(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint8_t adv_int_mode)
{

	union dm35425_ioctl_argument ioctl_request;
	unsigned int channel_start_offset;

	if (DM35425_Validate_Adv_Interrupt_Mode(adv_int_mode)) {
		errno = EINVAL;
		return -1;
	}

	channel_start_offset = func_block->control_offset +
						DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_ADV_INT_MODE;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = adv_int_mode;

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Adio_Get_Adv_Int_Mask(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t *adv_int_mask)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int channel_start_offset = func_block->control_offset +
					DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_ADV_INT_MASK;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;


	return_code = DM35425_Read(handle, &ioctl_request);

	*adv_int_mask = ioctl_request.readwrite.access.data.data32;

	return return_code;

}


DM35425LIB_API
int DM35425_Adio_Set_Adv_Int_Mask(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t adv_int_mask)
{

	union dm35425_ioctl_argument ioctl_request;
	unsigned int channel_start_offset;


	channel_start_offset = func_block->control_offset +
						DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_ADV_INT_MASK;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = adv_int_mask;

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Adio_Get_Adv_Int_Comp(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t *adv_int_comp)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int channel_start_offset = func_block->control_offset +
					DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_ADV_INT_COMP;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;


	return_code = DM35425_Read(handle, &ioctl_request);

	*adv_int_comp = ioctl_request.readwrite.access.data.data32;

	return return_code;

}


DM35425LIB_API
int DM35425_Adio_Set_Adv_Int_Comp(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t adv_int_comp)
{

	union dm35425_ioctl_argument ioctl_request;
	unsigned int channel_start_offset;


	channel_start_offset = func_block->control_offset +
						DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_ADV_INT_COMP;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = adv_int_comp;

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Adio_Get_Adv_Int_Capt(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t *adv_int_capt)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int channel_start_offset = func_block->control_offset +
					DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_ADV_INT_CAPT;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;


	return_code = DM35425_Read(handle, &ioctl_request);

	*adv_int_capt = ioctl_request.readwrite.access.data.data32;

	return return_code;

}



DM35425LIB_API
int DM35425_Adio_Set_Adv_Int_Capt(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t adv_int_capt)
{

	union dm35425_ioctl_argument ioctl_request;
	unsigned int channel_start_offset;


	channel_start_offset = func_block->control_offset +
						DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_ADV_INT_CAPT;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = adv_int_capt;

	return DM35425_Write(handle, &ioctl_request);

}



DM35425LIB_API
int DM35425_Adio_Get_P_Bus_Enable(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					int *p_bus_enabled)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int channel_start_offset = func_block->control_offset +
					DM35425_OFFSET_ADIO_CHAN_START;

	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_P_BUS_ENABLE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;


	return_code = DM35425_Read(handle, &ioctl_request);

	*p_bus_enabled = (int) (ioctl_request.readwrite.access.data.data8 ==
				DM35425_ADIO_P_BUS_ENABLED);

	return return_code;

}


DM35425LIB_API
int DM35425_Adio_Set_P_Bus_Enable(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					int p_bus_enabled)
{

	union dm35425_ioctl_argument ioctl_request;
	unsigned int channel_start_offset;


	channel_start_offset = func_block->control_offset +
						DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_P_BUS_ENABLE;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = p_bus_enabled ?
				DM35425_ADIO_P_BUS_ENABLED :
				DM35425_ADIO_P_BUS_DISABLED;

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Adio_Get_P_Bus_Ready_Enable(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					int *p_bus_ready_enabled)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int channel_start_offset = func_block->control_offset +
					DM35425_OFFSET_ADIO_CHAN_START;

	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_P_BUS_READY_ENABLE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;


	return_code = DM35425_Read(handle, &ioctl_request);

	*p_bus_ready_enabled = (int) (ioctl_request.readwrite.access.data.data8 ==
					DM35425_ADIO_P_BUS_READY_ENABLED);

	return return_code;

}



DM35425LIB_API
int DM35425_Adio_Set_P_Bus_Ready_Enable(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					int p_bus_ready_enabled)
{

	union dm35425_ioctl_argument ioctl_request;
	unsigned int channel_start_offset;


	channel_start_offset = func_block->control_offset +
						DM35425_OFFSET_ADIO_CHAN_START;
	ioctl_request.readwrite.access.offset = channel_start_offset +
						DM35425_OFFSET_ADIO_P_BUS_READY_ENABLE;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = p_bus_ready_enabled ?
						DM35425_ADIO_P_BUS_READY_ENABLED:
						DM35425_ADIO_P_BUS_READY_DISABLED;

	return DM35425_Write(handle, &ioctl_request);

}

DM35425LIB_API
int DM35425_Adio_Fifo_Channel_Read(struct DM35425_Board_Descriptor *handle,
                            		const struct DM35425_Function_Block *func_block,
                            		unsigned int channel,
                            		int32_t *value)
{
	union dm35425_ioctl_argument ioctl_request;
	int return_code = 0;

	if (channel >= func_block->num_dma_channels) {
		errno = EINVAL;
		return -1;
	}

	// Check that the function block revision ID is valid for direct
	// FIFO access.
	if(func_block->type_revision < DM35425_FIFO_ACCESS_FB_REVISION)
	{
		errno = EPERM;
		return -1;
	}

	// Set up the IOCTL struct
	ioctl_request.readwrite.access.offset = func_block->fb_offset +
		DM35425_OFFSET_ADIO_FIFO_CTRL_BLK_START +
		(channel * DM35425_OFFSET_ADIO_FIFO_CTRL_BLK_SIZE);
		ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
		ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;

	// Perform the read
	return_code = DM35425_Read(handle, &ioctl_request);

	*value = (int32_t)ioctl_request.readwrite.access.data.data32;

	return return_code;
}


DM35425LIB_API
int DM35425_Adio_Fifo_Channel_Write(struct DM35425_Board_Descriptor *handle,
                            		const struct DM35425_Function_Block *func_block,
                            		unsigned int channel,
                            		int32_t value)
{
	union dm35425_ioctl_argument ioctl_request;
	int return_code = 0;

	if (channel >= func_block->num_dma_channels) {
		errno = EINVAL;
		return -1;
	}

	// Check that the function block revision ID is valid for direct
	// FIFO access.
	if(func_block->type_revision < DM35425_FIFO_ACCESS_FB_REVISION)
	{
		errno = EPERM;
		return -1;
	}

	// Set up the IOCTL struct
	ioctl_request.readwrite.access.offset = func_block->fb_offset +
		DM35425_OFFSET_ADIO_FIFO_CTRL_BLK_START +
		(channel * DM35425_OFFSET_ADIO_FIFO_CTRL_BLK_SIZE);
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value;

	// Perform the write
	return_code = DM35425_Write(handle, &ioctl_request);

	return return_code;
}


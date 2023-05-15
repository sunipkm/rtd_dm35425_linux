/**
	@file

	@brief
		DM35425 DAC library source code


	$Id: librtd-dm35425_dac.c 103507 2016-10-12 20:58:56Z rgroner $
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "dm35425_registers.h"
#include "dm35425_gbc_library.h"
#include "dm35425.h"
#include "dm35425_util_library.h"
#include "dm35425_dac_library.h"
#include "dm35425_types.h"
#include "dm35425_board_access_structs.h"
#include "dm35425_board_access.h"

/******************************************************************************
 * DAC Library Functions
 *****************************************************************************/
/**
 * @defgroup DM35425_DAC_Library_Source DM35425 DAC library source code
 * @{
 */

/*=============================================================================
Private functions
 =============================================================================*/

/**
 * @defgroup DM35425_DAC_Library_Private_Functions DM35425 DAC library private functions source code
 * @{
 */

 /**
  * @internal
  */


/**
*******************************************************************************
@brief
    Validate the passed in trigger value

@param
    trigger

    The clock trigger value to validate

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL	Invalid trigger value

 */
int
DM35425_Dac_Validate_Trigger(uint8_t trigger)
{

	switch (trigger) {
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
    Validate the passed in clock select and source

@param
    clock_select

    The clock select to validate.

@param
    clock_driver

    The clock driver to validate.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL	Invalid clock select or source

 */
int
DM35425_Dac_Validate_Clock_Global_Src_Args(enum DM35425_Clock_Sources clock_select,
						enum DM35425_Dac_Clock_Events clock_driver)
{

	switch (clock_select) {
	case DM35425_CLK_SRC_BUS2:
		/* breaks intentionally omitted */
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

	switch (clock_driver) {
	case DM35425_DAC_CLK_BUS_SRC_DISABLE:
		/* breaks intentionally omitted */
	case DM35425_DAC_CLK_BUS_SRC_CONVERSION_SENT:
	case DM35425_DAC_CLK_BUS_SRC_CHAN_MARKER:
	case DM35425_DAC_CLK_BUS_SRC_START_TRIG:
	case DM35425_DAC_CLK_BUS_SRC_STOP_TRIG:
	case DM35425_DAC_CLK_BUS_SRC_CONV_COMPL:
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
    Validate the output range

@param
    range

    The desired output range of the DAC.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL	Invalid range selected

 */
int
DM35425_Dac_Validate_Output_Range(enum DM35425_Output_Ranges range)
{


	switch (range) {
	/* Breaks intentionally omitted */
	case DM35425_DAC_RNG_UNIPOLAR_5V:
	case DM35425_DAC_RNG_UNIPOLAR_10V:
	case DM35425_DAC_RNG_BIPOLAR_5V:
	case DM35425_DAC_RNG_BIPOLAR_10V:
		break;


	default:
		errno = EINVAL;
		return -1;
		break;
	}

	return 0;
}


static int
DM35425_Dac_Validate_Conv(enum DM35425_Output_Ranges range,
				int16_t dac_conversion)
{

	/**
	 * The DAC conversion value is offset, such that no matter
	 * the polarity, the value will always be between the
	 * unipolar min and max
	 */
	if (dac_conversion < DM35425_DAC_MIN ||
	    dac_conversion > DM35425_DAC_MAX) {
		errno = ERANGE;
		return -1;
	}

	return 0;

}


/**
 * @} DM35425_DAC_Library_Private_Functions
 */

/*=============================================================================
Public functions
 =============================================================================*/

/**
 * @defgroup DM35425_DAC_Library_Public_Functions DM35425 DAC source
 *		code for public library functions
 * @{
 */



DM35425LIB_API
int DM35425_Dac_Open(struct DM35425_Board_Descriptor *handle,
			unsigned int number_of_type,
			struct DM35425_Function_Block *func_block)
{

	return DM35425_Function_Block_Open_Module(handle,
						DM35425_FUNC_BLOCK_DAC,
						number_of_type,
						func_block);

}




DM35425LIB_API
int DM35425_Dac_Set_Clock_Src(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				enum DM35425_Clock_Sources source)
{

	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_CLK_SRC;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = (uint8_t) source;

	return DM35425_Write(handle, &ioctl_request);


}


DM35425LIB_API
int DM35425_Dac_Get_Clock_Src(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				enum DM35425_Clock_Sources *source)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_value;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_CLK_SRC;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_value = DM35425_Read(handle, &ioctl_request);

	*source = (enum DM35425_Clock_Sources) ioctl_request.readwrite.access.data.data8;

	return return_value;

}


DM35425LIB_API
int DM35425_Dac_Get_Clock_Div(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t *divider)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_CLK_DIV;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;

	return_code = DM35425_Read(handle, &ioctl_request);

	*divider = ioctl_request.readwrite.access.data.data32;

	return return_code;
}


DM35425LIB_API
int DM35425_Dac_Set_Clock_Div(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t divider)
{

	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_CLK_DIV;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = divider;

	return DM35425_Write(handle, &ioctl_request);
}



DM35425LIB_API
int DM35425_Dac_Set_Conversion_Rate(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint32_t requested_rate,
					uint32_t *actual_rate)
{


	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	uint32_t system_clock_rate;
	uint32_t divider;
	int is_std_clk = 1;

	if ((requested_rate == 0) || (requested_rate > DM35425_DAC_MAX_RATE)) {
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

	if (divider > system_clock_rate) {
		divider = system_clock_rate;
	}


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_CLK_DIV;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = divider;
	return DM35425_Write(handle, &ioctl_request);

}

DM35425LIB_API
int DM35425_Dac_Interrupt_Set_Config(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint16_t interrupt_src,
					int enable)
{
	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	uint32_t value = 0;

	if (enable) {
		value = (uint32_t) 0xFFFF0000;

		// clear the interrupt status before enabling so we don't get an interrupt
		// from previous data
		return_code = DM35425_Dac_Interrupt_Clear_Status(handle, func_block,
		    							interrupt_src);
		if (return_code != 0) {
			return return_code;
		}
	}

	value |= (uint32_t) interrupt_src;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_INT_ENABLE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value;

	return DM35425_Write(handle, &ioctl_request);


}

DM35425LIB_API
int DM35425_Dac_Interrupt_Get_Config(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint16_t *interrupt_ena)
{
	union dm35425_ioctl_argument ioctl_request;
	int result = 0;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_INT_ENABLE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;

	result = DM35425_Read(handle, &ioctl_request);

	*interrupt_ena = (ioctl_request.readwrite.access.data.data32 >> 16);

	return result;

}


DM35425LIB_API
int DM35425_Dac_Set_Start_Trigger(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint8_t trigger_value)
{

	union dm35425_ioctl_argument ioctl_request;

	if (DM35425_Dac_Validate_Trigger(trigger_value)) {
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_START_TRIG;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = trigger_value;

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Dac_Set_Stop_Trigger(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint8_t trigger_value)
{

	union dm35425_ioctl_argument ioctl_request;

	if (DM35425_Dac_Validate_Trigger(trigger_value)) {
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_STOP_TRIG;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = trigger_value;

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Dac_Get_Start_Trigger(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint8_t *trigger_value)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_START_TRIG;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_code = DM35425_Read(handle, &ioctl_request);

	*trigger_value = ioctl_request.readwrite.access.data.data8;

	return return_code;

}


DM35425LIB_API
int DM35425_Dac_Get_Stop_Trigger(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					uint8_t *trigger_value)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_STOP_TRIG;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_code = DM35425_Read(handle, &ioctl_request);

	*trigger_value = ioctl_request.readwrite.access.data.data8;

	return return_code;
}


DM35425LIB_API
int DM35425_Dac_Start(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block)
{

	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35425_DAC_MODE_GO_SINGLE_SHOT;

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Dac_Reset(struct DM35425_Board_Descriptor *handle,
		const struct DM35425_Function_Block *func_block)
{

	union dm35425_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35425_DAC_MODE_RESET;

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Dac_Pause(struct DM35425_Board_Descriptor *handle,
		const struct DM35425_Function_Block *func_block)
{

	union dm35425_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35425_DAC_MODE_PAUSE;

	return DM35425_Write(handle, &ioctl_request);

}



DM35425LIB_API
int DM35425_Dac_Get_Mode_Status(struct DM35425_Board_Descriptor *handle,
				const struct DM35425_Function_Block *func_block,
				uint8_t *mode_status)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_val;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_val = DM35425_Read(handle, &ioctl_request);

	*mode_status = ioctl_request.readwrite.access.data.data8;

	return return_val;
}


DM35425LIB_API
int DM35425_Dac_Get_Last_Conversion(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					unsigned int channel,
					uint8_t *marker,
					int16_t *value)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35425_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35425_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35425_OFFSET_DAC_CHAN_LAST_CONVERSION;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;


	return_code = DM35425_Read(handle, &ioctl_request);

	*value = (int16_t) (ioctl_request.readwrite.access.data.data32 & 0x0000FFFF);
	*marker = (uint8_t) (ioctl_request.readwrite.access.data.data32 >> 24);

	return return_code;

}


DM35425LIB_API
int DM35425_Dac_Set_Last_Conversion(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					unsigned int channel,
					uint8_t marker,
					int16_t value)
{

	union dm35425_ioctl_argument ioctl_request;
	uint32_t value_to_write;

	value_to_write = (uint32_t) marker;

	value_to_write <<= 24;
	value_to_write |= (uint32_t) value;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35425_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35425_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35425_OFFSET_DAC_CHAN_LAST_CONVERSION;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value_to_write;

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Dac_Get_Conversion_Count(struct DM35425_Board_Descriptor *handle,
						const struct DM35425_Function_Block *func_block,
						uint32_t *value)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_CONV_COUNT;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;


	return_code = DM35425_Read(handle, &ioctl_request);

	*value = ioctl_request.readwrite.access.data.data32;

	return return_code;

}


DM35425LIB_API
int DM35425_Dac_Interrupt_Get_Status(struct DM35425_Board_Descriptor *handle,
						const struct DM35425_Function_Block *func_block,
						uint16_t *value)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_INT_STAT;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_16;



	return_code = DM35425_Read(handle, &ioctl_request);

	*value = ioctl_request.readwrite.access.data.data16;

	return return_code;

}


DM35425LIB_API
int DM35425_Dac_Interrupt_Clear_Status(struct DM35425_Board_Descriptor *handle,
						const struct DM35425_Function_Block *func_block,
						uint16_t value)
{

	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_INT_STAT;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_16;
	ioctl_request.readwrite.access.data.data16 = value;


	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Dac_Set_Post_Stop_Conversion_Count(struct DM35425_Board_Descriptor *handle,
						const struct DM35425_Function_Block *func_block,
						uint32_t value)
{

	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_POST_STOP_CONV;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value;


	return DM35425_Write(handle, &ioctl_request);

}

DM35425LIB_API
int DM35425_Dac_Get_Post_Stop_Conversion_Count(struct DM35425_Board_Descriptor *handle,
						const struct DM35425_Function_Block *func_block,
						uint32_t *value)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_value;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35425_OFFSET_DAC_POST_STOP_CONV;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;

	return_value = DM35425_Read(handle, &ioctl_request);

	*value = ioctl_request.readwrite.access.data.data32;

	return return_value;
}




DM35425LIB_API
int DM35425_Dac_Set_Clock_Source_Global(struct DM35425_Board_Descriptor *handle,
					const struct DM35425_Function_Block *func_block,
					enum DM35425_Clock_Sources clock,
					enum DM35425_Dac_Clock_Events clock_driver)
{

	union dm35425_ioctl_argument ioctl_request;
	int result = 0;
	unsigned int offset = 0;

	result = DM35425_Dac_Validate_Clock_Global_Src_Args(clock,
								clock_driver);

	if (result) {
		return result;
	}

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = (uint8_t) clock_driver;

	switch (clock) {
	case DM35425_CLK_SRC_BUS2:
		offset = DM35425_OFFSET_DAC_CLK_BUS2;
		break;
	case DM35425_CLK_SRC_BUS3:
		offset = DM35425_OFFSET_DAC_CLK_BUS3;
		break;
	case DM35425_CLK_SRC_BUS4:
		offset = DM35425_OFFSET_DAC_CLK_BUS4;
		break;
	case DM35425_CLK_SRC_BUS5:
		offset = DM35425_OFFSET_DAC_CLK_BUS5;
		break;
	case DM35425_CLK_SRC_BUS6:
		offset = DM35425_OFFSET_DAC_CLK_BUS6;
		break;
	case DM35425_CLK_SRC_BUS7:
		offset = DM35425_OFFSET_DAC_CLK_BUS7;
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset + offset;


	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Dac_Channel_Setup(struct DM35425_Board_Descriptor *handle,
					struct DM35425_Function_Block *func_block,
					unsigned int channel,
					enum DM35425_Output_Ranges output_range)
{

	union dm35425_ioctl_argument ioctl_request;
	uint32_t fe_config;
	uint16_t fe_data;

	if (DM35425_Dac_Validate_Output_Range(output_range) ||
		channel >= DM35425_NUM_DAC_DMA_CHANNELS) {
		errno = EINVAL;
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35425_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35425_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35425_OFFSET_DAC_CHAN_FRONT_END_CONFIG;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;

	switch (output_range) {
	case DM35425_DAC_RNG_BIPOLAR_5V:
		fe_data = DM35425_DAC_FE_CONFIG_GAIN_1 |
			  DM35425_DAC_FE_CONFIG_BIPOLAR;
		break;
	case DM35425_DAC_RNG_BIPOLAR_10V:
		fe_data = DM35425_DAC_FE_CONFIG_GAIN_2 |
			  DM35425_DAC_FE_CONFIG_BIPOLAR;
		break;
	case DM35425_DAC_RNG_UNIPOLAR_5V:
		fe_data = DM35425_DAC_FE_CONFIG_GAIN_1 |
			  DM35425_DAC_FE_CONFIG_UNIPOLAR;
		break;
	default:
		fe_data = DM35425_DAC_FE_CONFIG_GAIN_2 |
			  DM35425_DAC_FE_CONFIG_UNIPOLAR;
		break;
	}

	fe_config = DM35425_Get_Maskable(DM35425_DAC_FE_CONFIG_OUTPUT_ENABLE |
					     fe_data,
					     DM35425_DAC_FE_CONFIG_ENABLE_MASK |
					     DM35425_DAC_FE_CONFIG_GAIN_MASK |
					     DM35425_DAC_FE_CONFIG_POLARITY_MASK);

	ioctl_request.readwrite.access.data.data32 = fe_config;

	return DM35425_Write(handle, &ioctl_request);

}




DM35425LIB_API
int DM35425_Dac_Channel_Reset(struct DM35425_Board_Descriptor *handle,
				struct DM35425_Function_Block *func_block,
				unsigned int channel)
{

	union dm35425_ioctl_argument ioctl_request;

	if (channel >= DM35425_NUM_DAC_DMA_CHANNELS) {
		errno = EINVAL;
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35425_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35425_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35425_OFFSET_DAC_CHAN_FRONT_END_CONFIG;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = DM35425_Get_Maskable(0,
										0xFFFF);

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Dac_Channel_Set_Marker_Config(struct DM35425_Board_Descriptor *handle,
					struct DM35425_Function_Block *func_block,
					unsigned int channel,
					uint8_t marker_enable)
{

	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35425_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35425_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35425_OFFSET_DAC_CHAN_MARKER_ENABLE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	ioctl_request.readwrite.access.data.data8 = marker_enable;

	return DM35425_Write(handle, &ioctl_request);

}



DM35425LIB_API
int DM35425_Dac_Channel_Get_Marker_Config(struct DM35425_Board_Descriptor *handle,
					struct DM35425_Function_Block *func_block,
					unsigned int channel,
					uint8_t *marker_enable)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_value;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35425_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35425_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35425_OFFSET_DAC_CHAN_MARKER_ENABLE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_value = DM35425_Read(handle, &ioctl_request);

	*marker_enable = ioctl_request.readwrite.access.data.data8;

	return return_value;

}




DM35425LIB_API
int DM35425_Dac_Channel_Get_Marker_Status(struct DM35425_Board_Descriptor *handle,
					struct DM35425_Function_Block *func_block,
					unsigned int channel,
					uint8_t *marker_status)
{

	union dm35425_ioctl_argument ioctl_request;
	int status = 0;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35425_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35425_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35425_OFFSET_DAC_CHAN_MARKER_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;


	status = DM35425_Read(handle, &ioctl_request);

	*marker_status = ioctl_request.readwrite.access.data.data8;

	return status;

}




DM35425LIB_API
int DM35425_Dac_Channel_Clear_Marker_Status(struct DM35425_Board_Descriptor *handle,
					struct DM35425_Function_Block *func_block,
					unsigned int channel,
					uint8_t marker_to_clear)
{

	union dm35425_ioctl_argument ioctl_request;
	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35425_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35425_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35425_OFFSET_DAC_CHAN_MARKER_STATUS;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	ioctl_request.readwrite.access.data.data8 = marker_to_clear;

	return DM35425_Write(handle, &ioctl_request);

}


DM35425LIB_API
int DM35425_Dac_Fifo_Channel_Write(struct DM35425_Board_Descriptor *handle,
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
					    DM35425_OFFSET_DAC_FIFO_CTRL_BLK_START +
					    (channel * DM35425_OFFSET_DAC_FIFO_CTRL_BLK_SIZE);
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value;

	// Perform the write
	return_code = DM35425_Write(handle, &ioctl_request);

	return return_code;
}


DM35425LIB_API
int DM35425_Dac_Volts_To_Conv(
				enum DM35425_Output_Ranges output_range,
				float volts,
				int16_t *dac_conversion)
{


	float lsb;
    int32_t temp_value;
	int16_t offset = DM35425_DAC_BIPOLAR_OFFSET;

	switch (output_range) {
	case DM35425_DAC_RNG_BIPOLAR_10V:
		lsb = DM35425_DAC_RNG_20_LSB;
		break;

	case DM35425_DAC_RNG_UNIPOLAR_10V:
		lsb = DM35425_DAC_RNG_10_LSB;
		offset = DM35425_DAC_UNIPOLAR_OFFSET;
		break;

	case DM35425_DAC_RNG_BIPOLAR_5V:
		lsb = DM35425_DAC_RNG_10_LSB;
		break;

	case DM35425_DAC_RNG_UNIPOLAR_5V:
		lsb = DM35425_DAC_RNG_5_LSB;
		offset = DM35425_DAC_UNIPOLAR_OFFSET;
		break;

	default:
		errno = EINVAL;
		return 1;
		break;

	}

	temp_value = ((int16_t) (volts / lsb)) + offset;

	/*
	 * Verify that the voltage being asked for isn't outside of the
	 * possible range of the DAC
	 */
	if (DM35425_Dac_Validate_Conv(output_range,
					*dac_conversion))
	{
		errno = EINVAL;
		return -1;
	}
    *dac_conversion = (int16_t) temp_value;

	return 0;
}


DM35425LIB_API
int DM35425_Dac_Conv_To_Volts(
				enum DM35425_Output_Ranges output_range,
				int16_t conversion,
				float *volts)
{

	float lsb;
	int16_t offset = DM35425_DAC_BIPOLAR_OFFSET;

	if (DM35425_Dac_Validate_Conv(output_range,
					conversion))
	{
		errno = ERANGE;
		return -1;
	}

	switch (output_range) {
	case DM35425_DAC_RNG_BIPOLAR_10V:
		lsb = DM35425_DAC_RNG_20_LSB;
		break;

	case DM35425_DAC_RNG_UNIPOLAR_10V:
		lsb = DM35425_DAC_RNG_10_LSB;
		offset = DM35425_DAC_UNIPOLAR_OFFSET;
		break;
	case DM35425_DAC_RNG_BIPOLAR_5V:
		lsb = DM35425_DAC_RNG_10_LSB;
		break;

	case DM35425_DAC_RNG_UNIPOLAR_5V:
		lsb = DM35425_DAC_RNG_5_LSB;
		offset = DM35425_DAC_UNIPOLAR_OFFSET;
		break;

	default:
		errno = EINVAL;
		return 1;
		break;

	}
	*volts = lsb * ((float) (conversion - offset));


	return 0;
}

/**
 * @} DM35425_DAC_Library_Public_Functions
 */

/**
* @} DM35425_DAC_Library_Source
*/



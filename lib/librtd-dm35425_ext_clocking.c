/**
	@file

	@brief
		DM35425 Global Clocking library source code

	$Id: librtd-dm35425_ext_clocking.c 103459 2016-10-12 14:05:11Z rgroner $
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
#include "dm35425_board_access_structs.h"
#include "dm35425_board_access.h"
#include "dm35425_ext_clocking_library.h"

/******************************************************************************
 * DIO Library Functions
 *****************************************************************************/

DM35425LIB_API
    int DM35425_Ext_Clocking_Open(struct DM35425_Board_Descriptor *handle,
				  unsigned int number_of_type,
				  struct DM35425_Function_Block *func_block)
{

	return DM35425_Function_Block_Open_Module(handle,
						  DM35425_FUNC_BLOCK_EXT_CLOCKING,
						  number_of_type, func_block);
}

DM35425LIB_API
    int DM35425_Ext_Clocking_Get_In(struct DM35425_Board_Descriptor *handle,
				    struct DM35425_Function_Block *func_block,
				    uint8_t * clk_curr_val)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
	    DM35425_OFFSET_EXT_CLOCKING_IN;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_code = DM35425_Read(handle, &ioctl_request);

	*clk_curr_val = ioctl_request.readwrite.access.data.data8;

	return return_code;

}

DM35425LIB_API
    int DM35425_Ext_Clocking_Get_Gate_In(struct DM35425_Board_Descriptor
					 *handle,
					 struct DM35425_Function_Block
					 *func_block, uint8_t * gate_curr_val)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
	    DM35425_OFFSET_EXT_CLOCKING_GATE_IN;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_code = DM35425_Read(handle, &ioctl_request);

	*gate_curr_val = ioctl_request.readwrite.access.data.data8;

	return return_code;

}

DM35425LIB_API
    int DM35425_Ext_Clocking_Get_Dir(struct DM35425_Board_Descriptor *handle,
				     struct DM35425_Function_Block *func_block,
				     uint8_t * clk_dir)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
	    DM35425_OFFSET_EXT_CLOCKING_DIR;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_code = DM35425_Read(handle, &ioctl_request);

	*clk_dir = ioctl_request.readwrite.access.data.data8;

	return return_code;

}

DM35425LIB_API
    int DM35425_Ext_Clocking_Set_Dir(struct DM35425_Board_Descriptor *handle,
				     const struct DM35425_Function_Block
				     *func_block, uint8_t clk_dir)
{

	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
	    DM35425_OFFSET_EXT_CLOCKING_DIR;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = clk_dir;

	return DM35425_Write(handle, &ioctl_request);

}

DM35425LIB_API
    int DM35425_Ext_Clocking_Get_Edge(struct DM35425_Board_Descriptor *handle,
				      struct DM35425_Function_Block *func_block,
				      uint8_t * clk_gbl_edge)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
	    DM35425_OFFSET_EXT_CLOCKING_EDGE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	return_code = DM35425_Read(handle, &ioctl_request);

	*clk_gbl_edge = ioctl_request.readwrite.access.data.data8;

	return return_code;

}

DM35425LIB_API
    int DM35425_Ext_Clocking_Set_Edge(struct DM35425_Board_Descriptor *handle,
				      const struct DM35425_Function_Block
				      *func_block, uint8_t clk_gbl_edge)
{

	union dm35425_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
	    DM35425_OFFSET_EXT_CLOCKING_EDGE;
	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = clk_gbl_edge;

	return DM35425_Write(handle, &ioctl_request);

}

DM35425LIB_API
    int DM35425_Ext_Clocking_Get_Pulse_Width(struct DM35425_Board_Descriptor
					     *handle,
					     struct DM35425_Function_Block
					     *func_block,
					     enum DM35425_Clock_Sources
					     clock_src,
					     uint8_t * clk_pulse_width)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int reg_offset = 0;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	switch (clock_src) {
	case DM35425_CLK_SRC_BUS2:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW2;
		break;
	case DM35425_CLK_SRC_BUS3:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW3;
		break;
	case DM35425_CLK_SRC_BUS4:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW4;
		break;
	case DM35425_CLK_SRC_BUS5:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW5;
		break;
	case DM35425_CLK_SRC_BUS6:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW6;
		break;
	case DM35425_CLK_SRC_BUS7:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW7;
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}
	ioctl_request.readwrite.access.offset =
	    func_block->control_offset + reg_offset;
	return_code = DM35425_Read(handle, &ioctl_request);

	*clk_pulse_width = ioctl_request.readwrite.access.data.data8;

	return return_code;

}

DM35425LIB_API
    int DM35425_Ext_Clocking_Set_Pulse_Width(struct DM35425_Board_Descriptor
					     *handle,
					     struct DM35425_Function_Block
					     *func_block,
					     enum DM35425_Clock_Sources
					     clock_src, uint8_t clk_pulse_width)
{

	union dm35425_ioctl_argument ioctl_request;
	unsigned int reg_offset = 0;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	switch (clock_src) {
	case DM35425_CLK_SRC_BUS2:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW2;
		break;
	case DM35425_CLK_SRC_BUS3:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW3;
		break;
	case DM35425_CLK_SRC_BUS4:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW4;
		break;
	case DM35425_CLK_SRC_BUS5:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW5;
		break;
	case DM35425_CLK_SRC_BUS6:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW6;
		break;
	case DM35425_CLK_SRC_BUS7:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_PW7;
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}
	ioctl_request.readwrite.access.offset =
	    func_block->control_offset + reg_offset;
	ioctl_request.readwrite.access.data.data8 = clk_pulse_width;

	return DM35425_Write(handle, &ioctl_request);

}

DM35425LIB_API
    int DM35425_Ext_Clocking_Get_Method(struct DM35425_Board_Descriptor *handle,
					struct DM35425_Function_Block
					*func_block,
					enum DM35425_Clock_Sources clock_src,
					enum DM35425_Ext_Clocking_Method
					*clocking_method)
{

	union dm35425_ioctl_argument ioctl_request;
	int return_code;
	unsigned int reg_offset = 0;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	switch (clock_src) {
	case DM35425_CLK_SRC_BUS2:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL2;
		break;
	case DM35425_CLK_SRC_BUS3:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL3;
		break;
	case DM35425_CLK_SRC_BUS4:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL4;
		break;
	case DM35425_CLK_SRC_BUS5:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL5;
		break;
	case DM35425_CLK_SRC_BUS6:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL6;
		break;
	case DM35425_CLK_SRC_BUS7:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL7;
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}
	ioctl_request.readwrite.access.offset =
	    func_block->control_offset + reg_offset;
	return_code = DM35425_Read(handle, &ioctl_request);

	*clocking_method = (enum DM35425_Ext_Clocking_Method)
	    ioctl_request.readwrite.access.data.data8;

	return return_code;

}

DM35425LIB_API
    int DM35425_Ext_Clocking_Set_Method(struct DM35425_Board_Descriptor *handle,
					struct DM35425_Function_Block
					*func_block,
					enum DM35425_Clock_Sources clock_src,
					enum DM35425_Ext_Clocking_Method
					clocking_method)
{

	union dm35425_ioctl_argument ioctl_request;
	unsigned int reg_offset = 0;

	ioctl_request.readwrite.access.region = DM35425_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35425_PCI_REGION_ACCESS_8;

	switch (clocking_method) {
	case DM35425_EXT_CLOCKING_DISABLED:
		/* Breaks intentionally omitted */
	case DM35425_EXT_CLOCKING_NOT_GATED:
	case DM35425_EXT_CLOCKING_GATED_HIGH:
	case DM35425_EXT_CLOCKING_GATED_LOW:
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	switch (clock_src) {
	case DM35425_CLK_SRC_BUS2:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL2;
		break;
	case DM35425_CLK_SRC_BUS3:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL3;
		break;
	case DM35425_CLK_SRC_BUS4:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL4;
		break;
	case DM35425_CLK_SRC_BUS5:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL5;
		break;
	case DM35425_CLK_SRC_BUS6:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL6;
		break;
	case DM35425_CLK_SRC_BUS7:
		reg_offset = DM35425_OFFSET_EXT_CLOCKING_SETUP_GBL7;
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	ioctl_request.readwrite.access.offset =
	    func_block->control_offset + reg_offset;
	ioctl_request.readwrite.access.data.data8 = clocking_method;

	return DM35425_Write(handle, &ioctl_request);

}

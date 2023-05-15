/**
    @file

    @brief
        Definitions for the DM35425 External Clocking Library.

    $Id: dm35425_ext_clocking_library.h 60276 2012-06-05 16:04:15Z rgroner $
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

#ifndef _DM35425_EXT_CLOCKING_LIBRARY__H_
#define _DM35425_EXT_CLOCKING_LIBRARY__H_

#include "dm35425_gbc_library.h"

#ifdef __cplusplus
extern "C" {
#endif

 /**
  * @defgroup DM35425_Ext_Clocking_Library_Constants DM35425 Global Clocking Library Constants
  * @{
  */

	enum DM35425_Ext_Clocking_Method {

		DM35425_EXT_CLOCKING_DISABLED = 0x00,

		DM35425_EXT_CLOCKING_NOT_GATED = 0x80,

		DM35425_EXT_CLOCKING_GATED_HIGH = 0x81,

		DM35425_EXT_CLOCKING_GATED_LOW = 0x82
	};

 /**
  * @} DM35425_Ext_Clocking_Library_Constants
  */

 /**
  * @defgroup DM35425_Ext_Clocking_Library_Functions DM35425 Global Clocking Library Public Functions
  * @{
  */

/**
*******************************************************************************
@brief
   Open the Global Clocking functional block, making it available for operations.

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   number_of_type

   Ordinal value of external clock function block to open (0th, 1st, etc).

@param
   func_block

   Pointer to the returned function block descriptor.

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Open(struct DM35425_Board_Descriptor
					  *handle, unsigned int number_of_type,
					  struct DM35425_Function_Block
					  *func_block);

/**
*******************************************************************************
@brief
   Get the current value on the external clocking pins

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   func_block

   Pointer to the returned function block descriptor.

@param
   clk_curr_val

   Pointer to the returned current value on the external clocking pins.

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Get_In(struct DM35425_Board_Descriptor
					    *handle,
					    struct DM35425_Function_Block
					    *func_block,
					    uint8_t * clk_curr_val);

/**
*******************************************************************************
@brief
   Get the current value on the external clocking gate pins

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   func_block

   Pointer to the returned function block descriptor.

@param
   gate_curr_val

   Pointer to the returned current value of the external clocking gate pins.

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Get_Gate_In(struct DM35425_Board_Descriptor
						 *handle,
						 struct DM35425_Function_Block
						 *func_block,
						 uint8_t * gate_curr_val);

/**
*******************************************************************************
@brief
   Get the current value of the external clocking pin direction

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   func_block

   Pointer to the returned function block descriptor.

@param
   dir

   Pointer to the returned value of the external clocking pin directions.
   (0 = input, 1 = output)

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Get_Dir(struct DM35425_Board_Descriptor
					     *handle,
					     struct DM35425_Function_Block
					     *func_block, uint8_t * dir);

/**
*******************************************************************************
@brief
   Set the current value of the external clocking pin direction

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   func_block

   Pointer to the returned function block descriptor.

@param
   dir

   Value of the external clocking pin directions.
   (0 = input, 1 = output)

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Set_Dir(struct DM35425_Board_Descriptor
					     *handle,
					     const struct DM35425_Function_Block
					     *func_block, uint8_t dir);

/**
*******************************************************************************
@brief
   Get the current value of the external clocking edge detect

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   func_block

   Pointer to the returned function block descriptor.

@param
   edge_detect

   Pointer to the returned value of the per-clock edge detect settings.
   (0 = rising edge, 1 = falling edge)

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Get_Edge(struct DM35425_Board_Descriptor
					      *handle,
					      struct DM35425_Function_Block
					      *func_block,
					      uint8_t * edge_detect);

/**
*******************************************************************************
@brief
   Set the current value of the external clocking edge detect

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   func_block

   Pointer to the returned function block descriptor.

@param
   edge_detect

   Value of the per-clock edge detect settings.
   (0 = rising edge, 1 = falling edge)

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Set_Edge(struct DM35425_Board_Descriptor
					      *handle,
					      const struct
					      DM35425_Function_Block
					      *func_block, uint8_t edge_detect);

/**
*******************************************************************************
@brief
   Get the pulse width setting for a specific external clock

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   func_block

   Pointer to the returned function block descriptor.

@param
   clock_src

   Which external clock the pulse width is associated with.

@param
   pulse_width

   Pointer to the returned value for the pulse width.

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Get_Pulse_Width(struct
						     DM35425_Board_Descriptor
						     *handle,
						     struct
						     DM35425_Function_Block
						     *func_block,
						     enum DM35425_Clock_Sources
						     clock_src,
						     uint8_t * pulse_width);

/**
*******************************************************************************
@brief
   Set the pulse width setting for a specific external clock

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   func_block

   Pointer to the returned function block descriptor.

@param
   clock_src

   Which external clock the pulse width is associated with.

@param
   pulse_width

   Value for the pulse width.

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Set_Pulse_Width(struct
						     DM35425_Board_Descriptor
						     *handle,
						     struct
						     DM35425_Function_Block
						     *func_block,
						     enum DM35425_Clock_Sources
						     clock_src,
						     uint8_t pulse_width);

/**
*******************************************************************************
@brief
   Get the setting for a specific external clock

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   func_block

   Pointer to the returned function block descriptor.

@param
   clock_src

   Which external clock the pulse width is associated with.

@param
   clocking_method

   Pointer to the returned value for the setting for this external clock.

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Get_Method(struct DM35425_Board_Descriptor
						*handle,
						struct DM35425_Function_Block
						*func_block,
						enum DM35425_Clock_Sources
						clock_src,
						enum DM35425_Ext_Clocking_Method
						*clocking_method);

/**
*******************************************************************************
@brief
   Set the setting for a specific external clock

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   func_block

   Pointer to the returned function block descriptor.

@param
   clock_src

   Which external clock the pulse width is associated with.

@param
   clocking_method

   Enumerated value for the setting for this external clock.

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
	 DM35425LIB_API
	    int DM35425_Ext_Clocking_Set_Method(struct DM35425_Board_Descriptor
						*handle,
						struct DM35425_Function_Block
						*func_block,
						enum DM35425_Clock_Sources
						clock_src,
						enum DM35425_Ext_Clocking_Method
						clocking_method);

/**
 * @} DM35425_Ext_Clocking_Library_Functions
 */

#ifdef __cplusplus
}
#endif
#endif

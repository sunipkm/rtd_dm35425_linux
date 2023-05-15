/**
    @file

    @brief
        Defines for the DM35425 (Device-specific values)

    @verbatim
    --------------------------------------------------------------------------
    This file and its contents are copyright (C) RTD Embedded Technologies,
    Inc.  All Rights Reserved.

    This software is licensed as described in the RTD End-User Software License
    Agreement.  For a copy of this agreement, refer to the file LICENSE.TXT
    (which should be included with this software) or contact RTD Embedded
    Technologies, Inc.
    --------------------------------------------------------------------------
    @endverbatim

    $Id: dm35425.h 80510 2014-07-17 15:46:23Z rgroner $
*/

#ifndef __DM35425_H__
#define __DM35425_H__

/**
 * @brief
 * DM35425 PCI vendor ID
 */

#define DM35425_PCI_VENDOR_ID	0x1435

/**
 * @brief
 *     DM35425 PCI device ID
 */
#define DM35425_PCI_DEVICE_ID	0x5425

/**
 * @brief
 *     Number of ADC on the DM35425
 */
#define DM35425_NUM_ADC_ON_BOARD 	1

/**
 * @brief
 *     Number of DAC on the DM35425
 */
#define DM35425_NUM_DAC_ON_BOARD 	1

/**
 * @brief
 *     Number of channels per ADC
 */
#define DM35425_NUM_ADC_DMA_CHANNELS	32

/**
 * @brief
 *     Number of buffers per ADC DMA channel
 */
#define DM35425_NUM_ADC_DMA_BUFFERS		7

/**
 * @brief
 *     Number of channels per DAC
 */
#define DM35425_NUM_DAC_DMA_CHANNELS	4

/**
 * @brief
 *     Number of buffers per DAC DMA channel
 */
#define DM35425_NUM_DAC_DMA_BUFFERS		7

/**
 * @brief
 *     Sample size of the FIFO
 */
#define DM35425_FIFO_SAMPLE_SIZE		511

#endif

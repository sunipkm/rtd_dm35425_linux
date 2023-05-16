/**
 * @file dm35425_multiboard.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Function declarations for the DM35425 that are Linux specific and
 * provides single ISR functionality for ADCs on multiple boards.
 * @version 0.1
 * @date 2023-05-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _DM35425_BOARD_ADC_MULTIBOARD__H_
#define _DM35425_BOARD_ADC_MULTIBOARD__H_

#include <stdarg.h>
#include <signal.h>
#include <pthread.h>
#include "dm35425_board_access.h"

#ifndef _Nullable
#define _Nullable
#endif

#ifndef _Nonnull
#define _Nonnull
#endif

typedef void (*DM35425_Multiboard_ISR)(int num_boards, struct DM35425_Board_Descriptor **boards, void *user_data);

struct DM35425_Multiboard_Descriptor {
    struct DM35425_Board_Descriptor **boards;
    int num_boards;
    pthread_t pid;
    DM35425_Multiboard_ISR isr;
    volatile sig_atomic_t done;
    unsigned int timeout_sec;
};


int DM35425_Multiboard_Init(struct DM35425_Multiboard_Descriptor _Nullable **mbd, int num_boards, ...);

int DM35425_Multiboard_Destroy(struct DM35425_Multiboard_Descriptor _Nonnull *mbd);

#endif // _DM35425_BOARD_ADC_MULTIBOARD__H_
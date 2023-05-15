Linux Software (DM35425)

SWP-700010163 rev D

Version v03.00.01.125638

Copyright (C) RTD Embedded Technologies, Inc.  All Rights Reserved.

This software package is dual-licensed.  Source code that is compiled for
kernel mode execution is licensed under the GNU General Public License
version 2.  For a copy of this license, refer to the file
LICENSE_GPLv2.TXT (which should be included with this software) or contact
the Free Software Foundation.  Source code that is compiled for user mode
execution is licensed under the RTD End-User Software License Agreement.
For a copy of this license, refer to LICENSE.TXT or contact RTD Embedded
Technologies, Inc.  Using this software indicates agreement with the
license terms listed above.


------------------
Supported Hardware
------------------

This software supports the RTD DM35425.


-------------------------
Supported Kernel Versions
-------------------------

This software has been tested with the following Linux distributions and kernel
versions:

    * openSUSE 12.2 (unmodified kernel 3.2)
    * Ubuntu 16.04 LTS (unmodified kernel 4.4)
    * Ubuntu 16.04 LTS (4.10 kernel built from kernel.org source)

 Due to API differences between kernel versions, RTD cannot guarantee compatibility
 with kernels and distributions not listed above.  If a user wishes to use an 
 unsupported kernel/distribution, it may be necessary to modify the driver module 
 code and/or Makefiles for the specific Linux environment.
 
--------------------------
Supported CPU Architecture
--------------------------
  
 This software has been validated on the following CPU architectures.
 
    * i586 (32-bit) dual-core
    * i586 (64-bit) single-core
    * i586 (64-bit) multi-core


-------------------
Supported Compilers
-------------------

The driver software and example programs were compiled using the GNU gcc 
compiler under openSUSE 12.2 and Ubuntu 16.04


-------------
Driver Design
-------------



        +------------------+        +----------------------------+
        | Example Programs |   OR   | User-Developed Application |
        | Provided by RTD  |        |                            |
        +------------------+        +----------------------------+
                 |                                |
                 |                                |
                 +--------------------------------+
                               |
                               | High-level C Functions
                               |
                          +---------+
                          | Library |
                          +---------+
                               |
                               | Low-level IOCTLs
    User Mode                  |
    ==========================================================================
    Kernel Mode                |
                           +--------+
                           | Driver |
                           +--------+
                               |
                               | Kernel Services
                               |
    ==========================================================================
                            Hardware
                            
                            
------
Driver
------

The directory driver/ contains source code related to the driver.


In order to use a driver, one must first compile it, load it into the kernel,
and create device files for the board(s).  To do this, issue the following
commands while sitting in the driver/ directory:

    * make
    * sudo make load



The driver module must be loaded before running any program which accesses a
DM35425 device.



-----------------
Library Interface
-----------------

The directory lib/ contains source code related to the user library.


The DM35425 library is created with a file name of librtd-dm35425.a and is
statically linked.


Please refer to the software manual for details on using the user level library
functions.  These functions are prototyped in the file include/dm35425_library.h;
this header file must be included in any code which wishes to call library
functions.


The library must be built before compiling the example programs or your
application.


To build the library, issue the command "make" within lib/.


------------
Header Files
------------

The directory include/ contains all header files needed by the driver, example
programs, library, and user applications.


----------------
Example Programs
----------------

The directory examples/ contains source code related to the example programs,
which demonstrate how to use features of the DM35425 boards, test the driver, or
test the library.  In addition to source files, examples/ holds other files as
well; the purpose of these files will be explained below.


To build the example programs, issue the command "make" within examples/.


The following files are provided in examples/:

    * Makefile
	    Make description file for building example programs.


    * dm35425_adc.c
            This example program demonstrates the use of the ADC and interrupt
            handling.  An interrupt is generated each time an ADC has taken a 
            sample.  When that interrupt happens, the programs gets the value
            of the last sample and displays it on the screen.
            
            Setup: Connect the signal of interest to AIN0 (pin 1 of CN3) and AGND
            (pin 21 of CN3).
            
            Usage: Display the command syntax by executing 
            
                           ./dm35425_adc --help.
            
            Hit CTRL-C to exit.

    * dm35425_adc_all_dma.c
            Demonstrate using the ADC and DMA, with all 32 channels active at
            the same time.  Data will be collected and written to files, 1 file
            per channel.
            
            Setup:  Connect the signal of interest to any analog input pin.
            
            Usage:  Display the command syntax by executing
            
                           ./dm35425_adc_all_dma --help

            Hit CTRL-C to exit.
            
    * dm35425_adc_continuous_dma.c
            This example program demonstrates the use of the ADC and DMA.  The 
            example will collect data from the ADC via DMA, and then write the
            data out to a file on disk.  The data can then be plotted using 
            gnuplot and the plot_adc_dma file.
            
            Setup: Connect the signal of interest to AIN0 (pin 1 of CN3) and AGND
            (pin 21 of CN3)
            
            Usage: Display the command syntax by executing 
            
                           ./dm35425_adc_continuous --help.
                
            Hit CTRL-C to exit.

    * dm35425_adc_fifo.c
            Demonstrates using the FIFO to collect data from the ADC, and 
            what interrupts occur while doing so.  The example will sample
            data enough to fill the FIFO, read the data from the FIFO and
            print it to the screen, and will trigger various interrupts.
            The example will complete and exit without user intervention.
            
            Setup: Connect the signal of interest to AIN0 (CN3 Pin 1)
            
            Usage: Display the command syntax by executing
                           ./dm35425_adc_fifo --help
                           
    * dm35425_adio.c
            This example program sets 16-bits of DIO to output, and
            16-bits to input.  We'll connect the output to the input and
            then write every possible 16-bit value to the output
            and verify the same value on the input pins.
            
            Setup: On CN3 and CN4, make these connections (using a standard jumper
            is usually the easiest way)
            
		Pin23 to Pin24
		Pin25 to Pin26
		Pin27 to Pin28
		Pin29 to Pin30
		Pin31 to Pin32
		Pin33 to Pin34
		Pin35 to Pin36
		Pin37 to Pin38            
            
            Usage: Display the command syntax by executing 
            
                           ./dm35425_adc_continuous --help.
                
            Hit CTRL-C to exit.

    * dm35425_adio_adv_int.c
            This example program uses the DIO match and event interrupts.
            It will match on the value 0xAA, and when that value passes
            through the input pins, it will throw an interrupt.

            The example will then wait for an event interrupt, which will occur
            if any input bit changes to zero.  This can be accomplished
            by touching a GND wire to any of the input ADIO pins.  Alternately,
            hitting Enter during the example will toggle the output bits,
            accomplishing the same thing.
            
            Setup: On CN3 and CN4, make these connections (using a standard jumper
            is usually the easiest way)
            
		Pin23 to Pin24
		Pin25 to Pin26
		Pin27 to Pin28
		Pin29 to Pin30
		Pin31 to Pin32
		Pin33 to Pin34
		Pin35 to Pin36
		Pin37 to Pin38  
            
            Usage: Display the command syntax by executing 
            
                           ./dm35425_adc_continuous --help.
                
            Hit CTRL-C to exit.

    * dm35425_adio_dma.c
            The example will make use of 3 DMA buffers for each
            of the three DMA channels (ADIO In, ADIO Out, and ADIO
            Direction).  Data will "play out" of the ADIO Out and
            Direction channels, and be stored in the ADIO In DMA
            buffer.  Doing this, we'll receive a pattern in the DMA
            In buffers that is the result of the output and changing
            bit direction values.
            
            Setup: On CN3 and CN4, make these connections (using a standard jumper
            is usually the easiest way)
            
		Pin23 to Pin24
		Pin25 to Pin26
		Pin27 to Pin28
		Pin29 to Pin30
		Pin31 to Pin32
		Pin33 to Pin34
		Pin35 to Pin36
		Pin37 to Pin38  
            
            Usage: Display the command syntax by executing 
            
                           ./dm35425_adc_continuous --help.
                
            Hit CTRL-C to exit.

    * dm35425_adio_parallel_bus.c
            The ADIO may be used as a parallel bus to transfer data from 1 board
	    to another.  In this mode, 3 ADIO signals are used for control, and
	    the remaining 29 bits are used for passing data. This board uses DMA
            and the parallel bus mode to transfer data from 1 board to another.
            
            Setup: All ADIO pins on CN3 (Pins 23-38) must be connected from 1 board
	    to CN3 (Pins 23-38) of the 2nd board.

	    The three control lines on CN4 must also be connected between the
	    boards:

	    CN4 Pin 24
	    CN4 Pin 26
	    CN4 Pin 28
            
            Usage: Display the command syntax by executing 
            
                           ./dm35425_adc_continuous --help.
                
            Hit CTRL-C to exit.

    * dm35425_dac.c
            This example program demonstrates the use of the DAC.  A voltage is
            put out on the pin corresponding to the input from the user.  The voltage
            can be easily changed to cover the full range.
            
            Setup: Connect an oscilloscope to the AOUT0 pin (CN3 Pin 17).
            
            Usage: Display the command syntax by executing
            
                       ./dm35425_dac --help.
            
            Follow the prompts on screen for changing the voltage.
            
            Hit CTRL-C to exit.

    * dm35425_dac_dma.c
            This example program demonstrates the use of the DAC and DMA.  A 
            wave pattern data is generated in the program, written to a DMA buffer,
            and then sent to the DAC in a repeating loop, thus provding a
            continuous cycling signal.
            
            Setup: Connect an oscilloscope to the AOUT0 pin (CN3 Pin 17).
            
            Usage: Display the command syntax by executing
            
                      ./dm35425_dac_dma --help.
            
            Hit CTRL-C to exit.          
            
    * dm35425_ext_clocking.c
            This example program uses function blocks to create signals which
	    are looped back into external clock inputs.  Each signal generated
            produces the equivalent of a square wave.
            
            The output shows that each function block runs at half the speed of
            the previous one due to the cascading clocks.
            
            Setup: Make connections as follows:

            CN3: Pin 17 to Pin 39
            CN3: Pin 37 to Pin 40
            
            Usage: Display the command syntax by executing 
            
                      ./dm35425_ext_clocking --help.
                      
            Hit CTRL-C to exit.                      
            
    * dm35425_list_fb.c
            This example program demonstrates using the board-level 
            registers to access the function blocks on the board.  The 
            program will query every function block location to see if a 
            valid function block type exists there, and if it does, it will
            display that type on the screen.  In this way, it will give an
            inventory of the function blocks on the board.
            
            Setup: No setup required.
            
            Usage: ./dm35425_list_fb --minor 0



-----------------
Known Limitations
-----------------

 1. This software was tested only on little-endian processors.  If you are using
    a big-endian CPU, you will need to examine the driver, example, and library
    source code for endianness issues and resolve them.

 2. Many conditions affect board throughput and interrupt performance.  For a
    discussion of these issues, please see the Application Note SWM-640000021
    (Linux Interrupt Performance) available on our web site.

 3. If you are using the interrupt wait mechanism, be aware that signals
    delivered to the application can cause the sleep to awaken prematurely.
    Interrupts may be missed if signals are delivered rapidly enough or at
    inopportune times.



-------------------------
Getting Technical Support
-------------------------

If you require additional support with this product, or any other products from
RTD Embedded Technologies, contact us using the information below:

	RTD Embedded Technologies, Inc.
	103 Innovation Boulevard
	State College, PA 16803 USA

	Telephone: (814) 234-8087
	Fax: (814) 234-5218
	Sales Information and Quotes: sales@rtd.com
	Technical Assistance: techsupport@rtd.com
	Web Site: http://www.rtd.com

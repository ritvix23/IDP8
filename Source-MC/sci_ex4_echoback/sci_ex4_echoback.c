//#############################################################################
//
// FILE:   sci_ex4_echoback.c
//
// TITLE:  SCI echoback example.
//
//! \addtogroup driver_example_list
//! <h1>SCI Echoback</h1>
//!
//!  This test receives and echo-backs data through the SCI-A port.
//!
//!  A terminal such as 'putty' can be used to view the data from
//!  the SCI and to send information to the SCI. Characters received
//!  by the SCI port are sent back to the host.
//!
//!  \b Running \b the \b Application
//!  Open a COM port with the following settings using a terminal:
//!  -  Find correct COM port
//!  -  Bits per second = 9600
//!  -  Data Bits = 8
//!  -  Parity = None
//!  -  Stop Bits = 1
//!  -  Hardware Control = None
//!
//!  The program will print out a greeting and then ask you to
//!  enter a character which it will echo back to the terminal.
//!
//!  \b Watch \b Variables \n
//!  - loopCounter - the number of characters sent
//!
//! \b External \b Connections \n
//!  Connect the SCI-A port to a PC via a transceiver and cable.
//!  - GPIO28 is SCI_A-RXD (Connect to Pin3, PC-TX, of serial DB9 cable)
//!  - GPIO29 is SCI_A-TXD (Connect to Pin2, PC-RX, of serial DB9 cable)
//
//#############################################################################
//
//
// $Copyright:
// Copyright (C) 2021 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include<stdlib.h>
#include<stdio.h>

//
// Defines
//
// Define AUTOBAUD to use the autobaud lock feature
//#define AUTOBAUD

//
// Globals
//

// union Float to handle casting of byte arrays to float
typedef union _Float{
    float f;
    unsigned long bytes;
} Float;




// read floats through SCI
// reads 4 bytes at a time and cast and stores to the receivedValue Float variable.
void SCI_readFloatBlockingFIFO(Float * receivedValue){
    int i = 0;

    //
    unsigned char byte[4];
    uint16_t rxStatus = 0U;
    for(; i<4; i++){
        byte[i] = SCI_readCharBlockingFIFO(SCIA_BASE);
        rxStatus = SCI_getRxStatus(SCIA_BASE);
        if((rxStatus & SCI_RXSTATUS_ERROR) != 0){
            ESTOP0;
        }
    }

    receivedValue->bytes = ((unsigned long)byte[0]<<24 | (unsigned long)byte[1] << 16 | (unsigned long)byte[2] << 8 | (unsigned long)byte[3]);
}





// copy the 4 bytes stored in fvalue into byteArr
void floatToByteArray(Float * fvalue, unsigned char * byteArr){


    unsigned long copy = fvalue->bytes, mask = 0xFF;
    int i = 3;
    for(; i>-1; i--){
        byteArr[i] = (copy >> 8*(3-i) & mask);
    }
}


void calculation(Float * fValue){
    fValue->f = fValue->f*400.05+100.128;
}



//
// Main
//
void main(void)
{
    //
    // Configure PLL, disable WD, enable peripheral clocks.
    //
    Device_init();

    //
    // Disable pin locks and enable internal pullups.
    //
    Device_initGPIO();

    //
    // GPIO28 is the SCI Rx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCIRXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    //
    // GPIO29 is the SCI Tx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCITXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);

    //
    // Initialize interrupt controller and vector table.
    //
    Interrupt_initModule();
    Interrupt_initVectorTable();

    //
    // Initialize SCIA and its FIFO.
    //
    SCI_performSoftwareReset(SCIA_BASE);

    //
    // Configure SCIA for echoback.
    //
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 9600, (SCI_CONFIG_WLEN_8 |
                                                        SCI_CONFIG_STOP_ONE |
                                                        SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);

#ifdef AUTOBAUD
    //
    // Perform an autobaud lock.
    // SCI expects an 'a' or 'A' to lock the baud rate.
    //
    SCI_lockAutobaud(SCIA_BASE);
#endif

    Float receivedValue;
    unsigned char byteArr[4];
    while(1){
        // all 4 bytes must be received to proceed further
        // read float through SCI
        SCI_readFloatBlockingFIFO(&receivedValue);


        //pass by reference- s changes the value in place
        // do some calculations
        calculation(&receivedValue);


        //converts Float variable 'receivedValue' to byte array
        floatToByteArray(&receivedValue, byteArr);

        // send this byte arr over the channel
        //inbuilt function
        //defined in sci.h
        SCI_writeCharArray(SCIA_BASE, (uint16_t*)byteArr, 4);
    }


}

//
// End of File
//


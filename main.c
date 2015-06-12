//*****************************************************************************
// Exosite Modifications:
// Copyright (C) 2014 Exosite LLC
//
// Original File:
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Exosite Cloud Demo
// Application Overview - This Application demonstrates using the CC3200
//                        LaunchPad with Exosite's cloud platform.
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup Exosite
//! @{
//
//****************************************************************************

// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Simplelink includes
#include "simplelink.h"
#include "netcfg.h"

// Driverlib includes
#include "hw_ints.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "utils.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "pin.h"
#include "timer.h"
#include "gpio.h"

// Common interface includes
#include "gpio_if.h"

// OS includes
#include "osi.h"

// Common interface includes
#include "gpio_if.h"
#include "uart_if.h"
#include "i2c_if.h"
#include "common.h"

#include "device_status.h"
#include "smartconfig.h"
#include "tmp006drv.h"
#include "bma222drv.h"
//#include "led_ring.h"
#include "pinmux.h"

#include <typedefs.h>
#include <exosite.h>
#include <config.h>
#include <platform/exosite_pal.h>

#define APPLICATION_VERSION              "0.1.0"
#define APP_NAME                         "Exosite Cloud Demo for Exosite Ready"
#define EXOSITE_TASK_PRIORITY            2
#define LOW_TASK_PRIORITY                1
#define SPAWN_TASK_PRIORITY              9
#define OSI_STACK_SIZE                   8192
#define SMALL_STACK_SIZE                 512
#define AP_SSID_LEN_MAX                 32
#define SH_GPIO_3                       3       /* P58 - Device Mode */
#define AUTO_CONNECTION_TIMEOUT_COUNT   50      /* 5 Sec */
#define SL_STOP_TIMEOUT                 200

typedef enum
{
  LED_OFF = 0,
  LED_ON,
  LED_BLINK
}eLEDStatus;

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
static const char pcDigits[] = "0123456789";
static unsigned char POST_token[] = "__SL_P_ULD";
static unsigned char GET_token_TEMP[]  = "__SL_G_UTP";
static unsigned char GET_token_ACC[]  = "__SL_G_UAC";
static unsigned char GET_token_UIC[]  = "__SL_G_UIC";
static int g_iInternetAccess = -1;
static unsigned char g_ucDryerRunning = 0;
static unsigned int g_uiDeviceModeConfig = ROLE_STA; //default is STA mode
static unsigned char g_ucLEDStatus = LED_OFF;
static unsigned long  g_ulStatus = 0;//SimpleLink Status
static unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
static unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID

// Exosite Demo Globals
volatile static unsigned long long g_SW3Counter = 0;
volatile static unsigned long long g_uptimeSec = 0;


volatile static float g_accXIntervalSum = 0;
volatile static float g_accYIntervalSum = 0;
volatile static float g_accZIntervalSum = 0;
volatile static long long g_accSampleCount = 0;

volatile static float g_accTotalAvg;
volatile static float g_accXAvg;
volatile static float g_accYAvg;
volatile static float g_accZAvg;

void AccSample();

void
	write_callback(	const exosite_data_port_t *dataPorts,
					int opStatus);

void
	read_callback(	const exosite_data_port_t *dataPort,
					int opStatus);

/*void
	subscribe_callback(	const exosite_data_port_t *dataPort,
						int opStatus);*/

static uint8_t
	getUuid(char *read_buffer);

static void
	download_firmware();

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


#ifdef USE_FREERTOS
//*****************************************************************************
//
//! Application defined hook (or callback) function - the tick hook.
//! The tick interrupt can optionally call this
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationTickHook( void )
{
}

//*****************************************************************************
//
//! Application defined hook (or callback) function - assert
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vAssertCalled( const char *pcFile, unsigned long ulLine )
{
    while(1)
    {
    }
}

//*****************************************************************************
//
//! Application defined idle task hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationIdleHook( void )
{
	//AccSample();
}

//*****************************************************************************
//
//! Application provided stack overflow hook function.
//!
//! \param  handle of the offending task
//! \param  name  of the offending task
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationStackOverflowHook( OsiTaskHandle *pxTask, signed char *pcTaskName)
{
    ( void ) pxTask;
    ( void ) pcTaskName;

    for( ;; );
}

void vApplicationMallocFailedHook()
{
    while(1)
  {
    // Infinite loop;
  }
}
#endif

//*****************************************************************************
//
//! itoa
//!
//!    @brief  Convert integer to ASCII in decimal base
//!
//!     @param  cNum is input integer number to convert
//!     @param  cString is output string
//!
//!     @return number of ASCII parameters
//!
//!
//
//*****************************************************************************
static unsigned short itoa(char cNum, char *cString)
{
    char* ptr;
    char uTemp = cNum;
    unsigned short length;

    // value 0 is a special case
    if (cNum == 0)
    {
        length = 1;
        *cString = '0';

        return length;
    }

    // Find out the length of the number, in decimal base
    length = 0;
    while (uTemp > 0)
    {
        uTemp /= 10;
        length++;
    }

    // Do the actual formatting, right to left
    uTemp = cNum;
    ptr = cString + length;
    while (uTemp > 0)
    {
        --ptr;
        *ptr = pcDigits[uTemp % 10];
        uTemp /= 10;
    }

    return length;
}


//*****************************************************************************
//
//! ReadAccSensor
//!
//!    @brief  Read Accelerometer Data from Sensor
//!
//!
//!     @return none
//!
//!
//
//*****************************************************************************
void ReadAccSensor()
{
    //Define Accelerometer Threshold to Detect Movement
    const short csAccThreshold    = 5;

    signed char cAccXT1,cAccYT1,cAccZT1;
    signed char cAccXT2,cAccYT2,cAccZT2;
    signed short sDelAccX, sDelAccY, sDelAccZ;
    int iRet = -1;
    int iCount = 0;
      
    iRet = BMA222ReadNew(&cAccXT1, &cAccYT1, &cAccZT1);
    if(iRet)
    {
        //In case of error/ No New Data return
        return;
    }
    for(iCount=0;iCount<2;iCount++)
    {
        MAP_UtilsDelay((90*80*1000)); //30msec
        iRet = BMA222ReadNew(&cAccXT2, &cAccYT2, &cAccZT2);
        if(iRet)
        {
            //In case of error/ No New Data continue
            iRet = 0;
            continue;
        }

        else
        {                       
            sDelAccX = abs((signed short)cAccXT2 - (signed short)cAccXT1);
            sDelAccY = abs((signed short)cAccYT2 - (signed short)cAccYT1);
            sDelAccZ = abs((signed short)cAccZT2 - (signed short)cAccZT1);

            //Compare with Pre defined Threshold
            if(sDelAccX > csAccThreshold || sDelAccY > csAccThreshold ||
               sDelAccZ > csAccThreshold)
            {
                //Device Movement Detected, Break and Return
                g_ucDryerRunning = 1;
                break;
            }
            else
            {
                //Device Movement Static
                g_ucDryerRunning = 0;
            }
        }
    }
       
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************


//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if(pWlanEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'
            // Applications can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] Device Connected to the AP: %s , "
                       "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                      g_ucConnectionSSID,g_ucConnectionBSSID[0],
                      g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                      g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                      g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_USER_INITIATED_DISCONNECTION
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT] Device disconnected from the AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on application's "
                           "request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR] Device disconnected from the AP AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        case SL_WLAN_STA_CONNECTED_EVENT:
        {
            // when device is in AP mode and any client connects to device cc3xxx
            //SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION_FAILED);

            //
            // Information about the connected client (like SSID, MAC etc) will
            // be available in 'slPeerInfoAsyncResponse_t' - Applications
            // can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.APModeStaConnected;
            //

            UART_PRINT("[WLAN EVENT] Station connected to device\n\r");
        }
        break;

        case SL_WLAN_STA_DISCONNECTED_EVENT:
        {
            // when client disconnects from device (AP)
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the connected client (like SSID, MAC etc) will
            // be available in 'slPeerInfoAsyncResponse_t' - Applications
            // can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.APModestaDisconnected;
            //
            UART_PRINT("[WLAN EVENT] Station disconnected from device\n\r");
        }
        break;

        case SL_WLAN_SMART_CONFIG_COMPLETE_EVENT:
        {
            //SET_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

            //
            // Information about the SmartConfig details (like Status, SSID,
            // Token etc) will be available in 'slSmartConfigStartAsyncResponse_t'
            // - Applications can use it if required
            //
            //  slSmartConfigStartAsyncResponse_t *pEventData = NULL;
            //  pEventData = &pSlWlanEvent->EventData.smartConfigStartResponse;
            //

        }
        break;

        case SL_WLAN_SMART_CONFIG_STOP_EVENT:
        {
            // SmartConfig operation finished
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

            //
            // Information about the SmartConfig details (like Status, padding
            // etc) will be available in 'slSmartConfigStopAsyncResponse_t' -
            // Applications can use it if required
            //
            // slSmartConfigStopAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.smartConfigStopResponse;
            //
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(pNetAppEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }

    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                       "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));

            UNUSED(pEventData);
        }
        break;

        case SL_NETAPP_IP_LEASED_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the IP-Leased details(like IP-Leased,lease-time,
            // mac etc) will be available in 'SlIpLeasedAsync_t' - Applications
            // can use it if required
            //
            // SlIpLeasedAsync_t *pEventData = NULL;
            // pEventData = &pNetAppEvent->EventData.ipLeased;
            //

        }
        break;

        case SL_NETAPP_IP_RELEASED_EVENT:
        {
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the IP-Released details (like IP-address, mac
            // etc) will be available in 'SlIpReleasedAsync_t' - Applications
            // can use it if required
            //
            // SlIpReleasedAsync_t *pEventData = NULL;
            // pEventData = &pNetAppEvent->EventData.ipReleased;
            //
        }
		break;

        default:
        {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}


//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent, 
                                SlHttpServerResponse_t *pSlHttpServerResponse)
{
    switch (pSlHttpServerEvent->Event)
    {
        case SL_NETAPP_HTTPGETTOKENVALUE_EVENT:
        {
            unsigned char *ptr;

            ptr = pSlHttpServerResponse->ResponseData.token_value.data;
            pSlHttpServerResponse->ResponseData.token_value.len = 0;
            if(memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, 
                    GET_token_TEMP, strlen((const char *)GET_token_TEMP)) == 0)
            {
                float fCurrentTemp;
                TMP006DrvGetTemp(&fCurrentTemp);
                char cTemp = (char)fCurrentTemp;
                short sTempLen = itoa(cTemp,(char*)ptr);
                ptr[sTempLen++] = ' ';
                ptr[sTempLen] = 'F';
                pSlHttpServerResponse->ResponseData.token_value.len += sTempLen;

            }

            if(memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, 
                      GET_token_UIC, strlen((const char *)GET_token_UIC)) == 0)
            {
                if(g_iInternetAccess==0)
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"1");
                else
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"0");
                pSlHttpServerResponse->ResponseData.token_value.len =  1;
            }

            if(memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, 
                       GET_token_ACC, strlen((const char *)GET_token_ACC)) == 0)
            {

                ReadAccSensor();
                if(g_ucDryerRunning)
                {
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"Running");
                    pSlHttpServerResponse->ResponseData.token_value.len += strlen("Running");
                }
                else
                {
                    strcpy((char*)pSlHttpServerResponse->ResponseData.token_value.data,"Stopped");
                    pSlHttpServerResponse->ResponseData.token_value.len += strlen("Stopped");
                }
            }



        }
            break;

        case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT:
        {
            unsigned char led;
            unsigned char *ptr = pSlHttpServerEvent->EventData.httpPostData.token_name.data;

            //g_ucLEDStatus = 0;
            if(memcmp(ptr, POST_token, strlen((const char *)POST_token)) == 0)
            {
                ptr = pSlHttpServerEvent->EventData.httpPostData.token_value.data;
                if(memcmp(ptr, "LED", 3) != 0)
                    break;
                ptr += 3;
                led = *ptr;
                ptr += 2;
                if(led == '1')
                {
                    if(memcmp(ptr, "ON", 2) == 0)
                    {
                        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
                        g_ucLEDStatus = LED_ON;

                    }
                    else if(memcmp(ptr, "Blink", 5) == 0)
                    {
                        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
                        g_ucLEDStatus = LED_BLINK;
                    }
                    else
                    {
                        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
                                                g_ucLEDStatus = LED_OFF;
                    }
                }
                else if(led == '2')
                {
                    if(memcmp(ptr, "ON", 2) == 0)
                    {
                        GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
                    }
                    else if(memcmp(ptr, "Blink", 5) == 0)
                    {
                        GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
                        g_ucLEDStatus = 1;
                    }
                    else
                    {
                        GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
                    }
                }

            }
        }
            break;
        default:
            break;
    }
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    if(pDevEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }

    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    if(pSock == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }
    //
    // This application doesn't work w/ socket - Events are not expected
    //
    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status )
            {
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                               "failed to transmit all queued packets\n\n",
                               pSock->socketAsyncEvent.SockTxFailData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , "
                               "reason (%d) \n\n",
                               pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n", \
                        pSock->Event);
    }
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************

//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param    None
//!
//! \return None
//!
//*****************************************************************************
static void InitializeAppVariables()
{
    g_ulStatus = 0;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
    g_iInternetAccess = -1;
    g_ucDryerRunning = 0;
    g_uiDeviceModeConfig = ROLE_STA; //default is STA mode
    g_ucLEDStatus = LED_OFF;    
}


//****************************************************************************
//
//! Confgiures the mode in which the device will work
//!
//! \param iMode is the current mode of the device
//!
//!
//! \return   SlWlanMode_t
//!                        
//
//****************************************************************************
static int ConfigureMode(int iMode)
{
    long   lRetVal = -1;

    lRetVal = sl_WlanSetMode(iMode);
    ASSERT_ON_ERROR(lRetVal);

    /* Restart Network processor */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);

    // reset status bits
    CLR_STATUS_BIT_ALL(g_ulStatus);

    return sl_Start(NULL,NULL,NULL);
}


//****************************************************************************
//
//!    \brief Connects to the Network in AP or STA Mode - If ForceAP Jumper is
//!                                             Placed, Force it to AP mode
//!
//! \return  0 - Success
//!            -1 - Failure
//
//****************************************************************************
long ConnectToNetwork()
{
    long lRetVal = -1;
    unsigned int uiConnectTimeoutCnt =0;

    // staring simplelink
    lRetVal =  sl_Start(NULL,NULL,NULL);
    ASSERT_ON_ERROR( lRetVal);

    // Device is in AP Mode and Force AP Jumper is not Connected
    if(ROLE_STA != lRetVal && g_uiDeviceModeConfig == ROLE_STA )
    {
        if (ROLE_AP == lRetVal)
        {
            // If the device is in AP mode, we need to wait for this event 
            // before doing anything 
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
            #ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
            #endif
            }
        }
        //Switch to STA Mode
        lRetVal = ConfigureMode(ROLE_STA);
        ASSERT_ON_ERROR( lRetVal);
    }

    //Device is in STA Mode and Force AP Jumper is Connected
    if(ROLE_AP != lRetVal && g_uiDeviceModeConfig == ROLE_AP )
    {
         //Switch to AP Mode
         lRetVal = ConfigureMode(ROLE_AP);
         ASSERT_ON_ERROR( lRetVal);

    }

    //No Mode Change Required
    if(lRetVal == ROLE_AP)
    {
        //waiting for the AP to acquire IP address from Internal DHCP Server
        // If the device is in AP mode, we need to wait for this event 
        // before doing anything 
        while(!IS_IP_ACQUIRED(g_ulStatus))
        {
        #ifndef SL_PLATFORM_MULTI_THREADED
            _SlNonOsMainLoopTask(); 
        #endif
        }
        //Stop Internal HTTP Server
        lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

        //Start Internal HTTP Server
        lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

       char cCount=0;
       
       //Blink LED 3 times to Indicate AP Mode
       for(cCount=0;cCount<3;cCount++)
       {
           //Turn RED LED On
           GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           osi_Sleep(400);
           
           //Turn RED LED Off
           GPIO_IF_LedOff(MCU_RED_LED_GPIO);
           osi_Sleep(400);
       }

       char ssid[32];
	   unsigned short len = 32;
	   unsigned short config_opt = WLAN_AP_OPT_SSID;
	   sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt , &len, (unsigned char* )ssid);
	   UART_PRINT("\n\r Connect to : \'%s\'\n\r\n\r",ssid);
    }
    else
    {
        //Stop Internal HTTP Server
        lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

        //Start Internal HTTP Server
        lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

    	//waiting for the device to Auto Connect
        while(uiConnectTimeoutCnt<AUTO_CONNECTION_TIMEOUT_COUNT &&
            ((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))) 
        {
            //Turn RED LED On
            GPIO_IF_LedOn(MCU_RED_LED_GPIO);
            osi_Sleep(50);
            
            //Turn RED LED Off
            GPIO_IF_LedOff(MCU_RED_LED_GPIO);
            osi_Sleep(50);
            
            uiConnectTimeoutCnt++;
        }
        //Couldn't connect Using Auto Profile
        if(uiConnectTimeoutCnt == AUTO_CONNECTION_TIMEOUT_COUNT)
        {
            //Blink Red LED to Indicate Connection Error
            GPIO_IF_LedOn(MCU_RED_LED_GPIO);
            
            CLR_STATUS_BIT_ALL(g_ulStatus);

            //Connect Using Smart Config
            lRetVal = SmartConfigConnect();
            ASSERT_ON_ERROR(lRetVal);

            //Waiting for the device to Auto Connect
            while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))
            {
                MAP_UtilsDelay(500);              
            }
    }
    //Turn RED LED Off
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);

    g_iInternetAccess = ConnectionTest();

    }
    return SUCCESS;
}


//****************************************************************************
//
//!    \brief Read Force AP GPIO and Configure Mode - 1(Access Point Mode)
//!                                                  - 0 (Station Mode)
//!
//! \return                        None
//
//****************************************************************************
static void ReadDeviceConfiguration()
{
    unsigned int uiGPIOPort;
    unsigned char pucGPIOPin;
    unsigned char ucPinValue;
        
    //Read GPIO
    GPIO_IF_GetPortNPin(SH_GPIO_3,&uiGPIOPort,&pucGPIOPin);
    ucPinValue = GPIO_IF_Get(SH_GPIO_3,uiGPIOPort,pucGPIOPin);
        
    //If Connected to VCC, Mode is AP
    if(ucPinValue == 1)
    {
        //AP Mode
        g_uiDeviceModeConfig = ROLE_AP;
    }
    else
    {
        //STA Mode
        g_uiDeviceModeConfig = ROLE_STA;
    }

}


//*****************************************************************************
//
//! AccSample
//!
//!    @brief  Read Accelerometer Data from Sensor to Globals
//!
//!
//!     @return none
//!
//!
//
//*****************************************************************************
void AccSample()
{
    signed char accX,accY,accZ;
    int iRet = -1;
    unsigned long critKey;

    critKey = osi_EnterCritical();

    iRet = BMA222ReadNew(&accX, &accY, &accZ);
    if(iRet)
    {
        //In case of error/ No New Data return
        return;
    }


    g_accXIntervalSum += accX;
    g_accYIntervalSum += accY;
    g_accZIntervalSum += accZ;

    g_accSampleCount++;
    osi_ExitCritical(critKey);

}


//*****************************************************************************
//
//! ReadAccSensor
//!
//!    @brief  Calculate Averages of Accelerometer Globals
//!
//!
//!     @return none
//!
//!
//
//*****************************************************************************
void SetAccAvg()
{
	unsigned long critKey;

    critKey = osi_EnterCritical();
    g_accXAvg = g_accXIntervalSum / g_accSampleCount;
    g_accYAvg = g_accYIntervalSum / g_accSampleCount;
    g_accZAvg = g_accZIntervalSum / g_accSampleCount;
    g_accTotalAvg = (g_accZIntervalSum + g_accYIntervalSum + g_accXIntervalSum ) /
		(g_accSampleCount * 3);

    g_accXIntervalSum = 0;
    g_accYIntervalSum = 0;
    g_accZIntervalSum = 0;
    g_accSampleCount = 0;
    osi_ExitCritical(critKey);

}

//****************************************************************************
//
//!    \brief Exosite Application Main Task - Initializes SimpleLink Driver
//!                                           and Handles HTTP Requests
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void ExositeTask(void *pvParameters)
{
    long   ret = -1;
    unsigned long long retry_delay = 100;
    unsigned long long counter = 0;
    char value[64];
    char uid[13];
    date_time_t curDateTime;

    //Read Device Mode Configuration
    ReadDeviceConfiguration();

    //Connect to Network
    ret = ConnectToNetwork();
    if(ret < 0)
    {
        ERR_PRINT(ret);
        LOOP_FOREVER();
    }

    // Wait here if we're in AP Mode
    while(g_uiDeviceModeConfig != ROLE_STA){
        osi_Sleep(1000);
    }

    getUuid(uid);

    UART_PRINT("uid = %s\r\n", uid);

	exo_init(	"texasinstruments",
				"cc3200lp_v1",
				uid);

	//download_firmware();
    //Handle Async Events
	exosite_pal_get_current_date_time(&curDateTime);
    while(1)
    {

       // UART_PRINT("[EXO] Writing Values...\r\n");

    	AccSample(); // Just do a single reading for now. TODO: Make Async.
        SetAccAvg();

        float sensorTemp;
        TMP006DrvGetTemp(&sensorTemp);

        UART_PRINT("[EXO] Writing ontime\r\n");
        snprintf(value, sizeof(value), "%llu", g_uptimeSec);
        exo_write(	"ontime",
        			value,
        			write_callback);
        UART_PRINT("[EXO] Writing ontime end = %s", value);

        snprintf(value, sizeof(value), "%.0f", g_accTotalAvg);
        exo_write(	"acc",
        			value,
        			write_callback);

        snprintf(value, sizeof(value), "%.0f", g_accXAvg);
        exo_write(	"accX",
        			value,
        			write_callback);

        snprintf(value, sizeof(value), "%.0f", g_accYAvg);
        exo_write(	"accY",
        			value,
        			write_callback);

        snprintf(value, sizeof(value), "%.0f", g_accZAvg);
        exo_write(	"accZ",
        			value,
        			write_callback);

		snprintf(value, sizeof(value), "%.2f", sensorTemp);
		exo_write(	"sensortemp",
					value,
					write_callback);

        snprintf(value, sizeof(value), "%llu", g_SW3Counter);
        exo_write(	"usrsw",
        			value,
        			write_callback);

    	exo_read(	"ledd7",
    				read_callback);

    	exo_loop_start();

    	/*exo_subscribe(	"ledring",
    					&curDateTime,
    					subscribe_callback);

        exo_loop_start();*/

    	//exosite_pal_get_current_date_time(&curDateTime);

        counter++;
    }
}

void
	write_callback(	const exosite_data_port_t *dataPorts,
					int opStatus)
{
    if(opStatus == 204)
    {
    	UART_PRINT("OK\r\n");
    }
    else
    {
    	UART_PRINT("ERROR\r\n");
    	ERR_PRINT(opStatus);
    }
}

void
	read_callback(	const exosite_data_port_t *dataPort,
					int opStatus)
{
	int ucLEDStatus;
    if(opStatus != 200)
    {
    	UART_PRINT("\r\nRead %s failed\r\n", dataPort->alias);
    	//ERR_PRINT(opStatus);
    }
    else
    {
    	sscanf(dataPort->value, "%d", &ucLEDStatus);
    	UART_PRINT("\r\nRead %s = %s\r\n", dataPort->alias, dataPort->value);

        if(ucLEDStatus == LED_ON)
        {
            GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        }
        if(ucLEDStatus == LED_OFF)
        {
            GPIO_IF_LedOff(MCU_RED_LED_GPIO);
        }
        if(ucLEDStatus == LED_BLINK)
        {
        	GPIO_IF_LedToggle(MCU_RED_LED_GPIO);
        }
    }
}

/*void
	subscribe_callback(	const exosite_data_port_t *dataPort,
						int opStatus)
{
	int ledRingPercentage;
    if(opStatus != 200 && opStatus != 304)
    {
    	UART_PRINT("\r\nSubscribe %s failed\r\n", dataPort->alias);
    	//ERR_PRINT(opStatus);
    }
    else if(opStatus == 304)
    {
    	UART_PRINT("\r\nSubscribe %s does not changed", dataPort->alias);
    }
    else
    {
    	sscanf(dataPort->value, "%d", &ledRingPercentage);
    	UART_PRINT("\r\nSubscribe %s = %s\r\n", dataPort->alias, dataPort->value);

    	Send24LedData(ledRingPercentage);
    	osi_Sleep(200);
    }
}*/

static void
	download_firmware()
{
	content_id_t idList[2];
	int listSize = 2;
	content_info_t contentInfo;
	uint8_t buf[512];
	int bufLen;
	unsigned int reuSize = 0;
	unsigned int start = 0;
	unsigned int end;
	unsigned int cnt = 0;
	int i;

	UART_PRINT("download start\r\n");

	while(TRUE)
	{
		if(exo_download_list_content(	"texasinstruments",
										"cc3200lp_v1",
										idList,
										&listSize) == EXO_ERROR_STATUS_SUC)
		{

			for(i = 0; i < listSize; ++i)
			{
				UART_PRINT("id = %s\r\n", idList[i].id);
			}
			break;
		}
	}

	if(exo_download_get_content_info(	"texasinstruments",
										"cc3200lp_v1",
										idList[0].id,
										&contentInfo) == EXO_ERROR_STATUS_SUC)
	{
		UART_PRINT("type = %s\r\nsize = %d\r\ntimeStamp = %s\r\n", contentInfo.contentType, contentInfo.contentSize, contentInfo.updatedTimeStamp.toString);
		reuSize = contentInfo.contentSize;
		end = sizeof(buf) - 1;
	}

	UART_PRINT("reuSize = %d\r\n", reuSize);
	while(reuSize > 0)
	{
		bufLen = sizeof(buf);
		UART_PRINT("download start\r\n");
		if(exo_download_get_content("texasinstruments",
									"cc3200lp_v1",
									idList[0].id,
									start,
									end,
									buf,
									&bufLen) == EXO_ERROR_STATUS_SUC)
		{
			UART_PRINT("download start\r\n");
			for(i = 0; i < bufLen; ++i)
			{
				if(i % 16 == 0)
					UART_PRINT("\r\n");
				UART_PRINT("%x ", buf[i]);
			}

			start += bufLen;
			end = start + (sizeof(buf) - 1);
			reuSize -= bufLen;
			cnt += bufLen;

			UART_PRINT("\r\nreduSize = %d\r\n", reuSize);
		}
		else
		{
			UART_PRINT("get content failed\r\n");
			break;
		}
	}
}


static uint8_t
	getUuid(char *read_buffer)
{
	unsigned char maclen = 6;
	unsigned char raw_mac[6];
	sl_NetCfgGet(SL_MAC_ADDRESS_GET,NULL,&maclen,(unsigned char *)raw_mac);
	//Report("MAC - %02x:%02x:%02x:%02x:%02x:%02x\r\n",
    //       raw_mac[0], raw_mac[1], raw_mac[2], raw_mac[3], raw_mac[4], raw_mac[5]);
	snprintf(read_buffer, 13, "%02x%02x%02x%02x%02x%02x", raw_mac[0], raw_mac[1], raw_mac[2], raw_mac[3], raw_mac[4], raw_mac[5]);

    return 0;
}

void GPIOs3IntHandler()
{
    unsigned long ulPinState =  GPIOIntStatus(GPIOA1_BASE,1);
    if(ulPinState & GPIO_PIN_5)
    {
    	//Disable GPIO Interrupt
        MAP_GPIOIntDisable(GPIOA1_BASE,GPIO_PIN_5);
        MAP_GPIOIntClear(GPIOA1_BASE,GPIO_PIN_5);
        MAP_IntDisable(INT_GPIOA1);

    	++g_SW3Counter;

    	MAP_UtilsDelay(8000000 / 3); //delay about 100ms for debounce

        //Enable GPIO Interrupt
        MAP_GPIOIntClear(GPIOA1_BASE,GPIO_PIN_5);
        MAP_IntPendClear(INT_GPIOA1);
        MAP_IntEnable(INT_GPIOA1);
        MAP_GPIOIntEnable(GPIOA1_BASE,GPIO_PIN_5);
    }
}

//*****************************************************************************
//
//! Sample the accelerometer.
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void AccSampleTask( void *pvParameters )
{
	while(1)
	{
		AccSample();
        osi_Sleep(100);
	}
}

//*****************************************************************************
//
//! Inc the uptime counter.
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void UptimeTask( void *pvParameters )
{
	while(1)
	{
		g_uptimeSec++;
        osi_Sleep(1000);
	}
}

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{
    UART_PRINT("\n\n\n\r");
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\t\t  CC3200 %s Application       \n\r", AppName);
    UART_PRINT("\t\t Victor Ferman\n\r");
    UART_PRINT("\t\t Julio Fajardo\n\r");
    UART_PRINT("\t\t Universidad Galileo\n\r");
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\n\n\n\r");
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif  //ccs
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif  //ewarm
    
#endif  //USE_TIRTOS

    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//****************************************************************************
//                            MAIN FUNCTION
//****************************************************************************
void main()
{
    long   lRetVal = -1;

    //
    // Board Initilization
    //
    BoardInit();
    
    // Configure the pinmux settings for the peripherals exercised
    //
    PinMuxConfig();

    PinConfigSet(PIN_58,PIN_STRENGTH_2MA|PIN_STRENGTH_4MA,PIN_TYPE_STD_PD);

    // Initialize Global Variables
    InitializeAppVariables();
    
    //
    // UART Init
    //
    InitTerm();
    
    DisplayBanner(APP_NAME);

    //config sw3
    PinTypeGPIO(PIN_04, PIN_MODE_0, false);
    GPIODirModeSet(GPIOA1_BASE, GPIO_PIN_5, GPIO_DIR_MODE_IN);

    MAP_GPIOIntTypeSet(GPIOA1_BASE,GPIO_PIN_5,GPIO_FALLING_EDGE);

    osi_InterruptRegister(	INT_GPIOA1,(P_OSI_INTR_ENTRY)GPIOs3IntHandler,
							INT_PRIORITY_LVL_1);

    MAP_GPIOIntClear(GPIOA1_BASE,GPIO_PIN_5);
    MAP_GPIOIntEnable(GPIOA1_BASE,GPIO_INT_PIN_5);

    //
    // LED Init
    //
    GPIO_IF_LedConfigure(LED1);
      
    //Turn Off the LEDs
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
       
    //
    // I2C Init
    //
    lRetVal = I2C_IF_Open(I2C_MASTER_MODE_FST);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //Init Temprature Sensor
    lRetVal = TMP006DrvOpen();
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //Init Accelerometer Sensor
    lRetVal = BMA222Open();
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Simplelinkspawntask
    //
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }    
    
    //
    // Create Exosite Task
    //
    lRetVal = osi_TaskCreate(ExositeTask, (signed char*)"ExositeTask", \
                                OSI_STACK_SIZE, NULL, \
                                EXOSITE_TASK_PRIORITY, NULL );
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }    

    //
    // Create Uptime Task
    //
    lRetVal = osi_TaskCreate(UptimeTask, (signed char*)"UptimeTask", \
                                SMALL_STACK_SIZE, NULL, \
                                LOW_TASK_PRIORITY, NULL );
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Create Accelerometer Task
    //
    lRetVal = osi_TaskCreate(AccSampleTask, (signed char*)"AccSampleTask", \
                             SMALL_STACK_SIZE, NULL, \
                             LOW_TASK_PRIORITY, NULL );
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start OS Scheduler
    //
    osi_start();

    while (1)
    {

    }

}

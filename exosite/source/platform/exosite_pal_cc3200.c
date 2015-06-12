#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>

#include "simplelink.h"
#include "systick.h"
#include "common.h"
#include <uart_if.h>

#include <typedefs.h>
#include <platform/exosite_pal.h>


#define CIK_FILENAME "cik"
#define ONE_PLATFORM_NAME "m2.exosite.com"

#define SL_SSL_CA_CERT_FILE_NAME "/cert/ca.pem"

static unsigned long onePIp = 0xADFFD11c;

static unsigned long msTimer;

static bool_t
    parse_date(	const char *parseData,
				int dataLen,
				char *value);

static bool_t
	get_current_date_time_string(	char *dateTimeStr,
									int len);

static void
	convert_to_sl_datetime(	const char *dateTimeStr,
							SlDateTime_t *slDateTime);

static int
  month_to_num(const char *month);

static void
	SysTickIntHandler();

void
	exosite_pal_init()
{
	long lRetVal = -1;
	char buf[128];
	int bufLen;
	SlDateTime_t slDateTime;

	sl_NetAppDnsGetHostByName(	(signed char *)ONE_PLATFORM_NAME,
								strlen(ONE_PLATFORM_NAME),
								&onePIp,
								SL_AF_INET);

	bufLen = sizeof(buf);

	//time sync
	if(!get_current_date_time_string(	buf,
										bufLen))
		assert(FALSE);

	convert_to_sl_datetime(buf, &slDateTime);

	sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
	          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
	          sizeof(SlDateTime_t),
	          (_u8 *)(&slDateTime));

	msTimer = 0;

	//Because we use freertos, we just can register the timer to the task queue instead of using systick directly.
    lRetVal = osi_TaskCreate(SysTickIntHandler, (signed char*)"SysTickIntHandler", \
                                512, NULL, \
                                1, NULL );

    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER()
    }

	//SysTickIntRegister(SysTickIntHandler);
	//SysTickPeriodSet(80000);
	//SysTickEnable();

}

bool_t
	exosite_pal_sock_connect(void *sock)
{
	SlSockAddrIn_t sAddr;
	int *sockIns = (int *)sock;

	sAddr.sin_family = SL_AF_INET;
	sAddr.sin_port = sl_Htons(443); //https port
	sAddr.sin_addr.s_addr = sl_Htonl(onePIp);

	//unsigned char ucMethod = SL_SO_SEC_METHOD_TLSV1_2;
	//unsigned int uiCipher = SL_SEC_MASK_SECURE_DEFAULT;

	if((*sockIns = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_SEC_SOCKET)) < 0)
	{
		return FALSE;
	}


	/*if(sl_SetSockOpt(	*sockIns,
						SL_SOL_SOCKET,
						SL_SO_SECMETHOD,
						&ucMethod,
						sizeof(ucMethod)) < 0)
	{
		sl_Close(*sockIns);
		return FALSE;
	}

	if(sl_SetSockOpt(	*sockIns,
						SL_SOL_SOCKET,
						SL_SO_SECURE_MASK,
						&uiCipher,
						sizeof(uiCipher)) < 0)
	{
		sl_Close(*sockIns);
		return FALSE;
	}*/

	if(sl_SetSockOpt(*sockIns,
    				SL_SOL_SOCKET,
					SL_SO_SECURE_FILES_CA_FILE_NAME,
					SL_SSL_CA_CERT_FILE_NAME,
					strlen(SL_SSL_CA_CERT_FILE_NAME)) < 0)
	{
		sl_Close(*sockIns);
		return FALSE;
	}

	if((sl_Connect(*sockIns, (SlSockAddr_t *)&sAddr, sizeof(SlSockAddrIn_t))) < 0)
	{
		sl_Close(*sockIns);
		return FALSE;
	}

    /*struct SlTimeval_t timeVal;
    timeVal.tv_sec =  2;             // Seconds
    timeVal.tv_usec = 0;             // Microseconds. 10000 microseconds resolution
    sl_SetSockOpt(*sockIns,     // Enable receive timeout
    		      SL_SOL_SOCKET,
    		      SL_SO_RCVTIMEO,
    		      (_u8 *)&timeVal,
    		      sizeof(timeVal));*/
	return TRUE;
}

bool_t
    exosite_pal_sock_is_connected(void *sock)
{
    //TODO: Implementation is dependent on your platform.
    return TRUE;
}

bool_t
    exosite_pal_sock_read(  void *sock,
                            char *data,
                            int *dataLen)
{
    int *sockfd = (int *)sock;

    bzero(data, *dataLen);
    *dataLen = recv(*sockfd, data, *dataLen, 0);

    return ((*dataLen) > 0 ? TRUE : FALSE);
}

bool_t
    exosite_pal_sock_write( void *sock,
                            const char *data,
                            int dataLen)
{
    int *sockfd = (int *)sock;

    return ((sl_Send(*sockfd, data, dataLen, 0) > 0) ? TRUE : FALSE);
}

void
	exosite_pal_sock_close(void *sock)
{
	int *sockfd = (int *)sock;
	sl_Close(*sockfd);

}

bool_t
	exosite_pal_load_cik(	char *cik,
							int cikLen)
{
    unsigned long ulToken;
    long lFileHandle;
    long lRetVal = -1;

    lRetVal = sl_FsOpen((unsigned char *)CIK_FILENAME,
                        FS_MODE_OPEN_READ,
                        &ulToken,
                        &lFileHandle);
    if(lRetVal < 0)
    {
        lRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
        return FALSE;
    }

	lRetVal = sl_FsRead(lFileHandle,
						(unsigned int)0,
						(unsigned char *)cik,
				 		cikLen);
	if ((lRetVal < 0) || (lRetVal != cikLen))
	{
		lRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
		return FALSE;
	}

    lRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
    if (SL_RET_CODE_OK != lRetVal)
    {
        return FALSE;
    }

    return TRUE;
}

void
	exosite_pal_save_cik(	const char *cik,
							int cikLen)
{
	int iRetVal;
	long lFileHandle;
	unsigned long ulToken;

	iRetVal = sl_FsOpen((unsigned char *)CIK_FILENAME,
			            FS_MODE_OPEN_CREATE(cikLen, _FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE|_FS_FILE_PUBLIC_READ),
	                    &ulToken,
	                    &lFileHandle);
	if(iRetVal < 0)
	{
		iRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
	}

	iRetVal = sl_FsWrite(lFileHandle,
	                     (unsigned int)0,
	                     (unsigned char *)cik,
	                     cikLen);
	if (iRetVal < 0)
	{
		iRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
	}

	iRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
}

void
	exosite_pal_remove_cik()
{
	sl_FsDel((unsigned char *)CIK_FILENAME, 0);
}

static void
	SysTickIntHandler()
{
	while(1)
	{
		msTimer+=1;
		osi_Sleep(1);
	}
}

bool_t
	exosite_pal_timer_expired(exosite_timer_t *timer)
{
	long left = timer->endTime - msTimer;

	return (left < 0);
}


void
	exosite_pal_timer_countdown_ms(	exosite_timer_t *timer,
									unsigned int timeout)
{
	timer->endTime = msTimer + timeout;
}


void
	exosite_pal_timer_countdown(exosite_timer_t *timer,
								unsigned int timeout)
{
	timer->endTime = msTimer + (timeout * 1000);
}


int
	exosite_pal_timer_left_ms(exosite_timer_t *timer)
{
	long left = timer->endTime - msTimer;

	return (left < 0) ? 0 : left;
}


void
	exosite_pal_timer_init(exosite_timer_t *timer)
{
	timer->endTime = 0;
}

bool_t
	exosite_pal_get_current_date_time(date_time_t *curDateTime)
{
	int bufLen = sizeof(curDateTime->toString);
	return get_current_date_time_string(curDateTime->toString,
										bufLen);
}

static bool_t
	get_current_date_time_string(	char *dateTimeStr,
									int dataLen)
{
	char *pdu = "GET /ip HTTP/1.1\r\n"
				"Host: m2.exosite.com\r\n"
				"Accept: application/x-www-form-urlencoded; charset=utf-8\r\n\r\n";
	int sock;
	char buf[128];
	int bufLen;

	SlSockAddrIn_t sAddr;

	sAddr.sin_family = SL_AF_INET;
	sAddr.sin_port = sl_Htons(80);
	sAddr.sin_addr.s_addr = sl_Htonl(onePIp);

	if((sock = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0)) < 0)
	{
		return FALSE;
	}


	if((sl_Connect(sock, (SlSockAddr_t *)&sAddr, sizeof(SlSockAddrIn_t))) < 0)
	{
		sl_Close(sock);
		return FALSE;
	}

	if(sl_Send(sock, pdu, strlen(pdu), 0) <= 0)
    {
        return FALSE;
    }

	while(1)
	{
		bufLen = sizeof(buf);
		bzero(buf, bufLen);

		if((bufLen = recv(sock, buf, bufLen, 0)) > 0)
			break;
	}

	sl_Close(sock);

	if(!parse_date(	buf,
					bufLen,
					dateTimeStr))
		return FALSE;

	return TRUE;
}

static bool_t
    parse_date(	const char *parseData,
				int dataLen,
				char *value)
{
    char *keyStart;
    char *valueStart;
    char *valueEnd;
    char *index;
	const char *key = "Date";

    if(value == NULL)
    {
        return FALSE;
    }

    keyStart = strstr(parseData, key);

    if(keyStart == NULL)
    {
        return FALSE;
    }

    valueStart = strstr(keyStart, ":");

    if(valueStart == NULL)
    {
        return FALSE;
    }

    valueEnd = strstr(valueStart, "GMT");

    if(valueEnd == NULL)
    {
        return FALSE;
    }

    for(index = valueStart + 2; index < valueEnd + 3; ++index)
    {
        *value = *index;

        ++value;
    }

    *value = 0;

    return TRUE;
}

static void
	convert_to_sl_datetime(	const char *dateTimeStr,
							SlDateTime_t *slDateTime)
{
	char copyStr[128];
	char *token;

	strncpy(copyStr, dateTimeStr, strlen(dateTimeStr));

	//Tue, 18 Nov 2014 08 53 45"

	//Tue
	if((token = strtok(copyStr, ",\r\n: ")) == NULL)
		assert(FALSE);

	//18
	if((token = strtok(NULL, ",\r\n: ")) == NULL)
		assert(FALSE);

	sscanf(token, "%d", &slDateTime->sl_tm_day);

	//Nov
	if((token = strtok(NULL, ",\r\n: ")) == NULL)
		assert(FALSE);

	slDateTime->sl_tm_mon = month_to_num(token);

	//2014
	if((token = strtok(NULL, ",\r\n: ")) == NULL)
		assert(FALSE);

	sscanf(token, "%d", &slDateTime->sl_tm_year);

	//08
	if((token = strtok(NULL, ",\r\n: ")) == NULL)
		assert(FALSE);

	sscanf(token, "%d", &slDateTime->sl_tm_hour);

	//53
	if((token = strtok(NULL, ",\r\n: ")) == NULL)
		assert(FALSE);

	sscanf(token, "%d", &slDateTime->sl_tm_min);

	//45
	if((token = strtok(NULL, ",\r\n: ")) == NULL)
		assert(FALSE);

	sscanf(token, "%d", &slDateTime->sl_tm_sec);
}

static int
  month_to_num(const char *month)
{
  const char *monStr[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  int i;

  for(i = 0; i < 12; ++i)
    if(0 == strcmp(month, monStr[i]))
      return i + 1;

  return 0;
}

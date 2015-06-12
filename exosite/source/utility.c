#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <config.h>
#include "utility.h"

static void
   format_http_command(	char *pdu,
						int *index,
						char *httpHeader);

static bool_t
	find_http_content(	const char *pdu,
						int length,
						int httpContentLen,
						char *content,
						int *contentLen);

static bool_t
    parse_key_value(const char *parseData,
					int dataLen,
					const char *key,
					char *value);

static bool_t
	get_http_content(	const char *parseData,
						int dataLen,
						char *content,
						int *contentLen);

bool_t
    build_msg_activate(	char *pdu,
						int length,
						const char *vendor,
						const char *model,
						const char *serialNumber)
{
    char *str1 = "POST /provision/activate HTTP/1.1\r\n";
    char *str2 = "Host: m2.exosite.com\r\n";
    char *str3 = "Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n";
    char command[HTTP_MSG_SIZE];
    sprintf(pdu, "%s", str1);
    sprintf(pdu + strlen(str1), "%s", str2);
    sprintf(pdu + strlen(str1) + strlen(str2), "%s", str3);
    sprintf(command, "vendor=%s&model=%s&sn=%s", vendor, model, serialNumber);
    sprintf(pdu + strlen(str1) + strlen(str2) + strlen(str3),
            "Content-Length: %lu\r\n\r\n%s", strlen(command), command);
 
    return TRUE;    
}

int
    convert_data_ports_to_read_string(	char *dataString,
										const exosite_data_port_t *dataPorts,
										uint8_t numofDataPorts)
{
    int i;
    int index = 0;
    bool_t isFirst = TRUE;

    if(numofDataPorts <= 0)
        return -1;

    for(i = 0; i < numofDataPorts; ++i)
    {
        if(!isFirst)
        {
            sprintf(dataString + index, "&");
            ++index;
        }

        sprintf(dataString + index, "%s", dataPorts[i].alias);
        index = strlen(dataString);
        isFirst = FALSE;
    }

    return index;
}

bool_t
    build_msg_read(	char *pdu,
					int lenght,
					const exosite_data_port_t *dataPorts,
					uint8_t numOfDataPorts,
					const char *cik)
{
    int index = 0;
    char command[HTTP_MSG_SIZE];
    char dataString[HTTP_MSG_SIZE];
    int dataStringLen;

    dataStringLen = convert_data_ports_to_read_string(	dataString,
                                                 		dataPorts,
                                                 		numOfDataPorts);

    if(dataStringLen < 0 || dataStringLen >= (HTTP_MSG_SIZE / 2))
    {
        return FALSE;
    }

    sprintf(command, "GET /onep:v1/stack/alias?%s HTTP/1.1\r\n", dataString);

   format_http_command(	pdu,
						&index,
						command);

   format_http_command(	pdu,
						&index,
						"Host: m2.exosite.com\r\n");

    sprintf(command, "X-Exosite-CIK: %s\r\n", cik);
   format_http_command(	pdu,
		                &index,
		                command);

   format_http_command(	pdu,
						&index,
						"Accept: application/x-www-form-urlencoded; charset=utf-8\r\n\r\n");

    return TRUE;
}

int
    convert_data_ports_to_write_string(	char *dataString,
										const exosite_data_port_t *dataPorts,
										uint8_t numofDataPorts)
{
    int i;
    int index = 0;
    bool_t isFirst = TRUE;

    if(numofDataPorts <= 0)
        return -1;

    for(i = 0; i < numofDataPorts; ++i)
    {
        if(!isFirst)
        {
            sprintf(dataString + index, "&");
            ++index;
        }

        sprintf(dataString + index, "%s=%s", dataPorts[i].alias, dataPorts[i].value);
        index = strlen(dataString);
        isFirst = FALSE;
    }

    return index;
}

bool_t 
    build_msg_write(char *pdu,
					int length,
					const exosite_data_port_t *dataPorts,
					uint8_t numOfDataPorts,
					const char *cik)
{
	int index = 0;
	char command[HTTP_MSG_SIZE];
	char dataString[HTTP_MSG_SIZE];
	int dataStringLen;

	format_http_command(pdu,
						&index,
						"POST /onep:v1/stack/alias HTTP/1.1\r\n");

	format_http_command(pdu,
						&index,
						"Host: m2.exosite.com\r\n");

	sprintf(command, "X-Exosite-CIK: %s\r\n", cik);
	format_http_command(pdu,
						&index,
						command);

	format_http_command(pdu,
						&index,
						"Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n");

	dataStringLen = convert_data_ports_to_write_string(	dataString,
														dataPorts,
														numOfDataPorts);

	if(dataStringLen < 0 || dataStringLen >= HTTP_MSG_SIZE)
	{
		return FALSE;
	}


	sprintf(command, "Content-Length: %d\r\n\r\n", dataStringLen);

	format_http_command(pdu,
						&index,
						command);

	sprintf(pdu + index, "%s", dataString);

	return TRUE;
}


bool_t
    build_msg_long_polling(	char *pdu,
							int length,
							char *recDateTime,
							const char *alias,
							uint32_t reqTimeout,
							const char *cik)
{
	int index = 0;
	char command[HTTP_MSG_SIZE];

	sprintf(command, "GET /onep:v1/stack/alias?%s HTTP/1.1\r\n", alias);
	format_http_command(pdu,
						&index,
						command);

	format_http_command(pdu,
	                    &index,
	                    "Host: m2.exosite.com\r\n");

	sprintf(command, "X-Exosite-CIK: %s\r\n", cik);
	format_http_command(pdu,
	                    &index,
	                    command);
	format_http_command(pdu,
	                    &index,
	                    "Accept: application/x-www-form-urlencoded; charset=utf-8\r\n");

	sprintf(command, "Request-Timeout: %u\r\n", reqTimeout);

	format_http_command(pdu,
	                    &index,
	                    command);

	if(*recDateTime == 0)
		sprintf(command, "\r\n");
	else
		sprintf(command, "If-Modified-Since: %s\r\n\r\n", recDateTime);

	format_http_command(pdu,
	                    &index,
	                    command);

	return TRUE;
}

bool_t
	build_msg_list_content(	char *pdu,
							int length,
							const char *vendor,
							const char *model,
							const char *cik)
{
	int index = 0;
	char command[HTTP_MSG_SIZE];

	sprintf(command, "GET /provision/download?vendor=%s&model=%s HTTP/1.1\r\n", vendor, model);
	format_http_command(pdu,
						&index,
						command);

	format_http_command(pdu,
	                    &index,
	                    "Host: m2.exosite.com\r\n");

	sprintf(command, "X-Exosite-CIK: %s\r\n\r\n", cik);
	format_http_command(pdu,
	                    &index,
	                    command);

	return TRUE;
}

bool_t
    build_msg_get_content_info(	char *pdu,
								int length,	
								const char *vendor,
								const char *model,
								const char *contentId,
								const char *cik)
{
	int index = 0;
	char command[HTTP_MSG_SIZE];

	sprintf(command, "GET /provision/download?vendor=%s&model=%s&id=%s&info=true HTTP/1.1\r\n", vendor, model, contentId);
	format_http_command(pdu,
						&index,
						command);

	format_http_command(pdu,
	                    &index,
	                    "Host: m2.exosite.com\r\n");

	sprintf(command, "X-Exosite-CIK: %s\r\n\r\n", cik);
	format_http_command(pdu,
	                    &index,
	                    command);

	return TRUE;
}

bool_t
	build_msg_get_content(	char *pdu,
							int length,
							const char *vendor,
							const char *model,
							const char *contentId,
							int startPos,
							int endPos,
							const char *cik)
{
	int index = 0;
	char command[HTTP_MSG_SIZE];

	//assert(startPos <= endPos);

	sprintf(command, "GET /provision/download?vendor=%s&model=%s&id=%s HTTP/1.1\r\n", vendor, model, contentId);
	format_http_command(pdu,
						&index,
						command);

	format_http_command(pdu,
	                    &index,
	                    "Host: m2.exosite.com\r\n");

	sprintf(command, "X-Exosite-CIK: %s\r\n", cik);
	format_http_command(pdu,
	                    &index,
	                    command);

	sprintf(command, "Range: bytes=%d-%d\r\n\r\n", startPos, endPos);
	format_http_command(pdu,
	                    &index,
	                    command);

	return TRUE;
}

bool_t
    parse_msg_read(	const char *parseData,
					int dataLen,
					exosite_data_port_t *dataPorts,
					int numofDataPorts) //pointer???
{
    char *pch;
	int incCnt;
	int dataPortIndex;

	char content[HTTP_MSG_SIZE];
	int contentLen;

	contentLen = sizeof(content);
	if(!get_http_content(	parseData,
							dataLen,
							content,
							&contentLen))
		return FALSE;

	incCnt = 0;
	dataPortIndex = 0;

	if((pch = strtok(content, "=&")) == NULL)
		return FALSE;

	strcpy(dataPorts[dataPortIndex].alias, pch);
	++incCnt;

	while((pch = strtok(NULL, "=&")) != NULL)
	{
		if(incCnt % 2 != 0)
		{
			strcpy(dataPorts[dataPortIndex].value, pch);
			++incCnt;
			++dataPortIndex;
		}
		else
		{
			strcpy(dataPorts[dataPortIndex].alias, pch);
			++incCnt;
		}
	}

	if(	incCnt % 2 != 0 ||
		dataPortIndex > numofDataPorts)
	{
		return FALSE;
	}

	return TRUE;
}

bool_t
    parse_rsp_status(	const char *parseData,
						int dataLen,
						int *status)
{
    *status = (parseData[9] - 0x30) * 100 +
                (parseData[10] - 0x30) * 10 +
                (parseData[11] - 0x30);

    return TRUE;
}

bool_t
    parse_cik_info(	const char *parseData,
					int dataLen,
					char *cik)
{
	char content[HTTP_MSG_SIZE];
	int contentLen;

	contentLen = sizeof(content);
	if(!get_http_content(	parseData,
							dataLen,
							content,
							&contentLen))
		return FALSE;

	assert(contentLen == CIK_LENGTH);

	memcpy(cik, content, CIK_LENGTH);

	return TRUE;

}

bool_t
	parse_content_list(	const char *parseData,
						int dataLen,
						content_id_t *idList,
						int *listSize)
{
	char content[HTTP_MSG_SIZE];
	int contentLen;
	int offset;
	int idIndex;

	contentLen = sizeof(content);
	if(!get_http_content(	parseData,
							dataLen,
							content,
							&contentLen))
		return FALSE;

	offset = 0;
	idIndex = 0;
	while(offset < contentLen)
	{
		if(!sscanf(content + offset, "%s", idList[idIndex].id))
			break;

		offset += (strlen(idList[idIndex].id) + 2); //plus "\r\n"
		++idIndex;
	}

	assert(*listSize >= idIndex);

	*listSize = idIndex;

	return TRUE;
}

bool_t
	parse_content_info(	const char *parseData,
						int dataLen,
						content_info_t *contentInfo)
{
	char content[HTTP_MSG_SIZE];
	int contentLen;
	char *pch;

	contentLen = sizeof(content);
	if(!get_http_content(	parseData,
							dataLen,
							content,
							&contentLen))
		return FALSE;

	pch = strtok(content, ",");

	if(!pch)
		return FALSE;

	strcpy(contentInfo->contentType, pch);
	//TODO: convert to enum

	pch = strtok(NULL, ",");

	if(!pch)
		return FALSE;

	sscanf(pch, "%d", &contentInfo->contentSize);

	pch = strtok(NULL, ",");

	if(!pch)
		return FALSE;

	strcpy(contentInfo->updatedTimeStamp.toString, pch);

	return TRUE;
}

bool_t
	parse_content(	const char *parseData,
					int dataLen,
					uint8_t *buf,
					int *bufSize)
{
	char content[HTTP_CONTENT_DOWNLOAD_MSG_SIZE];
	int contentLen;

	contentLen = sizeof(content);
	if(!get_http_content(	parseData,
							dataLen,
							content,
							&contentLen))
		return FALSE;

	assert(*bufSize >= contentLen);

	memcpy(buf, content, contentLen);
	*bufSize = contentLen;

	return TRUE;
}

static void
   format_http_command(	char *pdu,
						int *index,
						char *httpHeader)
{
    char command[HTTP_MSG_SIZE];

    sprintf(command, "%s", httpHeader);
    sprintf(pdu + *index, "%s", command);
    *index += strlen(command);
}



static bool_t
	get_http_content(	const char *parseData,
						int dataLen,
						char *content,
						int *contentLen)
{
	char value[16];
	int val;

	if(!parse_key_value(parseData,
						dataLen,
						"Content-Length",
						value))
		return FALSE;

	sscanf(value, "%d", &val);

	if(!find_http_content(	parseData,
							dataLen,
							val,
							content,
							contentLen))
		return FALSE;

	return TRUE;
}

static bool_t
	find_http_content(	const char *parseData,
						int dataLen,
						int httpContentLen,
						char *content,
						int *contentLen)
{
 	unsigned char crlfCnt = 0;
    int index = 0;
    char *px = parseData;
	int ax;

    while(index < dataLen)
    {
        if(*px == '\r' || *px == '\n')
        {
            ++crlfCnt;
        }
        else
        {
            crlfCnt = 0;
        }

        ++index;
        ++px;

        if(crlfCnt == 4)
        {
			ax = 0;
            while(ax < httpContentLen)
            {
                content[ax++] = *px;
                ++px;
            }
            content[ax] = 0;

			assert(*contentLen >= ax);
			*contentLen = ax;
            return TRUE;
        }
    }

    return FALSE;
}

//key: value\r\n
//e.g., Content-Length: 200\r\n
static bool_t
    parse_key_value(const char *parseData,
					int dataLen,
					const char *key,
					char *value)
{
    char *keyStart;
    char *valueStart;
    char *valueEnd;
    char *index;

    if(!value)
        return FALSE;

    keyStart = strstr(parseData, key);

    if(!keyStart)
        return FALSE;

    valueStart = strstr(keyStart, ":");

    if(!valueStart)
        return FALSE;

    valueEnd = strstr(valueStart, "\r\n");

    if(!valueEnd)
        return FALSE;

    for(index = valueStart + 2; index < valueEnd; ++index)
    {
        *value = *index;

        ++value;
    }

    *value = 0;

    return TRUE;
}

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <typedefs.h>
#include <exosite.h>
#include <config.h>
#include <platform/exosite_pal.h>

#include "utility.h"

#define PDU_LENGTH HTTP_MSG_SIZE
#define WAIT_TIME_MS 2000

enum
{
    EXO_SOCK_STATE_IDLE = 0,
	EXO_SOCK_STATE_REQ_READY,
    EXO_SOCK_STATE_WAIT_RSP,
    //EXO_SOCK_STATE_DISCONNECT,
    //EXO_SOCK_STATE_COMPLETED,
};

enum
{
	EXO_DEVICE_STATE_UNINITIALIZED,
	EXO_DEVICE_STATE_INITIALIZED
};

enum
{
    EXO_OP_MODE_NOP,
    EXO_OP_MODE_ACTIVATE,
    EXO_OP_MODE_READ,
    EXO_OP_MODE_WRITE,
    EXO_OP_MODE_SUBSCRIBE,
	EXO_OP_MODE_CONTENT_LIST,
	EXO_OP_MODE_CONTENT_INFO,
	EXO_OP_MODE_CONTENT_DOWNLOAD
};

typedef struct
{
    int sock;
    char pdu[PDU_LENGTH];
    exosite_data_port_t dataPorts[NUM_OF_DATA_PORTS];
	size_t dataPortIndex;
    //char recDateTime[128];
	//int reqTimeout;
	exo_result_callback_t resultCallback;

    exosite_timer_t waitTimer;
    uint8_t sockStatus;
    uint8_t opMode;

	int opStatus;
} exosite_http_context_t;

typedef struct
{
    exosite_http_context_t queueInstance[NUM_OF_SOCKETS];
	char vendor[64];
	char model[64];
	char sn[32];
    char cik[CIK_LENGTH + 1];

	uint8_t deviceStatus;
} exosite_http_context_queue_t;

static exosite_http_context_queue_t exoHttpContextQueue;

static char recDateTime[128];

static exo_error_status_t
	exo_activate(	const char *vendor,
					const char *model,
					const char *sn);

static void
	release_op(exosite_http_context_t *context);

static exosite_http_context_t *
	get_http_op_context(uint8_t opMode);

static bool_t
	build_http_msg(exosite_http_context_t *);

static bool_t
	assign_call_back(	exo_result_callback_t lp,
						const exo_result_callback_t rp);

static void
	check_http_error_status_to_determine_if_reset_cik(int status);

static void
	close_socket(int *sp);

//static exosite_http_context_t *downloadContext = NULL;

exo_error_status_t
	exo_download_list_content(	const char *vendor,
								const char *model,
								content_id_t *contentIdList,
								int *listSize)
{
	char pdu[PDU_LENGTH];
    int bufLen;
    int status;
    exosite_http_context_t *opContext;
	exosite_timer_t waitTimer;

	if(exoHttpContextQueue.deviceStatus != EXO_DEVICE_STATE_INITIALIZED)
	{
		while(exo_activate(	exoHttpContextQueue.vendor,
							exoHttpContextQueue.model,
							exoHttpContextQueue.sn) != EXO_ERROR_STATUS_SUC)
		{
		}

		exoHttpContextQueue.deviceStatus = EXO_DEVICE_STATE_INITIALIZED;
	}

    build_msg_list_content(	pdu,
							sizeof(pdu),	
							vendor,
							model,
							exoHttpContextQueue.cik);

    opContext = get_http_op_context(EXO_OP_MODE_CONTENT_LIST);

    if(!opContext)
    {
    	return EXO_ERROR_STATUS_FAILED;
    }

    if(!exosite_pal_sock_write( &opContext->sock,
                                pdu,
                                strlen(pdu)))
    {
    	close_socket(&opContext->sock);
    	release_op(opContext);
        return EXO_ERROR_STATUS_FAILED;
    }

	exosite_pal_timer_init(&waitTimer);
	exosite_pal_timer_countdown_ms(&waitTimer, WAIT_TIME_MS);
	while(1)
	{
		if(exosite_pal_timer_expired(&waitTimer))
		{
			close_socket(&opContext->sock);
	    	release_op(opContext);
			return EXO_ERROR_TIME_OUT_FAILED;
		}

		memset(pdu, 0, sizeof(pdu));
		bufLen = sizeof(pdu);
		if(exosite_pal_sock_read(	&opContext->sock,
									pdu,
		                        	&bufLen)  > 0)
			break;
	}

	if(!parse_rsp_status(	pdu,
							bufLen,
							&status))
	{
		close_socket(&opContext->sock);
    	release_op(opContext);
		return EXO_ERROR_STATUS_FAILED;
	}

	if(status != 200)
	{
		check_http_error_status_to_determine_if_reset_cik(status);
		release_op(opContext);
		return EXO_ERROR_STATUS_FAILED;
	}

	if(!parse_content_list(	pdu,
							bufLen,
							contentIdList,
							listSize))
	{
		release_op(opContext);
		return EXO_ERROR_STATUS_FAILED;
	}


	release_op(opContext);
	return EXO_ERROR_STATUS_SUC;
}

exo_error_status_t
	exo_download_get_content_info(	const char *vendor,
									const char *model,
									const char *contentId,
									content_info_t *contentInfo)
{
	char pdu[PDU_LENGTH];
    int bufLen;
    int status;
    exosite_http_context_t *opContext;
	exosite_timer_t waitTimer;

	if(exoHttpContextQueue.deviceStatus != EXO_DEVICE_STATE_INITIALIZED)
	{
		while(exo_activate(	exoHttpContextQueue.vendor,
							exoHttpContextQueue.model,
							exoHttpContextQueue.sn) != EXO_ERROR_STATUS_SUC)
		{
		}

		exoHttpContextQueue.deviceStatus = EXO_DEVICE_STATE_INITIALIZED;
	}

    build_msg_get_content_info(	pdu,
								sizeof(pdu),	
								vendor,
								model,
								contentId,
								exoHttpContextQueue.cik);

    opContext = get_http_op_context(EXO_OP_MODE_CONTENT_INFO);

    if(!opContext)
    {
    	return EXO_ERROR_STATUS_FAILED;
    }

    if(!exosite_pal_sock_write( &opContext->sock,
                                pdu,
                                strlen(pdu)))
    {
    	close_socket(&opContext->sock);
    	release_op(opContext);
        return EXO_ERROR_STATUS_FAILED;
    }

	exosite_pal_timer_init(&waitTimer);
	exosite_pal_timer_countdown_ms(&waitTimer, WAIT_TIME_MS);
	while(1)
	{
		if(exosite_pal_timer_expired(&waitTimer))
		{
			close_socket(&opContext->sock);
			release_op(opContext);
			return EXO_ERROR_TIME_OUT_FAILED;
		}

		memset(pdu, 0, sizeof(pdu));
		bufLen = sizeof(pdu);
		if(exosite_pal_sock_read(	&opContext->sock,
									pdu,
		                        	&bufLen)  > 0)
			break;
	}


	if(!parse_rsp_status(	pdu,
							bufLen,
							&status))
	{
		close_socket(&opContext->sock);
		release_op(opContext);
		return EXO_ERROR_STATUS_FAILED;
	}

	if(status != 200)
	{
		check_http_error_status_to_determine_if_reset_cik(status);
		release_op(opContext);
		return EXO_ERROR_STATUS_FAILED;
	}

	if(!parse_content_info(	pdu,
							bufLen,
							contentInfo))
	{
		release_op(opContext);
		return EXO_ERROR_STATUS_FAILED;
	}

	release_op(opContext);
	return EXO_ERROR_STATUS_SUC;
}

exo_error_status_t
	exo_download_get_content(	const char *vendor,
								const char *model,
								const char *contentId,
								uint32_t startPos,
				 				uint32_t endPos,
								uint8_t *buf,
								int *bufSize)
{
	char pdu[HTTP_CONTENT_DOWNLOAD_MSG_SIZE];
    int bufLen;
    int status;
    exosite_timer_t waitTimer;
    exosite_http_context_t *opContext;

	if(exoHttpContextQueue.deviceStatus != EXO_DEVICE_STATE_INITIALIZED)
	{
		//return -1;
		while(exo_activate(	exoHttpContextQueue.vendor,
							exoHttpContextQueue.model,
							exoHttpContextQueue.sn) != EXO_ERROR_STATUS_SUC)
		{
		}

		exoHttpContextQueue.deviceStatus = EXO_DEVICE_STATE_INITIALIZED;
	}

    opContext = get_http_op_context(EXO_OP_MODE_CONTENT_DOWNLOAD);

    if(!opContext)
    {
    	return EXO_ERROR_STATUS_FAILED;
    }

    build_msg_get_content(	pdu,
							sizeof(pdu),
							vendor,
							model,
							contentId,
							startPos,
							endPos,
							exoHttpContextQueue.cik);

    if(!exosite_pal_sock_write( &opContext->sock ,
                                pdu,
                                strlen(pdu)))
    {
		close_socket(&opContext->sock);
		release_op(opContext);
        return EXO_ERROR_STATUS_FAILED;
    }

	exosite_pal_timer_init(&waitTimer);
	exosite_pal_timer_countdown_ms(&waitTimer, WAIT_TIME_MS);

	while(1)
	{
		if(exosite_pal_timer_expired(&waitTimer))
		{
			close_socket(&opContext->sock);
			release_op(opContext);
			return EXO_ERROR_TIME_OUT_FAILED;
		}

		memset(pdu, 0, sizeof(pdu));
		bufLen = sizeof(pdu);
		if(exosite_pal_sock_read(	&opContext->sock,
									pdu,
		                        	&bufLen)  > 0)
		{
			break;
		}
	}

	if(!parse_rsp_status(	pdu,
							bufLen,
							&status))
	{
		close_socket(&opContext->sock);
		release_op(opContext);
		return EXO_ERROR_STATUS_FAILED;
	}

	if(status != 200 && status != 206)
	{
		check_http_error_status_to_determine_if_reset_cik(status);
		release_op(opContext);
		return EXO_ERROR_STATUS_FAILED;
	}

	if(!parse_content(	pdu,
						bufLen,
						buf,
						bufSize))
	{
		release_op(opContext);
		return EXO_ERROR_STATUS_FAILED;
	}

	release_op(opContext);
	return EXO_ERROR_STATUS_SUC;
}

void
	exo_init(	const char *vendor,
				const char *model,
				const char *sn)
{
	int i;

	memset(exoHttpContextQueue.cik, 0, sizeof(exoHttpContextQueue.cik));
	for(i = 0; i < NUM_OF_SOCKETS; ++i)
	{
		release_op(&exoHttpContextQueue.queueInstance[i]);
		exoHttpContextQueue.queueInstance[i].sock = -1;
	}

	strcpy(exoHttpContextQueue.vendor, vendor);
	strcpy(exoHttpContextQueue.model, model);
	strcpy(exoHttpContextQueue.sn, sn);

	exosite_pal_init();
}

void
	exo_loop_start()
{
	int i, j;
    char dataBuf[PDU_LENGTH];
    int bufLen;
	int status;
	int isExited;

	while(1)
	{
		for(i = 0; i < NUM_OF_SOCKETS; ++i)
		{
			if(exoHttpContextQueue.queueInstance[i].opMode != EXO_OP_MODE_NOP)
			{
				if(exoHttpContextQueue.queueInstance[i].sockStatus == EXO_SOCK_STATE_REQ_READY)
				{

					if(!build_http_msg(&exoHttpContextQueue.queueInstance[i]))
					{
						exoHttpContextQueue.queueInstance[i].opStatus = -1;
						exoHttpContextQueue.queueInstance[i].resultCallback(NULL,
																			exoHttpContextQueue.queueInstance[i].opStatus);

						release_op(&exoHttpContextQueue.queueInstance[i]);
						continue;
					}

					if(!exosite_pal_sock_write( &exoHttpContextQueue.queueInstance[i].sock,
										        exoHttpContextQueue.queueInstance[i].pdu,
										        strlen(exoHttpContextQueue.queueInstance[i].pdu)))
					{
						exoHttpContextQueue.queueInstance[i].opStatus = -1;

						exoHttpContextQueue.queueInstance[i].resultCallback(NULL,
																			exoHttpContextQueue.queueInstance[i].opStatus);

						close_socket(&exoHttpContextQueue.queueInstance[i].sock);
						release_op(&exoHttpContextQueue.queueInstance[i]);
						continue;
					}

					exosite_pal_timer_init(&exoHttpContextQueue.queueInstance[i].waitTimer);
					exosite_pal_timer_countdown_ms(&exoHttpContextQueue.queueInstance[i].waitTimer, WAIT_TIME_MS);
					exoHttpContextQueue.queueInstance[i].sockStatus = EXO_SOCK_STATE_WAIT_RSP;
				}
				else if(exoHttpContextQueue.queueInstance[i].sockStatus == EXO_SOCK_STATE_WAIT_RSP)
				{
					if(exosite_pal_timer_expired(&exoHttpContextQueue.queueInstance[i].waitTimer))
					{
						exoHttpContextQueue.queueInstance[i].opStatus = -1;

						exoHttpContextQueue.queueInstance[i].resultCallback(NULL,
																			exoHttpContextQueue.queueInstance[i].opStatus);

						close_socket(&exoHttpContextQueue.queueInstance[i].sock);
						release_op(&exoHttpContextQueue.queueInstance[i]);

					}
					else
					{
						memset(dataBuf, 0, sizeof(dataBuf));
		            	bufLen = sizeof(dataBuf);

						if(!exosite_pal_sock_read(  &exoHttpContextQueue.queueInstance[i].sock,
												    dataBuf,
												    &bufLen))
							continue;

						switch(exoHttpContextQueue.queueInstance[i].opMode)
						{
							case EXO_OP_MODE_READ:
								if(parse_rsp_status(dataBuf,
													bufLen,
													&status))
								{
									exoHttpContextQueue.queueInstance[i].opStatus = status;

									if(status == 200)
									{								
										if(!parse_msg_read(	dataBuf,
															bufLen,
															exoHttpContextQueue.queueInstance[i].dataPorts,
															exoHttpContextQueue.queueInstance[i].dataPortIndex))
										{
											assert(FALSE);
										}
									}

									check_http_error_status_to_determine_if_reset_cik(status);

									for(j = 0; j < exoHttpContextQueue.queueInstance[i].dataPortIndex; ++j)
									{
										exoHttpContextQueue.queueInstance[i].resultCallback(&exoHttpContextQueue.queueInstance[i].dataPorts[j],
																							exoHttpContextQueue.queueInstance[i].opStatus);	
									}

									/*exoHttpContextQueue.queueInstance[i].resultCallback(exoHttpContextQueue.queueInstance[i].dataPorts,
																						exoHttpContextQueue.queueInstance[i].dataPortIndex,
																						exoHttpContextQueue.queueInstance[i].opStatus);*/

									release_op(&exoHttpContextQueue.queueInstance[i]);
								}
							break;

							case EXO_OP_MODE_WRITE:
								if(parse_rsp_status(dataBuf,
													bufLen,
													&status))
								{
									exoHttpContextQueue.queueInstance[i].opStatus = status;
									exoHttpContextQueue.queueInstance[i].resultCallback(NULL,
																						exoHttpContextQueue.queueInstance[i].opStatus);

									release_op(&exoHttpContextQueue.queueInstance[i]);

									check_http_error_status_to_determine_if_reset_cik(status);
								}
							break;

							case EXO_OP_MODE_SUBSCRIBE:
								if(parse_rsp_status(dataBuf,
													bufLen,
													&status))
								{
									exoHttpContextQueue.queueInstance[i].opStatus = status;
									if(status == 304)
									{

										/*parse_key_value(dataBuf,
														bufLen,
														"Date",
														recDateTime);*/

									}
									else if(status == 200)
									{
										/*parse_key_value(dataBuf,
														bufLen,
														"Date",
														recDateTime);*/

										if(parse_msg_read(  dataBuf,
															bufLen,
															exoHttpContextQueue.queueInstance[i].dataPorts,
															exoHttpContextQueue.queueInstance[i].dataPortIndex))
										{
											printf("got subscribe data\r\n");
										}
									}

									check_http_error_status_to_determine_if_reset_cik(status);

									for(j = 0; j < exoHttpContextQueue.queueInstance[i].dataPortIndex; ++j)
									{
										exoHttpContextQueue.queueInstance[i].resultCallback(&exoHttpContextQueue.queueInstance[i].dataPorts[j],
																							exoHttpContextQueue.queueInstance[i].opStatus);
									}

									/*exoHttpContextQueue.queueInstance[i].resultCallback(exoHttpContextQueue.queueInstance[i].dataPorts,
																						exoHttpContextQueue.queueInstance[i].dataPortIndex,
																						exoHttpContextQueue.queueInstance[i].opStatus);*/

									release_op(&exoHttpContextQueue.queueInstance[i]);
								}
							break;

							default:
							break;
						}
					}
				}
			}
		}

		isExited = TRUE;
		for(i = 0; i < NUM_OF_SOCKETS; ++i)
		{
			if(exoHttpContextQueue.queueInstance[i].opMode != EXO_OP_MODE_NOP)//&&
				//exoHttpContextQueue.queueInstance[i].sockStatus != EXO_SOCK_STATE_COMPLETED)
			{
				isExited = FALSE;
				break;
			}
		}

		if(isExited)
		{
			//loop break
			break;
		}
	}
}

exo_desc_t
	exo_write(	const char *alias,
				const char *value,
				exo_result_callback_t writeCallback)
{
	if(exoHttpContextQueue.deviceStatus != EXO_DEVICE_STATE_INITIALIZED)
	{
		while(exo_activate(	exoHttpContextQueue.vendor,
							exoHttpContextQueue.model,
							exoHttpContextQueue.sn) != EXO_ERROR_STATUS_SUC)
		{
		}

		exoHttpContextQueue.deviceStatus = EXO_DEVICE_STATE_INITIALIZED;
	}

	exosite_http_context_t *opContext = get_http_op_context(EXO_OP_MODE_WRITE);
	if(opContext == NULL)
		return -1;

	if(opContext->dataPortIndex >= NUM_OF_DATA_PORTS)
		return -1;

	if(!assign_call_back(opContext->resultCallback, writeCallback))
		return -1;

	opContext->resultCallback = writeCallback;

	assert(opContext->resultCallback != NULL);

	//insert write data to op
	strcpy(opContext->dataPorts[opContext->dataPortIndex].alias, alias);
	strcpy(opContext->dataPorts[opContext->dataPortIndex].value, value);
	++opContext->dataPortIndex;

	opContext->sockStatus = EXO_SOCK_STATE_REQ_READY;
	
	return opContext->sock;
}

exo_desc_t
	exo_read(	const char *alias,
				exo_result_callback_t readCallback)
{
	if(exoHttpContextQueue.deviceStatus != EXO_DEVICE_STATE_INITIALIZED)
	{
		//return -1;
		while(exo_activate(	exoHttpContextQueue.vendor,
							exoHttpContextQueue.model,
							exoHttpContextQueue.sn) != EXO_ERROR_STATUS_SUC)
		{
			printf("activate failed\r\n");
		}

		exoHttpContextQueue.deviceStatus = EXO_DEVICE_STATE_INITIALIZED;
	}

	//insert read data to op
	exosite_http_context_t *opContext = get_http_op_context(EXO_OP_MODE_READ);

	if(opContext == NULL)
	{
		printf("no context\r\n");
		return -1;
	}

	if(opContext->dataPortIndex >= NUM_OF_DATA_PORTS)
	{
		printf("data port filled\r\n");
		return -1;
	}

	if(!assign_call_back(opContext->resultCallback, readCallback))
		return -1;

	opContext->resultCallback = readCallback;

	strcpy(opContext->dataPorts[opContext->dataPortIndex].alias, alias);
	++opContext->dataPortIndex;

	opContext->sockStatus = EXO_SOCK_STATE_REQ_READY;
	
	return opContext->sock;
}

exo_desc_t
	exo_subscribe(	const char *alias,
					const date_time_t *dateTime,
					exo_result_callback_t subscribeCallback)
{
	if(exoHttpContextQueue.deviceStatus != EXO_DEVICE_STATE_INITIALIZED)
	{
		//return -1;
		while(exo_activate(	exoHttpContextQueue.vendor,
							exoHttpContextQueue.model,
							exoHttpContextQueue.sn) != EXO_ERROR_STATUS_SUC)
		{
			printf("activate failed\r\n");
		}

		exoHttpContextQueue.deviceStatus = EXO_DEVICE_STATE_INITIALIZED;
	}

	//insert subscribe data to op
	exosite_http_context_t *opContext = get_http_op_context(EXO_OP_MODE_SUBSCRIBE);

	if(opContext == NULL)
		return -1;

	if(opContext->dataPortIndex >= 1) //only support one dataport subscribe now.
		return -1;

	if(!assign_call_back(opContext->resultCallback, subscribeCallback))
		return -1;

	opContext->resultCallback = subscribeCallback;

	strcpy(opContext->dataPorts[opContext->dataPortIndex].alias, alias);
	++opContext->dataPortIndex;

	opContext->sockStatus = EXO_SOCK_STATE_REQ_READY;

	strcpy(recDateTime, dateTime->toString);

	return opContext->sock;
}

static exo_error_status_t
	exo_activate(	const char *vendor,
					const char *model,
					const char *sn)
{
	char pdu[PDU_LENGTH];
    char dataBuf[PDU_LENGTH];
    int bufLen;
    int status;

	int sock;
	exosite_timer_t waitTimer;

	if(exosite_pal_load_cik(exoHttpContextQueue.cik,
							CIK_LENGTH))
	{
		return EXO_ERROR_STATUS_SUC;
	}

    build_msg_activate(	pdu,
						sizeof(pdu),
						vendor,
						model,
						sn);
	if(!exosite_pal_sock_connect(&sock))
	{
		exosite_pal_sock_close(&sock);
		return EXO_ERROR_STATUS_FAILED;
	}

    if(!exosite_pal_sock_write( &sock,
                                pdu,
                                strlen(pdu)))
    {
    	exosite_pal_sock_close(&sock);
        return EXO_ERROR_STATUS_FAILED;
    }

	exosite_pal_timer_init(&waitTimer);
	exosite_pal_timer_countdown_ms(&waitTimer, WAIT_TIME_MS);

	while(1)
	{
		if(exosite_pal_timer_expired(&waitTimer))
		{
			exosite_pal_sock_close(&sock);
			return EXO_ERROR_TIME_OUT_FAILED;
		}

		bufLen = sizeof(pdu);
		if(exosite_pal_sock_read(	&sock,
									dataBuf,
		                        	&bufLen)  > 0)
			break;
	}

	if(!parse_rsp_status(	dataBuf,
							bufLen,
							&status))
	{
		exosite_pal_sock_close(&sock);
		return EXO_ERROR_STATUS_FAILED;
	}

	if(status != 200)
	{
		exosite_pal_sock_close(&sock);
		return EXO_ERROR_STATUS_FAILED;
	}

	if(!parse_cik_info(	dataBuf,
						bufLen,
						exoHttpContextQueue.cik))
	{
		exosite_pal_sock_close(&sock);
		return EXO_ERROR_STATUS_FAILED;
	}

	exosite_pal_save_cik(	exoHttpContextQueue.cik,
							CIK_LENGTH);

	exosite_pal_sock_close(&sock);

	return EXO_ERROR_STATUS_SUC;
}

static void
	release_op(exosite_http_context_t *context)
{
	//exosite_pal_sock_close(&context->sock);
	//context->sock = -1;
	memset(context->pdu, 0, sizeof(context->pdu));
	context->dataPortIndex = 0;
	context->resultCallback = NULL;
	context->sockStatus = EXO_SOCK_STATE_IDLE;
	context->opMode = EXO_OP_MODE_NOP;
	context->opStatus = -1;
}

static exosite_http_context_t *
	get_http_op_context(uint8_t opMode)
{
	int i;

	// search existed opMode first
	for(i = 0; i < NUM_OF_SOCKETS; ++i)
	{
		if(opMode == exoHttpContextQueue.queueInstance[i].opMode)
			return &exoHttpContextQueue.queueInstance[i];
	}

	// search unoccupied context
	for(i = 0; i < NUM_OF_SOCKETS; ++i)
	{
		if(exoHttpContextQueue.queueInstance[i].opMode == EXO_OP_MODE_NOP)
		{
			if(exoHttpContextQueue.queueInstance[i].sock < 0)
			{
				if(exosite_pal_sock_connect(&exoHttpContextQueue.queueInstance[i].sock))
				{
					exoHttpContextQueue.queueInstance[i].opMode = opMode;
					return &exoHttpContextQueue.queueInstance[i];
				}
			}
			else
			{
				exoHttpContextQueue.queueInstance[i].opMode = opMode;
				return &exoHttpContextQueue.queueInstance[i];
			}
		}
	}

	return NULL;
}

static bool_t
	build_http_msg(exosite_http_context_t *cxt)
{
	switch(cxt->opMode)
	{
		case EXO_OP_MODE_READ:
			return build_msg_read(	cxt->pdu,
									PDU_LENGTH,
									cxt->dataPorts,
									cxt->dataPortIndex,
									exoHttpContextQueue.cik);
		//break;

		case EXO_OP_MODE_WRITE:
			return build_msg_write(	cxt->pdu,
									PDU_LENGTH,
									cxt->dataPorts,
									cxt->dataPortIndex,
									exoHttpContextQueue.cik);
		//break;

		case EXO_OP_MODE_SUBSCRIBE:
			return build_msg_long_polling(	cxt->pdu,
											PDU_LENGTH,
											recDateTime,
											cxt->dataPorts[0].alias,
											500, //cxt->reqTimeout,
											exoHttpContextQueue.cik);
		//break;

		default:
			assert(FALSE);
		break;
	}

	return FALSE;
}

static bool_t
	assign_call_back(	exo_result_callback_t lp,
						const exo_result_callback_t rp)
{
	assert(rp != NULL);

	if(lp != NULL)
	{
		if(lp != rp)
		{
			return FALSE;
		}

		return TRUE;
	}

	lp = rp; // TODO: check why assign fail?

	return TRUE;
}

static void
	check_http_error_status_to_determine_if_reset_cik(int status)
{
	if(status == 401 || status == 403)
	{
		exosite_pal_remove_cik();
		exoHttpContextQueue.deviceStatus = EXO_DEVICE_STATE_UNINITIALIZED;
	}
}

static void
	close_socket(int *sp)
{
	exosite_pal_sock_close((void *)sp);
	*sp = -1;
}


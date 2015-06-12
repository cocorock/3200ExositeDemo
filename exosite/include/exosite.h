#ifndef _EXOSITE_EXOSITE_H_
#define _EXOSITE_EXOSITE_H_

#include <stdio.h>
#include <stdlib.h>

#include <typedefs.h>

typedef enum
{
	EXO_ERROR_STATUS_SUC = 0,
	EXO_ERROR_STATUS_FAILED,
	EXO_ERROR_RSP_FAILED,
	EXO_ERROR_CIK_FAILED,
	EXO_ERROR_TIME_OUT_FAILED
} exo_error_status_t;

typedef void (*exo_result_callback_t)(	const exosite_data_port_t *,
										int opStatus);

void
	exo_init(	const char *vendor,
				const char *model,
				const char *sn); //must init at the first time before executing any operation
// control plane

exo_error_status_t
	exo_download_list_content(	const char *vendor,
								const char *model,	
								content_id_t *idList,
								int *listSize); // in/out, only got the first "listSize" number of contents

exo_error_status_t
	exo_download_get_content_info(	const char *vendor,
									const char *model,
									const char *contentId,
									content_info_t *);

exo_error_status_t
	exo_download_get_content(	const char *vendor,
								const char *model,
								const char *contentId,
								uint32_t startPos,
				 				uint32_t endPos,
								uint8_t *buf,
								int *bufSize); // in/out

// data plane

void
	exo_loop_start();

exo_desc_t
	exo_write(	const char *alias,
				const char *value,
				exo_result_callback_t);

exo_desc_t
	exo_read(	const char *alias,
				exo_result_callback_t);

exo_desc_t
	exo_subscribe(	const char *alias,
					const date_time_t *modifiedSinceDateTime,
					exo_result_callback_t);

#endif

#ifndef _EXOSITE_TYPEDEFS_H_
#define _EXOSITE_TYPEDEFS_H_

#define CIK_LENGTH 40

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
//typedef unsigned long long uint64_t;
typedef int exo_desc_t;

#if defined(FALSE)
	typedef unsigned char bool_t;
#else
	typedef
		enum
		{
			FALSE,
			TRUE
		} bool_t;
#endif

typedef struct
{
    char alias[64];
    char value[32];
} exosite_data_port_t;

typedef struct
{
	char toString[64];
} date_time_t;

typedef struct
{
	char id[32];
	//int idLen;
} content_id_t;

typedef struct
{
	//uint8_t contentType;
	char contentType[32];
	int contentSize;
	date_time_t updatedTimeStamp;
} content_info_t;

#endif

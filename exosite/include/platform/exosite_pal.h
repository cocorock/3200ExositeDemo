//A common interface for platform-dependent socket

#ifndef _EXOSITE_PAL_H_
#define _EXOSITE_PAL_H_

#include <typedefs.h>

#ifndef cc3200
#include <sys/time.h>
#endif

#ifdef cc3200
	typedef struct
	{
		unsigned long systickPeriod;
		unsigned long endTime;
	} exosite_timer_t;
#else
	typedef struct exosite_timer_t
	{
		struct timeval endTime;
	} exosite_timer_t;
#endif

void
	exosite_pal_init();

bool_t
	exosite_pal_sock_connect(void *sock);

bool_t
    exosite_pal_sock_is_connected(void *sock);

bool_t
    exosite_pal_sock_read(  void *sock,
                            char *data,
                            int *dataLen); //in/out function

bool_t
    exosite_pal_sock_write( void *sock,
                            const char *data,
                            int dataLen);

void
	exosite_pal_sock_close(void *sock);

bool_t
	exosite_pal_load_cik(	char *cik,
							int cikLen);

void
	exosite_pal_save_cik(	const char *cik,
							int cikLen);

void
	exosite_pal_remove_cik();

bool_t
	exosite_pal_timer_expired(exosite_timer_t *);

void
	exosite_pal_timer_countdown_ms(	exosite_timer_t *,
									unsigned int);
void
	exosite_pal_timer_countdown(exosite_timer_t *,
								unsigned int);

int
	exosite_pal_timer_left_ms(exosite_timer_t *);

void
	exosite_pal_timer_init(exosite_timer_t *);

bool_t
	exosite_pal_get_current_date_time(date_time_t *);

#endif

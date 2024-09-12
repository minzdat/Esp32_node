#ifndef UDP_LOGGING_MAX_PAYLOAD_LEN
#define UDP_LOGGING_MAX_PAYLOAD_LEN 2048
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"
extern int udp_log_fd;

int udp_logging_init(const char *ipaddr, unsigned long port, vprintf_like_t func);
int udp_logging_vprintf( const char *str, va_list l );
int vsprintfunc(const char* buf, const char *str, va_list l);
void udp_logging_free(va_list l);

#ifdef __cplusplus
}
#endif

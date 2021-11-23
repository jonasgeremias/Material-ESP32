#ifndef UDP_LOGGING_MAX_PAYLOAD_LEN
#define UDP_LOGGING_MAX_PAYLOAD_LEN 1024
#endif

const int LOG_UDP_PORT = 6000;
const char *LOG_UDP_IP = "192.168.1.167";

#ifdef __cplusplus
extern "C" {
#endif
extern int udp_log_fd;
int log_udp_init(const char *ipaddr, unsigned long port, vprintf_like_t func);
static int log_udp_vprintf(const char *str, va_list l);
void log_udp_free(va_list l);

#ifdef __cplusplus
}
#endif
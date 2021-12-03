#include "esp_system.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include <string.h>
#include "LOG_UDP.h"

int udp_log_fd;
struct sockaddr_in serveraddr;
uint8_t udp_log_buf[UDP_LOGGING_MAX_PAYLOAD_LEN];
const char *ESP_LOG_UDP = "LOG_UDP";
static volatile uint8_t log_udp_try_again = 0;

int get_socket_error_code(int socket)
{
   int result;
   u32_t optlen = sizeof(int);
   if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen) == -1)
   {
      return ESP_FAIL;
   }
   return result;
}

int show_socket_error_reason(int socket)
{
   int err = get_socket_error_code(socket);
   printf("UDP SOCKET ERROR %d %s", err, strerror(err));
   return err;
}

void log_udp_free(va_list l)
{
   int err = 0;
   char *err_buf;
   esp_log_set_vprintf(vprintf);
   if ((err = shutdown(udp_log_fd, 2)) == 0)
   {
      vprintf("\nUDP SOCKET SHUTDOWN!\n", l);
   }
   else
   {
      asprintf(&err_buf, "\nSHUTTING-DOWN UDP SOCKET FAILED: %d!\n", err);
      vprintf(err_buf, l);
   }

   if ((err = close(udp_log_fd)) == 0)
   {
      vprintf("\nUDP SOCKET CLOSED!\n", l);
   }
   else
   {
      asprintf(&err_buf, "\nCLOSING UDP SOCKET FAILED: %d!\n", err);
      vprintf(err_buf, l);
   }
   udp_log_fd = 0;
}

static int log_udp_vprintf(const char *str, va_list l)
{
   int err = 0;
   int len;
   char task_name[16];
   char *cur_task = pcTaskGetTaskName(xTaskGetCurrentTaskHandle());
   strncpy(task_name, cur_task, 16);
   task_name[15] = 0;
   if (strncmp(task_name, "tiT", 16) != 0)
   {
      len = vsprintf((char *)udp_log_buf, str, l);
      if ((err = sendto(udp_log_fd, udp_log_buf, len, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
      {
         show_socket_error_reason(udp_log_fd);
         log_udp_free(l);

         if (++log_udp_try_again < 10)
         {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
         }
         else
         {
            log_udp_try_again = 10;
            vTaskDelay(5000 / portTICK_PERIOD_MS);
         }

         log_udp_init(LOG_UDP_IP, LOG_UDP_PORT, log_udp_vprintf);
         // err = log_udp_init(LOG_UDP_IP, LOG_UDP_PORT, log_udp_vprintf);

         return vprintf("UDP LOGGING FREE!\n", l);
      }
   }
   return vprintf(str, l);
}

int log_udp_init(const char *ipaddr, unsigned long port, vprintf_like_t func)
{
   struct timeval send_timeout = {1, 0};
   udp_log_fd = 0;
   ESP_LOGI(ESP_LOG_UDP, "INITIALIZING LOG UDP");

   if ((udp_log_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      ESP_LOGE(ESP_LOG_UDP, "CANNOT OPEN SOCKET!");
      return ESP_FAIL;
   }

   uint32_t ip_addr_bytes;
   inet_aton(ipaddr, &ip_addr_bytes);
   ESP_LOGI(ESP_LOG_UDP, "LOGGING TO 0X%x\n", ip_addr_bytes);

   memset(&serveraddr, 0, sizeof(serveraddr));
   serveraddr.sin_family = AF_INET;
   serveraddr.sin_port = htons(port);
   serveraddr.sin_addr.s_addr = ip_addr_bytes;

   int err = setsockopt(udp_log_fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&send_timeout, sizeof(send_timeout));
   if (err < 0)
   {
      ESP_LOGE(ESP_LOG_UDP, "FAILED TO SET SO_SNDTIMEO. ERROR %d", err);
   }
   else
      log_udp_try_again = 0;

   esp_log_set_vprintf(func);

   return ESP_OK;
}
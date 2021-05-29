#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_system.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include <esp_ota_ops.h>

#ifdef DEBUG_AP_SERVER
const char *LOG_SERVER = "SERVER";
#endif

#ifdef DEBUG_OTA
const char *LOG_OTA = "OTA";
#endif

httpd_handle_t server = NULL;

typedef void (*function_t)(void);

// Carrega os arquivos da memoria do ESP32
#define ARRAY_SIZE_OF(a) (sizeof(a) / sizeof(a[0]))

// Index
extern const unsigned char index_html_start[] asm("_binary_index_html_start");
extern const unsigned char index_html_end[] asm("_binary_index_html_end");
// Style and frameworks
extern const unsigned char styles_css_start[] asm("_binary_styles_css_start");
extern const unsigned char styles_css_end[] asm("_binary_styles_css_end");
extern const unsigned char mini_css_start[] asm("_binary_mini_css_start");
extern const unsigned char mini_css_end[] asm("_binary_mini_css_end");
// favicon
extern const unsigned char favicon_png_start[] asm("_binary_favicon_png_start");
extern const unsigned char favicon_png_end[] asm("_binary_favicon_png_end");
// OTA
extern const unsigned char ota_html_start[] asm("_binary_ota_html_start");
extern const unsigned char ota_html_end[] asm("_binary_ota_html_end");
// Restart
extern const unsigned char restart_html_start[] asm("_binary_restart_html_start");
extern const unsigned char restart_html_end[] asm("_binary_restart_html_end");

// Strutura para verifiar arquivos estáticos
typedef struct static_content {
   const char *path;
   const unsigned char *data_start;
   const unsigned char *data_end;
   const char *content_type;
   bool is_gzip;
   function_t function;
} static_content_t;

int status_update_ota = 0;
static void ota_clear_status() {
   status_update_ota = 0;
}

static void restart_set_reset() {
   esp_restart_now = 1;
}

// Restart, não está na rota de arquivos estáticos, porque precisa configurar o tempo de restart.

// Método GET -----------------------------------------------------------------
static esp_err_t
   http_handle_get(httpd_req_t *req);

// Método para OTA ------------------------------------------------------------
static esp_err_t receive_ota_handle(httpd_req_t *req);

// Registros de estruturar de eventos -----------------------------------------
static httpd_uri_t http_uri_get = {
   .uri = "/*",
   .method = HTTP_GET,
   .handler = http_handle_get,
   .user_ctx = NULL};

static httpd_uri_t http_receive_ota_handle = {
   .uri = "/otaupdatefirmware",
   .method = HTTP_POST,
   .handler = receive_ota_handle,
   .user_ctx = NULL};

// Função de inicio da aplicaçõo de servidor
static void start_webserver(void);

static static_content_t content_list[] = {
   {"/", index_html_start, index_html_end, "text/html", false, NULL},
   {"/index.html", index_html_start, index_html_end, "text/html", false, NULL},
   {"/ota.html", ota_html_start, ota_html_end, "text/html", false, &ota_clear_status},
   {"/restart.html", restart_html_start, restart_html_end, "text/html", false, &restart_set_reset},
   {"/mini.css", mini_css_start, mini_css_end, "text/css", false, NULL},
   {"/styles.css", styles_css_start, styles_css_end, "text/css", false, NULL},
   {"/favicon.png", favicon_png_start, favicon_png_end, "image/x-icon", false, NULL}};
// Método GET -----------------------------------------------------------------
static esp_err_t http_handle_get(httpd_req_t *req) {
   
#ifdef DEBUG_AP_SERVER
   ESP_LOGW(LOG_SERVER, "Req->%s", req->uri);
#endif

   static_content_t *content = NULL;
   for (uint32_t i = 0; i < ARRAY_SIZE_OF(content_list); i++) {
      // Existe a rota
      if (strstr(req->uri, content_list[i].path) != NULL) {
         // Os textos são do mesmo tamanho
         if (strlen(req->uri) == strlen(content_list[i].path)) {
            // Conteudo não é nulo
            if (content_list[i].data_start != NULL) {
               content = &(content_list[i]);
               break;
            }
         }
      }
   }

   // Se foi alguma requisição de arquivo estático
   if (content != NULL) {
      // Executa a função declarada
      if (content->function != NULL) {
         content->function();
      }
      
      // Coloca no header o tipo de conteudo
      ESP_ERROR_CHECK(httpd_resp_set_type(req, content->content_type));
   
      // Se for do tipo GZIP, informa no header http
      if (content->is_gzip) {
         ESP_ERROR_CHECK(httpd_resp_set_hdr(req, "Content-Encoding", "gzip"));
      }
      
      // Envia o arquivo
      httpd_resp_send_chunk(req, (const char *) content->data_start, content->data_end - content->data_start);
      //  Envia um caracter nulo no final 
      httpd_resp_send_chunk(req, NULL, 0);
      return ESP_OK;
   }
   else if (strstr(req->uri, "/otastatus.json") != NULL) {
      char saida[100];
      sprintf(saida, "{\"status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", status_update_ota, __TIME__, __DATE__);
      httpd_resp_sendstr(req, saida);
      return ESP_OK;
   }

   // Se chegou até aqui, nao existe a rota.
   httpd_resp_send_404(req);
   return ESP_OK;
}

// OTA
static esp_err_t receive_ota_handle(httpd_req_t *req)
{
   
#ifdef DEBUG_OTA
   ESP_LOGI(LOG_OTA, "receive_ota_handle");
#endif
   esp_ota_handle_t ota_handle;

   char ota_buff[2048];

   int content_length = req->content_len;
   int content_received = 0;
   int recv_len;
   bool is_req_body_started = false;
   const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
#ifdef DEBUG_OTA
   ESP_LOGW(LOG_OTA, "---->Partition label: '%s'\n", update_partition->label);
   ESP_LOGW(LOG_OTA, "---->Partition size: '%d'\n", update_partition->size);
#endif

   status_update_ota = -1; // Código de erro 
   do
   {
      // leitura do dado recebido
      if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
      {
         // Erro
         if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
         {
#ifdef DEBUG_OTA
            ESP_LOGI(LOG_OTA, "Socket Timeout");
#endif
            continue;
         }
#ifdef DEBUG_OTA
         ESP_LOGI(LOG_OTA, "OTA Other Error %d", recv_len);
#endif
         return ESP_FAIL;
      }

#ifdef DEBUG_OTA
      ESP_LOGI(LOG_OTA, "RX: %d of %d\r", content_received, content_length);
#endif
      // Primeiro pacote completo, separa o body
      if (!is_req_body_started)
      {
         is_req_body_started = true;
         char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
         int body_part_len = recv_len - (body_start_p - ota_buff);

#ifdef DEBUG_OTA
         int body_part_sta = recv_len - body_part_len;
         ESP_LOGI(LOG_OTA, "File Size: %d : Start Location:%d - End Location:%d", content_length, body_part_sta, body_part_len);
         ESP_LOGI(LOG_OTA, "File Size: %d", content_length);
#endif
      
         // Iniciando a partição que a nova OTA ocupara
         esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
         if (err != ESP_OK)
         {
#ifdef DEBUG_OTA
            ESP_LOGE(LOG_OTA, "Error With OTA Begin, Cancelling OTA");
#endif
            return ESP_FAIL;
         }
         else
         {
#ifdef DEBUG_OTA
            ESP_LOGI(LOG_OTA, "Writing to partition subtype %d at offset 0x%x", update_partition->subtype, update_partition->address);
#endif
         }

         // Começa a escrever na OTA
         esp_ota_write(ota_handle, body_start_p, body_part_len);
      }
      else
      {
         // Escreve na OTA a cada pacote recebido
         esp_ota_write(ota_handle, ota_buff, recv_len);
         
         content_received += recv_len;
      }
   } while (recv_len > 0 && content_received < content_length);

   // Finaliza a OT
   if (esp_ota_end(ota_handle) == ESP_OK)
   {
      // Seta para que o boot inicie n nova partição
      if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
      {

#ifdef DEBUG_OTA
         const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
         ESP_LOGI(LOG_OTA, "Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, boot_partition->address);
         ESP_LOGI(LOG_OTA, "Please Restart System...");
#endif
         status_update_ota = 1;
      }
      else
      {
#ifdef DEBUG_OTA
         ESP_LOGE(LOG_OTA, "!!! Flashed Error !!!");
#endif
         status_update_ota = -1;
      }
   }
   else
   {
#ifdef DEBUG_OTA
      ESP_LOGE(LOG_OTA, "!!! OTA End Error !!!");
#endif
      status_update_ota = -1;
   }

   return ESP_OK;
}


// Função de inicio da aplicaçõo de servidor
static void start_webserver(void) {
   // Configuração padrão
   httpd_config_t config = HTTPD_DEFAULT_CONFIG();
   
   // Configurações Personalisadas
   config.stack_size = (8192);
   config.core_id = 0;
   config.uri_match_fn = httpd_uri_match_wildcard;
   config.max_resp_headers = 50;
   config.task_priority = 2;
   config.server_port = 80;
   
   // inicia o serviço httpd
   esp_err_t err;
   err = httpd_start(&server, &config);
   
   // Aqui é onde adiciona as rotas da API REST
   if (err == ESP_OK) {
      ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_receive_ota_handle));
      ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_get));
   }

#ifdef DEBUG_AP_SERVER
   ESP_LOGI(LOG_SERVER, "run->%d", err);
#endif
}
#include <anus/http_interface.hpp>

#include <cstring>

#include <esp_log.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

//========================================

static const char* TAG = "ANUS/http_interface";

#ifdef CONFIG_ANUS_API_OTA_ENABLED

constexpr auto OTA_ENABLED = true;

#else

constexpr auto OTA_ENABLED = false;

#endif // CONFIG_ANUS_API_OTA_ENABLED

//========================================

const char* ExtractPropertyName(const char* uri);

void SendJson(httpd_req_t* request, cJSON* object);
void SendError(httpd_req_t* request, const char* status, const char* message);

//========================================

namespace anus
{

//========================================

void HttpInterface::init()
{
	m_info_json = cJSON_CreateObject();
	cJSON_AddStringToObject(m_info_json, "name", CONFIG_ANUS_API_DEVICE_NAME);
	cJSON_AddBoolToObject(m_info_json, "ota", OTA_ENABLED);
	m_properties_json = cJSON_AddArrayToObject(m_info_json, "properties");
	
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = CONFIG_ANUS_API_HTTP_PORT;
	config.uri_match_fn = httpd_uri_match_wildcard;
	config.lru_purge_enable = true;
	config.stack_size = 8192;
	
	ESP_ERROR_CHECK(httpd_start(&m_httpd_handle, &config));
	ESP_LOGI(TAG, "webserver started on port %d", CONFIG_ANUS_API_HTTP_PORT);

#ifdef CONFIG_ANUS_API_ALLOW_CORS
	registerUri<&HttpInterface::preflightHandler, HTTP_OPTIONS>("/*");
#endif
	
	registerUri<&HttpInterface::firmwareHandler, HTTP_POST>("/firmware"  );
	registerUri<&HttpInterface::infoHandler,     HTTP_GET >("/info"      );
	registerUri<&HttpInterface::propertyHandler, HTTP_GET >("/property/*");
	registerUri<&HttpInterface::propertyHandler, HTTP_POST>("/property/*");
}

//========================================

void HttpInterface::preflightHandler(httpd_req_t* request)
{
	httpd_resp_set_status(request, HTTPD_204);
	httpd_resp_send(request, nullptr, 0);
}

//========================================

void HttpInterface::onUpdateProgress(std::function<void(float)> callback)
{
	m_update_progress_callback = callback;
}

void HttpInterface::firmwareHandler(httpd_req_t* request)
{
	if constexpr (OTA_ENABLED)
	{
		char buffer[1024] = "";
	
		size_t firmware_size = request->content_len;
		ESP_LOGI(TAG, "firmware size: %zu bytes", firmware_size);
		
		if (!firmware_size)
		{
			ESP_LOGE(TAG, "invalid partition size");
			SendError(request, HTTPD_400, "Content-Length is 0");
			return;
		}
		
		const esp_partition_t* running_partition = esp_ota_get_running_partition();
		ESP_LOGI(TAG, "running partition is %s", running_partition->label);
		
		const esp_partition_t* update_partition = esp_ota_get_next_update_partition(running_partition);
		if (!update_partition)
		{
			ESP_LOGE(TAG, "no valid OTA partition found");
			SendError(request, HTTPD_500, "no valid OTA partition found");
			return;
		}
		
		ESP_LOGI(TAG, "firmware update will be written to partition %s", update_partition->label);
		
		auto update_start_ticks = xTaskGetTickCount();
		
		esp_ota_handle_t ota_handle = 0;
		esp_err_t err = 0;
		if ((err = esp_ota_begin(update_partition, 0, &ota_handle)) != ESP_OK)
		{
			ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
			
			SendError(request, HTTPD_500, "esp_ota_begin failed");
			return;
		}
		
		ESP_LOGI(TAG, "performing firmware update...");
		
		uint8_t last_progress = 0;
		size_t written = 0;
		int len = 0;
		
		while (len = httpd_req_recv(request, buffer, sizeof(buffer)))
		{
			if ((err = esp_ota_write(ota_handle, buffer, len)) != ESP_OK)
			{
				ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
				esp_ota_abort(ota_handle);
				
				SendError(request, HTTPD_500, "esp_ota_write failed");
				return;
			}
			
			written += len;
			auto progress = static_cast<float>(written) / firmware_size;
			
			if (m_update_progress_callback)
				m_update_progress_callback(progress);
				
			if (progress != last_progress)
			{
				printf("update progress: %d%%...        \r", static_cast<int>(100.f * progress));
				last_progress = progress;
			}
			
			vTaskDelay(pdMS_TO_TICKS(10));
		}
		
		printf("\n");
		
		if ((err = esp_ota_end(ota_handle)) != ESP_OK)
		{
			ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
			
			SendError(request, HTTPD_500, "esp_ota_end failed");
			return;
		}
		
		if ((err = esp_ota_set_boot_partition(update_partition)) != ESP_OK)
		{
			ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
			
			SendError(request, HTTPD_500, "esp_ota_set_boot_partition failed");
			return;
		}
		
		auto elapsed = static_cast<float>(pdTICKS_TO_MS(xTaskGetTickCount() - update_start_ticks)) / 1000;
		
		ESP_LOGI(TAG, "firmware update done successfully in %.2f s", elapsed);
		
		auto* object = cJSON_CreateObject();
		cJSON_AddNumberToObject(object, "elapsed_time", elapsed);
		SendJson(request, object);
		cJSON_Delete(object);
		
		xTaskCreate(
			[](void* ctx) -> void {
				vTaskDelay(10);
				httpd_stop(reinterpret_cast<HttpInterface*>(ctx)->m_httpd_handle);
				
				ESP_LOGI(TAG, "restarting...");
				esp_restart();
			},
			"Suicide",
			4096,
			this,
			5,
			nullptr
		);
	}
	
	else
	{
		SendError(request, "400", "OTA is disabled");
	}
}

//========================================

void HttpInterface::infoHandler(httpd_req_t* request)
{
	SendJson(request, m_info_json);
}

void HttpInterface::propertyHandler(httpd_req_t* request)
{
	const char* name = ExtractPropertyName(request->uri);
	if (!name)
	{
		SendError(request, HTTPD_400, "invalid property name");
		return;
	}
	
	if (!m_properties.contains(name))
	{
		SendError(request, HTTPD_400, "no such property");
		return;
	}
	
	auto* property = m_properties[name];
	
	switch (request->method)
	{
		case HTTP_GET:
		{
			auto* object = cJSON_CreateObject();
			property->serialize(object);
			SendJson(request, object);
			cJSON_Delete(object);
			
			break;
		}
		
		case HTTP_POST:
		{
			auto* buffer = new char[request->content_len + 1];
			httpd_req_recv(request, buffer, request->content_len + 1);
			
			auto* object = cJSON_Parse(buffer);
			delete[] buffer;
			
			if (!object)
			{
				SendError(request, HTTPD_400, "invalid json");
				return;
			}
			
			bool success = property->deserialize(object);
			cJSON_Delete(object);
			
			if (!success)
				SendError(request, HTTPD_400, "invalid property value");
			
			break;
		}
		
	}
	
	httpd_resp_sendstr(request, "{}");
}

//========================================

} // namespace anus

//========================================

const char* ExtractPropertyName(const char* uri)
{
	if (!*uri)
		return nullptr;
	
	const char* property = strchr(uri, '/');
	if (!property)
		return nullptr;
	
	if (!(property = strchr(property + 1, '/')))
		return nullptr;
	
	return property + 1;
}

void SendJson(httpd_req_t* request, cJSON* object)
{
	char* response = cJSON_PrintUnformatted(object);
	httpd_resp_sendstr(request, response);
	free(response);
}

void SendError(httpd_req_t* request, const char* status, const char* message)
{
	httpd_resp_set_status(request, status);
	
	auto* object = cJSON_CreateObject();
	cJSON_AddStringToObject(object, "error", message);
	
	SendJson(request, object);
	cJSON_Delete(object);
}

//========================================
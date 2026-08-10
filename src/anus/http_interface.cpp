#include <anus/http_interface.hpp>

#include <cstring>

#include <esp_log.h>

//========================================

static const char* TAG = "ANUS/webserver";

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
	m_properties_json = cJSON_CreateArray();
	
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = CONFIG_ANUS_API_HTTP_PORT;
	config.uri_match_fn = httpd_uri_match_wildcard;
	config.lru_purge_enable = true;
	config.stack_size = 8192;
	
	ESP_ERROR_CHECK(httpd_start(&m_httpd_handle, &config));
	ESP_LOGI(TAG, "webserver started on port %d", CONFIG_ANUS_API_HTTP_PORT);
	
	registerUri<&HttpInterface::propertiesHandler, HTTP_GET >("/properties");
	registerUri<&HttpInterface::propertyHandler,   HTTP_GET >("/property/*");
	registerUri<&HttpInterface::propertyHandler,   HTTP_POST>("/property/*");
}

//========================================

void HttpInterface::propertiesHandler(httpd_req_t* request)
{
	SendJson(request, m_properties_json);
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
	
	if (request->method == HTTP_GET)
	{
		auto* object = cJSON_CreateObject();
		property->serialize(object);
		SendJson(request, object);
		cJSON_Delete(object);
	}
	
	else
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
	}
	
	httpd_resp_send(request, nullptr, 0);
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
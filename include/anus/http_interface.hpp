#pragma once

// ANUS - Application Network Unification Service

#include <anus/property/property.hpp>

#include <functional>
#include <map>

#include <cJSON.h>

#include <esp_http_server.h>

//========================================

namespace anus
{

//========================================

class HttpInterface
{
public:
	HttpInterface() = default;
	
	void init();
	
	template<typename T>
	T* addProperty(T* property);
	
	template<typename T>
	T* operator+=(T* property);
	
	void onUpdateProgress(std::function<void(float)> callback);
	
private:
	std::function<void(float)> m_update_progress_callback;
	
	httpd_handle_t m_httpd_handle = 0;
	
	cJSON* m_info_json = nullptr;
	cJSON* m_properties_json = nullptr;
	
	std::map<std::string_view, property::Property*> m_properties {};
	
	template<auto Handler, httpd_method_t Method = HTTP_GET>
	void registerUri(const char* uri);
	
	void preflightHandler(httpd_req_t* request);
	void infoHandler(httpd_req_t* request);
	void propertyHandler(httpd_req_t* request);
	void firmwareHandler(httpd_req_t* request);
	
};

//========================================

template<auto Handler, httpd_method_t Method /*= HTTP_GET*/>
void HttpInterface::registerUri(const char* uri)
{
	httpd_uri_t config = {};
	config.uri = uri;
	config.method = Method;
	config.user_ctx = this;
	config.handler = [](httpd_req_t* request) -> esp_err_t {
		httpd_resp_set_type(request, "application/json");

#ifdef CONFIG_ANUS_API_ALLOW_CORS
		httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
		httpd_resp_set_hdr(request, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
		httpd_resp_set_hdr(request, "Access-Control-Allow-Headers", "Auth, Content-Type, Authentication");
		httpd_resp_set_hdr(request, "Access-Control-Max-Age", "600");
#endif
		
		(reinterpret_cast<HttpInterface*>(request->user_ctx)->*Handler)(request);
		return ESP_OK;
	};
	
	ESP_ERROR_CHECK(httpd_register_uri_handler(m_httpd_handle, &config));
}

template<typename T>
T* HttpInterface::addProperty(T* property)
{
	m_properties[property->getName()] = property;
	
	auto* object = cJSON_CreateObject();
	property->info(object);
	
	cJSON_AddItemToArray(m_properties_json, object);
	return property;
}

template<typename T>
T* HttpInterface::operator+=(T* property)
{
	return addProperty(property);
}

//========================================

} // namespace anus

//========================================
#include <anus/wifi.hpp>

#include <cstring>

#include <esp_log.h>

#include <nvs_flash.h>

//========================================

static const char* TAG = "ANUS/WiFi";

#ifdef CONFIG_ANUS_WIFI_SECURITY_WPA2

constexpr auto WIFI_SECURITY = WIFI_AUTH_WPA2_PSK;

#else

constexpr auto WIFI_SECURITY = WIFI_AUTH_WPA3_PSK;

#endif // CONFIG_ANUS_WIFI_SECURITY_WPA2

#ifndef CONFIG_ANUS_WIFI_HOSTNAME

#define CONFIG_ANUS_WIFI_HOSTNAME CONFIG_ANUS_API_DEVICE_NAME

#endif // CONFIG_ANUS_WIFI_HOSTNAME

constexpr auto WIFI_CONNECTED_BIT = BIT0;

//========================================

namespace anus
{

//========================================

void WiFi::init(bool blocking /*= false*/)
{
	auto result = nvs_flash_init();
	if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		result = nvs_flash_init();
	}
	ESP_ERROR_CHECK(result);
	
	ESP_LOGI(TAG, "NVS initialized");
	
	m_event_group = xEventGroupCreate();
	
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	auto* netif = esp_netif_create_default_wifi_sta();
	ESP_ERROR_CHECK(esp_netif_set_hostname(netif, CONFIG_ANUS_WIFI_HOSTNAME));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id, instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&WiFiEventHandler,
		this,
		&instance_any_id
	));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&WiFiEventHandler,
		this,
		&instance_got_ip
	));

	wifi_config_t wifi_config = {};
	wifi_config.sta.threshold.authmode = WIFI_SECURITY;

	strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid    ), CONFIG_ANUS_WIFI_SSID,     sizeof(wifi_config.sta.ssid    ));
	strncpy(reinterpret_cast<char*>(wifi_config.sta.password), CONFIG_ANUS_WIFI_PASSWORD, sizeof(wifi_config.sta.password));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	
	ESP_LOGI(TAG, "WiFi initialized");
	
	if (blocking)
		xEventGroupWaitBits(
			m_event_group,
			WIFI_CONNECTED_BIT,
			false,
			false,
			portMAX_DELAY
		);
}

void WiFi::WiFiEventHandler(
	void* ctx,
	esp_event_base_t event_base,
	int32_t event_id,
	void* event_data
)
{
	auto& instance = *reinterpret_cast<WiFi*>(ctx);
	
	if (event_base == WIFI_EVENT)
		instance.onWiFiEvent(event_id, event_data);
	
	else if (event_base == IP_EVENT)
		instance.onIpEvent(event_id, event_data);
}

void WiFi::onWiFiEvent(uint32_t id, void* data)
{
	switch (id)
	{
		case WIFI_EVENT_STA_START:
			ESP_ERROR_CHECK(esp_wifi_connect());
			ESP_LOGI(TAG, "connecting to " CONFIG_ANUS_WIFI_SSID "...");
			
			break;
			
		case WIFI_EVENT_STA_DISCONNECTED:
		{
			auto* event = reinterpret_cast<wifi_event_sta_disconnected_t*>(data);
			
			ESP_ERROR_CHECK(esp_wifi_connect());
			ESP_LOGI(TAG, "wifi disconnected (0x%02X); reconnecting...", event->reason);
			
			break;
		}
	}
}

void WiFi::onIpEvent(uint32_t id, void* data)
{
	switch (id)
	{
		case IP_EVENT_STA_GOT_IP:
		{
			m_connected = true;
			
			auto* event = reinterpret_cast<ip_event_got_ip_t*>(data);
			ESP_LOGI(TAG, "successfully connected to the AP; got ip: " IPSTR, IP2STR(&event->ip_info.ip));
			
			xEventGroupSetBits(m_event_group, WIFI_CONNECTED_BIT);
			break;
		}
	}
}

//========================================

bool WiFi::isConnected() const
{
	return m_connected;
}

//========================================

} // namespace anus

//========================================
#pragma once

// ANUS - Application Network Unification Service

#include <esp_netif.h>
#include <esp_wifi.h>

//========================================

namespace anus
{

//========================================

class WiFi
{
public:
	WiFi() = default;
	
	void init(bool blocking = false);
	
	bool isConnected() const;
	
private:
	EventGroupHandle_t m_event_group = 0;
	bool m_connected = false;
	
	static void WiFiEventHandler(
		void* ctx,
		esp_event_base_t event_base,
		int32_t event_id,
		void* event_data
	);
	
	void onWiFiEvent(uint32_t id, void* data);
	void onIpEvent(uint32_t id, void* data);

};

//========================================

} // namespace anus

//========================================
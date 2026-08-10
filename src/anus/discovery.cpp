#include <anus/discovery.hpp>
#include <anus/utils.hpp>

#include <iterator>

#include <esp_log.h>

//========================================

static const char* TAG = "ANUS/discovery";

//========================================

namespace anus
{

//========================================

void Discovery::init()
{
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CONFIG_ANUS_DISCOVERY_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	ANUS_NET_CHECK(bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
	
	ip_mreq mreq = {};
	if (!inet_aton(CONFIG_ANUS_DISCOVERY_GROUP, &mreq.imr_multiaddr))
	{
		ESP_LOGE(TAG, "invalid group address address: " CONFIG_ANUS_DISCOVERY_GROUP "; aborting");
		abort();
	}
	
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	ANUS_NET_CHECK(setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)));
	
	xTaskCreate(
		[](void* ctx) -> void {
			reinterpret_cast<Discovery*>(ctx)->task();
		},
		"Discovery listener",
		4096,
		this,
		5,
		nullptr
	);
	
	ESP_LOGI(TAG, "discovery initialized");
}

//========================================

void Discovery::task()
{
	ESP_LOGI(TAG, "discovery listener task started");
	
	while (true)
	{
		char buffer[128];
		
		sockaddr_in addr = {};
		socklen_t addr_len = sizeof(addr);
		
		ANUS_NET_CHECK(
			recvfrom(
				m_socket,
				buffer,
				std::size(buffer),
				0,
				reinterpret_cast<sockaddr*>(&addr),
				&addr_len
			)
		);

		ESP_LOGI(TAG, "got discovery message from %s; responding", inet_ntoa(addr.sin_addr));
		
		ANUS_NET_CHECK(
			sendto(
				m_socket,
				CONFIG_ANUS_API_DEVICE_NAME,
				strlen(CONFIG_ANUS_API_DEVICE_NAME),
				0,
				reinterpret_cast<sockaddr*>(&addr),
				sizeof(addr)
			)
		);
	}
}

//========================================

} // namespace anus

//========================================
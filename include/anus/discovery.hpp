#pragma once

// ANUS - Application Network Unification Service

#include <lwip/err.h>
#include <lwip/sockets.h>

//========================================

namespace anus
{

//========================================
	
class Discovery
{
public:
	Discovery() = default;
	
	void init();
	
private:
	int m_socket = 0;
	
	void task();

};

//========================================

} // namespace anus

//========================================
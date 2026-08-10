#pragma once

// ANUS - Application Network Unification Service

//========================================

#ifdef CONFIG_ANUS_NET_CHECK_ENABLED

#define ANUS_NET_CHECK(expr) anus::NetCheck(expr, #expr)

#else

#define ANUS_NET_CHECK(expr) expr

#endif // CONFIG_ANUS_NET_CHECK_ENABLED

//========================================

namespace anus
{

//========================================

int NetCheck(int retval, const char* expr);

//========================================

} // namespace anus

//========================================
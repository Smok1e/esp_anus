#include <anus/utils.hpp>

#include <cerrno>
#include <cstring>

#include <esp_log.h>

//========================================

static const char* TAG = "ANUS/utils";

//========================================

namespace anus
{

//========================================

int NetCheck(int retval, const char* expr)
{
	if (retval < 0)
	{
		ESP_LOGE(TAG, "%s failed with errno %d: %s; Aborting", expr, errno, strerror(errno));
		abort();
	}

	return retval;
}

//========================================

} // namespace anus

//========================================
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

void DumpHex(FILE* stream, const void* data, size_t len)
{
	constexpr size_t line_size = 16;
	const auto* bytes = reinterpret_cast<const char*>(data);
	
	for (size_t offset = 0; offset < len; offset += line_size)
	{
		fprintf(stream, "%04zu: ", offset);
		
		for (size_t i = 0; i < line_size; i++)
		{
			if (offset + i >= len)
			{
				printf("   ");
				continue;
			}
			
			fprintf(stream, "%02x ", bytes[offset + i]);
		}
		
		fprintf(stream, " ");
		
		for (size_t i = 0; i < line_size && offset + i < len; i++)
		{
			auto byte = bytes[offset + i];
			fprintf(
				stream,
				"%c",
				0x20 <= byte && byte <= 0x7E
					? byte
					: '.'
			);
		}
		
		fprintf(stream, "\n");
	}
}

//========================================

} // namespace anus

//========================================
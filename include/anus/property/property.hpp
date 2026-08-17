#pragma once

// ANUS - Application Network Unification Service

#include <string_view>
#include <functional>

#include <esp_http_server.h>

#include <cJSON.h>

//========================================

namespace anus::property
{

//========================================

class Property
{
public:
	Property(const char* name, const char* description);
	virtual ~Property();
	
	const char* getName() const;
	const char* getDescription() const;
	
	virtual void onValueUpdated(std::function<void(Property*)> callback);
	
	virtual void info(cJSON* object);
	virtual void serialize(cJSON* object) = 0;
	virtual bool deserialize(cJSON* object) = 0;
	
protected:
	std::function<void(Property*)> m_callback;
	const char* m_name;
	const char* m_description;
	
	void updateValue();
	
};

//========================================

} // namespace anus::property

//========================================
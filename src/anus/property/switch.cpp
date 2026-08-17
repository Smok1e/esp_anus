#include <anus/property/switch.hpp>

//========================================

namespace anus::property
{

//========================================

Switch::Switch(
	const char* name,
	const char* description,
	bool value /*= false*/
):
	Property(name, description),
	m_value(value)
{}

//========================================

bool Switch::getValue() const
{
	return m_value;
}

void Switch::setValue(bool value)
{
	m_value = value;
}

//========================================

void Switch::info(cJSON* object)
{
	Property::info(object);
	cJSON_AddStringToObject(object, "type", "switch");
}

void Switch::serialize(cJSON* object)
{
	cJSON_AddBoolToObject(object, "value", m_value);
}

bool Switch::deserialize(cJSON* object)
{
	auto* item = cJSON_GetObjectItem(object, "value");
	if (!item || !cJSON_IsBool(item))
		return false;
	
	m_value = cJSON_IsTrue(item);
	updateValue();
	
	return true;
}

//========================================

} // namespace anus::property

//========================================
#include <anus/property/slider.hpp>

#include <algorithm>

//========================================

namespace anus::property
{

//========================================

Slider::Slider(
	const char* name,
	const char* description,
	float value /*= 0.f*/,
	float min /*= 0.f*/,
	float max /*= 1.f*/
):
	Property(name, description),
	m_value(value),
	m_min(min),
	m_max(max)
{}

//========================================

float Slider::getValue() const
{
	return m_value;
}

void Slider::setValue(float value)
{
	m_value = value;
}

//========================================

void Slider::info(cJSON* object)
{
	Property::info(object);
	cJSON_AddStringToObject(object, "type", "slider");
	
	cJSON_AddNumberToObject(object, "min", m_min);
	cJSON_AddNumberToObject(object, "max", m_max);
}

void Slider::serialize(cJSON* object)
{
	cJSON_AddNumberToObject(object, "value", m_value);
}

bool Slider::deserialize(cJSON* object)
{
	auto* item = cJSON_GetObjectItem(object, "value");
	
	if (!item || !cJSON_IsNumber(item))
		return false;
	
	m_value = std::clamp<float>(cJSON_GetNumberValue(item), m_min, m_max);
	updateValue();
	
	return true;
}

//========================================

} // namespace anus::property

//========================================
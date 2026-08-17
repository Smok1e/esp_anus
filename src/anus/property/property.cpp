#include <anus/property/property.hpp>

//========================================

namespace anus::property
{

//========================================

Property::Property(const char* name, const char* description):
	m_name(name),
	m_description(description)
{}

Property::~Property()
{
}

const char* Property::getName() const
{
	return m_name;
}

const char* Property::getDescription() const
{
	return m_description;
}

void Property::onValueUpdated(std::function<void(Property*)> callback)
{
	m_callback = callback;
}

void Property::info(cJSON* object)
{
	cJSON_AddStringToObject(object, "name", m_name);
	cJSON_AddStringToObject(object, "description", m_description);
}

void Property::updateValue()
{
	if (m_callback)
		m_callback(this);
}

//========================================

} // namespace anus::property

//========================================
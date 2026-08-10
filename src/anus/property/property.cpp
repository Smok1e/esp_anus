#include <anus/property/property.hpp>

//========================================

namespace anus::property
{

//========================================

Property::Property(const char* name):
	m_name(name)
{}

Property::~Property()
{
}

void Property::onValueUpdated(std::function<void(Property*)> callback)
{
	m_callback = callback;
}

void Property::info(cJSON* object)
{
	cJSON_AddStringToObject(object, "name", m_name);
}

const char* Property::getName() const
{
	return m_name;
}

void Property::updateValue()
{
	if (m_callback)
		m_callback(this);
}

//========================================

} // namespace anus::property

//========================================
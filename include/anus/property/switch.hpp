#pragma once

// ANUS - Application Network Unification Service

#include <anus/property/property.hpp>

//========================================

namespace anus::property
{

//========================================

class Switch: public Property
{
public:
	Switch(const char* name, bool state = false);
	
	bool getValue() const;
	void setValue(bool value);
	
	virtual void info(cJSON* object) override;
	virtual void serialize(cJSON* object) override;
	virtual bool deserialize(cJSON* object) override;
	
protected:
	bool m_value;
	
};

//========================================

} // namespace anus::property

//========================================
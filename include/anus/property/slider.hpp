#pragma once

// ANUS - Application Network Unification Service

#include <anus/property/property.hpp>

//========================================

namespace anus::property
{

//========================================

class Slider: public Property
{
public:
	Slider(
		const char* name,
		const char* description,
		float value = 0.f,
		float min = 0.f,
		float max = 1.f
	);
	
	float getValue() const;
	void setValue(float value);
	
	virtual void info(cJSON* object) override;
	virtual void serialize(cJSON* object) override;
	virtual bool deserialize(cJSON* object) override;
	
protected:
	float m_value;
	float m_min;
	float m_max;
	
};

//========================================

} // namespace anus::property

//========================================
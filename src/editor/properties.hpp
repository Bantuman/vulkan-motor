#pragma once
#include <unordered_map>
#include <vector>
#include "ui/viewport_gui.hpp"
#include <ecs/ecs.hpp>
#include <core/instance.hpp>

#define DECLARE_PROPERTY(Class, PropertyName, PropertyLabel) g_propertyTable[#Class][#PropertyName] = {#PropertyName, offsetof(Class, #PropertyLabel), sizeof(decltype(Class::PropertyLabel))} 
#define DEFINE_PROPERTY(Class, PropertyName, Getter, Setter) g_propertyTable[#Class][#PropertyName] = {[](){}}
using namespace Game;
struct Properties
{
	struct Result 
	{
		const void* m_data;
		size_t m_size;
	};

	using getter = std::function<Result(void)>;

	static inline std::unordered_map<const char*, std::unordered_map<const char*, getter>> g_getterTable;
	static void inline properties_init()
	{

		//g_getterTable["ViewportGui"]["AbsoluteSize"] = getter{ [](ViewportGui& gui) {
		//	decltype(auto) r = gui.get_absolute_size();
		//	return Result{ reinterpret_cast<const void*>(&r), sizeof(r) };
		//	}
		//};

		//	g_propertyTable["ViewportGui"]["AbsoluteSize"] = { "AbsoluteSize", offsetof(ViewportGui, m_absoluteSize), sizeof(ViewportGui::m_absoluteSize) };
		//	g_propertyTable["Test"]["foo"] = { "foo", offsetof(Property, m_name), sizeof(decltype(Property::m_name))};
		//DECLARE_PROPERTY(ViewportGui, AbsoluteSize, m_absoluteSize);
	};
};



idf_component_register(
	SRCS
		"src/anus/wifi.cpp"
		"src/anus/discovery.cpp"
		"src/anus/utils.cpp"
		"src/anus/http_interface.cpp"
		"src/anus/property/property.cpp"
		"src/anus/property/slider.cpp"
		"src/anus/property/switch.cpp"
	
	INCLUDE_DIRS
		"include/"
	
	REQUIRES
		"app_update"
		"cjson"
		"esp_http_server"
		"esp_wifi"
		"nvs_flash"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-parentheses)
// Colour strings from WML and markup: font::string_to_color (font/standard_colors.cpp, pango
// colour names / #rrggbb) and color_t::from_rgba_string / from_rgb_string / from_hex_string.
#include "fuzz_common.hpp"

#include "color.hpp"
#include "font/standard_colors.hpp"

#include <stdexcept>

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	const std::string in(reinterpret_cast<const char*>(data), size);
	(void)font::string_to_color(in).to_hex_string();
	try {
		(void)color_t::from_rgba_string(in).to_rgba_string();
	} catch(const std::invalid_argument&) {
	}
	try {
		(void)color_t::from_rgb_string(in);
	} catch(const std::invalid_argument&) {
	}
	try {
		(void)color_t::from_hex_string(in).to_hex_string();
	} catch(const std::invalid_argument&) {
	}
	return 0;
}

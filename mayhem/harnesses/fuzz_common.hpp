// Shared setup for the wesnoth fuzz harnesses (mayhem/ layer, not upstream code).
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

#include "log.hpp"

namespace wesnoth_fuzz {
// Silence every log domain once at startup: logging only costs time per input. The severity is
// process-global but never changed by an input, so each input's result depends only on that input.
inline void quiet_logs()
{
	lg::set_log_domain_severity("all", lg::severity::LG_NONE);
}

// Splits the input at the first NUL byte into two independent strings (second is empty if none).
inline std::pair<std::string, std::string> split_nul(const uint8_t* data, size_t size)
{
	std::string all(reinterpret_cast<const char*>(data), size);
	const auto pos = all.find('\0');
	if(pos == std::string::npos) {
		return {all, std::string()};
	}
	return {all.substr(0, pos), all.substr(pos + 1)};
}
} // namespace wesnoth_fuzz

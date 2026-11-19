// String helpers applied to nearly every WML attribute value: utils::split family, range
// parsing, map_split, escape/unescape, wildcard matching and the UTF-8 helpers (base64/crypt64
// have their own target, fuzz_base64.cpp).
#include "fuzz_common.hpp"

#include "serialization/string_utils.hpp"
#include "serialization/unicode.hpp"
#include "serialization/utf8_exception.hpp"

#include <stdexcept>

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	const auto [in, pat] = wesnoth_fuzz::split_nul(data, size);
	(void)utils::split(in);
	(void)utils::quoted_split(in);
	(void)utils::parenthetical_split(in, ',');
	(void)utils::square_parenthetical_split(in);
	(void)utils::map_split(in);
	(void)utils::unescape(in);
	(void)utils::escape(in, ",\\");
	(void)utils::urlencode(in);
	(void)utils::wildcard_string_match(in, pat);
	(void)utils::isvalid_username(in);
	(void)utils::isvalid_wildcard(in);
	try {
		(void)utils::parse_range(in);
		(void)utils::parse_ranges_unsigned(in);
		(void)utils::parse_ranges_int(in);
		(void)utils::parse_range_real(in);
		(void)utils::parse_ranges_real(in);
	} catch(const std::invalid_argument&) {
	}
	// apply_modifier: every upstream caller (units/unit.cpp, units/attack_type.cpp effects) only
	// calls it for a non-empty "increase"-style attribute, so an empty amount is outside its contract.
	if(!in.empty()) {
		(void)utils::apply_modifier(100, in);
		(void)utils::apply_modifier(-7, in, 1);
	}
	try {
		(void)utf8::size(in);
		(void)utf8::lowercase(in);
		std::string t = in;
		utf8::truncate(t, 5);
		std::string e = in;
		utf8::erase(e, 1, 2);
		(void)unicode_cast<std::u32string>(in);
	} catch(const utf8::invalid_utf8_exception&) {
	}
	return 0;
}

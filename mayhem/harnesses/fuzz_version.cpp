// version_info string parsing and comparison (add-on [versions], savegame and MP handshake
// version strings). Input: two version strings separated by the first NUL byte.
#include "fuzz_common.hpp"

#include "game_version.hpp"

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	const auto [s1, s2] = wesnoth_fuzz::split_nul(data, size);
	const version_info a(s1), b(s2);
	(void)a.str();
	(void)a.is_canonical();
	(void)(a < b);
	(void)(a == b);
	(void)(a >= b);
	(void)a.special_version();
	(void)version_info(a.str()).str();
	for(VERSION_COMP_OP op : {OP_EQUAL, OP_NOT_EQUAL, OP_LESS, OP_LESS_OR_EQUAL,
			OP_GREATER, OP_GREATER_OR_EQUAL}) {
		(void)do_version_check(a, op, b);
	}
	(void)parse_version_op(s2);
	return 0;
}

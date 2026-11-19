// WML diff/patch: config::get_diff / apply_diff / merge_with, used for [scenario_diff] /
// [gamelist_diff] network updates and `wesnoth --diff/--patch`. Input: two WML documents
// separated by the first NUL byte.
#include "fuzz_common.hpp"

#include "config.hpp"
#include "serialization/parser.hpp"
#include "serialization/utf8_exception.hpp"

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	const auto [first, second] = wesnoth_fuzz::split_nul(data, size);
	try {
		const config a = io::read(first);
		const config b = io::read(second);
		// diff computed between two documents and replayed
		config patched = a;
		patched.apply_diff(a.get_diff(b));
		// the second document applied directly as an (untrusted) diff, with tracking
		config direct = a;
		direct.apply_diff(b, true);
		direct.clear_diff_track(b);
		config merged = a;
		merged.merge_with(b);
	} catch(const game::error&) {
	} catch(const utf8::invalid_utf8_exception&) {
	}
	return 0;
}

// simple_wml::node::apply_diff: how wesnothd applies an owner-sent [scenario_diff] /
// [change_child] / [insert_child] / [delete_child] update to a stored game level
// (server.cpp handle_player_in_game). Input: base document, NUL, diff document.
#include "fuzz_common.hpp"

#include "server/common/simple_wml.hpp"

#include <vector>

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	const auto [first, second] = wesnoth_fuzz::split_nul(data, size);
	try {
		simple_wml::document base(first.c_str(), simple_wml::INIT_STATIC);
		simple_wml::document diff(second.c_str(), simple_wml::INIT_STATIC);
		base.root().apply_diff(diff.root());
		(void)base.output();
		(void)base.root().children("side").size();
		(void)base.root().nattributes_recursive();
	} catch(const simple_wml::error&) {
	}
	return 0;
}

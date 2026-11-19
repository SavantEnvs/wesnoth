// WML preprocessor: preprocess_string (#define/#ifdef/#arg/macro expansion/#textdomain/{includes})
// as applied to every add-on and campaign .cfg, then the WML parser on its output.
// game_config::path stays empty, so {file} includes can only resolve under the userdata dir ({~x})
// or the working directory ({./x}); the macro table is a fresh local per input. The game sets up its userdata
// directory before preprocessing anything ({~path} includes resolve there), so the harness does
// the same once at startup, in a scratch directory under $TMPDIR; inputs only read from it.
#include "fuzz_common.hpp"

#include "config.hpp"
#include "filesystem.hpp"
#include "serialization/binary_or_text.hpp"
#include "serialization/parser.hpp"
#include "serialization/preprocessor.hpp"
#include "serialization/utf8_exception.hpp"

#include <cstdlib>
#include <unistd.h>
#include <sstream>

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	const char* tmp = std::getenv("TMPDIR");
	const std::string scratch = std::string(tmp && *tmp ? tmp : "/tmp");
	filesystem::set_user_data_dir(scratch + "/wesnoth-fuzz-userdata");
	// "{./...}" includes resolve against the working directory: run from an empty one so the
	// result does not depend on whatever directory the fuzzer was started in.
	const std::string cwd = scratch + "/wesnoth-fuzz-cwd";
	filesystem::create_directory_if_missing(cwd);
	if(chdir(cwd.c_str()) != 0) {
		std::abort();
	}
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	const std::string in(reinterpret_cast<const char*>(data), size);
	try {
		preproc_map defines;
		filesystem::scoped_istream stream = preprocess_string(in, "wesnoth", defines);
		config cfg = io::read(*stream);
		std::ostringstream os;
		config_writer writer(os, false);
		for(const auto& [name, def] : defines) {
			def.write(writer, name);
		}
	} catch(const game::error&) {
	} catch(const utf8::invalid_utf8_exception&) {
	}
	return 0;
}

// WML text parser: io::read (tokenizer + parser + config) as used for every .cfg, savegame and
// replay, then the io::write serializer and a re-parse of what it wrote (round trip).
#include "fuzz_common.hpp"

#include "config.hpp"
#include "serialization/parser.hpp"
#include "serialization/utf8_exception.hpp"

#include <sstream>

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	const std::string in(reinterpret_cast<const char*>(data), size);
	try {
		config cfg = io::read(in);
		std::ostringstream out;
		io::write(out, cfg);
		config again = io::read(out.str());
		(void)(again == cfg);
		(void)cfg.debug();
	} catch(const game::error&) {
	} catch(const utf8::invalid_utf8_exception&) {
	}
	return 0;
}

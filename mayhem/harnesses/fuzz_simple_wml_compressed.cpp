// simple_wml compressed path: document(string_span) inflates a gzip or bzip2 packet exactly as
// wesnothd/campaignd do on bytes received from a socket, then parses it.
#include "fuzz_common.hpp"

#include "server/common/simple_wml.hpp"

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	// Precondition from the callers: server_base::coro_receive_doc (server_base.cpp) rejects a
	// zero-length packet before it builds a document from the compressed buffer.
	if(size == 0) {
		return -1;
	}
	try {
		simple_wml::document doc(simple_wml::string_span(reinterpret_cast<const char*>(data), size));
		(void)doc.output();
		(void)doc.root().first_child();
	} catch(const simple_wml::error&) {
	}
	return 0;
}

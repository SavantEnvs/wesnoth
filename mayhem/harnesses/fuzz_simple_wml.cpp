// simple_wml: the in-place WML parser wesnothd and campaignd run on every network packet.
// Parses a NUL-terminated copy (INIT_STATIC), re-serializes it, compresses/decompresses it
// (gzip and bzip2) and walks the tree.
#include "fuzz_common.hpp"

#include "server/common/simple_wml.hpp"

#include <cstring>
#include <vector>

namespace {
void inspect(const simple_wml::node& n)
{
	(void)n.nchildren();
	(void)n.nattributes_recursive();
	for(const char* key : {"name", "id", "side", "version", "message"}) {
		const simple_wml::string_span& v = n.attr(key);
		(void)v.to_string();
		(void)v.to_int();
		(void)v.to_bool();
	}
	const simple_wml::string_span& first = n.first_child();
	if(!first.empty()) {
		for(const simple_wml::node* c : n.children(first.to_string().c_str())) {
			(void)c->nchildren();
		}
	}
}
} // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	std::vector<char> buf(data, data + size);
	buf.push_back('\0');
	try {
		simple_wml::document doc(buf.data(), simple_wml::INIT_STATIC);
		inspect(doc.root());
		(void)doc.output();
		auto copy = doc.clone();
		(void)copy->root().child("side");
		// a document caches one compressed form, so each document uses a single format
		simple_wml::document from_bz2(copy->output_compressed(true));
		(void)from_bz2.output();
		simple_wml::document re(doc.output_compressed(false));
		(void)re.output();
	} catch(const simple_wml::error&) {
	}
	return 0;
}

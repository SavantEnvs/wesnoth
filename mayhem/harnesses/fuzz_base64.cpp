// base64 / crypt64 decoding (serialization/base64.cpp): base64 backs data: URIs in image paths
// (picture.cpp) and crypt64 the password-hash salts (hash.cpp). Decodes the input with both
// alphabets, then re-encodes and re-decodes the result.
#include "fuzz_common.hpp"

#include "serialization/base64.hpp"

#include <vector>

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	const std::string_view in(reinterpret_cast<const char*>(data), size);
	const std::vector<uint8_t> b = base64::decode(in);
	(void)base64::decode(base64::encode(b));
	const std::vector<uint8_t> c = crypt64::decode(in);
	(void)crypt64::decode(crypt64::encode(c));
	return 0;
}

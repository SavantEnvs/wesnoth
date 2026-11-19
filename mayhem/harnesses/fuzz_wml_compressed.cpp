// Compressed WML (gzip / bzip2-wrapped savegames, replays and network blobs): io::read_gz or
// io::read_bz2 over boost::iostreams, chosen by the first input byte.
#include "fuzz_common.hpp"

#include "config.hpp"
#include "serialization/parser.hpp"
#include "serialization/utf8_exception.hpp"

#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <ios>
#include <sstream>

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	if(size < 1) {
		return 0;
	}
	const bool bz2 = data[0] & 1;
	std::istringstream in(std::string(reinterpret_cast<const char*>(data) + 1, size - 1));
	try {
		config cfg = bz2 ? io::read_bz2(in) : io::read_gz(in);
		std::ostringstream out;
		io::write_gz(out, cfg);
	} catch(const game::error&) {
	} catch(const utf8::invalid_utf8_exception&) {
	} catch(const boost::iostreams::gzip_error&) {
	} catch(const boost::iostreams::bzip2_error&) {
	} catch(const std::ios_base::failure&) {
	}
	return 0;
}

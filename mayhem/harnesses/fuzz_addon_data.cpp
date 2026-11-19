// Add-on upload checks (campaignd): an uploaded add-on's [data] tree goes through
// check_names_legal / check_case_insensitive_duplicates, delta uploads apply [removelist] and
// [addlist] (data_apply_removelist / data_apply_addlist), the server derives hash lists and update
// packs, and file contents are binary-unencoded. Input: stored add-on data WML, NUL, upload WML.
#include "fuzz_common.hpp"

#include "addon/validation.hpp"
#include "config.hpp"
#include "serialization/parser.hpp"
#include "serialization/utf8_exception.hpp"
#include "server/campaignd/addon_utils.hpp"

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
		config stored = io::read(first);
		const config upload = io::read(second);
		std::vector<std::string> bad;
		(void)check_names_legal(upload, &bad);
		(void)check_case_insensitive_duplicates(upload, &bad);
		for(const config& f : upload.child_range("file")) {
			(void)unencode_binary(f["contents"].str());
			(void)file_hash(f);
		}
		(void)addon_name_legal(upload["name"].str());
		(void)addon_icon_too_large(upload["icon"].str());
		const config before = stored;
		campaignd::data_apply_removelist(stored, upload.child_or_empty("removelist"));
		campaignd::data_apply_addlist(stored, upload.child_or_empty("addlist"));
		config pack;
		make_updatepack(pack, before, stored);
		config hashlist;
		write_hashlist(hashlist, stored);
		(void)contains_hashlist(hashlist, before);
		config addon = upload;
		(void)campaignd::get_version_map(addon);
		campaignd::find_translations(stored, addon);
	} catch(const game::error&) {
	} catch(const utf8::invalid_utf8_exception&) {
	}
	return 0;
}

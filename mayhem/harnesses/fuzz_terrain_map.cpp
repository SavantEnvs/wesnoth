// Terrain map strings: t_translation::read_game_map (every .map file / [scenario] map_data),
// write_game_map round trip, plus read_list / read_terrain_code on the same text.
#include "fuzz_common.hpp"

#include "terrain/translation.hpp"

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	wesnoth_fuzz::quiet_logs();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	const std::string in(reinterpret_cast<const char*>(data), size);
	try {
		t_translation::starting_positions positions;
		const t_translation::ter_map map = t_translation::read_game_map(in, positions);
		const std::string out = t_translation::write_game_map(map, positions);
		t_translation::starting_positions again;
		(void)t_translation::read_game_map(out, again);
	} catch(const t_translation::error&) {
	}
	try {
		const t_translation::ter_list list = t_translation::read_list(in);
		(void)t_translation::write_list(list);
	} catch(const t_translation::error&) {
	}
	try {
		(void)t_translation::write_terrain_code(t_translation::read_terrain_code(in));
	} catch(const t_translation::error&) {
	}
	return 0;
}

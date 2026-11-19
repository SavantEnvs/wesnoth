// Boost.Test module for the subset of upstream's src/tests suite that only needs wesnoth-common
// (upstream's own tests/main.cpp pulls in SDL and the GUI, which this integration does not build).
#define BOOST_TEST_MODULE wesnoth core unit tests
#include <boost/test/unit_test.hpp>

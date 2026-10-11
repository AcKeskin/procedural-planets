#include <catch2/catch_test_macros.hpp>

#include "types/BodyRegistry.h"

#include <filesystem>
#include <fstream>

using planets::render::types::FindBody;
using planets::render::types::ScanBodies;

namespace
{
std::filesystem::path MakeDir(const char* name)
{
    auto dir = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

void Write(const std::filesystem::path& file, const char* text)
{
    std::ofstream(file) << text;
}
} // namespace

TEST_CASE("body_registry: lists every json, earth first, then by id", "[body_registry]")
{
    auto dir = MakeDir("pp_registry_test");
    Write(dir / "zeta.json", R"({"metadata": {"name": "Zeta World"}})");
    Write(dir / "earth.json", R"({"metadata": {"name": "Earth"}})");
    Write(dir / "alpha.json", R"({"metadata": {}})");
    Write(dir / "notes.txt", "not a body");

    auto bodies = ScanBodies(dir.string());
    REQUIRE(bodies.size() == 3);
    CHECK(bodies[0].id == "earth");
    CHECK(bodies[1].id == "alpha");
    CHECK(bodies[2].id == "zeta");
    CHECK(bodies[2].name == "Zeta World");
    CHECK(bodies[1].name == "alpha"); // no name: falls back to the id
    CHECK(FindBody(bodies, "zeta") == 2);
    CHECK(FindBody(bodies, "missing") == -1);
    std::filesystem::remove_all(dir);
}

TEST_CASE("body_registry: a broken json is listed under its id, a missing dir gives nothing", "[body_registry]")
{
    auto dir = MakeDir("pp_registry_broken");
    Write(dir / "broken.json", "{ not json");
    auto bodies = ScanBodies(dir.string());
    REQUIRE(bodies.size() == 1);
    CHECK(bodies[0].name == "broken");
    std::filesystem::remove_all(dir);

    CHECK(ScanBodies((dir / "nope").string()).empty());
}

TEST_CASE("body_registry: the shipped bodies folder is found", "[body_registry]")
{
    auto bodies = ScanBodies("data/bodies");
    REQUIRE_FALSE(bodies.empty());
    CHECK(bodies[0].id == "earth");
}

#include <catch2/catch_test_macros.hpp>

#include "backend/ShaderRegistry.h"

// Step 8a: ShaderRegistry unit tests — no GPU required, just the compile/destroy logic

namespace
{

struct FakeCompiler
{
    uint32_t nextHandle = 100;
    int compileCalls = 0;
    int destroyCalls = 0;

    uint32_t compile(const std::string&)
    {
        ++compileCalls;
        return nextHandle++;
    }

    void destroy(uint32_t)
    {
        ++destroyCalls;
    }
};

} // namespace

TEST_CASE("ShaderRegistry compile and resolve", "[registry]")
{
    FakeCompiler fc;
    planetgen::ShaderRegistry reg;
    reg.SetCompiler(
        [&](const std::string& s) { return fc.compile(s); },
        [&](uint32_t h)           { fc.destroy(h); }
    );

    REQUIRE(reg.RegisterProgram("myshader", "source code"));
    REQUIRE(reg.Resolve("myshader") == 100u);
    REQUIRE(fc.compileCalls == 1);
    REQUIRE(fc.destroyCalls == 0);
}

TEST_CASE("ShaderRegistry re-register evicts old handle", "[registry]")
{
    FakeCompiler fc;
    planetgen::ShaderRegistry reg;
    reg.SetCompiler(
        [&](const std::string& s) { return fc.compile(s); },
        [&](uint32_t h)           { fc.destroy(h); }
    );

    reg.RegisterProgram("k", "first");
    reg.RegisterProgram("k", "second");

    REQUIRE(fc.compileCalls == 2);
    REQUIRE(fc.destroyCalls == 1); // first handle destroyed on evict
    REQUIRE(reg.Resolve("k") == 101u);
}

TEST_CASE("ShaderRegistry BuiltinProgram ids", "[registry]")
{
    FakeCompiler fc;
    planetgen::ShaderRegistry reg;
    reg.SetCompiler(
        [&](const std::string& s) { return fc.compile(s); },
        [&](uint32_t h)           { fc.destroy(h); }
    );

    using BP = planetgen::BuiltinProgram;
    reg.RegisterProgram(BP::Height,         "h");
    reg.RegisterProgram(BP::ShadingEarth,   "se");
    reg.RegisterProgram(BP::ShadingGeneric, "sg");
    reg.RegisterProgram(BP::Erosion,        "e");

    REQUIRE(reg.Resolve(BP::Height)         == 100u);
    REQUIRE(reg.Resolve(BP::ShadingEarth)   == 101u);
    REQUIRE(reg.Resolve(BP::ShadingGeneric) == 102u);
    REQUIRE(reg.Resolve(BP::Erosion)        == 103u);
}

TEST_CASE("ShaderRegistry resolve unknown key returns 0", "[registry]")
{
    planetgen::ShaderRegistry reg;
    REQUIRE(reg.Resolve("nonexistent") == 0u);
}

TEST_CASE("ShaderRegistry Clear destroys all handles", "[registry]")
{
    FakeCompiler fc;
    planetgen::ShaderRegistry reg;
    reg.SetCompiler(
        [&](const std::string& s) { return fc.compile(s); },
        [&](uint32_t h)           { fc.destroy(h); }
    );

    reg.RegisterProgram("a", "s1");
    reg.RegisterProgram("b", "s2");
    reg.Clear();

    REQUIRE(fc.destroyCalls == 2);
    REQUIRE(reg.Resolve("a") == 0u);
}

TEST_CASE("ShaderRegistry custom key dispatch", "[registry]")
{
    // Simulates what a consumer would do: register a custom compute shader by string id
    FakeCompiler fc;
    planetgen::ShaderRegistry reg;
    reg.SetCompiler(
        [&](const std::string& s) { return fc.compile(s); },
        [&](uint32_t h)           { fc.destroy(h); }
    );

    const std::string customId = "consumer::my_pass";
    const std::string glsl = "#version 450\nlayout(local_size_x=1) in;\nvoid main(){}\n";

    REQUIRE(reg.RegisterProgram(customId, glsl));
    REQUIRE(reg.Resolve(customId) != 0u);
}

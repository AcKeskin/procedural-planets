#include "Bookmarks.h"
#include "HelpOverlay.h"
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

using namespace planets::app;

namespace
{
// One bookmark entry for slot 1; each argument is raw JSON text.
std::string Entry(const std::string& position = "[1,2,3]",
                  const std::string& forward = "[0,0,-1]",
                  const std::string& up = "[0,1,0]",
                  const std::string& mode = "\"fly\"",
                  const std::string& sunDir = "[0,1,0]")
{
    return R"({"bookmarks":[{"slot":1,"mode":)" + mode + R"(,"position":)" + position + R"(,"forward":)" + forward +
           R"(,"up":)" + up + R"(,"sunDir":)" + sunDir + "}]}";
}

bool Loads(const std::string& text)
{
    Bookmarks b;
    REQUIRE_NOTHROW(b.LoadFromJson(text));
    return b.Get(1).used;
}
} // namespace

TEST_CASE("a valid bookmark loads", "[bookmarks]")
{
    Bookmarks b;
    b.LoadFromJson(Entry());
    REQUIRE(b.Get(1).used);
    REQUIRE(b.Get(1).pose.position.z == 3.0f);
    REQUIRE(b.Get(1).pose.mode == planets::core::CameraMode::FreeFly);
    REQUIRE(b.Get(1).sunDir.y == 1.0f);
}

TEST_CASE("wrong value types do not throw and skip the entry", "[bookmarks]")
{
    REQUIRE_FALSE(Loads(Entry("[\"1\",0,0]")));
    REQUIRE_FALSE(Loads(Entry("[1,0,null]")));
    REQUIRE_FALSE(Loads(Entry("[1,0]")));
    REQUIRE_FALSE(Loads(Entry("[1,2,3]", "[0,0,-1]", "[0,1,0]", "1")));
    REQUIRE_FALSE(Loads(R"({"bookmarks":[{"slot":"1"}]})"));
    REQUIRE_FALSE(Loads(R"({"bookmarks":5})"));
    REQUIRE_FALSE(Loads("not json"));
}

TEST_CASE("a bad entry does not hide a good one", "[bookmarks]")
{
    Bookmarks b;
    b.LoadFromJson(R"({"bookmarks":[{"slot":1,"mode":1,"position":[0,0,0],"forward":[0,0,-1],"up":[0,1,0]},
        {"slot":2,"position":[0,0,5],"forward":[0,0,-1],"up":[0,1,0]}]})");
    REQUIRE_FALSE(b.Get(1).used);
    REQUIRE(b.Get(2).used);
}

TEST_CASE("degenerate vectors are skipped", "[bookmarks]")
{
    REQUIRE_FALSE(Loads(Entry("[1,2,3]", "[0,0,0]")));
    REQUIRE_FALSE(Loads(Entry("[1,2,3]", "[0,0,-1]", "[0,0,0]")));
    REQUIRE_FALSE(Loads(Entry("[1,2,3]", "[0,1,0]", "[0,1,0]")));
    REQUIRE_FALSE(Loads(Entry("[1,2,3]", "[0,1,0]", "[0,-5,0]")));
    REQUIRE_FALSE(Loads(Entry("[1e999,2,3]")));
    REQUIRE_FALSE(Loads(Entry("[1,2,3]", "[1e39,0,0]")));
}

TEST_CASE("a zero or bad sunDir gets the default", "[bookmarks]")
{
    for (const char* sun : {"[0,0,0]", "[\"a\",0,0]", "[1,2]"})
    {
        Bookmarks b;
        b.LoadFromJson(Entry("[1,2,3]", "[0,0,-1]", "[0,1,0]", "\"orbit\"", sun));
        REQUIRE(b.Get(1).used);
        REQUIRE(b.Get(1).sunDir == Bookmark{}.sunDir);
    }
}

TEST_CASE("helpSeen reads only a true boolean", "[help]")
{
    using nlohmann::json;
    REQUIRE(IsHelpSeen(json{{"helpSeen", true}}));
    REQUIRE_FALSE(IsHelpSeen(json{{"helpSeen", false}}));
    REQUIRE_FALSE(IsHelpSeen(json{{"helpSeen", "yes"}}));
    REQUIRE_FALSE(IsHelpSeen(json{{"helpSeen", 1}}));
    REQUIRE_FALSE(IsHelpSeen(json::object()));
    REQUIRE_FALSE(IsHelpSeen(json::array()));
    REQUIRE_FALSE(IsHelpSeen(json()));
}

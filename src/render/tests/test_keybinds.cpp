#include "Keybinds.h"
#include <catch2/catch_test_macros.hpp>

using namespace planets::app;
using namespace planets::app::Keybinds;

TEST_CASE("only digit rows have a Ctrl twin", "[keybinds]")
{
    REQUIRE(HasCtrlTwin(Find(Action::RecallBookmark)));
    REQUIRE_FALSE(HasCtrlTwin(Find(Action::SaveBookmark)));
    REQUIRE_FALSE(HasCtrlTwin(Find(Action::ToggleMode)));
    REQUIRE_FALSE(HasCtrlTwin(Find(Action::Escape)));
}

TEST_CASE("held Ctrl does not block plain rows", "[keybinds]")
{
    for (Action action : {Action::ToggleMode, Action::FramePlanet, Action::Reroll, Action::Screenshot, Action::Escape})
    {
        const KeyBind& row = Find(action);
        REQUIRE(ModifierMatches(true, row.modifier, HasCtrlTwin(row)));
        REQUIRE(ModifierMatches(false, row.modifier, HasCtrlTwin(row)));
    }
}

TEST_CASE("Ctrl picks save over recall on the digits", "[keybinds]")
{
    const KeyBind& recall = Find(Action::RecallBookmark);
    const KeyBind& save = Find(Action::SaveBookmark);
    REQUIRE(ModifierMatches(false, recall.modifier, HasCtrlTwin(recall)));
    REQUIRE_FALSE(ModifierMatches(true, recall.modifier, HasCtrlTwin(recall)));
    REQUIRE(ModifierMatches(true, save.modifier, HasCtrlTwin(save)));
    REQUIRE_FALSE(ModifierMatches(false, save.modifier, HasCtrlTwin(save)));
}

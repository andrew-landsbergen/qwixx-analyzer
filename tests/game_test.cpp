#include <catch2/catch_test_macros.hpp>

#include "game_test.hpp"
#include "../src/game.hpp"

TEST_CASE( "Index-value conversions work as expected", "[conversion]" ) {
    REQUIRE( index_to_value(Color::red, 0) == 2 );
    REQUIRE( index_to_value(Color::yellow, 5) == 7 );
    REQUIRE( index_to_value(Color::green, 4) == 8 );
    REQUIRE( index_to_value(Color::blue, 10) == 2 );

    REQUIRE( value_to_index(Color::red, 10) == 8 );
    REQUIRE( value_to_index(Color::yellow, 5) == 3 );
    REQUIRE( value_to_index(Color::green, 12) == 0 );
    REQUIRE( value_to_index(Color::blue, 3) == 9 );
}

TEST_CASE( "Out-of-bounds index-value conversions are protected against", "[conversion][!mayfail]" ) {
    REQUIRE_THROWS( index_to_value(Color::red, 12) );
    REQUIRE_THROWS( index_to_value(Color::yellow, -1) );
    REQUIRE_THROWS( index_to_value(Color::green, 1000) );
    REQUIRE_THROWS( index_to_value(Color::blue, 11) );

    REQUIRE_THROWS( value_to_index(Color::red, 1) );
    REQUIRE_THROWS( value_to_index(Color::yellow, 13) );
    REQUIRE_THROWS( value_to_index(Color::green, 0) );
    REQUIRE_THROWS( value_to_index(Color::blue, 1000) );
}

void ScorepadTester::run_tests() {    
    static size_t red = static_cast<size_t>(Color::red);
    static size_t green = static_cast<size_t>(Color::green);
    
    // Basic initialization checks. We trust that all values are set correctly if these are.
    REQUIRE( m_rows[red][0] == false );
    REQUIRE( m_rows[green][0] == false );

    REQUIRE( m_rightmost_mark_indices[red] == std::nullopt );
    REQUIRE( m_num_marks[red] == 0 );

    REQUIRE ( m_penalties == 0 );

    SECTION( "Marking a move sets the corresponding index to true" ) {
        Color color = Color::red;
        size_t index = 0;
        mark_move( { color, index } );
        REQUIRE( m_rows[static_cast<size_t>(color)][index] == true );
    }

    SECTION( "Marking a move increments the number of marks by exactly 1 for non-locks" ) {
        Color color = Color::yellow;
        size_t index = 3;
        mark_move( { color, index } );
        REQUIRE( m_rows[static_cast<size_t>(color)][index] == true );
        REQUIRE( m_num_marks[static_cast<size_t>(color)] == 1 );
    }

    SECTION( "Marking a move increments the number of marks by exactly 2 for locks" ) {
        Color color = Color::yellow;
        size_t index = GameConstants::LOCK_INDEX;
        mark_move( { color, index } );
        REQUIRE( m_rows[static_cast<size_t>(color)][index] == true );
        REQUIRE( m_num_marks[static_cast<size_t>(color)] == 2 );
    }

    SECTION( "Marking a move updates the array of rightmost mark indices" ) {
        Color color = Color::green;
        size_t index = 8;
        mark_move( { color, index } );
        REQUIRE( m_rows[static_cast<size_t>(color)][index] == true );
        REQUIRE( m_rightmost_mark_indices[static_cast<size_t>(color)] == 8 );
    }

    SECTION( "Marking a penalty increments the penalty variable" ) {
        bool ret = mark_penalty();
        REQUIRE ( ret == false );
        REQUIRE ( m_penalties == 1 );
    }

    SECTION ( "Marking a penalty four or more times returns true, else false" ) {
        bool ret = mark_penalty();
        REQUIRE ( ret == false );
        ret = mark_penalty();
        REQUIRE ( ret == false );
        ret = mark_penalty();
        REQUIRE ( ret == false );
        ret = mark_penalty();
        REQUIRE ( ret == true );
        ret = mark_penalty();
        REQUIRE ( ret == true );
    }
}

TEST_CASE( "Scorepad tests", "[scorepad]" ) {
    ScorepadTester().run_tests();
}

/*
void GameTester::run_tests() {
    auto mock = []() {};
    
    std::vector<Color> dice = { Color::red, Color::yellow, Color::green, Color::blue };
    std::vector<int> rolls = { 0, 0, 0, 0, 0, 0 };
    std::array<Move, GameConstants::MAX_LEGAL_MOVES> legal_moves{};
    std::array<std::optional<Move>, GameConstants::MAX_PLAYERS> registered_moves{};

    MoveContext ctxt = {
        std::span<Color>(dice),
        std::span<int>(rolls),
        std::span<Move>(legal_moves),
        std::span<std::optional<Move>>(registered_moves)
    };

    bool ret = Game::resolve_action<ActionType::First>(ctxt, mock);
    REQUIRE( ret == false );
}

TEST_CASE( "Gane tests", "[game]" ) {
    GameTester(4).run_tests();
}
*/
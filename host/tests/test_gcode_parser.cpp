#include <gtest/gtest.h>
#include "gcode/parser.h"

using namespace hydra::gcode;

class ParserTest : public ::testing::Test {
protected:
    Parser parser;
};

TEST_F(ParserTest, ParseG0Rapid) {
    auto cmd = parser.parse("G0 X10.5 Y20.3 F6000");
    auto* move = std::get_if<MoveCommand>(&cmd);
    ASSERT_NE(move, nullptr);
    EXPECT_TRUE(move->rapid);
    EXPECT_DOUBLE_EQ(*move->x, 10.5);
    EXPECT_DOUBLE_EQ(*move->y, 20.3);
    EXPECT_DOUBLE_EQ(*move->f, 6000.0);
    EXPECT_FALSE(move->z.has_value());
    EXPECT_FALSE(move->e.has_value());
}

TEST_F(ParserTest, ParseG1Extrusion) {
    auto cmd = parser.parse("G1 X50 Y50 E1.234 F1800");
    auto* move = std::get_if<MoveCommand>(&cmd);
    ASSERT_NE(move, nullptr);
    EXPECT_FALSE(move->rapid);
    EXPECT_DOUBLE_EQ(*move->e, 1.234);
}

TEST_F(ParserTest, ParseG28HomeAll) {
    auto cmd = parser.parse("G28");
    auto* home = std::get_if<HomeCommand>(&cmd);
    ASSERT_NE(home, nullptr);
    EXPECT_TRUE(home->x);
    EXPECT_TRUE(home->y);
    EXPECT_TRUE(home->z);
}

TEST_F(ParserTest, ParseM104NozzleTemp) {
    auto cmd = parser.parse("M104 S210");
    auto* temp = std::get_if<TempCommand>(&cmd);
    ASSERT_NE(temp, nullptr);
    EXPECT_EQ(temp->target, TempCommand::Nozzle);
    EXPECT_DOUBLE_EQ(temp->temp_c, 210.0);
    EXPECT_FALSE(temp->wait);
}

TEST_F(ParserTest, ParseM109NozzleTempWait) {
    auto cmd = parser.parse("M109 S235 T1");
    auto* temp = std::get_if<TempCommand>(&cmd);
    ASSERT_NE(temp, nullptr);
    EXPECT_TRUE(temp->wait);
    EXPECT_EQ(temp->nozzle_id, 1);
    EXPECT_DOUBLE_EQ(temp->temp_c, 235.0);
}

TEST_F(ParserTest, EmptyLineReturnsNop) {
    auto cmd = parser.parse("");
    EXPECT_TRUE(std::holds_alternative<NopCommand>(cmd));
}

TEST_F(ParserTest, CommentOnlyReturnsNop) {
    auto cmd = parser.parse("; this is a normal comment");
    EXPECT_TRUE(std::holds_alternative<NopCommand>(cmd));
}

TEST_F(ParserTest, ParseFanOn) {
    auto cmd = parser.parse("M106 S255");
    auto* fan = std::get_if<FanCommand>(&cmd);
    ASSERT_NE(fan, nullptr);
    EXPECT_EQ(fan->speed, 255);
}

TEST_F(ParserTest, ParseFanOff) {
    auto cmd = parser.parse("M107");
    auto* fan = std::get_if<FanCommand>(&cmd);
    ASSERT_NE(fan, nullptr);
    EXPECT_EQ(fan->speed, -1);
}

TEST_F(ParserTest, ParseM600ValveSet) {
    auto cmd = parser.parse("M600 V15");
    auto* valve = std::get_if<ValveSet>(&cmd);
    ASSERT_NE(valve, nullptr);
    EXPECT_EQ(valve->mask, 0x0F);
}

TEST_F(ParserTest, ParseM600ValveQuality) {
    auto cmd = parser.parse("M600 V1");
    auto* valve = std::get_if<ValveSet>(&cmd);
    ASSERT_NE(valve, nullptr);
    EXPECT_EQ(valve->mask, 0x01);
}

TEST_F(ParserTest, ParseM600ValveInfill) {
    auto cmd = parser.parse("M600 V14");
    auto* valve = std::get_if<ValveSet>(&cmd);
    ASSERT_NE(valve, nullptr);
    EXPECT_EQ(valve->mask, 0x0E);
}

TEST_F(ParserTest, ParseM600FilamentChange) {
    auto cmd = parser.parse("M600");
    EXPECT_TRUE(std::holds_alternative<FilamentChange>(cmd));
}

TEST_F(ParserTest, ParseM600ValveCloseAll) {
    auto cmd = parser.parse("M600 V0");
    auto* valve = std::get_if<ValveSet>(&cmd);
    ASSERT_NE(valve, nullptr);
    EXPECT_EQ(valve->mask, 0x00);
}

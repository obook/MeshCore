#include <gtest/gtest.h>
#include "../../variants/thinknode_m6/StatusLed.h"

TEST(M6RedLed, SteadyForTenSecondsAtBoot) {
  uint32_t wait;
  EXPECT_TRUE(m6RedLed(false, 4000, false, 3900, wait));
  EXPECT_EQ(wait, 6000u);
}

TEST(M6RedLed, BlinksWhileChargingAndStaysOnWhenFull) {
  uint32_t wait;
  EXPECT_TRUE(m6RedLed(false, 20000, true, 3900, wait));
  EXPECT_FALSE(m6RedLed(true, 21000, true, 3900, wait));
  EXPECT_EQ(wait, 1000u);
  EXPECT_TRUE(m6RedLed(true, 22000, true, 4100, wait));
}

TEST(M6RedLed, OneMillisecondFlashEveryThreeSecondsOnBattery) {
  uint32_t wait;
  EXPECT_TRUE(m6RedLed(false, 20000, false, 3900, wait));
  EXPECT_EQ(wait, 1u);
  EXPECT_FALSE(m6RedLed(true, 20001, false, 3900, wait));
  EXPECT_EQ(wait, 2999u);
}

TEST(M6RedLed, FastBlinksOnlyAtTheStartOfEachHalfMinuteWhenCritical) {
  uint32_t wait;
  EXPECT_TRUE(m6RedLed(false, 60500, false, 3450, wait));
  EXPECT_EQ(wait, 250u);
  EXPECT_FALSE(m6RedLed(true, 60750, false, 3450, wait));
  EXPECT_FALSE(m6RedLed(false, 75000, false, 3450, wait));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

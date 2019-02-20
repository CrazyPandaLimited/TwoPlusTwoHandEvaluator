#include <iostream>

#include "gtest/gtest.h"

#include <pokerlib.hpp>

namespace pokerlib {
    extern int HR[32487834];
}

using namespace std;
using namespace pokerlib;

// init lookup tables
struct TestEnvironment : ::testing::Environment {
  virtual void SetUp() { pokerlib::init(); }
};

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new TestEnvironment);
  return RUN_ALL_TESTS();
}

TEST(TestCards, Basic)
{
    int cards[] = {"2c"_c, "2d"_c, "2h"_c, "2s"_c, "Ac"_c, "Tc"_c, "3c"_c};
    ASSERT_EQ(dump_cards(cards, 7), "2c2d2h2sAcTc3c");
}

TEST(TestSingleLookup, Basic)
{
    // Create a 7-card poker hand (each card gets a value between 0 and 52)
    int cards[] = {"2c"_c, "2d"_c, "2h"_c, "2s"_c, "Ac"_c, "Tc"_c, "3c"_c};
    int result  = lookup_7hand(cards);
    _PDEBUG("Hand: %s", dump_cards(cards, 7).c_str());
    _PDEBUG("Result: %d", result);
    _PDEBUG("Category: %d", result >> 12);
    _PDEBUG("Salt: %d", result & 0x00000FFF);
}

// Enumerate every possible 7-card poker hand (133784560)
TEST(DISABLED_TestEnumerate, Basic)
{
    int u0, u1, u2, u3, u4, u5;
    int c0, c1, c2, c3, c4, c5, c6;
    int handTypeSum[10];                         // Frequency of hand category (flush, 2 pair, etc)
    int count = 0;                               // total number of hands enumerated

    memset(handTypeSum, 0, sizeof(handTypeSum)); // do init..

    _PDEBUG("Enumerating and evaluating all 133,784,560 possible 7-card poker hands...");

    // On your mark, get set, go...
    // DWORD dwTime = GetTickCount();

    for (c0 = 1; c0 < 47; c0++) {
        u0 = HR[53 + c0];
        for (c1 = c0 + 1; c1 < 48; c1++) {
            u1 = HR[u0 + c1];
            for (c2 = c1 + 1; c2 < 49; c2++) {
                u2 = HR[u1 + c2];
                for (c3 = c2 + 1; c3 < 50; c3++) {
                    u3 = HR[u2 + c3];
                    for (c4 = c3 + 1; c4 < 51; c4++) {
                        u4 = HR[u3 + c4];
                        for (c5 = c4 + 1; c5 < 52; c5++) {
                            u5 = HR[u4 + c5];
                            for (c6 = c5 + 1; c6 < 53; c6++) {

                                handTypeSum[HR[u5 + c6] >> 12]++;

                                // JMD: The above line of code is equivalent to:
                                // int finalValue = HR[u5+c6];
                                // int handCategory = finalValue >> 12;
                                // handTypeSum[handCategory]++;

                                count++;
                            }
                        }
                    }
                }
            }
        }
    }

    // dwTime = GetTickCount() - dwTime;

    _PDEBUG("BAD:              %d", handTypeSum[0]);
    _PDEBUG("High Card:        %d", handTypeSum[1]);
    _PDEBUG("One Pair:         %d", handTypeSum[2]);
    _PDEBUG("Two Pair:         %d", handTypeSum[3]);
    _PDEBUG("Trips:            %d", handTypeSum[4]);
    _PDEBUG("Straight:         %d", handTypeSum[5]);
    _PDEBUG("Flush:            %d", handTypeSum[6]);
    _PDEBUG("Full House:       %d", handTypeSum[7]);
    _PDEBUG("Quads:            %d", handTypeSum[8]);
    _PDEBUG("Straight Flush:   %d", handTypeSum[9]);

    // Perform sanity checks.. make sure numbers are where they should be
    int testCount = 0;
    for (int index = 0; index < 10; index++)
        testCount += handTypeSum[index];

    ASSERT_FALSE(testCount != count || count != 133784560 || handTypeSum[0] != 0);

    _PDEBUG("Enumerated %d hands.", count);
}

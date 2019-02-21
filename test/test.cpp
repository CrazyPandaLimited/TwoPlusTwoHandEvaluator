#include <cmath>
#include <chrono>
#include <sstream>
#include <iostream>
#include <iomanip>

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

std::string with_suffix(long count) {
    std::stringstream ss;
    if (count < 1000) {
        ss << std::fixed << std::setprecision(2) << count;
        return ss.str();
    }

    int exp = std::log(count) / std::log(1000);
    static const char* suffixes = "kMGTPE";
    ss << std::fixed << std::setprecision(2) << count / std::pow(1000, exp) << suffixes[exp - 1];
    return ss.str();
}

TEST(TestCardStrToCardNum, Basic)
{
    ASSERT_EQ("2h"_c, 1);
    ASSERT_EQ("2d"_c, 2);
    ASSERT_EQ("2c"_c, 3);
    ASSERT_EQ("2s"_c, 4);

    ASSERT_EQ("Ah"_c, 49);
    ASSERT_EQ("Ad"_c, 50);
    ASSERT_EQ("Ac"_c, 51);
    ASSERT_EQ("As"_c, 52);
}

TEST(TestCards, Basic)
{
    ASSERT_EQ(dump_hand(Cards<7>{"2c"_c,"2d"_c,"2h"_c,"2s"_c,"Ac"_c,"Tc"_c,"3c"_c}), "2c2d2h2sAcTc3c");
    ASSERT_EQ(dump_hand(Cards<7>{"Ac"_c,"Ad"_c,"Ah"_c,"As"_c,"3c"_c,"4c"_c,"5c"_c}), "AcAdAhAs3c4c5c");
}

void test_hand(const Cards<7>& cards, Hand hand) {
    int result  = lookup(cards);
    _PDEBUG("Cards: %s, Result: %d, Hand: %d, Salt: %d", dump_hand(cards).c_str(), result, result >> 12, result & 0x00000FFF);
    ASSERT_EQ(hand, to_hand(result));
}

TEST(TestMainCombinations, Basic)
{
    test_hand({"3c"_c, "5c"_c, "8c"_c, "Tc"_c, "Js"_c, "Ks"_c, "As"_c}, Hand::HIGH_CARD);
    test_hand({"3c"_c, "3d"_c, "8c"_c, "Tc"_c, "Js"_c, "Ks"_c, "As"_c}, Hand::ONE_PAIR);
    test_hand({"3c"_c, "3d"_c, "8c"_c, "8s"_c, "Js"_c, "Ks"_c, "As"_c}, Hand::TWO_PAIR);
    test_hand({"3c"_c, "3d"_c, "3s"_c, "8s"_c, "Jh"_c, "Ks"_c, "Ah"_c}, Hand::THREE_OF_A_KIND);
    test_hand({"6c"_c, "7s"_c, "8s"_c, "9c"_c, "Td"_c, "2s"_c, "3s"_c}, Hand::STRAIGHT);
    test_hand({"3c"_c, "4c"_c, "8c"_c, "9c"_c, "Jc"_c, "Ks"_c, "As"_c}, Hand::FLUSH);
    test_hand({"6c"_c, "6s"_c, "6d"_c, "Kc"_c, "Kd"_c, "2d"_c, "3s"_c}, Hand::FULLHOUSE);
    test_hand({"3c"_c, "3d"_c, "3s"_c, "3h"_c, "Js"_c, "Ks"_c, "As"_c}, Hand::FOUR_OF_A_KIND);
    test_hand({"6h"_c, "7h"_c, "8h"_c, "9h"_c, "Th"_c, "2s"_c, "3s"_c}, Hand::STRAIGHT_FLUSH);
}

// Enumerate every possible 7-card poker hand (133784560)
TEST(DISABLED_TestEnumerate, Basic)
{
    // Store the count of each type of hand (One Pair, Flush, etc)
    int handTypeSum[10] = {0};

    // Total number of hands enumerated
    int count = 0;

    _PDEBUG("Enumerating and evaluating all 133,784,560 possible 7-card poker hands...");

    chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();

    int u0, u1, u2, u3, u4, u5;
    int c0, c1, c2, c3, c4, c5, c6;
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

    chrono::time_point<chrono::system_clock> stop = chrono::system_clock::now();

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
    _PDEBUG("Enumerated %d hands in: %fs", count, chrono::duration<double>(stop - start).count());
    _PDEBUG("Speed: %s", with_suffix(count / chrono::duration<double>(stop - start).count()).c_str());

    int testCount = 0;
    for (int index = 0; index < 10; index++)
        testCount += handTypeSum[index];

    ASSERT_EQ(handTypeSum[0], 0); // no bad cards
    ASSERT_EQ(testCount, count);
    ASSERT_EQ(count, 133784560);
}

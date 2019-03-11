#include <cmath>
#include <chrono>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <bitset>

#include "gtest/gtest.h"

#include <pokerlib.hpp>

namespace pokerlib {
    extern mio::mmap_source ranks_map;
}

using namespace std;
using namespace pokerlib;

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

void test_kev_hand(const std::string& str, Hand hand) {
    std::vector<int> cards = str_to_kev(str);
    int result_kev  = eval_hand(cards);
    int result  = convert_kev_rank(result_kev);
    Hand result_hand = to_hand(result);
    _PTRACE("Cards: %s, ResultKev: %d, Result: %d,  Hand: %d (%s)", kev_to_str(cards).c_str(), result_kev, result, result_hand, to_string(result_hand));
    for(int i = 0; i < cards.size(); ++i) {
        std::cout << bitset<32>(cards[i]) << " ";
    }
    std::cout << endl;
    ASSERT_EQ(hand, result_hand);
}

void test_hand(const std::string& str, Hand hand) {
    std::vector<int> cards = str_to_cards(str);
    int result  = lookup(&cards[0], cards.size());
    Hand result_hand = to_hand(result);
    _PTRACE("Cards: %s, Result: %d, Hand: %d (%s)", dump_hand(cards).c_str(), result, result_hand, to_string(result_hand));
    ASSERT_EQ(hand, result_hand);
}

TEST(TestKevToStrLoop, Basic)
{
    static const std::string ranks = "23456789TJQKA";
    static const std::string suits = "schd";
    for (char suit : suits) {
        for (char rank : ranks) {
            std::string card{rank, suit};
            ASSERT_EQ(kev_to_str(str_to_kev(card)), card);
        }
    }
}

TEST(TestKevToStrRand, Basic)
{
    ASSERT_EQ(kev_to_str(str_to_kev("2s3c4h5dAs")), "2s3c4h5dAs");
    ASSERT_EQ(kev_to_str(str_to_kev("Ad3cJh5dAs")), "Ad3cJh5dAs");
    ASSERT_EQ(kev_to_str(str_to_kev("Td3cJh5dQs")), "Td3cJh5dQs");
}

TEST(TestKevToStrJoker, Basic)
{
    ASSERT_EQ(kev_to_str(str_to_kev("Xs")), "Xs");
    ASSERT_EQ(kev_to_str(str_to_kev("Xc")), "Xc");
    ASSERT_EQ(kev_to_str(str_to_kev("Xh")), "Xh");
    ASSERT_EQ(kev_to_str(str_to_kev("Xd")), "Xd");
}

TEST(TestCards, Basic)
{
    ASSERT_EQ(dump_hand(str_to_cards("2c2d2h2sAcTc3c")), "2c2d2h2sAcTc3c");
    ASSERT_EQ(dump_hand(str_to_cards("AcAdAhAs3c4c5c")), "AcAdAhAs3c4c5c");
}

TEST(TestMainCombinationsFor5CardsKev, BasicKev)
{
    test_kev_hand("3c5c8cTcJs", Hand::HIGH_CARD);
    test_kev_hand("3c3d8cTcJs", Hand::ONE_PAIR);
    test_kev_hand("3c3d8c8sJs", Hand::TWO_PAIR);
    test_kev_hand("3c3d3s8sJh", Hand::THREE_OF_A_KIND);
    test_kev_hand("3cJh8d3d3s", Hand::THREE_OF_A_KIND);
    test_kev_hand("6c7s8s9cTd", Hand::STRAIGHT);
    test_kev_hand("3c4c8c9cJc", Hand::FLUSH);
    test_kev_hand("6c6s6dKcKd", Hand::FULLHOUSE);
    test_kev_hand("3c3d3s3hJs", Hand::FOUR_OF_A_KIND);
    test_kev_hand("6h7h8h9hTh", Hand::STRAIGHT_FLUSH);
}

TEST(TestMainCombinationsFor6CardsKev, BasicKev)
{
    test_kev_hand("3c5c8cTcJsKs", Hand::HIGH_CARD);
    test_kev_hand("3c3d8cTcJsKs", Hand::ONE_PAIR);
    test_kev_hand("3c3d8c8sJsKs", Hand::TWO_PAIR);
    test_kev_hand("3c3d3s8sJhKs", Hand::THREE_OF_A_KIND);
    test_kev_hand("6c7s8s9cTd2s", Hand::STRAIGHT);
    test_kev_hand("3c4c8c9cJcKs", Hand::FLUSH);
    test_kev_hand("6c6s6dKcKd2d", Hand::FULLHOUSE);
    test_kev_hand("3c3d3s3hJsKs", Hand::FOUR_OF_A_KIND);
    test_kev_hand("6h7h8h9hTh2s", Hand::STRAIGHT_FLUSH);

    test_kev_hand("As8h5d4s3c2s", Hand::STRAIGHT);
}

TEST(TestMainCombinationsFor7CardsKev, BasicKev)
{
    test_kev_hand("3c5c8cTcJsKsAs", Hand::HIGH_CARD);
    test_kev_hand("3c3d8cTcJsKsAs", Hand::ONE_PAIR);
    test_kev_hand("3c3d8c8sJsKsAs", Hand::TWO_PAIR);
    test_kev_hand("3c3d3s8sJhKsAh", Hand::THREE_OF_A_KIND);
    test_kev_hand("6c7s8s9cTd2s3s", Hand::STRAIGHT);
    test_kev_hand("3c4c8c9cJcKsAs", Hand::FLUSH);
    test_kev_hand("6c6s6dKcKd2d3s", Hand::FULLHOUSE);
    test_kev_hand("3c3d3s3hJsKsAs", Hand::FOUR_OF_A_KIND);
    test_kev_hand("6h7h8h9hTh2s3s", Hand::STRAIGHT_FLUSH);
}

TEST(TestMainCombinationsFor5Cards, BasicTable)
{
    test_hand("3c5c8cTcJs", Hand::HIGH_CARD);
    test_hand("3c3d8cTcJs", Hand::ONE_PAIR);
    test_hand("3c3d8c8sJs", Hand::TWO_PAIR);
    test_hand("3c3d3s8sJh", Hand::THREE_OF_A_KIND);
    test_hand("6c7s8s9cTd", Hand::STRAIGHT);
    test_hand("3c4c8c9cJc", Hand::FLUSH);
    test_hand("6c6s6dKcKd", Hand::FULLHOUSE);
    test_hand("3c3d3s3hJs", Hand::FOUR_OF_A_KIND);
    test_hand("6h7h8h9hTh", Hand::STRAIGHT_FLUSH);
}

TEST(TestMainCombinationsFor6Cards, BasicTable)
{
    test_hand("3c5c8cTcJsKs", Hand::HIGH_CARD);
    test_hand("3c3d8cTcJsKs", Hand::ONE_PAIR);
    test_hand("3c3d8c8sJsKs", Hand::TWO_PAIR);
    test_hand("3c3d3s8sJhKs", Hand::THREE_OF_A_KIND);
    test_hand("6c7s8s9cTd2s", Hand::STRAIGHT);
    test_hand("3c4c8c9cJcKs", Hand::FLUSH);
    test_hand("6c6s6dKcKd2d", Hand::FULLHOUSE);
    test_hand("3c3d3s3hJsKs", Hand::FOUR_OF_A_KIND);
    test_hand("6h7h8h9hTh2s", Hand::STRAIGHT_FLUSH);
}

TEST(TestMainCombinationsFor7Cards, BasicTable)
{
    test_hand("3c5c8cTcJsKsAs", Hand::HIGH_CARD);
    test_hand("3c3d8cTcJsKsAs", Hand::ONE_PAIR);
    test_hand("3c3d8c8sJsKsAs", Hand::TWO_PAIR);
    test_hand("3c3d3s8sJhKsAh", Hand::THREE_OF_A_KIND);
    test_hand("6c7s8s9cTd2s3s", Hand::STRAIGHT);
    test_hand("3c4c8c9cJcKsAs", Hand::FLUSH);
    test_hand("6c6s6dKcKd2d3s", Hand::FULLHOUSE);
    test_hand("3c3d3s3hJsKsAs", Hand::FOUR_OF_A_KIND);
    test_hand("6h7h8h9hTh2s3s", Hand::STRAIGHT_FLUSH);
}

TEST(Test1JokerCombinations, BasicJoker5)
{
    test_hand("Xc3d8h5cAd", Hand::ONE_PAIR);
    test_hand("XcJh8d3d3s", Hand::THREE_OF_A_KIND);
    test_hand("3cJh8d3d3s", Hand::THREE_OF_A_KIND);
    test_hand("Xc6h3d2d2h", Hand::THREE_OF_A_KIND);
    test_hand("Xc7s8s9cTd", Hand::STRAIGHT);
    test_hand("XcJc9c6c3c", Hand::FLUSH);
    test_hand("Jc9c6c3cXc", Hand::FLUSH);
    test_hand("XhJs3c3d3s", Hand::FOUR_OF_A_KIND);
    test_hand("6h7hXc9hTh", Hand::STRAIGHT_FLUSH);
    test_hand("JhJcJdJsXh", Hand::FIVE_OF_A_KIND);
}

TEST(Test2JokerCombinations, BasicJoker5)
{
    test_hand("XcXdJh8d2s", Hand::THREE_OF_A_KIND);
    test_hand("XcXdJh8d3s", Hand::THREE_OF_A_KIND);
    test_hand("XcXd8s9cTd", Hand::STRAIGHT);
    test_hand("XdXs9c6c3c", Hand::FLUSH);
    test_hand("XhXs3c3d4s", Hand::FOUR_OF_A_KIND);
    test_hand("Xs7hXc9hTh", Hand::STRAIGHT_FLUSH);
    test_hand("XsJhJdJsXh", Hand::FIVE_OF_A_KIND);
}

TEST(Test3JokerCombinations, BasicJoker5)
{
    test_hand("XhXsXc3d4s", Hand::FOUR_OF_A_KIND);
    test_hand("XsXhXc9hTh", Hand::STRAIGHT_FLUSH);
    test_hand("XsXdJdJsXh", Hand::FIVE_OF_A_KIND);
}

TEST(Test4JokerCombinations, BasicJoker5)
{
    test_hand("XcXdXhXs2s", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs3s", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs4s", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs5s", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs6s", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs7s", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs8s", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs9s", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXsTs", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXsJs", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXsQs", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXsKs", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXsAs", Hand::FIVE_OF_A_KIND);
}

TEST(Test2JokerCombinationsMixedOrder, BasicJoker5)
{
    test_hand("XcXdJh8d2s", Hand::THREE_OF_A_KIND);
    test_hand("XcXdJh8d2c", Hand::THREE_OF_A_KIND);
    test_hand("XcXdJh8d2d", Hand::THREE_OF_A_KIND);
    test_hand("XcXdJh8d2h", Hand::THREE_OF_A_KIND);

    test_hand("XcXdJh8d3s", Hand::THREE_OF_A_KIND);
    test_hand("XcXdJh8d3c", Hand::THREE_OF_A_KIND);
    test_hand("XcXdJh8d3d", Hand::THREE_OF_A_KIND);
    test_hand("XcXdJh8d3h", Hand::THREE_OF_A_KIND);

    test_hand("XcXdJh8d3s", Hand::THREE_OF_A_KIND);
    test_hand("Jh8d3cXcXh", Hand::THREE_OF_A_KIND);
    test_hand("JhXd8dXs3d", Hand::THREE_OF_A_KIND);
    test_hand("XsJh8d3hXh", Hand::THREE_OF_A_KIND);
}

TEST(Test1JokerCombinations, BasicJoker6)
{
    test_hand("Xc3d8h5cAd9s", Hand::ONE_PAIR);
    test_hand("XcJh8d3d3s2d", Hand::THREE_OF_A_KIND);
    test_hand("3cJh8d3d3s2d", Hand::THREE_OF_A_KIND);
    test_hand("Xc6h3d2d2h9s", Hand::THREE_OF_A_KIND);
    test_hand("Xc7s8s9cTd2d", Hand::STRAIGHT);
    test_hand("Ad2d3dXc5c8h", Hand::STRAIGHT);
    test_hand("XcJc9c6c3c2d", Hand::FLUSH);
    test_hand("Jc9c6c3cXc2d", Hand::FLUSH);
    test_hand("XhJs3c3d3s2d", Hand::FOUR_OF_A_KIND);
    test_hand("6h7hXc9hTh2d", Hand::STRAIGHT_FLUSH);
    test_hand("JhJcJdJsXh2d", Hand::FIVE_OF_A_KIND);
}

TEST(Test2JokerCombinations, BasicJoker6)
{
    test_hand("XcXdJh8d3s2c", Hand::THREE_OF_A_KIND);
    test_hand("XcXd8s9cTd2c", Hand::STRAIGHT);
    test_hand("XdXs9c6c3c2h", Hand::FLUSH);
    test_hand("XhXs3c3d4s2c", Hand::FOUR_OF_A_KIND);
    test_hand("Xs7hXc9hTh2c", Hand::STRAIGHT_FLUSH);
    test_hand("XsJhJdJsXh2c", Hand::FIVE_OF_A_KIND);
}

TEST(Test3JokerCombinations, BasicJoker6)
{
    test_hand("XhXsXc3d4s2c", Hand::FOUR_OF_A_KIND);
    test_hand("XsXhXc9hTh2c", Hand::STRAIGHT_FLUSH);
    test_hand("XsXdJdJsXh2c", Hand::FIVE_OF_A_KIND);
}

TEST(Test4JokerCombinations, BasicJoker6)
{
    test_hand("XcXdXhXs2s5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs3s5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs4s5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs5s5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs6s5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs7s5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs8s5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXs9s5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXsTs5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXsJs5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXsQs5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXsKs5h", Hand::FIVE_OF_A_KIND);
    test_hand("XcXdXhXsAs5h", Hand::FIVE_OF_A_KIND);
}

TEST(Test1JokerCombinations, BasicJoker7)
{
    test_hand("Xc3d8h5cAd9sJh", Hand::ONE_PAIR);
    test_hand("XcJh8d3d3s2dAh", Hand::THREE_OF_A_KIND);
    test_hand("3cJh8d3d3s2dAh", Hand::THREE_OF_A_KIND);
    test_hand("Xc6h3d2d2hTs9h", Hand::THREE_OF_A_KIND);
    test_hand("Xc7s8s9cTd2d9h", Hand::STRAIGHT);
    test_hand("Ad2d3dXc5c8h9h", Hand::STRAIGHT);
    test_hand("XcJc9c6c3c2d9h", Hand::FLUSH);
    test_hand("Jc9c6c3cXc2d9h", Hand::FLUSH);
    test_hand("XhJs3c3d3s2d9h", Hand::FOUR_OF_A_KIND);
    test_hand("6h7hXc9hTh2dQh", Hand::STRAIGHT_FLUSH);
    test_hand("JhJcJdJsXh2dQh", Hand::FIVE_OF_A_KIND);
}

TEST(TestEnumerate56, Basic)
{
    const int* HR = get_table();

    // Store the count of each type of hand (One Pair, Flush, etc)
    int handTypeSum[11] = {0};

    // Total number of hands enumerated
    int count = 0;

    //_PDEBUG("Enumerating and evaluating all 133,784,560 possible 7-card poker hands...");

    chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();

    int u0, u1, u2, u3, u4, u5;
    int c0, c1, c2, c3, c4, c5, c6;
    for (c0 = 1; c0 < 51; c0++) {
        u0 = HR[57 + c0];
        for (c1 = c0 + 1; c1 < 52; c1++) {
            u1 = HR[u0 + c1];
            for (c2 = c1 + 1; c2 < 53; c2++) {
                u2 = HR[u1 + c2];
                for (c3 = c2 + 1; c3 < 54; c3++) {
                    u3 = HR[u2 + c3];
                    for (c4 = c3 + 1; c4 < 55; c4++) {
                        u4 = HR[u3 + c4];
                        for (c5 = c4 + 1; c5 < 56; c5++) {
                            u5 = HR[u4 + c5];
                            for (c6 = c5 + 1; c6 < 57; c6++) {
                                handTypeSum[HR[u5 + c6] >> 12]++;
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
    _PDEBUG("Five of a kind:   %d", handTypeSum[10]);
    _PDEBUG("Total:            %d", count);
    _PDEBUG("Enumerated %d hands in: %fs", count, chrono::duration<double>(stop - start).count());
    _PDEBUG("Speed: %s", with_suffix(count / chrono::duration<double>(stop - start).count()).c_str());

    int testCount = 0;
    for (int index = 0; index < 11; index++)
        testCount += handTypeSum[index];

    ASSERT_EQ(handTypeSum[0], 0); // no bad cards
    ASSERT_EQ(testCount, count);
    ASSERT_EQ(count, 231917400);
}

TEST(TestEnumerate55, Basic)
{
    const int* HR = get_table();

    // Store the count of each type of hand (One Pair, Flush, etc)
    int handTypeSum[11] = {0};

    // Total number of hands enumerated
    int count = 0;

    chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();

    int u0, u1, u2, u3, u4, u5;
    int c0, c1, c2, c3, c4, c5, c6;
    for (c0 = 1; c0 < 50; c0++) {
        u0 = HR[57 + c0];
        for (c1 = c0 + 1; c1 < 51; c1++) {
            u1 = HR[u0 + c1];
            for (c2 = c1 + 1; c2 < 52; c2++) {
                u2 = HR[u1 + c2];
                for (c3 = c2 + 1; c3 < 53; c3++) {
                    u3 = HR[u2 + c3];
                    for (c4 = c3 + 1; c4 < 54; c4++) {
                        u4 = HR[u3 + c4];
                        for (c5 = c4 + 1; c5 < 55; c5++) {
                            u5 = HR[u4 + c5];
                            for (c6 = c5 + 1; c6 < 56; c6++) {
                                handTypeSum[HR[u5 + c6] >> 12]++;
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
    _PDEBUG("Five of a kind:   %d", handTypeSum[10]);
    _PDEBUG("Total:            %d", count);
    _PDEBUG("Enumerated %d hands in: %fs", count, chrono::duration<double>(stop - start).count());
    _PDEBUG("Speed: %s", with_suffix(count / chrono::duration<double>(stop - start).count()).c_str());

    int testCount = 0;
    for (int index = 0; index < 11; index++)
        testCount += handTypeSum[index];

    ASSERT_EQ(handTypeSum[0], 0); // no bad cards
    ASSERT_EQ(testCount, count);
    ASSERT_EQ(count, 202927725);
}

TEST(TestEnumerate54, Basic)
{
    const int* HR = get_table();

    // Store the count of each type of hand (One Pair, Flush, etc)
    int handTypeSum[11] = {0};

    // Total number of hands enumerated
    int count = 0;

    chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();

    int u0, u1, u2, u3, u4, u5;
    int c0, c1, c2, c3, c4, c5, c6;
    for (c0 = 1; c0 < 49; c0++) {
        u0 = HR[57 + c0];
        for (c1 = c0 + 1; c1 < 50; c1++) {
            u1 = HR[u0 + c1];
            for (c2 = c1 + 1; c2 < 51; c2++) {
                u2 = HR[u1 + c2];
                for (c3 = c2 + 1; c3 < 52; c3++) {
                    u3 = HR[u2 + c3];
                    for (c4 = c3 + 1; c4 < 53; c4++) {
                        u4 = HR[u3 + c4];
                        for (c5 = c4 + 1; c5 < 54; c5++) {
                            u5 = HR[u4 + c5];
                            for (c6 = c5 + 1; c6 < 55; c6++) {
                                handTypeSum[HR[u5 + c6] >> 12]++;
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
    _PDEBUG("Five of a kind:   %d", handTypeSum[10]);
    _PDEBUG("Total:            %d", count);
    _PDEBUG("Enumerated %d hands in: %fs", count, chrono::duration<double>(stop - start).count());
    _PDEBUG("Speed: %s", with_suffix(count / chrono::duration<double>(stop - start).count()).c_str());

    int testCount = 0;
    for (int index = 0; index < 11; index++)
        testCount += handTypeSum[index];

    ASSERT_EQ(handTypeSum[0], 0); // no bad cards
    ASSERT_EQ(testCount, count);
    ASSERT_EQ(count, 177100560);
}

TEST(TestEnumerate53, Basic)
{
    const int* HR = get_table();

    // Store the count of each type of hand (One Pair, Flush, etc)
    int handTypeSum[11] = {0};

    // Total number of hands enumerated
    int count = 0;

    chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();

    int u0, u1, u2, u3, u4, u5;
    int c0, c1, c2, c3, c4, c5, c6;
    for (c0 = 1; c0 < 48; c0++) {
        u0 = HR[57 + c0];
        for (c1 = c0 + 1; c1 < 49; c1++) {
            u1 = HR[u0 + c1];
            for (c2 = c1 + 1; c2 < 50; c2++) {
                u2 = HR[u1 + c2];
                for (c3 = c2 + 1; c3 < 51; c3++) {
                    u3 = HR[u2 + c3];
                    for (c4 = c3 + 1; c4 < 52; c4++) {
                        u4 = HR[u3 + c4];
                        for (c5 = c4 + 1; c5 < 53; c5++) {
                            u5 = HR[u4 + c5];
                            for (c6 = c5 + 1; c6 < 54; c6++) {
                                handTypeSum[HR[u5 + c6] >> 12]++;
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
    _PDEBUG("Five of a kind:   %d", handTypeSum[10]);
    _PDEBUG("Total:            %d", count);
    _PDEBUG("Enumerated %d hands in: %fs", count, chrono::duration<double>(stop - start).count());
    _PDEBUG("Speed: %s", with_suffix(count / chrono::duration<double>(stop - start).count()).c_str());

    int testCount = 0;
    for (int index = 0; index < 11; index++)
        testCount += handTypeSum[index];

    ASSERT_EQ(handTypeSum[0], 0); // no bad cards
    ASSERT_EQ(testCount, count);
    ASSERT_EQ(count, 154143080);
}

// Enumerate every possible 7-card poker hand (133784560)
TEST(TestEnumerate52, Basic)
{
    const int* HR = get_table();

    // Store the count of each type of hand (One Pair, Flush, etc)
    int handTypeSum[10] = {0};

    // Total number of hands enumerated
    int count = 0;

    _PDEBUG("Enumerating and evaluating all 133,784,560 possible 7-card poker hands...");

    chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();

    int u0, u1, u2, u3, u4, u5;
    int c0, c1, c2, c3, c4, c5, c6;
    for (c0 = 1; c0 < 47; c0++) {
        u0 = HR[57 + c0];
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


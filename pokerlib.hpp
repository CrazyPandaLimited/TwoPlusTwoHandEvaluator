#pragma once

#include <string>
#include <cstdio>
#include <array>
#include <cassert>

#define POKER_LIB_DEBUG 1
#define _PDEBUG(fmt, ...) do { if (POKER_LIB_DEBUG >= 1) fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while (0)
#define _PPRINT(fmt, ...) do { if (POKER_LIB_DEBUG >= 1) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

namespace pokerlib {

int64_t IDs[612978];
int     HR[32487834];

enum Hand {
    HIGH_CARD       = 1,
    ONE_PAIR        = 2,
    TWO_PAIR        = 3,
    THREE_OF_A_KIND = 4,
    STRAIGHT        = 5,
    FLUSH           = 6,
    FULLHOUSE       = 7,
    FOUR_OF_A_KIND  = 8,
    STRAIGHT_FLUSH  = 9,
    FIVE_OF_A_KIND  = 10
};

// convert card rank and suit to card index [1-52]
// index: 1 2 3 4 5 6 7 8 9 .. 52
// card:  2 2 2 2 3 3 3 3 4 .. A
inline int to_card(char rank, char suit) {
    int card;

    if(rank == 'T') card = 8;
    else if(rank == 'J') card = 9;
    else if(rank == 'Q') card = 10;
    else if(rank == 'K') card = 11;
    else if(rank == 'A') card = 12;
    else card = rank - '2';

    card <<= 2;

    if(suit == 's') card += 3;
    else if(suit == 'c') card += 2;
    else if(suit == 'd') card += 1;

    return card + 1;
}

inline int operator"" _c(const char* card, size_t size) {
    assert(size == 2);
    return to_card(card[0], card[1]);
}

template <std::size_t N> using Cards = int[N];

void        init();
void        init_deck(int* deck);
int         find_card(int rank, int suit, int* deck);
short       eval_5hand(int* hand);
int         eval_5hand_fast(int c1, int c2, int c3, int c4, int c5);
int         hand_rank(short val);
std::string print_hand(int* hand, int n);
void        shuffle_deck(int* deck, int size);
short       eval_5cards(int c1, int c2, int c3, int c4, int c5);
short       eval_7hand(int* hand);
int         lookup(const int* cards);
inline Hand to_hand(int result) { return static_cast<Hand>(result >> 12); }

template <std::size_t N> std::string dump_hand(const Cards<N>& hand) {
    static const char* ranks = "23456789TJQKA";
    static const char* suits = "hdcs";
    std::string        result;
    result.reserve(N * 2);
    for (size_t i = 0; i < N; i++) {
        result.push_back(ranks[(int)((hand[i] - 1) / 4)]);
        result.push_back(suits[(int)((hand[i] - 1) % 4)]);
    }
    return result;
}

} // namespace pokerlib

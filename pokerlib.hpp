#pragma once

#include <string>
#include <cstdio>

#define POKER_LIB_DEBUG 1
#define _PDEBUG(fmt, ...) do { if (POKER_LIB_DEBUG >= 1) fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while (0)
#define _PPRINT(fmt, ...) do { if (POKER_LIB_DEBUG >= 1) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

namespace pokerlib {

int64_t IDs[612978];
int     HR[32487834];

inline int to_card(char rank, char suit) {
    int card = 0;
    if(suit == 'c') card = 0;
    else if(suit == 'd') card = 13;
    else if(suit == 'h') card = 26;
    else card = 39;

    if(rank == 'T') card += 8;
    else if(rank == 'J') card += 9;
    else if(rank == 'Q') card += 10;
    else if(rank == 'K') card += 11;
    else if(rank == 'A') card += 12;
    else card += rank - '2';
    return card;
}

inline int operator"" _c(const char* card, size_t size) {
    return to_card(card[0], card[1]);
}

void        init();
void        init_deck(int* deck);
int         find_card(int rank, int suit, int* deck);
short       eval_5hand(int* hand);
int         eval_5hand_fast(int c1, int c2, int c3, int c4, int c5);
int         hand_rank(short val);
std::string dump_cards(int* hand, int n);
std::string print_hand(int* hand, int n);
void        shuffle_deck(int* deck, int size);
short       eval_5cards(int c1, int c2, int c3, int c4, int c5);
short       eval_7hand(int* hand);
int         lookup_7hand(int* cards);

} // namespace pokerlib

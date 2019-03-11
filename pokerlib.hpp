#pragma once

#include <string>
#include <cstdio>
#include <array>
#include <vector>
#include <cassert>
#include <exception>
#include <climits>

#include <iostream>

#include "mio.hpp"

#define POKER_LIB_DEBUG 2
#define _PDEBUG(fmt, ...) do { if (POKER_LIB_DEBUG >= 1) fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while (0)
#define _PPRINT(fmt, ...) do { if (POKER_LIB_DEBUG >= 1) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)
#define _PTRACE(fmt, ...) do { if (POKER_LIB_DEBUG >= 2) fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while (0)

namespace pokerlib {

const char* RANKS_FILE_NAME = "handranks.dat";
const char* STANDARD_RANKS_FILE_NAME = "standard_handranks.dat";

static mio::mmap_source ranks_map __attribute__((init_priority(101)));
static mio::mmap_source standard_ranks_map __attribute__((init_priority(101)));

const int* get_table();

// Not optimized combinations for 7 cards: 52!/(7!*(52-7)!) = 133,784,560
// There is an optimization here starting with 4 cards - we can remove suits for impossible combinations.
// 1 card hands - 52!/(1!*(52-1)!)=52
// 2 card hands - 52!/(2!*(52-2)!)=1326
// 3 card hands - 52!/(3!*(52-3)!)=22100
// 4 card hands - 52!/(4!*(52-4)!)=270725 or 84448 with optimization
// 5 card hands - 52!/(5!*(52-5)!)=2598960 or 152607 with optimization
// 6 card hands - 52!/(6!*(52-6)!)=20358520 or 352443 with optimization
// So we have 612976 = 352443 + 152607 + 84448 + 22100 + 1326 + 52 hands in total
const int64_t STANDARD_IDS_COUNT = 612978;
const int STANDARD_HAND_RANKS_COUNT = 32487834;

// Total not optimized combinations: 56!/(7!*(56-7)!) = 231,917,400
const int64_t JOKER_IDS_COUNT = 1019493;
const int JOKER_HAND_RANKS_COUNT = 57541214;

const int STANDARD_DECK_SIZE = 52;
const int JOKER_DECK_SIZE = 56;
const int RANKS_COUNT = 13;
const int JOKER_RANKS_COUNT = 14;
const int SUITS_COUNT = 4;

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

inline const char* to_string(Hand h)
{
    switch (h)
    {
	case HIGH_CARD      : return "HighCard";
	case ONE_PAIR       : return "OnePair";
	case TWO_PAIR       : return "TwoPair";
	case THREE_OF_A_KIND: return "ThreeOfAKind";
	case STRAIGHT       : return "Straight";
	case FLUSH          : return "Flush";
	case FULLHOUSE      : return "Fullhouse";
	case FOUR_OF_A_KIND : return "FourOfAKind";
	case STRAIGHT_FLUSH : return "StraightFlush";
	case FIVE_OF_A_KIND : return "FiveOfAKind";
	default             : return "Unknown";
    }
}

/*
** each of the thirteen card ranks has its own prime number
**
** deuce = 2
** trey  = 3
** four  = 5
** five  = 7
** ...
** king  = 37
** ace   = 41
** joker = 43
*/
const int primes[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43 };

class Error : public std::runtime_error {
public:
    Error(const std::string& error) : std::runtime_error(error) {}
};

struct Debug {
    bool on = false;
    bool verbose = false;
    int64_t id = INT_MAX;
    int cardnum = 0;
    int jokernum = 0;
    std::function<bool(int*, int, int)> id_callback;
    std::function<bool(int64_t, int)> eval_callback;
};

static Debug nodebug = {false, false, 0, 0, 0, [](int*, int, int){ return false; }, [](int64_t, int){ return false; }};

// Converts card rank and suit to card index. Up to four jokers (Xs,Xh,Xd,Xc) of different suits are possible.
// index: 1 2 3 4 5 6 7 8 9            52 53 54 55 56
// card:  2 2 2 2 3 3 3 3 4 .. A  A  A  A  X  X  X  X
// suits order: shdc
inline int to_card(char rank, char suit) {
    int card;

    if(rank == 'T') card = 8;
    else if(rank == 'J') card = 9;
    else if(rank == 'Q') card = 10;
    else if(rank == 'K') card = 11;
    else if(rank == 'A') card = 12;
    else if(rank == 'X') card = 13;
    else card = rank - '2';

    card <<= 2;

    if(suit == 'c') card += 3;
    else if(suit == 'd') card += 2;
    else if(suit == 'h') card += 1;

    return card + 1;
}

inline std::string cards_to_str(const int* cards, size_t size) {
    static const char* ranks = "23456789TJQKAX";
    static const char* suits = "shdcx";
    std::string        result;
    result.reserve(size * 2);
    for (size_t i = 0; i < size; i++) {
        result.push_back(ranks[(int)((cards[i]-1) / 4)]);
        result.push_back(suits[(int)((cards[i]-1) % 4)]);
    }
    return result;
}

inline int get_kev_rank(int kev_card) {
    return (kev_card >> 8) & 0xF;
}

//  +--------+--------+--------+--------+
//  |xxxbbbbb|bbbbbbbb|cdhsrrrr|xxpppppp|
//  +--------+--------+--------+--------+
//  p = prime number of rank (deuce=2,trey=3,four=5,five=7,...,ace=41)
//  r = rank of card (deuce=0,trey=1,four=2,five=3,...,ace=12)
//  cdhs = suit of card
//  b = bit turned on depending on rank of card
//
//  http://suffe.cool/poker/evaluator.html
inline int to_kev(int rank, int suit) {
    return primes[rank] | (rank << 8) | (1 << (suit + 12)) | (1 << (16 + rank));
}

inline int to_kev(int card) {
    int rank = card / 4;
    int suit = card % 4;
    return to_kev(rank, suit);
}

inline std::vector<int> str_to_cards(const std::string& str) {
    std::vector<int> result;
    result.reserve(str.length()/2);
    for(int i = 0; i < str.length(); i+=2) {
        result.push_back(to_card(str[i], str[i+1]));
    }
    return result;
}

inline std::string kev_to_str(const int* hand, int size) {
    static const char* ranks = "23456789TJQKAX";
    static const char* suits = "shdcx";
    std::string        result;
    result.reserve(size * 2);
    for (size_t i = 0; i < size; i++) {
        int rank = hand[i] >> 8 & 0x0F;
        int suit = hand[i] >> 12 & 0x0F;
        if((suit & 0b1111) == 0b1111)
            suit = 4;
        else if((suit & 0b0001) == 0b0001)
            suit = 0;
        else if((suit & 0b0010) == 0b0010)
            suit = 1;
        else if((suit & 0b0100) == 0b0100)
            suit = 2;
        else
            suit = 3;

        result.push_back(ranks[rank]);
        result.push_back(suits[suit]);
    }
    return result;
}

inline std::string kev_to_str(const std::vector<int>& hand) {
    return kev_to_str(&hand[0], hand.size());
}

inline std::vector<int> str_to_kev(const std::string& kev_str) {
    std::vector<int> result;
    result.reserve(kev_str.length()/2);
    for(int i = 0; i < kev_str.length(); i+=2) {
        result.push_back(to_kev(to_card(kev_str[i], kev_str[i+1])-1));
    }
    return result;
}

inline int convert_kev_rank(int holdrank) {
    int result = 7463 - holdrank; // now the worst hand = 1
    if (result < 1278)
        result = result - 0 + 4096 * 1; // 1277 high card
    else if (result < 4138)
        result = result - 1277 + 4096 * 2; // 2860 one pair
    else if (result < 4996)
        result = result - 4137 + 4096 * 3; // 858 two pair
    else if (result < 5854)
        result = result - 4995 + 4096 * 4; // 858 three-kind
    else if (result < 5864)
        result = result - 5853 + 4096 * 5; // 10 straights
    else if (result < 7141)
        result = result - 5863 + 4096 * 6; // 1277 flushes
    else if (result < 7297)
        result = result - 7140 + 4096 * 7; // 156 full house
    else if (result < 7453)
        result = result - 7296 + 4096 * 8; // 156 four-kind
    else
        result = result - 7452 + 4096 * 9; // 10 str.flushes
    return result;
}

uint64_t pack_to_id(int* c) {
    return (uint64_t)c[0]
        + ((uint64_t)c[1] << 8)
        + ((uint64_t)c[2] << 16)
        + ((uint64_t)c[3] << 24)
        + ((uint64_t)c[4] << 32)
        + ((uint64_t)c[5] << 40)
        + ((uint64_t)c[6] << 48);
}

inline int operator"" _c(const char* card, size_t size) {
    assert(size == 2);
    return to_card(card[0], card[1]);
}

template <std::size_t N> using Cards = int[N];

void generate(const std::string& file_name);

void init() __attribute__((constructor));
void fini() __attribute__((destructor));
//void init();
//void fini();

void        init_deck(int* deck);
int         find_card(int rank, int suit, int* deck);
int         hand_rank(short val);
std::string print_hand(int* hand, int n);
void        shuffle_deck(int* deck, int size);
int         eval_5cards(int c1, int c2, int c3, int c4, int c5);
int         eval_5hand(const int* hand);
int         eval_6hand(const int* hand);
int         eval_7hand(const int* hand);
int         standard_lookup(const int* cards, int size);
int         lookup(const int* cards, int size);
inline Hand to_hand(int result) { return static_cast<Hand>(result >> 12); }

int64_t     make_id(int64_t IDin, int newcard, int& numcards, bool with_joker=false, const Debug& debug = nodebug);
int         save_id(int64_t ID, std::vector<int64_t>& IDs, int64_t& maxID, int& numIDs);
int         do_eval(int64_t IDin, int numcards, const Debug& debug = nodebug);
int         do_joker_eval(int64_t IDin, int numcards, const Debug& debug = nodebug);

inline int eval_hand(const std::vector<int>& hand) {
    switch(hand.size()) {
        case 5:
            return eval_5hand(&hand[0]);
        case 6:
            return eval_6hand(&hand[0]);
        case 7:
            return eval_7hand(&hand[0]);
        default:
            throw Error("Bad hand size: " + std::to_string(hand.size()));
    }
}

inline bool skip_duplicated(int* wk, int size) {
    for (int i = 0; i < size; ++i) {
        for (int j = i + 1; j < size; ++j) {
            if (wk[i] == wk[j]) {
                return true;
            }
        }
    }
    return false;
}

inline int mutate5(int* wk, int* jokers, int joker=0, bool verbose=false) {
    int best = 0;
    for(int c=1; c < 53; ++c) {
        if(jokers[joker+1])
            best = std::max(mutate5(wk, jokers, joker+1, verbose), best);
        wk[jokers[joker]] = c;
        if(skip_duplicated(wk, 5)) {
            if(verbose) {
                std::string comb = cards_to_str(wk, 5);
                fprintf(stdout, "skip best (%d) j:%d %d [%.*s]\n", best, joker, jokers[joker], (int)comb.length(), comb.data());
            }
            continue;
        }
        int hand = standard_lookup(wk, 5);

        if(verbose) {
            std::string comb = cards_to_str(wk, 5);
            fprintf(stdout, "hand/best (%d/%d) j:%d %d [%.*s]\n", hand, best, joker, jokers[joker], (int)comb.length(), comb.data());
        }

        assert(hand);

        best = std::max(hand, best);
    }
    //_PDEBUG("res: %d", best);
    return best;
}

inline int mutate6(int* wk, int* jokers, int joker=0, bool verbose=false) {
    int best = 0;
    for(int c=1; c < 53; ++c) {
        if(jokers[joker+1])
            best = std::max(mutate6(wk, jokers, joker+1, verbose), best);
        wk[jokers[joker]] = c;
        if(skip_duplicated(wk, 6)) {
            if(verbose) {
                std::string comb = cards_to_str(wk, 6);
                fprintf(stdout, "skip best (%d) j:%d %d [%.*s]\n", best, joker, jokers[joker], (int)comb.length(), comb.data());
            }
            continue;
        }
        int hand = standard_lookup(wk, 6);

        if(verbose) {
            std::string comb = cards_to_str(wk, 6);
            fprintf(stdout, "hand/best (%d/%d) j:%d %d [%.*s]\n", hand, best, joker, jokers[joker], (int)comb.length(), comb.data());
        }

        assert(hand);

        best = std::max(hand, best);
    }
    //_PDEBUG("res: %d", best);
    return best;
}

inline int mutate7(int* wk, int* jokers, int joker=0, bool verbose=false) {
    int best = 0;
    for(int c=1; c < 53; ++c) {
        if(jokers[joker+1])
            best = std::max(mutate7(wk, jokers, joker+1, verbose), best);
        wk[jokers[joker]] = c;
        if(skip_duplicated(wk, 7)) {
            if(verbose) {
                std::string comb = cards_to_str(wk, 7);
                fprintf(stdout, "skip best (%d) j:%d %d [%.*s]\n", best, joker, jokers[joker], (int)comb.length(), comb.data());
            }
            continue;
        }
        int hand = standard_lookup(wk, 7);

        if(verbose) {
            std::string comb = cards_to_str(wk, 7);
            fprintf(stdout, "hand/best (%d/%d) j:%d %d [%.*s]\n", hand, best, joker, jokers[joker], (int)comb.length(), comb.data());
        }

        assert(hand);

        best = std::max(hand, best);
    }
    //_PDEBUG("res: %d", best);
    return best;
}

//inline int mutate5(int* wk, int* jokers, int joker=0, bool verbose=false) {
    ////int best = 9999;
    //int best = 0;
    //for(int c=1; c < 52; ++c) {
        //if(jokers[joker+1])
            ////best = std::min(mutate5(wk, jokers, joker+1, verbose), best);
            //best = std::max(mutate5(wk, jokers, joker+1, verbose), best);
        //int kev = to_kev(c);
        //wk[jokers[joker]] = kev;
        //if(skip_duplicated(wk, 5)) {
            //if(verbose) {
                //std::string comb = kev_to_str(wk, 5);
                //fprintf(stdout, "skip best (%d) j:%d %d [%.*s]\n", best, joker, jokers[joker], (int)comb.length(), comb.data());
            //}
            //continue;
        //}
        //int hand = standard_lookup(wk, 5);
        ////int hand = eval_5hand(wk);

        //if(verbose) {
            //std::string comb = kev_to_str(wk, 5);
            //fprintf(stdout, "hand/best (%d/%d) j:%d %d [%.*s]\n", hand, best, joker, jokers[joker], (int)comb.length(), comb.data());
        //}

        //assert(hand);

        ////best = std::min(hand, best);
        //best = std::max(hand, best);
    //}
    ////_PDEBUG("res: %d", best);
    //return best;
//}

//inline int mutate6(int* wk, int* jokers, int joker=0, bool verbose=false) {
    //int best = 9999;
    //for(int c=1; c < 52; ++c) {
        //if(jokers[joker+1])
            //best = std::min(mutate6(wk, jokers, joker+1, verbose), best);
        //int kev = to_kev(c);
        //wk[jokers[joker]] = kev;
        //if(skip_duplicated(wk, 6)) {
            //if(verbose) {
                //std::string comb = kev_to_str(wk, 6);
                //fprintf(stdout, "skip best (%d) j:%d %d [%.*s]\n", best, joker, jokers[joker], (int)comb.length(), comb.data());
            //}
            //continue;
        //}
        //int hand = eval_6hand(wk);

        //if(verbose) {
            //std::string comb = kev_to_str(wk, 6);
            //fprintf(stdout, "hand/best (%d/%d) j:%d %d [%.*s]\n", hand, best, joker, jokers[joker], (int)comb.length(), comb.data());
        //}

        //assert(hand);

        //best = std::min(hand, best);
    //}
    ////_PDEBUG("res: %d", best);
    //return best;
//}

//inline int mutate7(int* wk, int* jokers, int joker=0, bool verbose=false) {
    //int best = 9999;
    //for(int c=1; c < 52; ++c) {
        //if(jokers[joker+1])
            //best = std::min(mutate7(wk, jokers, joker+1, verbose), best);
        //int kev = to_kev(c);
        //wk[jokers[joker]] = kev;
        //if(skip_duplicated(wk, 7)) {
            //if(verbose) {
                //std::string comb = kev_to_str(wk, 7);
                //fprintf(stdout, "skip best (%d) j:%d %d [%.*s]\n", best, joker, jokers[joker], (int)comb.length(), comb.data());
            //}
            //continue;
        //}
        //int hand = eval_7hand(wk);

        //if(verbose) {
            //std::string comb = kev_to_str(wk, 7);
            //fprintf(stdout, "hand/best (%d/%d) j:%d %d [%.*s]\n", hand, best, joker, jokers[joker], (int)comb.length(), comb.data());
        //}

        //assert(hand);

        //best = std::min(hand, best);
    //}
    ////_PDEBUG("res: %d", best);
    //return best;
//}

inline std::string dump_hand(const std::vector<int>& hand) {
    static const char* ranks = "23456789TJQKAX";
    static const char* suits = "shdcx";
    std::string        result;
    result.reserve(hand.size() * 2);
    for (size_t i = 0; i < hand.size(); i++) {
        result.push_back(ranks[(int)((hand[i]-1) / 4)]);
        result.push_back(suits[(int)((hand[i]-1) % 4)]);
    }
    return result;
}

// Sort Using XOR. Network for N=7, using Bose-Nelson Algorithm, Thanks to the thread!
// http://pages.ripco.net/~jgamble/nw.html
static inline void bose_nelson_sort_7(int* wk) {
#define SWAP(i,j) if (wk[i] < wk[j]) { wk[i] ^= wk[j]; wk[j] ^= wk[i]; wk[i] ^= wk[j]; }
    SWAP(0, 4);
    SWAP(1, 5);
    SWAP(2, 6);
    SWAP(0, 2);
    SWAP(1, 3);
    SWAP(4, 6);
    SWAP(2, 4);
    SWAP(3, 5);
    SWAP(0, 1);
    SWAP(2, 3);
    SWAP(4, 5);
    SWAP(1, 4);
    SWAP(3, 6);
    SWAP(1, 2);
    SWAP(3, 4);
    SWAP(5, 6);
#undef SWAP
}

// permutations 5 out of 6
// for x in itertools.combinations(range(0, 6), 5): print x
int perm6[6][5] = {
  {0, 1, 2, 3, 4},
  {0, 1, 2, 3, 5},
  {0, 1, 2, 4, 5},
  {0, 1, 3, 4, 5},
  {0, 2, 3, 4, 5},
  {1, 2, 3, 4, 5}
};

// permutations 5 out of 7
// for x in itertools.combinations(range(0, 7), 5): print x
int perm7[21][5] = {
  { 0, 1, 2, 3, 4 },
  { 0, 1, 2, 3, 5 },
  { 0, 1, 2, 3, 6 },
  { 0, 1, 2, 4, 5 },
  { 0, 1, 2, 4, 6 },
  { 0, 1, 2, 5, 6 },
  { 0, 1, 3, 4, 5 },
  { 0, 1, 3, 4, 6 },
  { 0, 1, 3, 5, 6 },
  { 0, 1, 4, 5, 6 },
  { 0, 2, 3, 4, 5 },
  { 0, 2, 3, 4, 6 },
  { 0, 2, 3, 5, 6 },
  { 0, 2, 4, 5, 6 },
  { 0, 3, 4, 5, 6 },
  { 1, 2, 3, 4, 5 },
  { 1, 2, 3, 4, 6 },
  { 1, 2, 3, 5, 6 },
  { 1, 2, 4, 5, 6 },
  { 1, 3, 4, 5, 6 },
  { 2, 3, 4, 5, 6 }
};

} // namespace pokerlib

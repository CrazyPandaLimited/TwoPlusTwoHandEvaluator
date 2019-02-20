// Paul D. Senzee's Optimized Hand Evaluator for Cactus Kev's Poker Hand Evaluator
//
// Replaces binary search with a perfect hash.
//
// (c) Paul D. Senzee psenzee@yahoo.com
// Portions (in eval_5hand_fast) (c) Kevin L. Suffecool.
//
// Senzee 5
// http://senzee.blogspot.com
//
// Cactus Kev Poker Hand Evaluator
// http://suffe.cool/poker/evaluator.html

#include <ctime>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <type_traits>

#include "lookup_tables.hpp"
#include "pokerlib.hpp"

//#define FIVE_OF_A_KIND  0
#define STRAIGHT_FLUSH  1
#define FOUR_OF_A_KIND  2
#define FULL_HOUSE      3
#define FLUSH           4
#define STRAIGHT        5
#define THREE_OF_A_KIND 6
#define TWO_PAIR        7
#define ONE_PAIR        8
#define HIGH_CARD       9

#define RANK(x) ((x >> 8) & 0xF)

namespace pokerlib {

extern unsigned short hash_adjust[];
extern unsigned short hash_values[];

const char HandRanks[][16] = {
    "Bad",             // 0
    "High Card",       // 1
    "Pair",            // 2
    "Two Pair",        // 3
    "Three of a Kind", // 4
    "Straight",        // 5
    "Flush",           // 6
    "Full House",      // 7
    "Four of a Kind",  // 8
    "Straight Flush"   // 9
};

int     numIDs   = 1;
int     numcards = 0;
int     maxHR    = 0;
int64_t maxID    = 0;

inline void swap(int* wk, int i, int j) {
    if (wk[i] < wk[j]) {
        wk[i] ^= wk[j];
        wk[j] ^= wk[i];
        wk[i] ^= wk[j];
    }
}

// Sort Using XOR. Network for N=7, using Bose-Nelson Algorithm: Thanks to the thread!
inline void bose_nelson_sort_7(int* wk) {
    swap(wk, 0, 4);
    swap(wk, 1, 5);
    swap(wk, 2, 6);
    swap(wk, 0, 2);
    swap(wk, 1, 3);
    swap(wk, 4, 6);
    swap(wk, 2, 4);
    swap(wk, 3, 5);
    swap(wk, 0, 1);
    swap(wk, 2, 3);
    swap(wk, 4, 5);
    swap(wk, 1, 4);
    swap(wk, 3, 6);
    swap(wk, 1, 2);
    swap(wk, 3, 4);
    swap(wk, 5, 6);
}

// returns a 64-bit hand ID, for up to 8 cards, stored 1 per byte.
int64_t make_id(int64_t IDin, int newcard) {
    int suitcount[4 + 1];
    int rankcount[13 + 1];
    int wk[8]; // intentially keeping one as a 0 end
    int cardnum;
    int getout = 0;

    memset(wk, 0, sizeof(wk));
    memset(rankcount, 0, sizeof(rankcount));
    memset(suitcount, 0, sizeof(suitcount));

    // can't have more than 6 cards!
    for (cardnum = 0; cardnum < 6; cardnum++) {
        // leave the 0 hole for new card
        wk[cardnum + 1] = (int)((IDin >> (8 * cardnum)) & 0xFF);
    }

    // my cards are 2c = 1, 2d = 2  ... As = 52
    newcard--; // make 0 based!

    // add next card. formats card to rrrr00ss
    wk[0] = (((newcard >> 2) + 1) << 4) + (newcard & 3) + 1;

    for (numcards = 0; wk[numcards]; numcards++) {
        // need to see if suit is significant
        suitcount[wk[numcards] & 0xf]++;
        // and rank to be sure we don't have 4!
        rankcount[(wk[numcards] >> 4) & 0xF]++;
        if (numcards) {
            // can't have the same card twice
            // if so need to get out after counting numcards
            if (wk[0] == wk[numcards])
                getout = 1;
        }
    }
    if (getout)
        return 0; // duplicated another card (ignore this one)

    // for suit to be significant, need to have n-2 of same suit
    int needsuited = numcards - 2;
    if (numcards > 4) {
        for (int rank = 1; rank < 14; rank++) {
            // if I have more than 4 of a rank then I shouldn't do this one!!
            // can't have more than 4 of a rank so return an ID that can't be!
            if (rankcount[rank] > 4)
                return 0;
        }
    }

    // However in the ID process I prefered that
    // 2s = 0x21, 3s = 0x31,.... Kc = 0xD4, Ac = 0xE4
    // This allows me to sort in Rank then Suit order

    // if we don't have at least 2 cards of the same suit for 4,
    // we make this card suit 0.
    if (needsuited > 1) {
        for (cardnum = 0; cardnum < numcards; cardnum++) { // for each card
            if (suitcount[wk[cardnum] & 0xF] < needsuited) {
                // check suitcount to the number I need to have suits significant
                // if not enough - 0 out the suit - now this suit would be a 0 vs 1-4
                wk[cardnum] &= 0xF0;
            }
        }
    }

    bose_nelson_sort_7(wk);

    // long winded way to put the pieces into a int64_t cards in bytes --66554433221100
    // the resulting ID is a 64 bit value with each card represented by 8 bits.
    return (int64_t)wk[0]
        + ((int64_t)wk[1] << 8)
        + ((int64_t)wk[2] << 16)
        + ((int64_t)wk[3] << 24)
        + ((int64_t)wk[4] << 32)
        + ((int64_t)wk[5] << 40)
        + ((int64_t)wk[6] << 48);
}

// this inserts a hand ID into the IDs array.
int save_id(int64_t ID) {
    if (ID == 0) {
        // don't use up a record for a 0!
        return 0;
    }

    // take care of the most likely first goes on the end...
    if (ID >= maxID) {
        if (ID > maxID) {       // greater than create new else it was the last one!
            IDs[numIDs++] = ID; // add the new ID
            maxID         = ID;
        }
        return numIDs - 1;
    }

    // find the slot (by a pseudo bsearch algorithm)
    int     low  = 0;
    int     high = numIDs - 1;
    int64_t testval;
    int     holdtest;

    while (high - low > 1) {
        holdtest = (high + low + 1) / 2;
        testval  = IDs[holdtest] - ID;
        if (testval > 0)
            high = holdtest;
        else if (testval < 0)
            low = holdtest;
        else
            return holdtest; // got it!!
    }
    // it couldn't be found so must be added to the current location (high)
    // make space...  // don't expect this much!
    memmove(&IDs[high + 1], &IDs[high], (numIDs - high) * sizeof(IDs[0]));

    IDs[high] = ID; // do the insert into the hole created
    numIDs++;
    return high;
}

// Converts a 64bit handID to an absolute ranking.
// I guess I have some explaining to do here...
// I used the Cactus Kevs Eval http://suffe.cool/poker/evaluator.html
// I Love the pokersource for speed, but I needed to do some tweaking to get it my way and Cactus Kevs stuff was easy to tweak ;-)
int do_eval(int64_t IDin) {
    int result = 0;
    int cardnum;
    int wkcard;
    int rank;
    int suit;
    int mainsuit = 20; // just something that will never hit...

    // ODO: need to eliminate the main suit from the iterator
    // int suititerator = 0;

    // changed as per Ray Wotton's comment at http://archives1.twoplustwo.com/showflat.php?Cat=0&Number=8513906&page=0&fpart=18&vc=1
    int suititerator = 1;
    int holdrank;
    int wk[8]; // "work" intentially keeping one as a 0 end
    int holdcards[8];
    int numevalcards = 0;

    // See Cactus Kevs page for explainations for this type of stuff...
    const int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41};

    memset(wk, 0, sizeof(wk));
    memset(holdcards, 0, sizeof(holdcards));

    // if I have a good ID then do it...
    if (IDin) {
        // convert all 7 cards (0s are ok)
        for (cardnum = 0; cardnum < 7; cardnum++) {
            holdcards[cardnum] = (int)((IDin >> (8 * cardnum)) & 0xFF);

            // once I hit a 0 I know I am done
            if (holdcards[cardnum] == 0)
                break;
            numevalcards++;

            // if not 0 then count the card
            if ((suit = holdcards[cardnum] & 0xF)) {
                // find out what suit (if any) was significant and remember it
                mainsuit = suit;
            }
        }

        for (cardnum = 0; cardnum < numevalcards; cardnum++) {
            // just have numcards...
            wkcard = holdcards[cardnum];

            // convert to cactus kevs way!!
            // http://suffe.cool/poker/evaluator.html
            //   +--------+--------+--------+--------+
            //   |xxxbbbbb|bbbbbbbb|cdhsrrrr|xxpppppp|
            //   +--------+--------+--------+--------+
            //   p = prime number of rank (deuce=2,trey=3,four=5,five=7,...,ace=41)
            //   r = rank of card (deuce=0,trey=1,four=2,five=3,...,ace=12)
            //   cdhs = suit of card
            //   b = bit turned on depending on rank of card

            rank = (wkcard >> 4) - 1; // my rank is top 4 bits 1-13 so convert
            suit = wkcard & 0xF;      // my suit is bottom 4 bits 1-4, order is different, but who cares?
            if (suit == 0) {
                // if suit wasn't significant though...
                suit = suititerator++; // Cactus Kev needs a suit!
                if (suititerator == 5) // loop through available suits
                    suititerator = 1;
                if (suit == mainsuit) {    // if it was the sigificant suit...  Don't want extras!!
                    suit = suititerator++; // skip it
                    if (suititerator == 5) // roll 1-4
                        suititerator = 1;
                }
            }
            // now make Cactus Kev's Card
            wk[cardnum] = primes[rank] | (rank << 8) | (1 << (suit + 11)) | (1 << (16 + rank));
        }

        // James Devlin: replaced all calls to Cactus Kev's eval_5cards with calls to Senzee's improved eval_5hand_fast
        // run Cactus Kev's routines
        switch (numevalcards) {
            case 5:
                holdrank = eval_5hand_fast(wk[0], wk[1], wk[2], wk[3], wk[4]);
                break;
            case 6:
                // if 6 cards I would like to find Result for them
                // Cactus Key is 1 = highest - 7362 lowest
                // I need to get the min for the permutations
                holdrank = eval_5hand_fast(wk[0], wk[1], wk[2], wk[3], wk[4]);
                holdrank = std::min(holdrank, eval_5hand_fast(wk[0], wk[1], wk[2], wk[3], wk[5]));
                holdrank = std::min(holdrank, eval_5hand_fast(wk[0], wk[1], wk[2], wk[4], wk[5]));
                holdrank = std::min(holdrank, eval_5hand_fast(wk[0], wk[1], wk[3], wk[4], wk[5]));
                holdrank = std::min(holdrank, eval_5hand_fast(wk[0], wk[2], wk[3], wk[4], wk[5]));
                holdrank = std::min(holdrank, eval_5hand_fast(wk[1], wk[2], wk[3], wk[4], wk[5]));
                break;
            case 7:
                holdrank = eval_7hand(wk);
                break;
            default:
                // problem!!  shouldn't hit this...
                _PDEBUG("    Problem with numcards = %d!!", numcards);
                break;
        }

        // I would like to change the format of Catus Kev's ret value to:
        // hhhhrrrrrrrrrrrr   hhhh = 1 high card -> 9 straight flush
        //                    r..r = rank within the above	1 to max of 2861
        result = 7463 - holdrank; // now the worst hand = 1

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
    }
    return result; // now a handrank that I like
}

void init() {
    int     IDslot, card = 0, count = 0;
    int64_t ID;

    clock_t timer = clock(); // remember when I started

    // Store the count of each type of hand (One Pair, Flush, etc)
    int handTypeSum[10];

    // Clear our arrays
    memset(handTypeSum, 0, sizeof(handTypeSum));
    memset(IDs, 0, sizeof(IDs));
    memset(HR, 0, sizeof(HR));

    // step through the ID array - always shifting the current ID and
    // adding 52 cards to the end of the array.
    // when I am at 7 cards put the Hand Rank in!!
    // stepping through the ID array is perfect!!

    int IDnum;
    int holdid;

    _PDEBUG("Getting Card IDs!");

    // Jmd: Okay, this loop is going to fill up the IDs[] array which has
    // 612,967 slots. as this loops through and find new combinations it
    // adds them to the end. I need this list to be stable when I set the
    // handranks (next set)  (I do the insertion sort on new IDs these)
    // so I had to get the IDs first and then set the handranks
    for (IDnum = 0; IDs[IDnum] || IDnum == 0; IDnum++) {
        // start at 1 so I have a zero catching entry (just in case)
        for (card = 1; card < 53; card++) {
            // the ids above contain cards upto the current card.  Now add a new card
            ID = make_id(IDs[IDnum], card); // get the new ID for it
            // and save it in the list if I am not on the 7th card
            if (numcards < 7)
                holdid = save_id(ID);
        }
        _PPRINT("\rID - %d", IDnum); // show progress -- this counts up to 612976
    }

    _PDEBUG("\nSetting HandRanks!");

    // this is as above, but will not add anything to the ID list, so it is stable
    for (IDnum = 0; IDs[IDnum] || IDnum == 0; IDnum++) {
        // start at 1 so I have a zero catching entry (just in case)
        for (card = 1; card < 53; card++) {
            ID = make_id(IDs[IDnum], card);

            if (numcards < 7) {
                // when in the index mode (< 7 cards) get the id to save
                IDslot = save_id(ID) * 53 + 53;
            } else {
                // if I am at the 7th card, get the equivalence class ("hand rank") to save
                IDslot = do_eval(ID);
            }

            maxHR     = IDnum * 53 + card + 53; // find where to put it
            HR[maxHR] = IDslot;                 // and save the pointer to the next card or the handrank
        }

        if (numcards == 6 || numcards == 7) {
            // an extra, If you want to know what the handrank when there is 5 or 6 cards
            // you can just do HR[u3] or HR[u4] from below code for Handrank of the 5 or
            // 6 card hand
            // this puts the above handrank into the array
            HR[IDnum * 53 + 53] = do_eval(IDs[IDnum]);
        }

        _PPRINT("\rID - %d", IDnum); // show the progress -- counts to 612976 again
    }

    _PDEBUG("\nNumber IDs = %d\nmaxHR = %d", numIDs, maxHR); // for warm fuzzys

    timer = clock() - timer; // end the timer

    _PDEBUG("Training seconds = %.2f", (float)timer / CLOCKS_PER_SEC);

    timer = clock(); // now get current time for Testing!

    // another algorithm right off the thread

    int c0, c1, c2, c3, c4, c5, c6;
    int u0, u1, u2, u3, u4, u5;

    for (c0 = 1; c0 < 53; c0++) {
        u0 = HR[53 + c0];
        for (c1 = c0 + 1; c1 < 53; c1++) {
            u1 = HR[u0 + c1];
            for (c2 = c1 + 1; c2 < 53; c2++) {
                u2 = HR[u1 + c2];
                for (c3 = c2 + 1; c3 < 53; c3++) {
                    u3 = HR[u2 + c3];
                    for (c4 = c3 + 1; c4 < 53; c4++) {
                        u4 = HR[u3 + c4];
                        for (c5 = c4 + 1; c5 < 53; c5++) {
                            u5 = HR[u4 + c5];
                            for (c6 = c5 + 1; c6 < 53; c6++) {
                                handTypeSum[HR[u5 + c6] >> 12]++;
                                count++;
                            }
                        }
                    }
                }
            }
        }
    }

    timer = clock() - timer; // get the time in this

    for (int i = 0; i <= 9; i++) // display the results
        _PDEBUG("%16s = %d", HandRanks[i], handTypeSum[i]);

    _PDEBUG("Total Hands = %d", count);
}

//   This routine initializes the deck.  A deck of cards is
//   simply an integer array of length 52 (no jokers).  This
//   array is populated with each card, using the following
//   scheme:
//
//   An integer is made up of four bytes.  The high-order
//   bytes are used to hold the rank bit pattern, whereas
//   the low-order bytes hold the suit/rank/prime value
//   of the card.
//
//   +--------+--------+--------+--------+
//   |xxxbbbbb|bbbbbbbb|cdhsrrrr|xxpppppp|
//   +--------+--------+--------+--------+
//
//   p = prime number of rank (deuce=2,trey=3,four=5,five=7,...,ace=41)
//   r = rank of card (deuce=0,trey=1,four=2,five=3,...,ace=12)
//   cdhs = suit of card
//   b = bit turned on depending on rank of card
void init_deck(int* deck) {
    int n = 0, suit = 0x8000;
    for (int i = 0; i < 4; i++, suit >>= 1)
        for (int j = 0; j < 13; j++, n++)
            deck[n] = primes[j] | (j << 8) | suit | (1 << (16 + j));
}

//  This routine will search a deck for a specific card
//  (specified by rank/suit), and return the INDEX giving
//  the position of the found card.  If it is not found,
//  then it returns -1
int find_card(int rank, int suit, int* deck) {
    for (int i = 0; i < 52; i++) {
        int c = deck[i];
        if ((c & suit) && (RANK(c) == rank))
            return i;
    }
    return -1;
}

//  This routine takes a deck and randomly mixes up the order of the cards.
void shuffle_deck(int* deck, int size) {
    std::random_shuffle(deck, deck + size);
}

// Prints poker hand, cards should be a pointer to an array
// of integers each with value between 1 and 52 inclusive.
std::string dump_cards(int* hand, int size) {
    static const char* ranks = "23456789TJQKA";
    static const char* suits = "cdhs";
    std::string result;
    result.reserve(size*2);
    for (int i = 0; i < size; i++, hand++) {
        _PDEBUG("card: %d - rank: %d suit: %d", *hand, *hand % 13, *hand / 13);
        result.push_back(ranks[(int)(*hand % 13)]);
        result.push_back(suits[(int)(*hand / 13)]);
    }
    return result;
}

std::string print_hand(int* hand, int size) {
    static const char* ranks = "23456789TJQKA";
    std::string result;
    result.reserve(size*2);
    for (int i = 0; i < size; i++, hand++) {
        char suit;
        if (*hand & 0x8000)
            suit = 'c';
        else if (*hand & 0x4000)
            suit = 'd';
        else if (*hand & 0x2000)
            suit = 'h';
        else
            suit = 's';

        _PDEBUG("card: %X - rank: %d suit: %d", *hand, ranks[(*hand >> 8) & 0xF], suit);
        result.push_back(ranks[(*hand >> 8) & 0xF]);
        result.push_back(suit);
    }
    return result;
}

int hand_rank(short val) {
    if (val > 6185) return HIGH_CARD; // 1277 high card
    if (val > 3325) return ONE_PAIR; // 2860 one pair
    if (val > 2467) return TWO_PAIR; // 858 two pair
    if (val > 1609) return THREE_OF_A_KIND; // 858 three-kind
    if (val > 1599) return STRAIGHT; // 10 straights
    if (val > 322) return FLUSH; // 1277 flushes
    if (val > 166) return FULL_HOUSE; // 156 full house
    if (val > 10) return FOUR_OF_A_KIND; // 156 four-kind
    return STRAIGHT_FLUSH; // 10 straight-flushes
}

unsigned find_fast(unsigned u) {
    unsigned a, b, r;
    u += 0xE91AAA35;
    u ^= u >> 16;
    u += u << 8;
    u ^= u >> 4;
    b = (u >> 8) & 0x1FF;
    a = (u + (u << 2)) >> 19;
    r = a ^ hash_adjust[b];
    return r;
}

int eval_5hand_fast(int c1, int c2, int c3, int c4, int c5) {
    int q = (c1 | c2 | c3 | c4 | c5) >> 16;

    // check for flushes and straight flushes
    if (c1 & c2 & c3 & c4 & c5 & 0xF000)
        return flushes[q];

    // check for straights and high card hands
    short s;
    if ((s = unique5[q]))
        return s;

    return hash_values[find_fast((c1 & 0xFF) * (c2 & 0xFF) * (c3 & 0xFF) * (c4 & 0xFF) * (c5 & 0xFF))];
}

short eval_5hand(int* hand) {
    int c1 = *hand++;
    int c2 = *hand++;
    int c3 = *hand++;
    int c4 = *hand++;
    int c5 = *hand;
    return eval_5hand_fast(c1, c2, c3, c4, c5);
}

// This is a non-optimized method of determining the best five-card hand possible out of seven cards.
short eval_7hand(int* hand) {
    int best = 9999, subhand[5];
    for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 5; j++)
            subhand[j] = hand[perm7[i][j]];
        int q = eval_5hand(subhand);
        if (q < best)
            best = q;
    }
    return best;
}

// Lookup of a 7-card poker hand, cards should be a pointer to an array
// of 7 integers each with value between 1 and 52 inclusive.
int lookup_7hand(int* cards) {
    int p;
    p = HR[1 + 53 + *cards++];
    p = HR[1 + p + *cards++];
    p = HR[1 + p + *cards++];
    p = HR[1 + p + *cards++];
    p = HR[1 + p + *cards++];
    p = HR[1 + p + *cards++];
    p = HR[1 + p + *cards++];
    return p;
}

} // namespace pokerlib

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
//
// John M. Gamble Sorting Networks
// http://pages.ripco.net/~jgamble/nw.html


#include <set>
#include <vector>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include <atomic>

#include <sys/mman.h>
#include <sys/stat.h>

#include "lookup_tables.hpp"
#include "pokerlib.hpp"

#include <tbb/tbb.h>

#define RANK(x) ((x >> 8) & 0xF)

namespace pokerlib {

extern unsigned short hash_adjust[];
extern unsigned short hash_values[];

const int* get_table() {
    return reinterpret_cast<const int*>(ranks_map.data());
}

// returns a 64-bit hand ID, for up to 8 cards, stored 1 per byte.
int64_t make_id(int64_t IDin, int newcard, int &numcards, bool with_joker, const Debug& debug) {
    int suitcount[SUITS_COUNT + 1] = {};
    int rankcount[JOKER_RANKS_COUNT + 1] = {};
    int wk[8] = {}; // intentially keeping one as a 0 end
    int cardnum;
    int getout = 0;
    int jokercount = 0;

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
        int suit = wk[numcards] & 0xF;
        int rank = wk[numcards] >> 4 & 0xF;

        if (rank == 14) {
            // is a joker
            jokercount++;
        }

        // need to see if suit is significant
        suitcount[suit]++;
        // and rank to be sure we don't have 4!
        rankcount[rank]++;

        if (numcards) {
            // can't have the same card twice
            // if so need to get out after counting numcards
            if (wk[0] == wk[numcards])
                getout = 1;
        }
    }
    // duplicated another card (ignore this one)
    if (getout)
        return 0;

    if (numcards > 4) {
        for (int rank = 1; rank < RANKS_COUNT + 1 + (int)with_joker; rank++) {
            // if I have more than 4 of a rank then I shouldn't do this one!!
            // can't have more than 4 of a rank so return an ID that can't be!
            if (rankcount[rank] > 4) {
                //_PDEBUG("r:%d c:%d", rank, rankcount[rank]);
                return 0;
            }
        }
    }

    // However in the ID process I prefered that
    // 2s = 0x21, 3s = 0x31,.... Kc = 0xD4, Ac = 0xE4
    // This allows me to sort in Rank then Suit order

    // for suit to be significant, need to have n-2 of same suit
    int needsuited = numcards - 2;

    // if we don't have at least 2 cards of the same suit for 4,
    // we make this card suit 0.
    if (needsuited > 1) {
        for (cardnum = 0; cardnum < numcards; cardnum++) { // for each card
            if ((suitcount[wk[cardnum] & 0xF] + jokercount < needsuited)
                    || ((wk[cardnum] >> 4 & 0xF) == 14)
                    || jokercount == 4) {
                // check suitcount to the number I need to have suits significant
                // if not enough - 0 out the suit - now this suit would be a 0 vs 1-4
                // SA: I don't need suits if I have 4 jokers or if this card is a joker
                wk[cardnum] &= 0xF0;
            }
        }
    }

    bose_nelson_sort_7(wk);

    if(debug.on && numcards == debug.cardnum) {
        if(debug.id_callback(wk, numcards, jokercount)) {
            return -1;
        }
    }

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
int save_id(int64_t ID, std::vector<int64_t>& IDs, int64_t& maxID, int& numIDs) {
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

int eval(int* wk, int numevalcards, int numcards) {
    switch (numevalcards) {
        case 5:
            return eval_5hand(wk);
        case 6:
            return eval_6hand(wk);
        case 7:
            return eval_7hand(wk);
        default:
            throw Error("Problem with numcards = " + std::to_string(numcards));
    }
}

// Converts a 64bit handID to an absolute ranking.
// I guess I have some explaining to do here...
// I used the Cactus Kevs Eval http://suffe.cool/poker/evaluator.html
// I Love the pokersource for speed, but I needed to do some tweaking to get it my way and Cactus Kevs stuff was easy to tweak ;-)
int do_eval(int64_t IDin, int numcards, const Debug& debug) {
    if (IDin == 0) {
        return 0;
    }

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
    int numevalcards = 0;
    int wk[8] = {}; // "work" intentially keeping one as a 0 end
    int holdcards[8] = {};

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

        if(rank == RANKS_COUNT) {
            return 1;
        }

        // now make Cactus Kev's Card
        wk[cardnum] = primes[rank] | (rank << 8) | (1 << (suit + 11)) | (1 << (16 + rank));
    }

    bool verbose = false;
    if(debug.on && debug.verbose && IDin == debug.id) {
        verbose = true;
    }

    switch (numevalcards) {
        case 5:
            result = eval_5hand(wk);
            break;
        case 6:
            result = eval_6hand(wk);
            break;
        case 7:
            result = eval_7hand(wk);
            break;
        default:
            throw Error("Problem with numcards = " + std::to_string(numcards));
    }

    if(debug.on && IDin == debug.id) {
        if(!debug.eval_callback(IDin, result)) {
            return -1;
        }
    }

    return convert_kev_rank(result);
}

// Converts a 64bit handID to an absolute ranking.
// I guess I have some explaining to do here...
// I used the Cactus Kevs Eval http://suffe.cool/poker/evaluator.html
// I Love the pokersource for speed, but I needed to do some tweaking to get it my way and Cactus Kevs stuff was easy to tweak ;-)
int do_joker_eval(int64_t IDin, int numcards, const Debug& debug) {
    if (IDin == 0) {
        return 0;
    }

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
    int numevalcards = 0;
    int jokercount = 0;
    int wk[8] = {}; // "work" intentially keeping one as a 0 end
    int holdcards[8] = {};
    int jokers[5] = {};
    int rankcount[JOKER_RANKS_COUNT + 1] = {};

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

        rankcount[(holdcards[cardnum] >> 4) - 1]++;
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

        if(rank == 13) {
            jokers[jokercount++] = cardnum;
            wk[cardnum] = jokercount;
            continue;
        }

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

        if(rank == RANKS_COUNT) {
            return 1;
        }

        wk[cardnum] = rank*4 + suit;
    }

    int mainrank = 0;
    int duprank = 0;
    int dupcount = 1;
    if(jokercount) {
        // count pairs, triples and quads to later use in five of a kind
        for(int i = 0; i < RANKS_COUNT; ++i) {
            if(rankcount[i] > dupcount) {
                dupcount = rankcount[i];
                duprank = i;
            }
            if(rankcount[i]) {
                mainrank = i;
            }
        }
    }

    if (dupcount + jokercount >= 5) {
        return mainrank + 4096 * 10; // 13 five of a kind
    }

    bool verbose = false;
    if(debug.on && debug.verbose && IDin == debug.id) {
        verbose = true;
    }

    if (jokercount) {
        switch (numevalcards) {
            case 5:
                result = mutate5(wk, jokers, 0, verbose);
                break;
            case 6:
                result = mutate6(wk, jokers, 0, verbose);
                break;
            case 7:
                result = mutate7(wk, jokers, 0, verbose);
                break;
            default:
                throw Error("Problem with numcards = " + std::to_string(numcards));
        }
    } else {
        result = standard_lookup(wk, numevalcards);
    }

    if(debug.on && IDin == debug.id) {
        if(!debug.eval_callback(IDin, result)) {
            return -1;
        }
    }

    return result;
}

void generate_standard(const std::string& file_name) {
    int     IDslot, card = 0;
    int64_t ID;

    clock_t timer = clock(); // remember when I started

    std::vector<int> HR(STANDARD_HAND_RANKS_COUNT);
    std::vector<int64_t> IDs(STANDARD_IDS_COUNT);

    int     numIDs   = 1;
    int     numcards = 0;
    int     maxHR    = 0;
    int64_t maxID    = 0;

    // step through the ID array - always shifting the current ID and
    // adding 52 cards to the end of the array.
    // when I am at 7 cards put the Hand Rank in!!
    // stepping through the ID array is perfect!!
    // SA: indeed

    int IDnum;

    _PDEBUG("Getting Card IDs!");

    // Jmd: Okay, this loop is going to fill up the IDs[] array which has
    // 612,967 slots. as this loops through and find new combinations it
    // adds them to the end. I need this list to be stable when I set the
    // handranks (next set)  (I do the insertion sort on new IDs these)
    // so I had to get the IDs first and then set the handranks
    // SA: will stop if there are no combinations left
    for (IDnum = 0; IDs[IDnum] || IDnum == 0; IDnum++) {
        // start at 1 so I have a zero catching entry (just in case)
        for (card = 1; card < STANDARD_DECK_SIZE + 1; card++) {
            // the ids above contain cards upto the current card.  Now add a new card
            ID = make_id(IDs[IDnum], card, numcards); // get the new ID for it
            //count_jokercombs(ID, jokercombs);
            // and save it in the list if I am not on the 7th card
            if (numcards < 7)
                save_id(ID, IDs, maxID, numIDs);
        }
        // show progress -- this counts up to 612976
        // SA: if there are jokers, counts up to ~1M
        _PPRINT("\rID - %d %llX %llX", IDnum, ID, IDs[IDnum+1]);
    }

    _PDEBUG("\nSetting HandRanks! %d", IDnum);

    // this is as above, but will not add anything to the ID list, so it is stable
    for (IDnum = 0; IDs[IDnum] || IDnum == 0; IDnum++) {
        // start at 1 so I have a zero catching entry (just in case)
        for (card = 1; card < STANDARD_DECK_SIZE + 1; card++) {
            ID = make_id(IDs[IDnum], card, numcards);

            if (numcards < 7) {
                // when in the index mode (< 7 cards) get the id to save
                IDslot = save_id(ID, IDs, maxID, numIDs) * (STANDARD_DECK_SIZE + 1) + STANDARD_DECK_SIZE + 1;
            }
            else {
                // if I am at the 7th card, get the equivalence class ("hand rank") to save
                IDslot = do_eval(ID, numcards);
            }

            // find where to put it
            maxHR     = IDnum * (STANDARD_DECK_SIZE + 1) + card + STANDARD_DECK_SIZE + 1;
            // and save the pointer to the next card or the handrank
            HR[maxHR] = IDslot;
            //_PDEBUG("ID0 - %d %d", IDnum, maxHR); // show the progress -- counts to 612976 again
        }

        if (numcards == 6 || numcards == 7) {
            // an extra, If you want to know what the handrank when there is 5 or 6 cards
            // you can just do HR[u3] or HR[u4] from below code for Handrank of the 5 or
            // 6 card hand
            // this puts the above handrank into the array
            HR[IDnum * (STANDARD_DECK_SIZE + 1) + STANDARD_DECK_SIZE + 1] = do_eval(IDs[IDnum], numcards);
            //_PDEBUG("ID6 - %d %d", IDnum, IDnum * (DECK_SIZE + 1) + DECK_SIZE + 1); // show the progress -- counts to 612976 again
        }

        _PPRINT("\rID - %d", IDnum); // show the progress -- counts to 612976 again
    }


    _PDEBUG("\nNumber IDs = %d\nmaxHR = %d", numIDs, maxHR); // for warm fuzzys

    timer = clock() - timer; // end the timer

    _PDEBUG("Training seconds = %.2f", (float)timer / CLOCKS_PER_SEC);

    FILE* fout = fopen(file_name.c_str(), "wb");
    if (!fout) {
        throw Error("Write to file failed: " + file_name);
    }
    std::fwrite(&HR[0], sizeof(int) * HR.size(), 1, fout);
    fclose(fout);
}

void generate(const std::string& file_name) {
    int     IDslot, card = 0;
    int64_t ID;

    clock_t timer = clock(); // remember when I started

    std::vector<int> HR(JOKER_HAND_RANKS_COUNT);
    std::vector<int64_t> IDs(JOKER_IDS_COUNT);

    int     numIDs   = 1;
    int     numcards = 0;
    int     maxHR    = 0;
    int64_t maxID    = 0;

    // step through the ID array - always shifting the current ID and
    // adding 52 cards to the end of the array.
    // when I am at 7 cards put the Hand Rank in!!
    // stepping through the ID array is perfect!!
    // SA: indeed

    int IDnum;

    _PDEBUG("Getting Card IDs!");

    // Jmd: Okay, this loop is going to fill up the IDs[] array which has
    // 612,967 slots. as this loops through and find new combinations it
    // adds them to the end. I need this list to be stable when I set the
    // handranks (next set)  (I do the insertion sort on new IDs these)
    // so I had to get the IDs first and then set the handranks
    // SA: will stop if there are no combinations left
    for (IDnum = 0; IDs[IDnum] || IDnum == 0; IDnum++) {
        // start at 1 so I have a zero catching entry (just in case)
        for (card = 1; card < JOKER_DECK_SIZE + 1; card++) {
            // the ids above contain cards upto the current card.  Now add a new card
            ID = make_id(IDs[IDnum], card, numcards, true); // get the new ID for it
            //count_jokercombs(ID, jokercombs);
            // and save it in the list if I am not on the 7th card
            if (numcards < 7)
                save_id(ID, IDs, maxID, numIDs);
        }
        // show progress -- this counts up to 612976
        // SA: if there are jokers, counts up to ~1M
        _PPRINT("\rID - %d %llX %llX", IDnum, ID, IDs[IDnum+1]);
    }

    _PDEBUG("\nSetting HandRanks! %d", IDnum);

    // this is as above, but will not add anything to the ID list, so it is stable
    for (IDnum = 0; IDs[IDnum] || IDnum == 0; IDnum++) {
        // start at 1 so I have a zero catching entry (just in case)
        for (card = 1; card < JOKER_DECK_SIZE + 1; card++) {
            ID = make_id(IDs[IDnum], card, numcards, true);

            if (numcards < 7) {
                // when in the index mode (< 7 cards) get the id to save
                IDslot = save_id(ID, IDs, maxID, numIDs) * (JOKER_DECK_SIZE + 1) + JOKER_DECK_SIZE + 1;
            }
            else {
                // if I am at the 7th card, get the equivalence class ("hand rank") to save
                IDslot = do_joker_eval(ID, numcards);
            }

            // find where to put it
            maxHR     = IDnum * (JOKER_DECK_SIZE + 1) + card + JOKER_DECK_SIZE + 1;
            // and save the pointer to the next card or the handrank
            HR[maxHR] = IDslot;
            //_PDEBUG("ID0 - %d %d", IDnum, maxHR); // show the progress -- counts to 612976 again
        }

        if (numcards == 6 || numcards == 7) {
            // an extra, If you want to know what the handrank when there is 5 or 6 cards
            // you can just do HR[u3] or HR[u4] from below code for Handrank of the 5 or
            // 6 card hand
            // this puts the above handrank into the array
            HR[IDnum * (JOKER_DECK_SIZE + 1) + JOKER_DECK_SIZE + 1] = do_joker_eval(IDs[IDnum], numcards);
            //_PDEBUG("ID6 - %d %d", IDnum, IDnum * (DECK_SIZE + 1) + DECK_SIZE + 1); // show the progress -- counts to 612976 again
        }

        _PPRINT("\rID - %d", IDnum); // show the progress -- counts to 612976 again
    }

    _PDEBUG("\nNumber IDs = %d\nmaxHR = %d", numIDs, maxHR); // for warm fuzzys

    timer = clock() - timer; // end the timer

    _PDEBUG("Training seconds = %.2f", (float)timer / CLOCKS_PER_SEC);

    FILE* fout = fopen(file_name.c_str(), "wb");
    if (!fout) {
        throw Error("Write to file failed: " + file_name);
    }
    std::fwrite(&HR[0], sizeof(int) * HR.size(), 1, fout);
    fclose(fout);
}

void init() try {
    std::string ranks_file_name = RANKS_FILE_NAME;

    std::error_code error;
    ranks_map.map(ranks_file_name, error);
    if (!error) {
        _PDEBUG("Mapped: %.*s", (int)ranks_file_name.length(), ranks_file_name.data());
        return;
    }

    _PDEBUG("Generating new file: %.*s", (int)ranks_file_name.length(), ranks_file_name.data());

    // use standard handranks as lookup service
    std::string standard_ranks_file_name = STANDARD_RANKS_FILE_NAME;
    standard_ranks_map.map(standard_ranks_file_name, error);
    if (error) {
        _PDEBUG("Generating new file: %.*s", (int)standard_ranks_file_name.length(), standard_ranks_file_name.data());
        generate_standard(standard_ranks_file_name);
        standard_ranks_map.map(standard_ranks_file_name, error);
        if (error) {
            throw Error("Map file failed");
            abort();
        }
    }

    _PDEBUG("Mapped: %.*s", (int)standard_ranks_file_name.length(), standard_ranks_file_name.data());

    // can't mmap, generate new file
    generate(ranks_file_name);

    ranks_map.map(ranks_file_name, error);
    if (error) {
        throw Error("Map file failed");
        abort();
    }
}
catch(Error& e) {
    fprintf(stderr, "Init failed: %s", e.what());
    abort();
}

void fini() {
    ranks_map.unmap();
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

std::string print_hand(int* hand, int size) {
    static const char* ranks = "23456789TJQKA";
    std::string result;
    result.reserve(size*2);
    for (int i = 0; i < size; i++) {
        char suit;
        if (hand[i] & 0x8000)
            suit = 'c';
        else if (hand[i] & 0x4000)
            suit = 'd';
        else if (hand[i] & 0x2000)
            suit = 'h';
        else
            suit = 's';

        result.push_back(ranks[(hand[i] >> 8) & 0xF]);
        result.push_back(suit);
    }
    return result;
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

int eval_5cards(int c1, int c2, int c3, int c4, int c5) {
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

int eval_5hand(const int* hand) {
    int c1 = *hand++;
    int c2 = *hand++;
    int c3 = *hand++;
    int c4 = *hand++;
    int c5 = *hand;
    return eval_5cards(c1, c2, c3, c4, c5);
}

// This is a non-optimized method of determining the best five-card hand possible out of six cards.
// If 6 cards I would like to find Result for them
// Cactus Key is 1 = highest - 7362 lowest
// I need to get the min for the permutations
int eval_6hand(const int* hand) {
    int subhand[5];
    int best = 9999;
    //std::atomic<int> best;
    //best = 9999;
    //tbb::parallel_for(0, 6, [&](int i){
    for (int i = 0; i < 6; i++) {
        //int subhand[5];
        for (int j = 0; j < 5; j++)
            subhand[j] = hand[perm6[i][j]];
        best = std::min(best, eval_5hand(subhand));
    }
    return best;
}

// This is a non-optimized method of determining the best five-card hand possible out of seven cards.
int eval_7hand(const int* hand) {
    int subhand[5];
    int best = 9999;
    //std::atomic<int> best;
    //best = 9999;
    //tbb::parallel_for(0, 21, [&](int i){
        //int subhand[5];
    for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 5; j++)
            subhand[j] = hand[perm7[i][j]];
        best = std::min(best, eval_5hand(subhand));
    }
    return best;
}

// Lookup of a poker hand, cards should be a pointer to an array
// of integers each with value between 1 and DECK_SIZE inclusive.
int standard_lookup(const int* cards, int size) {
    const int* ranks = reinterpret_cast<const int*>(standard_ranks_map.data());

    int p = STANDARD_DECK_SIZE + 1;
    for (int i = 0; i < size; ++i) {
        p = ranks[p + cards[i]];
    }

    if (size == 5 || size == 6) {
        p = ranks[p];
    }

    return p;
}

// Lookup of a poker hand, cards should be a pointer to an array
// of integers each with value between 1 and DECK_SIZE inclusive.
int lookup(const int* cards, int size) {
    const int* ranks = reinterpret_cast<const int*>(ranks_map.data());

    int p = JOKER_DECK_SIZE + 1;
    for (int i = 0; i < size; ++i) {
        p = ranks[p + cards[i]];
    }

    if (size == 5 || size == 6) {
        p = ranks[p];
    }

    return p;
}

} // namespace pokerlib

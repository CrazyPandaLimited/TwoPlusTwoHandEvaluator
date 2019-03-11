#include <chrono>
#include <set>
#include <iostream>
#include <iomanip>

#include "pokerlib.hpp"

using namespace std;
using namespace pokerlib;

struct Stat {
    int deck = 52;
    int five = 0;
    int flush = 0;
    int total = 0;
    set<uint64_t> five_set;
    set<uint64_t> flush_set;
};

using Hand5 = std::array<int, 5>;
using Hand7 = std::array<int, 7>;

uint64_t to_id(Hand7 c) {
    bose_nelson_sort_7(&c[0]);
    return (uint64_t)c[0]
        + ((uint64_t)c[1] << 8)
        + ((uint64_t)c[2] << 16)
        + ((uint64_t)c[3] << 24)
        + ((uint64_t)c[4] << 32)
        + ((uint64_t)c[5] << 40)
        + ((uint64_t)c[6] << 48);
}

Hand7 undress(Hand7 c) {
    for (int i = 0; i < 7; ++i) {
        c[i] &= 0xFFFF0FFF;
    }
    return c;
}

bool is_five(const Hand7& c) {
    int ranks[14] = {};
    for (int i = 0; i < 7; ++i) {
        ranks[get_kev_rank(c[i])] ++;
    }

    for(int i = 0; i < 13; ++i) {
        if(ranks[i] + ranks[13] >= 5) {
            return true;
        }
    }
    return false;
}

bool is_flush(const Hand5& c) {
    int q = (c[0] | c[1] | c[2] | c[3] | c[4]) >> 16;

    // check for flushes and straight flushes
    if (c[0] & c[1] & c[2] & c[3] & c[4] & 0xF000)
        return true;
    return false;
}

bool is_flush7(const Hand7& c) {
    Hand5 subhand;
    bool joker = false;
    for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 5; j++) {
            subhand[j] = c[perm7[i][j]];
            if(get_kev_rank(subhand[j]) == 13) {
                //cout << "Joker: " << kev_to_str(subhand) << endl;
                subhand[j] |= 0xF000;
                //cout << "Joker: " << kev_to_str(subhand) << endl;
                //joker = true;
            }
        }

        if(is_flush(subhand)) {
            //if(joker) {
                //cout << "Joker: " << kev_to_str(subhand) << endl;
            //}
            return true;
        }
    }
    return false;
}

void calculate(int deck_size, Stat& stat) {
    stat.deck = deck_size;
    Hand7 k;
    int c0, c1, c2, c3, c4, c5, c6;
    for (c0 = 0; c0 < deck_size; c0++) {
        //cout << c0 << " ";
        //cout.flush();
        k[0] = to_kev(c0);
        for (c1 = c0 + 1; c1 < deck_size; c1++) {
            k[1] = to_kev(c1);
            for (c2 = c1 + 1; c2 < deck_size; c2++) {
                k[2] = to_kev(c2);
                for (c3 = c2 + 1; c3 < deck_size; c3++) {
                    k[3] = to_kev(c3);
                    for (c4 = c3 + 1; c4 < deck_size; c4++) {
                        k[4] = to_kev(c4);
                        for (c5 = c4 + 1; c5 < deck_size; c5++) {
                            k[5] = to_kev(c5);
                            for (c6 = c5 + 1; c6 < deck_size; c6++) {
                                k[6] = to_kev(c6);

                                int ranks[14] = {};
                                for (int i = 0; i < 7; ++i) {
                                    ranks[get_kev_rank(k[i])] ++;
                                }

                                for(int i = 0; i < 13; ++i) {
                                    if(ranks[i] + ranks[13] >= 5) {
                                        stat.five++;
                                        stat.five_set.insert(to_id(undress(k)));
                                        goto next;
                                    }
                                }

                                Hand5 subhand;
                                for (int i = 0; i < 21; i++) {
                                    for (int j = 0; j < 5; j++) {
                                        subhand[j] = k[perm7[i][j]];
                                        if(get_kev_rank(subhand[j]) == 13) {
                                            subhand[j] |= 0xF000;
                                        }
                                    }
                                    // find max
                                    if(is_flush(subhand)) {
                                        stat.flush++;
                                        stat.flush_set.insert(to_id(k));
                                        goto next;
                                        //if(joker) {
                                            //cout << "Joker: " << kev_to_str(subhand) << endl;
                                        //}
                                        //return true;
                                    }
                                }

                                //if (is_five(k)) {
                                    //stat.five++;
                                    //stat.five_set.insert(to_id(k));
                                //}
                                //else if(is_flush7(k)) {
                                    ////cout << "Flush: " << kev_to_str(id) << endl;
                                    //stat.flush++;
                                    //stat.flush_set.insert(to_id(k));
                                    ////stat.flush_set.insert(id);
                                //} else {
                                    //////cout << "Plush: " << kev_to_str(k) << endl;
                                //}
next:
                                stat.total++;
                            }
                        }
                    }
                }
            }
        }
    }
    //stat.flush = ids.size();
    //cout << endl;
}

class InputParser {
public:
    InputParser(int& argc, char** argv) {
        for (int i = 1; i < argc; ++i)
            this->tokens.push_back(std::string(argv[i]));
    }

    const std::string& getCmdOption(const std::string& option) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->tokens.begin(), this->tokens.end(), option);
        if (itr != this->tokens.end() && ++itr != this->tokens.end()) {
            return *itr;
        }
        static const std::string empty_string("");
        return empty_string;
    }

    bool cmdOptionExists(const std::string& option) const {
        return std::find(this->tokens.begin(), this->tokens.end(), option) != this->tokens.end();
    }

private:
    std::vector<std::string> tokens;
};

void list_id(const Debug& debug) {
    int     IDslot, card = 0;
    int64_t ID;

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
            ID = make_id(IDs[IDnum], card, numcards, true, debug); // get the new ID for it
            //count_jokercombs(ID, jokercombs);
            // and save it in the list if I am not on the 7th card
            if (numcards < 7)
                save_id(ID, IDs, maxID, numIDs);
        }
    }
}

void find_id(std::vector<int64_t>& IDs, const Debug& debug) {
    int     IDslot, card = 0;
    int64_t ID;

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

    // Jmd: Okay, this loop is going to fill up the IDs[] array which has
    // 612,967 slots. as this loops through and find new combinations it
    // adds them to the end. I need this list to be stable when I set the
    // handranks (next set)  (I do the insertion sort on new IDs these)
    // so I had to get the IDs first and then set the handranks
    // SA: will stop if there are no combinations left
    for (IDnum = 0; IDs[IDnum] || IDnum == 0; IDnum++) {
        // start at 1 so I have a zero catching entry (just in case)
        for (card = 1; card < JOKER_DECK_SIZE + 1; card++) { // the ids above contain cards upto the current card.  Now add a new card
            ID = make_id(IDs[IDnum], card, numcards, true, debug); // get the new ID for it
            if(ID == -1) {
                return;
            }
            // and save it in the list if I am not on the 7th card
            if (numcards < 7)
                save_id(ID, IDs, maxID, numIDs);
        }
    }
}

void eval_id(std::vector<int64_t>& IDs, const Debug& debug) {
    int     IDslot, card = 0;
    int64_t ID;

    int     numIDs   = 1;
    int     numcards = 0;
    int     maxHR    = 0;
    int64_t maxID    = 0;

    // this is as above, but will not add anything to the ID list, so it is stable
    for (int IDnum = 0; IDs[IDnum] || IDnum == 0; IDnum++) {
        // start at 1 so I have a zero catching entry (just in case)
        for (card = 1; card < JOKER_DECK_SIZE + 1; card++) {
            ID = make_id(IDs[IDnum], card, numcards);

            if (numcards < 7) {
                // when in the index mode (< 7 cards) get the id to save
                IDslot = save_id(ID, IDs, maxID, numIDs) * (JOKER_DECK_SIZE + 1) + JOKER_DECK_SIZE + 1;
            }
            else {
                // if I am at the 7th card, get the equivalence class ("hand rank") to save
                IDslot = do_eval(ID, numcards);
                if(IDslot == -1) {
                    return;
                }
            }
        }

        if (numcards == 6 || numcards == 7) {
            if(do_eval(IDs[IDnum], numcards, debug) == -1) {
                return;
            }
        }
    }

}

int main(int argc, char** argv) {
    Debug debug;
    debug.on = true;
    InputParser input(argc, argv);
    const std::string& cardnum_str = input.getCmdOption("--cardnum");
    if(cardnum_str.empty()) {
        debug.cardnum = 5;
    }
    else {
        debug.cardnum = std::stoi(cardnum_str);
    }

    const std::string& jokernum_str = input.getCmdOption("--jokernum");
    if(jokernum_str.empty()) {
        debug.jokernum = -1;
    }
    else {
        debug.jokernum = std::stoi(jokernum_str);
    }

    debug.verbose = input.cmdOptionExists("--verbose");
    bool continue_search = input.cmdOptionExists("--continue");

    if (input.cmdOptionExists("--list")) {
        debug.id_callback = [&](int* wk, int numcards, int jokers){
            if(debug.jokernum == -1 || jokers == debug.jokernum) {
                char cards[14] = {};
                for (int i = 0; i<numcards; i++) {
                    static const char* ranks = "23456789TJQKAX";
                    static const char* suits = "xshdc";
                    int suit = wk[i] & 0xF;
                    int rank = wk[i] >> 4 & 0xF;
                    cards[i * 2 + 0] = ranks[rank-1];
                    cards[i * 2 + 1] = suits[suit];
                }
                fprintf(stdout, "%llX %s\n", pack_to_id(wk), cards);
            }
            return false;
        };

        list_id(debug);
    }

    const std::string& combo = input.getCmdOption("--find");
    if (!combo.empty()) {
        assert(!(combo.length() % 2));
        std::string combo_normalized_joker = combo;
	for(size_t i = 0; i < combo_normalized_joker.length(); ++i) {
            if(combo_normalized_joker[i] == 'X') {
                combo_normalized_joker[i+1] = 'x';
            }
	}

        debug.cardnum = combo.length() / 2;
        debug.id_callback = [&](int* wk, int numcards, int jokers){
            char cards[14] = {};
            for (int i = 0; i<numcards; i++) {
                static const char* ranks = "23456789TJQKAX";
                static const char* suits = "xshdc";
                int suit = wk[i] & 0xF;
                int rank = wk[i] >> 4 & 0xF;
                cards[i * 2 + 0] = ranks[rank-1];
                cards[i * 2 + 1] = suits[suit];
            }
            if(string(cards, numcards*2) == combo_normalized_joker) {
                fprintf(stdout, "%llX %s\n", pack_to_id(wk), cards);
                return !continue_search;
            }
            return false;
        };
        std::vector<int64_t> IDs(JOKER_IDS_COUNT);
        find_id(IDs, debug);
    }

    const std::string& eval_combo = input.getCmdOption("--eval");
    if (!eval_combo.empty()) {
        assert(!(eval_combo.length() % 2));
        std::string combo_normalized_joker = eval_combo;
	for(size_t i = 0; i < combo_normalized_joker.length(); ++i) {
            if(combo_normalized_joker[i] == 'X') {
                combo_normalized_joker[i+1] = 'x';
            }
	}

        debug.cardnum = eval_combo.length() / 2;
        debug.id_callback = [&](int* wk, int numcards, int jokers){
            char cards[14] = {};
            for (int i = 0; i<numcards; i++) {
                static const char* ranks = "23456789TJQKAX";
                static const char* suits = "xshdc";
                int suit = wk[i] & 0xF;
                int rank = wk[i] >> 4 & 0xF;
                cards[i * 2 + 0] = ranks[rank-1];
                cards[i * 2 + 1] = suits[suit];
            }
            if(string(cards, numcards*2) == combo_normalized_joker) {
                debug.id = pack_to_id(wk);
                fprintf(stdout, "Found ID: %llX %s\n", debug.id, cards);
                return true;
            }
            return false;
        };

        debug.eval_callback = [&](int64_t id, int result){
            fprintf(stdout, "Found %llX %d/%d %s\n", debug.id, result, convert_kev_rank(result), to_string(to_hand(convert_kev_rank(result))));
            return continue_search;
        };

        std::vector<int64_t> IDs(JOKER_IDS_COUNT);
        find_id(IDs, debug);
        if(!debug.id) {
            fprintf(stdout, "Not found\n");
            return 0;
        }

        eval_id(IDs, debug);
    }

    //chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();
    //vector<Stat> stat(5);
    //for (int i = 0; i < 5; ++i) {
    ////for (int i = 1; i < 2; ++i) {
        //calculate(52+i, stat[i]);
        //chrono::time_point<chrono::system_clock> stop = chrono::system_clock::now();
        //cout << "Deck: " << stat[i].deck << endl;
        //cout << "Combinations: " << stat[i].total << endl;
        //cout << "Five: " << stat[i].five << " " << stat[i].five_set.size() << endl;
        //cout << "Flushes: " << stat[i].flush << " " << stat[i].flush_set.size() << endl;
        //cout << "Duration: " << chrono::duration<double>(stop - start).count() << endl;
    //}
    return 0;
}

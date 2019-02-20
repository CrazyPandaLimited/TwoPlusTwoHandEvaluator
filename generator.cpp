#include <chrono>
#include <iostream>
#include <iomanip>

#include "pokerlib.hpp"

using namespace std;

int main() {
    chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();
    pokerlib::init();
    chrono::time_point<chrono::system_clock> stop = chrono::system_clock::now();
    cout << "Duration: " << chrono::duration<double>(stop - start).count() << endl;
    return 0;
}

#ifndef COLLATZ_HPP
#define COLLATZ_HPP

#include <cassert>
#include <map>

inline uint64_t calcCollatz(InfInt n) {
    // It's ok even if the value overflow
    uint64_t count = 0;
    assert(n > 0);

    while (n != 1) {
        ++count;
        if (n % 2 == 1) {
            n *= 3;
            n += 1;
        } else {
            n /= 2;
        }
    }

    return count;
}

inline uint64_t calcCollatzShared(InfInt n, std::map<InfInt, uint64_t> &map) {
    // It's ok even if the value overflow
    uint64_t count = 0;
    assert(n > 0);

    std::vector<std::pair<InfInt, uint64_t>> numbersToAdd;
    uint64_t foundNumCounter = -1;

    while (n != 1) {
        ++count;
        if(map.count(n)) {
            foundNumCounter = map.at(n);
            break;
        } else {
            numbersToAdd.push_back({n, count});
        }
        if (n % 2 == 1) {
            n *= 3;
            n += 1;
        } else {
            n /= 2;
        }
    }

    for(auto &numberToAdd : numbersToAdd) {
        map.insert({numberToAdd.first,
                    foundNumCounter + count - numberToAdd.second + 1});
        //TODO sprawdzić
    }

    return count;
}

#endif
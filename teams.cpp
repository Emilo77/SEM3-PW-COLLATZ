#include <utility>
#include <deque>
#include <future>
#include <iostream>
#include <fstream>
#include <locale>
#include <string>
#include <list>
#include <codecvt>
#include <future>
#include <vector>

#include "teams.hpp"
#include "contest.hpp"
#include "collatz.hpp"

ContestResult TeamNewThreads::runContestImpl(ContestInput const & contestInput)
{
    ContestResult result(contestInput.size());
    std::vector<std::promise<uint64_t>> promiseVector(contestInput.size());
    std::vector<std::future<uint64_t>> futureVector;
    std::mutex mutex;
    std::mutex protection;


    for (int i = 0; i < contestInput.size(); i++) {
        futureVector.push_back(promiseVector.at(i).get_future());
    }

    std::atomic_int threadCount(0);
    for(int i = 0; i < contestInput.size(); i++) {
        if(threadCount >= getSize()) {
            mutex.lock();
        }
        threadCount++;
        auto t = createThread([i, &promiseVector, contestInput,
                               &threadCount, &mutex, &protection]{
            promiseVector.at(i).set_value(calcCollatz(contestInput.at(i)));

            protection.lock();
            if(threadCount >= contestInput.size()) {
                mutex.unlock();
            }
            threadCount--;
            protection.unlock();
        });
        t.detach();
    }

    for (int i = 0; i < contestInput.size(); i++) {
        result.at(i) = futureVector.at(i).get();
    }

    return result;
}

ContestResult TeamConstThreads::runContestImpl(ContestInput const & contestInput)
{
    ContestResult r;
    //TODO
    return r;
}

ContestResult TeamPool::runContest(ContestInput const & contestInput)
{
    ContestResult r;
    //TODO
    return r;
}

ContestResult TeamNewProcesses::runContest(ContestInput const & contestInput)
{
    ContestResult r;
    //TODO
    return r;
}

ContestResult TeamConstProcesses::runContest(ContestInput const & contestInput)
{
    ContestResult r;
    //TODO
    return r;
}

ContestResult TeamAsync::runContest(ContestInput const & contestInput)
{
    ContestResult r;
    //TODO
    return r;
}

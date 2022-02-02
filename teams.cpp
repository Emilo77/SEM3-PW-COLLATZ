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
#include <unistd.h>

#include "teams.hpp"
#include "contest.hpp"
#include "collatz.hpp"

//ContestResult TeamNewThreads::runContestImpl(ContestInput const &contestInput) {
//    ContestResult result;
//    result.resize(contestInput.size());
//
//    std::vector<std::promise<uint64_t>> promiseVector(contestInput.size());
//    std::vector<std::future<uint64_t>> futureVector;
//    std::mutex mutex;
//    std::mutex protection;
//
//
//    for (int i = 0; i < contestInput.size(); i++) {
//        futureVector.push_back(promiseVector.at(i).get_future());
//    }
//
//    std::atomic_int threadCount(0);
//    for (uint64_t i = 0; i < contestInput.size(); i++) {
//        if (threadCount >= getSize()) {
//            mutex.lock();
//        }
//        threadCount++; // może trzeba to obudowac mutexem z ifem
//        auto t = createThread([i, &promiseVector, contestInput,
//                                      &threadCount, &mutex, &protection] {
//            promiseVector.at(i).set_value(calcCollatz(contestInput.at(i)));
//
//            protection.lock();
//            if (threadCount >= contestInput.size()) {
//                mutex.unlock();
//            }
//            threadCount--;
//            protection.unlock();
//        });
//        t.detach();
//    }
//
//    for (int i = 0; i < contestInput.size(); i++) {
//        result.at(i) = futureVector.at(i).get();
//    }
//
//    return result;
//}

//ContestResult
//TeamConstThreads::runContestImpl(ContestInput const &contestInput) {
//
//    ContestResult result;
//    result.resize(contestInput.size());
//
//    std::vector<std::promise<uint64_t>> promiseVector(contestInput.size());
//    std::vector<std::future<uint64_t>> futureVector;
//
//    for (int i = 0; i < contestInput.size(); i++) {
//        futureVector.push_back(promiseVector.at(i).get_future());
//    }
//
//    const uint32_t threadNum = getSize();
//    uint32_t avgWork = contestInput.size() / getSize();
//    std::vector<uint32_t> work(threadNum);
//    std::vector<std::pair<uint32_t, uint32_t>> interval;
//
//    if (contestInput.size() % threadNum == 0) {
//        std::fill(work.begin(), work.end(), avgWork);
//    } else {
//        avgWork++;
//        std::fill(work.begin(), work.end(), avgWork);
//        uint32_t tempWork = avgWork * threadNum;
//        uint32_t index = 0;
//        while (tempWork > contestInput.size()) {
//            work.at(index)--;
//            tempWork--;
//            index++;
//        }
//    }
//
//    uint32_t ind = 0;
//    for (uint32_t i = 0; i < threadNum; i++) {
//        std::pair<uint32_t, uint32_t> newPair = {ind, ind + work.at(i)};
//        interval.push_back(newPair);
//        ind += work.at(i);
//    }
//
//    for(int i = 0; i < threadNum; i++) {
//        auto t = createThread([i, interval, &promiseVector] {
//            for(uint32_t index = interval.at(i).first;
//            index < interval.at(i).second; index++) {
//                promiseVector.at(index).set_value(calcCollatz(index));
//            }
//        });
//        t.detach();
//    }
//
//    for (int i = 0; i < contestInput.size(); i++) {
//        result.at(i) = futureVector.at(i).get();
//    }
//
//    return result;
//}

//ContestResult TeamPool::runContest(ContestInput const &contestInput) {
//    ContestResult result;
//    result.resize(contestInput.size());
//
//    std::vector<std::future<uint64_t>> futureVector(contestInput.size());
//
//    for (int i = 0; i < contestInput.size(); i++) {
//        futureVector.at(i) = pool.push([contestInput, i] {
//            return calcCollatz(contestInput.at(i));
//        });
//    }
//
//    for (int i = 0; i < contestInput.size(); i++) {
//        result.at(i) = futureVector.at(i).get();
//    }
//
//    return result;
//}

ContestResult TeamNewProcesses::runContest(ContestInput const &contestInput) {
    ContestResult r;
    //TODO
    return r;
}

void calcCollatzProcess(int index, std::vector<std::pair<uint32_t,uint32_t>>
        interval, std::vector<std::promise<uint64_t>> &promiseVector) {

    for(uint32_t index = interval.at(index).first;
        index < interval.at(index).second; index++) {
        promiseVector.at(index).set_value(calcCollatz(index));
    }

}

ContestResult TeamConstProcesses::runContest(ContestInput const &contestInput) {
    ContestResult result;
    result.resize(contestInput.size());

    std::vector<std::promise<uint64_t>> promiseVector(contestInput.size());
    std::vector<std::future<uint64_t>> futureVector;
    pid_t pid;

    for (int i = 0; i < contestInput.size(); i++) {
        futureVector.push_back(promiseVector.at(i).get_future());
    }

    const uint32_t threadNum = getSize();
    uint32_t avgWork = contestInput.size() / getSize();
    std::vector<uint32_t> work(threadNum);
    std::vector<std::pair<uint32_t, uint32_t>> interval;

    if (contestInput.size() % threadNum == 0) {
        std::fill(work.begin(), work.end(), avgWork);
    } else {
        avgWork++;
        std::fill(work.begin(), work.end(), avgWork);
        uint32_t tempWork = avgWork * threadNum;
        uint32_t index = 0;
        while (tempWork > contestInput.size()) {
            work.at(index)--;
            tempWork--;
            index++;
        }
    }

    uint32_t ind = 0;
    for (uint32_t i = 0; i < threadNum; i++) {
        std::pair<uint32_t, uint32_t> newPair = {ind, ind + work.at(i)};
        interval.push_back(newPair);
        ind += work.at(i);
    }

    for(int i = 0; i < threadNum; i++) {
        switch(pid = fork()){
            case -1:
                syserr("fork");
            case 0:
                //calcCollatzProcess();
                //return 0;
            default:
                break;
        }
    }

    for (int i = 0; i < contestInput.size(); i++) {
        result.at(i) = futureVector.at(i).get();
    }

    return result;
}

//ContestResult TeamAsync::runContest(ContestInput const &contestInput) {
//    ContestResult result;
//    result.resize(contestInput.size());
//    uint64_t idx = 0;
//    std::vector<std::future<uint64_t>> futureVector(contestInput.size());
//
//    rtimers::cxx11::DefaultTimer soloTimer("CalcCollatzSoloTimer");
//
//    for(InfInt const & singleInput : contestInput)
//    {
//        auto scopedStartStop = soloTimer.scopedStart();
//        futureVector.at(idx) = std::async(std::launch::async,
//                                          [idx, contestInput] {
//            return calcCollatz(contestInput.at(idx));
//        });
//        ++idx;
//    }
//
//    for (int i = 0; i < contestInput.size(); i++) {
//        result.at(i) = futureVector.at(i).get();
//    }
//
//    return result;
//}

#ifndef TEAMS_HPP
#define TEAMS_HPP

#include <thread>
#include <cassert>

#include "lib/rtimers/cxx11.hpp"
#include "lib/pool/cxxpool.h"

#include "contest.hpp"
#include "collatz.hpp"
#include "sharedresults.hpp"
#include "err.h"

static void splitWork(const uint32_t contestInputSize, const uint32_t threadNum,
                      std::vector<std::pair<uint32_t, uint32_t>> &interval) {

    std::vector<uint32_t> work((int) threadNum);
    uint32_t avgWork = contestInputSize / threadNum;
    if (contestInputSize % threadNum == 0) {
        std::fill(work.begin(), work.end(), avgWork);
    } else {
        avgWork++;
        std::fill(work.begin(), work.end(), avgWork);
        uint32_t tempWork = avgWork * threadNum;
        auto index = 0;
        while (tempWork > contestInputSize) {
            work.at(index)--;
            tempWork--;
            index++;
        }
    }

    uint32_t ind = 0;
    for (auto i = 0; i < threadNum; i++) {
        std::pair<uint32_t, uint32_t> newPair = {ind, ind + work.at(i)};
        interval.push_back(newPair);
        ind += work.at(i);
    }
}


class Team {
public:
    Team(uint32_t sizeArg, bool shareResults) : size(sizeArg), sharedResults() {
        assert(this->size > 0);

        if (shareResults) {
            this->sharedResults.reset(new SharedResults{});
        }
    }

    virtual ~Team() {}

    virtual std::string getInnerName() = 0;

    std::shared_ptr<SharedResults> getSharedResults() {
        return this->sharedResults;
    }

    virtual ContestResult runContest(ContestInput const &contest) = 0;

    std::string getXname() { return this->getSharedResults() ? "X" : ""; }

    virtual std::string getTeamName() {
        return this->getInnerName() + this->getXname() + "<" +
               std::to_string(this->size) + ">";
    }

    uint32_t getSize() { return this->size; }


private:
    std::shared_ptr<SharedResults> sharedResults;
    uint32_t size;
};

class TeamSolo : public Team {
public:
    TeamSolo(uint32_t sizeArg) : Team(1, false) {} // ignore size, don't share

    virtual ContestResult runContest(ContestInput const &contestInput) {
        ContestResult result;
        result.resize(contestInput.size());
        uint64_t idx = 0;

        rtimers::cxx11::DefaultTimer soloTimer("CalcCollatzSoloTimer");

        for (InfInt const &singleInput: contestInput) {
            auto scopedStartStop = soloTimer.scopedStart();
            result[idx] = calcCollatz(singleInput);
            ++idx;
        }
        return result;
    }

    virtual std::string getInnerName() { return "TeamSolo"; }
};

class TeamThreads : public Team {
public:
    TeamThreads(uint32_t sizeArg, bool shareResults) : Team(sizeArg,
                                                            shareResults),
                                                       createdThreads(0) {}

    template<class Function, class... Args>
    std::thread createThread(Function &&f, Args &&... args) {
        ++this->createdThreads;
        return std::thread(std::forward<Function>(f),
                           std::forward<Args>(args)...);
    }

    void resetThreads() { this->createdThreads = 0; }

    uint64_t getCreatedThreads() { return this->createdThreads; }

private:
    uint64_t createdThreads;
};

class TeamNewThreads : public TeamThreads {
public:
    TeamNewThreads(uint32_t sizeArg, bool shareResults) : TeamThreads(sizeArg,
                                                                      shareResults) {}

    virtual ContestResult runContest(ContestInput const &contestInput) {
        this->resetThreads();
        ContestResult result = this->runContestImpl(contestInput);
        assert(contestInput.size() == this->getCreatedThreads());
        return result;
    }

    virtual ContestResult runContestImpl(ContestInput const &contestInput) {
        ContestResult result;
        result.resize(contestInput.size());

        if (getSize() == 0) {
            return result; // czy getsize może być równe 0?
        }

        std::vector<std::promise<uint64_t>> promiseVector(contestInput.size());
        std::vector<std::future<uint64_t>> futureVector;
        std::vector<std::thread> threads;

        uint64_t size = getSize();

        for (int i = 0; i < contestInput.size(); i++) {
            futureVector.push_back(promiseVector.at(i).get_future());
        }

        uint64_t threadCount = 0;
        auto indexToJoin = 0;
        for (auto i = 0; i < contestInput.size(); i++) {

            threadCount++;
            threads.push_back(createThread(
                    [i, &promiseVector, contestInput] {
                        promiseVector.at(i).set_value(
                                calcCollatz(contestInput.at(i)));
                    }));
            if (threadCount == size) {
                threads.at(indexToJoin).join();
                indexToJoin++;
                threadCount--;
            }
        }
        while (indexToJoin < contestInput.size()) {
            threads.at(indexToJoin).join();
            indexToJoin++;
        }

        for (int i = 0; i < contestInput.size(); i++) {
            result.at(i) = futureVector.at(i).get();
        }

        return result;
    }

    virtual std::string getInnerName() { return "TeamNewThreads"; }
};

class TeamConstThreads : public TeamThreads {
public:
    TeamConstThreads(uint32_t sizeArg, bool shareResults) : TeamThreads(sizeArg,
                                                                        shareResults) {}

    virtual ContestResult runContest(ContestInput const &contestInput) {
        this->resetThreads();
        ContestResult result = this->runContestImpl(contestInput);
        assert(this->getSize() == this->getCreatedThreads());
        return result;
    }

    virtual ContestResult runContestImpl(ContestInput const &contestInput) {
        ContestResult result;
        result.resize(contestInput.size());

        auto threadNum = getSize();
        std::vector<std::promise<uint64_t>> promiseVector(contestInput.size());
        std::vector<std::future<uint64_t>> futureVector;
        std::vector<std::pair<uint32_t, uint32_t>> interval;


        for (int i = 0; i < contestInput.size(); i++) {
            futureVector.push_back(promiseVector.at(i).get_future());
        }

        splitWork(contestInput.size(), threadNum, interval);

//        if(!getSharedResults()) {}

        for (int i = 0; i < threadNum; i++) {
            auto t = createThread([i, interval, &promiseVector, contestInput] {
                for (uint32_t index = interval.at(i).first;
                     index < interval.at(i).second; index++) {
                    promiseVector.at((int) index).set_value(calcCollatz
                                                                    (contestInput.at(
                                                                            (int) index)));
                }
            });
            t.detach();
        }

        for (int i = 0; i < contestInput.size(); i++) {
            result.at(i) = futureVector.at(i).get();
        }

        return result;
    }

    virtual std::string getInnerName() { return "TeamConstThreads"; }
};

class TeamPool : public Team {
public:
    TeamPool(uint32_t sizeArg, bool shareResults) : Team(sizeArg, shareResults),
                                                    pool(sizeArg) {}

    virtual ContestResult runContest(ContestInput const &contestInput) {

        ContestResult result;
        int threadNum = (int) 10; //getSize()
        std::vector<std::future<std::vector<uint64_t>>> futureVector(threadNum);
        std::vector<std::vector<uint64_t>> resultVector(threadNum);
        std::vector<std::pair<uint32_t, uint32_t>> interval;

        splitWork(contestInput.size(), threadNum, interval);

        for (int i = 0; i < threadNum; i++) {
            futureVector.at(i) = pool.push([i, interval, contestInput] {
                std::vector<uint64_t> res;
                for (uint32_t index = interval.at(i).first;
                     index < interval.at(i).second; index++) {
                    res.push_back(calcCollatz(contestInput.at((int) index)));
                }
                return res;
            });
        }

        for (int i = 0; i < threadNum; i++) {
            resultVector.at(i) = futureVector.at(i).get();
            for (auto &j: resultVector.at(i)) {
                result.push_back(j);
            }
        }

        return result;
    }

    virtual std::string getInnerName() { return "TeamPool"; }

private:
    cxxpool::thread_pool pool;
}; // TODO

class TeamNewProcesses : public Team {
public:
    TeamNewProcesses(uint32_t sizeArg, bool shareResults) : Team(sizeArg,
                                                                 shareResults) {}

    virtual ContestResult runContest(ContestInput const &contestInput) {
        ContestResult result;
        return result;
    }

    virtual std::string getInnerName() { return "TeamNewProcesses"; }
}; // TODO

class TeamConstProcesses : public Team {
public:
    TeamConstProcesses(uint32_t sizeArg, bool shareResults) : Team(sizeArg,
                                                                   shareResults) {}

    virtual ContestResult runContest(ContestInput const &contestInput) {
        ContestResult result;
        return result;
    }

    virtual std::string getInnerName() { return "TeamConstProcesses"; }
}; // TODO

class TeamAsync : public Team {
public:
    TeamAsync(uint32_t sizeArg, bool shareResults) : Team(1,
                                                          shareResults) {} // ignore size

    virtual ContestResult runContest(ContestInput const &contestInput) {

        ContestResult result;
        int threadNum = (int) getSize();
        std::vector<std::future<std::vector<uint64_t>>> futureVector(threadNum);
        std::vector<std::vector<uint64_t>> resultVector(threadNum);
        std::vector<std::pair<uint32_t, uint32_t>> interval;

        splitWork(contestInput.size(), threadNum, interval);

        for (int i = 0; i < threadNum; i++) {
            futureVector.at(i) = std::async([i, interval, contestInput] {
                std::vector<uint64_t> res;
                for (uint32_t index = interval.at(i).first;
                     index < interval.at(i).second; index++) {
                    res.push_back(calcCollatz(contestInput.at((int) index)));
                }
                return res;
            });
        }

        for (int i = 0; i < threadNum; i++) {
            resultVector.at(i) = futureVector.at(i).get();
            for (auto &j: resultVector.at(i)) {
                result.push_back(j);
            }
        }

        return result;
    }

    virtual std::string getInnerName() { return "TeamAsync"; }
};

#endif
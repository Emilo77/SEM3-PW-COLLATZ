#ifndef TEAMS_HPP
#define TEAMS_HPP

#include <thread>
#include <assert.h>

#include "lib/rtimers/cxx11.hpp"
#include "lib/pool/cxxpool.h"

#include "contest.hpp"
#include "collatz.hpp"
#include "sharedresults.hpp"
#include "err.h"

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

    uint32_t getSize() const { return this->size; }


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
        uint64_t indexToJoin = 0;
        for (uint64_t i = 0; i < contestInput.size(); i++) {

            threadCount++;
            threads.push_back(createThread(
                    [i, &promiseVector, contestInput] {
                        promiseVector.at(i).set_value(
                                calcCollatz(contestInput.at(i)));
                    }));
            if (threadCount == size) {
                threads.at(indexToJoin).join();
                //czy trzeba sprawdzać joinable?
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

        std::vector<std::promise<uint64_t>> promiseVector(contestInput.size());
        std::vector<std::future<uint64_t>> futureVector;

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

        for (int i = 0; i < threadNum; i++) {
            auto t = createThread([i, interval, &promiseVector, contestInput] {
                for (uint32_t index = interval.at(i).first;
                     index < interval.at(i).second; index++) {
                    promiseVector.at(index).set_value(calcCollatz
                                                              (contestInput.at(
                                                                      index)));
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
        result.resize(contestInput.size());

        std::vector<std::future<uint64_t>> futureVector(contestInput.size());

        for (int i = 0; i < contestInput.size(); i++) {
            futureVector.at(i) = pool.push([contestInput, i] {
                return calcCollatz(contestInput.at(i));
            });
        }

        for (int i = 0; i < contestInput.size(); i++) {
            result.at(i) = futureVector.at(i).get();
        }

        return result;
    }

    virtual std::string getInnerName() { return "TeamPool"; }

private:
    cxxpool::thread_pool pool;
};

class TeamNewProcesses : public Team {
public:
    TeamNewProcesses(uint32_t sizeArg, bool shareResults) : Team(sizeArg,
                                                                 shareResults) {}

    virtual ContestResult runContest(ContestInput const &contestInput);

    virtual std::string getInnerName() { return "TeamNewProcesses"; }
};

class TeamConstProcesses : public Team {
public:
    TeamConstProcesses(uint32_t sizeArg, bool shareResults) : Team(sizeArg,
                                                                   shareResults) {}

    virtual ContestResult runContest(ContestInput const &contestInput);

    virtual std::string getInnerName() { return "TeamConstProcesses"; }
};

class TeamAsync : public Team {
public:
    TeamAsync(uint32_t sizeArg, bool shareResults) : Team(1,
                                                          shareResults) {} // ignore size

    virtual ContestResult runContest(ContestInput const &contestInput) {
        ContestResult result;
        result.resize(contestInput.size());
        uint64_t idx = 0;
        std::vector<std::future<uint64_t>> futureVector(contestInput.size());

        for (int i = 0; i < contestInput.size(); i++) {
            auto singleInput = contestInput.at(i);
            futureVector.at(idx) = std::async([singleInput] {
                return calcCollatz(singleInput);
            });
            ++idx;
        }

        for (int i = 0; i < contestInput.size(); i++) {
            result.at(i) = futureVector.at(i).get();
        }

        return result;
    }

    virtual std::string getInnerName() { return "TeamAsync"; }
};

#endif
#include <utility>
#include <deque>
#include <future>

#include "teams.hpp"
#include "contest.hpp"
#include "collatz.hpp"

ContestResult TeamNewThreads::runContestImpl(ContestInput const & contestInput)
{
    ContestResult r;

//    for(int i = 0; i < contestInput.size(); i++) {
//
//    }
    return r;
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

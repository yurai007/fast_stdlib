#include "cooperative_scheduler.hh"
#include "channel.hh"
#include <boost/range/algorithm/for_each.hpp>
#include <tuple>
#include <map>
#include <vector>
#include <cassert>
#include <iostream>
#include <future>

enum class State { INITIAL, DONE };

struct Message {
    virtual ~Message() = default;
};

struct HeartBeat : Message {
    bool done = false;
};

struct AppendEntriesReq : Message {
    std::tuple<char, int> entry;
    int term, prevIndex, prevTerm, leaderCommit;
};

struct AppendEntriesResp : Message {
    int term;
    bool success;
};

struct InternalLogEntry {
    std::tuple<char, int> entry;
    int term;
};

class Node {
public:
    virtual void run() = 0;
    virtual ~Node() noexcept(false) {}
private:
    std::map<char, int> logState;
    std::vector<InternalLogEntry> log;
    State state = State::INITIAL;
    int currentTerm = 0, commitIndex = 0, lastApplied = 0;
};

class Follower : public Node {
public:
    void run() override {
        std::cout << "Follower: " << this << " done" << std::endl;
    }
    bool verifyLog(const std::vector<InternalLogEntry>&) const { return true; }
    virtual ~Follower() noexcept(false) {}
private:
    channel<Message*, bypass_lock> channelToLeader;

#if 0
    std::future<int> sendHeartbeat(bool done) {
        auto msg = HeartBeat{done};
        co_await channelToLeader.write(&msg);
        co_return 0;
    }

    std::future<HeartBeat*> receiveHeartbeat() {
        auto [msg, ok] = co_await channelToLeader.read();
        co_return dynamic_cast<HeartBeat*>(msg);
    }
#endif
};

class Leader : public Node {
public:
    Leader(const std::vector<Follower> &, const std::map<char, int> &) {}
    void run() override {
        std::cout << "Leader: done" << std::endl;
    }
};

// temporary workaround - currently fibers cannot take arguments
Leader *_leader;
std::vector<Follower> *_followers;
std::promise<bool> done;
std::future<bool> donef = done.get_future();

static void leaderFiber() {
    auto task = []() -> std::future<int> {
        try {
            _leader->run();
        } catch (...) { std::cout << "Leader: failed" << std::endl; }
        co_return 0;
    };
    task().get();
    done.set_value(true);
}

static void followersFiber() {
    auto task = []() -> std::future<int> {
            boost::for_each(*_followers, [](auto &&follower){
                try {
                    follower.run();
                } catch (...) {  std::cout << "Follower: failed" << std::endl; }
        });
        co_return 0;
    };
    task().get();
    donef.get();
}

static void launchLeaderAndFollowers(Leader &leader, std::vector<Follower> &followers) {
    _leader = &leader;
    _followers = &followers;
    cooperative_scheduler{leaderFiber, followersFiber};
}

static void oneLeaderOneFollowerScenarioWithConsensus() {
    std::vector<Follower> followers(1u);
    std::map<char, int> entriesToReplicate{{'x', 1}, {'y', 2}};
    auto leader = Leader(followers, entriesToReplicate);
    launchLeaderAndFollowers(leader, followers);
    auto expectedLog = std::vector{InternalLogEntry{std::tuple{'x', 1}, 1},
                                   InternalLogEntry{std::tuple{'y', 2}, 1} };
    boost::for_each(followers, [&expectedLog](auto &&follower){
        assert(follower.verifyLog(expectedLog));
    });
    std::cout << std::endl;
}

int main() {
    oneLeaderOneFollowerScenarioWithConsensus();
    return 0;
}

﻿#include "cooperative_scheduler.hh"
#include "channel.hh"
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/irange.hpp>
#include <tuple>
#include <map>
#include <vector>
#include <cassert>
#include <iostream>
#include <future>
#include <stdexcept>

enum class State { INITIAL, DONE };

struct Message {
    virtual ~Message() = default;
};

struct HeartBeat : Message {
    HeartBeat(bool _done) noexcept : done(_done) {}
    bool done = false;
};

struct AppendEntriesReq : Message {
    AppendEntriesReq(const std::tuple<char, int> &_entry, int _term, int _prevIndex, int _prevTerm, int _leaderCommit) noexcept
        : entry(_entry), term(_term), prevIndex(_prevIndex), prevTerm(_prevTerm), leaderCommit(_leaderCommit) {}
    std::tuple<char, int> entry;
    int term, prevIndex, prevTerm, leaderCommit;
};

struct AppendEntriesResp : Message {
    AppendEntriesResp(int _term, bool _success) noexcept
        : term(_term), success(_success) {}
    int term;
    bool success;
    bool operator!=(const AppendEntriesResp &other) const {
        return term != other.term || success != other.success;
    }
};

struct InternalLogEntry {
    InternalLogEntry(const std::tuple<char, int> &_entry, int _term) noexcept
        : entry(_entry), term(_term) {}
    std::tuple<char, int> entry;
    int term;
};

class Node {
public:
    virtual void run() = 0;
    virtual ~Node() noexcept(false) {}

    void trackLog() const {
        if (debug) {
            throw std::logic_error("Unimplemented yet");
        }
    }

protected:
    std::map<char, int> logState;
    std::vector<InternalLogEntry> log;
    State state = State::INITIAL;
    int currentTerm = 0, commitIndex = 0, lastApplied = 0;
    constexpr static bool debug = false;
};

class Follower : public Node {
public:
    void run() override {
        state = State::INITIAL;
        currentTerm++;
        commitIndex = std::max(static_cast<int>(log.size()) - 1, 0);
        std::cout << "Follower " << this << " starts" << std::endl;
        while (!done(*receiveHeartbeat().get())) {

            auto appendEntries = receiveAppendEntriesReq().get();
            auto apply = true;
            auto prevIndex = appendEntries->prevIndex;
            auto prevTerm = appendEntries->prevTerm;
            if (prevIndex >= 0) {
                if (prevIndex < log.size()) {
                    // [2]
                    if (log[prevIndex].term != prevTerm) {
                        apply = false;
                    }
                } else {
                    apply = false;
                }
                if (prevIndex + 1 < log.size()) {
                    // [3]
                    if (log[prevIndex + 1].term != appendEntries->term) {
                        // cleanup because of inconsistency, leave only log[0..prevIndex] prefix,
                        // with assumption that prefix is valid we can append entry in this iteration
                        shrinkUntil(prevIndex);
                    }
                }
            }
            sendAppendEntriesResp(AppendEntriesResp(appendEntries->term, apply));
            auto [id, value] = appendEntries->entry;
            if (apply) {
                log.emplace_back(appendEntries->entry, appendEntries->term);
                logState[id] = value;
                std::cout << "Follower " << this << ": " << id << ":= " << value << std::endl;
                commitIndex++;
                lastApplied++;
                if (appendEntries->term > currentTerm) {
                    currentTerm = appendEntries->term;
                }
            } else {
                std::cout << "Follower " << this << " no consensus for " << id << std::endl;
                trackLog();
            }
        }
        state = State::DONE;
        std::cout << "Follower: " << this << " done" << std::endl;
        trackLog();
    }
    bool verifyLog(const std::vector<InternalLogEntry>&) const { return true; }
    virtual ~Follower() noexcept(false) {}
private:
    channel<Message*, bypass_lock> channelToLeader;
    std::unique_ptr<Message> msg;
    Message *msg_p = nullptr;

   void shrinkUntil(int index) {
        for (auto i : boost::irange(index + 1, static_cast<int>(log.size()))) {
            auto &&[keyValue, _] = log[i];
            auto &&[key, __] = keyValue;
            logState.erase(key);
            // log.removeAt(i); FIXME
        }
    }

    bool done(const HeartBeat &msg) const {
        return msg.done;
    }
public:
    std::future<int> sendHeartbeat(bool done) {
        msg = std::make_unique<HeartBeat>(done);
        msg_p = msg.get();
        co_await channelToLeader.write(msg_p);
        co_return 0;
    }

    std::future<HeartBeat*> receiveHeartbeat() {
        auto [_msg, ok] = co_await channelToLeader.read();
        co_return dynamic_cast<HeartBeat*>(_msg);
    }

    std::future<int> sendAppendEntriesReq(AppendEntriesReq &&req) {
        msg = std::make_unique<AppendEntriesReq>(req);
        msg_p = msg.get();
        co_await channelToLeader.write(msg_p);
        co_return 0;
    }

    std::future<AppendEntriesReq*> receiveAppendEntriesReq() {
        auto [_msg, ok] = co_await channelToLeader.read();
        co_return dynamic_cast<AppendEntriesReq*>(_msg);
    }

     std::future<int> sendAppendEntriesResp(AppendEntriesResp &&rep) {
         msg = std::make_unique<AppendEntriesResp>(rep);
         msg_p = msg.get();
         co_await channelToLeader.write(msg_p);
         co_return 0;
     }

    std::future<AppendEntriesResp*> receiveAppendEntriesResp() {
        auto [_msg, ok] = co_await channelToLeader.read();
        co_return dynamic_cast<AppendEntriesResp*>(_msg);
    }
};

class Leader : public Node {
public:
    Leader(std::vector<Follower> &_followers, const std::map<char, int> &_entriesToReplicate) :
        followers(_followers),
        entriesToReplicate(_entriesToReplicate) {
        boost::for_each(followers, [this](auto &&follower){
            nextIndex[&follower] = 0;
        });
    }

    void run() override {
        state = State::INITIAL;
        currentTerm++;
        std::cout << "Leader of term " << currentTerm << std::endl;
         boost::for_each(entriesToReplicate, [this](auto &entry){
            auto [id, value] = entry;
            boost::for_each(followers, [](auto &&follower){
                follower.sendHeartbeat(false).get(); // FIXME: do in parallel + when_all helper
            });
            lastApplied++;
            log.emplace_back(entry, currentTerm);
            // x = x, as workaround for http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0588r1.html
            boost::for_each(followers, [this, entry = std::make_tuple(entry.first, entry.second), id = id, value = value, followerIsDone = false](auto &&it) mutable {
                 do {
                    auto prevIndex = std::min(replicaNextIndex(&it) - 1, static_cast<int>(log.size()) - 1);
                    auto prevTerm = (prevIndex >= 0)? log[prevIndex].term  : 0;
                    auto term =  (prevIndex + 1 >= 0 && prevIndex + 1 < log.size()) ? log[prevIndex + 1].term  : currentTerm;
                    it.sendAppendEntriesReq(AppendEntriesReq(entry, term, prevIndex, prevTerm, commitIndex)).get();
                    auto response = *it.receiveAppendEntriesResp().get();
                    auto expected = AppendEntriesResp(term, true);

                     if (response != expected) {
                        std::cout << "Leader: No consensus for " << id << " " << value << std::endl;
                        nextIndex[&it] = replicaNextIndex(&it) - 1;
                        if (replicaNextIndex(&it) >= 0) {
                            entry = log[replicaNextIndex(&it)].entry;
                        }
                        it.sendHeartbeat(false).get();
                        trackLog();
                    } else if (term == currentTerm) {
                        followerIsDone = true;
                    } else {
                        nextIndex[&it] = replicaNextIndex(&it) + 1;
                        if (replicaNextIndex(&it) < log.size()) {
                            entry = log[replicaNextIndex(&it)].entry;
                        } else {
                            entry = std::tie(id, value);
                        }
                        it.sendHeartbeat(false).get();
                    }

                 } while (!followerIsDone);
            });
            commitIndex++;
            logState[id] = value;
            boost::for_each(followers, [this](auto &&follower){
                nextIndex[&follower] = replicaNextIndex(&follower) + 1;
            });
            std::cout << "Leader: " << id << " := " << value << std::endl;
         });
        std::cout << "Leader: done" << std::endl;
    }

private:
     std::vector<Follower> &followers;
     std::map<Follower*, int> nextIndex, matchIndex;
     std::map<char, int> entriesToReplicate;

     int replicaNextIndex(Follower *follower) const {
         auto it = nextIndex.find(follower);
         return (it != nextIndex.end())? *it : throw std::logic_error("Shouldn't happen"), 0;
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
        } catch (const std::exception &e) { std::cout << "Leader: failed with: " << e.what() << std::endl; }
        co_return 0;
    };
    task().get();
    donef.get();
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
    done.set_value(true);
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

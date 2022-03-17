
#ifndef DOMPASCH_MALLOB_ANYTIME_SAT_CLAUSE_COMMUNICATOR_H
#define DOMPASCH_MALLOB_ANYTIME_SAT_CLAUSE_COMMUNICATOR_H

#include <future>

#include "util/params.hpp"
#include "util/hashing.hpp"
#include "hordesat/sharing/adaptive_clause_database.hpp"
#include "data/job_transfer.hpp"
#include "app/job.hpp"
#include "base_sat_job.hpp"
#include "clause_history.hpp"
#include "distributed_clause_filter.hpp"
#include "comm/job_tree_all_reduction.hpp"

class AnytimeSatClauseCommunicator {

public:
    enum Stage {
        PREPARING_CLAUSES, 
        MERGING, 
        WAITING_FOR_CLAUSE_BCAST, 
        PREPARING_FILTER, 
        WAITING_FOR_FILTER_BCAST
    };

private:
    const Parameters& _params;
    BaseSatJob* _job = NULL;
    bool _suspended = false;

    const int _clause_buf_base_size;
    const float _clause_buf_discount_factor;
    const bool _use_cls_history;

    AdaptiveClauseDatabase _cdb;
    ClauseHistory _cls_history;
    DistributedClauseFilter _filter;

    struct Session {

        const Parameters& _params;
        BaseSatJob* _job;
        AdaptiveClauseDatabase& _cdb;
        DistributedClauseFilter& _filter;
        int _epoch;

        std::vector<int> _excess_clauses_from_merge;
        std::vector<int> _broadcast_clause_buffer;

        JobTreeAllReduction _allreduce_clauses;
        JobTreeAllReduction _allreduce_filter;

        Session(const Parameters& params, BaseSatJob* job, AdaptiveClauseDatabase& cdb, DistributedClauseFilter& filter, int epoch) : 
            _params(params), _job(job), _cdb(cdb), _filter(filter), _epoch(epoch),
            _allreduce_clauses(
                job->getJobTree(),
                // Base message 
                JobMessage(_job->getId(), _job->getRevision(), epoch, MSG_ALLREDUCE_CLAUSES),
                // Neutral element
                std::vector<int>(1, 1), 
                // Aggregator for local + incoming elements
                [&](std::list<std::vector<int>>& elems) {
                    int numAggregated = 0;
                    for (auto& elem : elems) {
                        numAggregated += elem.back();
                        elem.pop_back();
                    }
                    auto merger = _cdb.getBufferMerger(_job->getBufferLimit(numAggregated, MyMpi::ALL));
                    for (auto& elem : elems) {
                        merger.add(_cdb.getBufferReader(elem.data(), elem.size()));
                    }
                    return merger.merge(&_excess_clauses_from_merge);
                }
            ),
            _allreduce_filter(
                job->getJobTree(), 
                // Base message
                JobMessage(_job->getId(), _job->getRevision(), epoch, MSG_ALLREDUCE_FILTER),
                // Neutral element
                std::vector<int>(),
                // Aggregator for local + incoming elements
                [&](std::list<std::vector<int>>& elems) {
                    std::vector<int> filter = std::move(elems.front());
                    elems.pop_front();
                    for (auto& elem : elems) {
                        if (filter.size() < elem.size()) 
                            filter.resize(elem.size());
                        for (size_t i = 0; i < elem.size(); i++) {
                            filter[i] |= elem[i];
                        }
                    }
                    return filter;
                }
            ) { }

        void storePreparedClauseBuffer();

        void initiateMergeOfClauseBuffers();
        std::vector<int> getMergedClauseBuffer();
        void publishMergedClauses();

        void processBroadcastClauses(std::vector<int>& clauses);

        void addFilter(std::vector<int>& filter);
        void publishLocalAggregatedFilter();
        std::vector<int> applyGlobalFilter(const std::vector<int>& filter, std::vector<int>& clauses);

        std::vector<int> merge(size_t maxSize);

        bool isValid() const {
            return _allreduce_clauses.isValid() || _allreduce_filter.isValid();
        }

        bool isDestructible() {
            return _allreduce_clauses.isDestructible() && _allreduce_filter.isDestructible();
        }
    };

    std::list<Session> _sessions;

    int _current_epoch = 0;
    float _time_of_last_epoch_initiation = 0;
    float _time_of_last_epoch_conclusion = 0.1;

public:
    AnytimeSatClauseCommunicator(const Parameters& params, BaseSatJob* job) : _params(params), _job(job), 
        _clause_buf_base_size(_params.clauseBufferBaseSize()), 
        _clause_buf_discount_factor(_params.clauseBufferDiscountFactor()),
        _use_cls_history(params.collectClauseHistory()),
        _cdb([&]() {
            AdaptiveClauseDatabase::Setup setup;
            setup.maxClauseLength = _params.strictClauseLengthLimit();
            setup.maxLbdPartitionedSize = _params.maxLbdPartitioningSize();
            setup.slotsForSumOfLengthAndLbd = _params.groupClausesByLengthLbdSum();
            setup.numLiterals = 0;
            setup.numProducers = 0;
            return setup;
        }()),
        _cls_history(_params, _job->getBufferLimit(_job->getJobTree().getCommSize(), MyMpi::ALL), *job, _cdb),
        _filter(params.clauseFilterClearInterval()) {

        _time_of_last_epoch_initiation = Timer::elapsedSeconds();
        _time_of_last_epoch_conclusion = Timer::elapsedSeconds();
    }

    ~AnytimeSatClauseCommunicator() {}

    void communicate();
    void handle(int source, int mpiTag, JobMessage& msg);
    void feedHistoryIntoSolver();
    bool isDestructible() {
        for (auto& session : _sessions) if (!session.isDestructible()) return false;
        return true;
    }

private:
    inline Session& currentSession() {return _sessions.back();}
    void learnClauses(std::vector<int>& clauses, int epoch, bool writeIntoClauseHistory);
};

#endif
#pragma once
#include <NovusTypes.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <Utils/ConcurrentQueue.h>

struct ScriptSingleton
{
    ScriptSingleton() : _systemCompleteCount(0), _queueCreateLock(), _transactionQueues()
    {
        _transactionQueues.reserve(16);
        _transactionQueues.push_back(moodycamel::ConcurrentQueue<std::function<void()>>());
    }

    ScriptSingleton& operator=(const ScriptSingleton& o)
    {
        return *this;
    }

    void CompleteSystem()
    {
        _systemCompleteCount++;
        if (_systemCompleteCount >= _transactionQueues.size())
        {
            std::lock_guard lock(_queueCreateLock);
            _transactionQueues.push_back(moodycamel::ConcurrentQueue<std::function<void()>>());
        }
    }

    void ResetCompletedSystems()
    {
        _systemCompleteCount = 0;
    }

    void AddTransaction(std::function<void()> const& transaction)
    {
        _transactionQueues[_systemCompleteCount].enqueue(transaction);
    }

    void ExecuteTransactions()
    {
        for (moodycamel::ConcurrentQueue<std::function<void()>>& transactionQueue : _transactionQueues)
        {
            std::function<void()> transaction;
            while (transactionQueue.try_dequeue(transaction))
            {
                transaction();
            }
        }
    }

private:
    std::atomic<u32> _systemCompleteCount;
    std::mutex _queueCreateLock;
    std::vector<moodycamel::ConcurrentQueue<std::function<void()>>> _transactionQueues;
};
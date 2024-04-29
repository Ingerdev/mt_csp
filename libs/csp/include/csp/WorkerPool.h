#ifndef CSP_EXAMPLE_WORKERPOOL_H
#define CSP_EXAMPLE_WORKERPOOL_H

#include <stdexcept>
#include <set>
#include "Worker.h"

namespace csp {
    class WorkerPool {
    public:
        WorkerPool(size_t workers_count,const std::string& pattern) : _pattern(pattern)
        {
            if (workers_count == 0)
                throw std::invalid_argument("Workers count cannot be 0");

            for(size_t k = 0; k!=workers_count;++k)
            {
                //_workers.emplace(pair);
                auto it = _workers.emplace(k,std::unique_ptr<Worker>(new Worker(k)));
                _threads.emplace_back(&Worker::Run,it.first->second.get(),pattern);

                _freeWorkers.emplace(k);
            }
        }

        ~WorkerPool() {
            for(auto& pair: _workers)
            {
                pair.second->get_comm().get_input_channel().push(ParseOrder::make_exit_order());
            }
           for(auto& thread: _threads)
             thread.join();
        }

        bool is_free_workers_available()
        {
            return !_freeWorkers.empty();
        }
        void wait_free_workers()
        {

        }
        WorkerComm aquire_worker()
        {
            if (_freeWorkers.empty())
                throw new std::logic_error("No free workers available");

            auto worker_id = *_freeWorkers.begin();
            return move_worker_between_sets(worker_id,_freeWorkers,_busyWorkers);
        }

        void release_worker(const WorkerComm& workerComm)  {
            auto it = _busyWorkers.find(workerComm.get_worker_id());
            if (it == _busyWorkers.end())
                throw std::logic_error("Cannot release free worker.");

            move_worker_between_sets(*it,_busyWorkers,_freeWorkers);
        }
    private:
        WorkerComm move_worker_between_sets(
                                               size_t worker_id,
                                               std::set<size_t>& left,
                                               std::set<size_t>& right)
        {
            left.erase(worker_id);
            right.emplace(worker_id);
            return _workers[worker_id]->get_comm();
        }

        std::set<size_t> _freeWorkers;
        std::set<size_t> _busyWorkers;
        std::unordered_map<size_t,std::unique_ptr<Worker>> _workers;
        std::vector<std::thread> _threads;
        std::string _pattern;
    };
}

#endif //CSP_EXAMPLE_WORKERPOOL_H

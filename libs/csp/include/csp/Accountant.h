#ifndef CSP_ACCOUNTANT_H
#define CSP_ACCOUNTANT_H

#define NOMINMAX
#include <format>
#include <fstream>
#include <boost/fiber/all.hpp>
#include <boost/fiber/buffered_channel.hpp>
#include <filesystem>
#include <memory>
#include <functional>
#include "utils.h"
#include "Worker.h"
#include "WorkerPool.h"
#include "FileReader.h"
#include "csp/WorkerPool.h"

namespace csp {


    class Accountant {
    public:
        Accountant(const std::string &pattern,
                   const std::string &input_file_name,
                   const std::string &output_file_name,
                   size_t _read_threads_count);

        ~Accountant();

        void run();


    private:
        const size_t AVAILABLE_WORK_MEMORY = 10;//10 * 1024 * 1024; // 100 mb of work memory
        void wait_any(WorkerPool& pool,std::vector<WorkerComm>& workers,std::vector<TaskResult>& results);
        void wait_all(WorkerPool& pool,std::vector<WorkerComm>& workers,std::vector<TaskResult>& results);
        size_t _data_size;
        std::ifstream _input_file;
        std::ofstream _output_file;
        std::string _pattern;

        std::unique_ptr<FileReader> _reader;
        size_t _threads_count;
        size_t _splits_count;
    };
    
    using channel_op_status = boost::fibers::channel_op_status;
    bool wait_any_impl(WorkerPool& pool,
                       std::vector<WorkerComm>& workers,
                       std::vector<TaskResult>& results,
                       std::function<channel_op_status(task_result_channel_t&,
                                                       TaskResult&)> channel_read);

    void write_results(std::ostream& output,
                       std::vector<TaskResult>&,
                       size_t pattern_length);

    bool need_discard_last_match(const TaskResult& prev,
                                 const TaskResult& next,
                                 size_t pattern_length);

}
#endif //CSP_ACCOUNTANT_H

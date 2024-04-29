#include <numeric>
#include "csp/Accountant.h"
#include "csp/WorkerPool.h"

namespace csp {

    Accountant::Accountant(const std::string &pattern,
               const std::string &input_file_name,
               const std::string &output_file_name,
               size_t _read_threads_count) {

        _pattern = pattern;

        _data_size = std::filesystem::file_size(input_file_name);
        if (_data_size == 0)
            throw new std::invalid_argument("Input file cannot be of size 0.");

        _input_file = std::ifstream(input_file_name,std::ios::in | std::ios :: binary);
        if (!_input_file.is_open())
            throw std::invalid_argument(std::format("Could not open file {}", input_file_name));


        if (output_file_name.empty())
        {
            _output_file.set_rdbuf(std::cout.rdbuf());
        }
        else {
            _output_file = std::ofstream(output_file_name);
            if (!_output_file.is_open())
                throw std::invalid_argument(std::format("Could not open file {}", output_file_name));
        }

        _splits_count = csp::calculate_optimal_splits_count(
                _pattern.length(),
                _data_size);

        _threads_count = _read_threads_count != 0
                         ? _read_threads_count
                         : csp::calculate_optimal_threads_count(_splits_count);

        size_t buffer_size = _data_size>AVAILABLE_WORK_MEMORY?AVAILABLE_WORK_MEMORY:_data_size;
        _reader = std::unique_ptr<FileReader>(new FileReader(_input_file,
                                                             buffer_size,
                                                             _pattern.length()));
    }

    Accountant::~Accountant()
    {
        _input_file.close();
        _output_file.close();
    }

    void Accountant::run() {

        auto split_size = std::max(
                AVAILABLE_WORK_MEMORY/_splits_count,
                std::min(_data_size,2*_pattern.length()+1));

        std::vector<TaskResult> results;
        WorkerPool pool{_threads_count,_pattern};
        auto order_id = 0;

        while(_reader->read_update())
        {   auto workers = std::vector<WorkerComm>();
            auto slices = _reader->get_slices(split_size);

            while(slices.size()!=0)
            {
                if (!pool.is_free_workers_available())
                    wait_any(pool,workers,results);

                auto slice = slices.front();
                slices.pop_front();

                auto worker= pool.aquire_worker();
                worker.get_input_channel().push(ParseOrder(order_id++,
                                                           slice.get_global_offset(),
                                                           slice.get_span()));
                workers.push_back(worker);
            }
            //wait all remaining workers because we should rewrite our buffer data.
            wait_all(pool,workers,results);
        }
        write_results(_output_file,results,_pattern.length());
    }

    void Accountant::wait_any(WorkerPool& pool,
                              std::vector<WorkerComm>& workers,
                              std::vector<TaskResult>& results)
    {
        if (wait_any_impl(pool,workers,
                          results,
                          [](task_result_channel_t& channel,
                                  TaskResult& placeholder)
                    {return channel.try_pop(placeholder);}))
            return;

        //fallback to try_pop_timeout
        while(true)
        {
            if (wait_any_impl(pool,workers,
                              results,
                              [](task_result_channel_t& channel,
                                 TaskResult& placeholder)
                              {return channel.pop_wait_for(placeholder,
                                                           std::chrono::microseconds(50));}))
                return;
        }

    }

    void Accountant::wait_all(WorkerPool& pool,
                              std::vector<WorkerComm>& workers,
                              std::vector<TaskResult>& results)
    {
        for(auto worker : workers)
        {
            TaskResult result;
            auto ch_state = worker.get_output_channel().pop(result);
            pool.release_worker(worker);
            if (ch_state ==boost::fibers::channel_op_status::success)
                results.push_back(result);

        }
        workers.clear();
    }

    bool wait_any_impl(WorkerPool& pool,
                                   std::vector<WorkerComm>& workers,
                                   std::vector<TaskResult>& results,
                                   std::function<channel_op_status(task_result_channel_t&,TaskResult&)> channel_read)
    {
        TaskResult result;
        bool found_free_agent = false;
        //ugly implementation of wait_any but boost offers worser solution - using threads for each waitable.
        for(auto it =workers.begin();it!=workers.end();)
        {
            channel_op_status ch_status = channel_read((*it).get_output_channel(),result);

            if (ch_status == channel_op_status::closed)
                throw std::logic_error("Workers shouldn't close channels without order.");

            if (ch_status == channel_op_status::success) {
                found_free_agent = true;
                results.push_back(result);
                pool.release_worker(*it);

                it = workers.erase(it);
            }
            else
                ++it;

        }
        return found_free_agent;
    }

    size_t count_of_matches(const std::vector<TaskResult>& results, size_t pattern_length)
    {
        size_t found_matches = 0;
        if (results.size() == 0)
            return found_matches;

        for(const auto& res:results) {
            {
                auto matches = res.positions();
                found_matches += matches->size();
            }
        }
        return found_matches;
    }

    bool need_discard_last_match(const TaskResult& prev,const TaskResult& next,size_t pattern_length)
    {
        if (prev.tail_chars_used() == 0)
            return false;

        if (next.positions()->empty())
            return false;

        auto first_match =next.positions()->front();
        if (first_match.Line!=0)
            return false;

        if (first_match.Offset>pattern_length-1)
            return false;

        if (first_match.Offset> prev.tail_chars_used())
            return false;
        return true;
    }

    class Stats
    {
    public:
        inline void update_stats(const TaskResult& result)
        {
            if (!result.lines_found())
                return;

            _lines_passed+= result.lines_found();
            _last_line_global_offset = result.get_global_data_offset()+result.get_last_line_offset();
        }
        inline size_t get_global_line_number(const Match& match) const
        {
            return _lines_passed+match.Line;
        }
        size_t get_match_offset(const TaskResult& result, Match& match) const
        {
            if (match.Line!=0)
                return match.Offset;
            return (result.get_global_data_offset()
                    + match.Offset - _last_line_global_offset);
        }
    private:
        size_t _lines_passed{0};
        size_t _last_line_global_offset{0};

    };

    void remove_duplicate_overrides(std::vector<TaskResult>& results,size_t pattern_length)
    {
        auto base = results.begin();
        for(auto it=results.begin();it!=results.end();++it) {
            if (base == it)
                continue;
            if ((*it).positions()->empty())
                continue;

            if (need_discard_last_match(*base, *it, pattern_length))
                (*it).positions()->pop_back();
            else
                base = it;
            }
    }

    void write_results(std::ostream& output,std::vector<TaskResult>& results,size_t pattern_length)
    {
        //calculate total found results:
        std::sort(results.begin(), results.end());
        remove_duplicate_overrides(results,pattern_length);

        output << count_of_matches(results, pattern_length) << std::endl;
        if (results.empty())
            return;

        Stats stats;
        for(auto& result: results)
        {
            auto matches = result.positions();
            if (matches->empty()) {
                stats.update_stats(result);
                continue;
            }

            for(auto match: matches.value())
            {
               output << std::format("id:{} o:{} {} {} {}\n",
                                     result.get_id(),
                                     result.get_global_data_offset(),
                                     stats.get_global_line_number(match) + 1,
                                     stats.get_match_offset(result,match)+ 1,
                                     match.Text);
            }
            stats.update_stats(result);
        }

    }

}
#ifndef CSP_WORKER_H
#define CSP_WORKER_H
#define NOMINMAX
#include <string>
#include <boost/fiber/all.hpp>
#include <vector>
#include <span>
#include "exceptions.h"

namespace csp {

    struct Match {
        Match(size_t line, size_t offset,const std::string& text) : Line(line), Offset(offset),Text(text) {}

        size_t Line;
        size_t Offset;
        std::string Text;
    };

    struct ParseResult
    {
        std::vector<Match> Matches;
        size_t LinesFound;
        size_t LastLineOffset;
        std::optional<size_t> TailCharsUsed;
    };

    class ParseOrder {
    public:
        ParseOrder(const size_t id,
                   const size_t global_data_offset,
                   const std::span<char> data) :
        _id(id),_global_data_offset(global_data_offset), _data(data), _exit(false) {};
        ParseOrder() = default;

        static ParseOrder make_exit_order()
        {
            auto order = ParseOrder();
            order._exit = true;
            return order;
        }

        const std::optional<std::span<char>> &get_data() const {
            return _data;
        }

        size_t get_id() const {
            return _id;
        }
        size_t get_global_data_offset() const {
            return _global_data_offset;
        }

        bool exit_requested() const {
            return _exit;
        }

    private:
        std::optional<std::span<char>> _data;
        size_t _id;
        size_t _global_data_offset;
        bool _exit;
    };


    ParseResult parse(const std::string &pattern, const ParseOrder &order);

    class TaskResult {
    public:
        static TaskResult make_task_result(const ParseOrder &input_order,
                                           const ParseResult& parse_result)
        {
            TaskResult result;
            result._positions = std::move(parse_result.Matches);
            result._id = input_order.get_id();
            result._lines_found = parse_result.LinesFound;
            result._tail_chars_used = parse_result.TailCharsUsed;
            result._global_data_offset = input_order.get_global_data_offset();
            result._last_line_offset = parse_result.LastLineOffset;
            return result;
        }


        std::optional<std::vector<Match>> positions() const {
            return _positions;
        }

        size_t get_id() const {
            return _id;
        }

        size_t lines_found() const{
            return _lines_found;
        }

        size_t get_global_data_offset() const {
            return _global_data_offset;
        }
        size_t get_last_line_offset() const {
            return _last_line_offset;
        }

        std::optional<size_t> tail_chars_used() const
        {
            return _tail_chars_used;
        };

    private:
        std::optional<std::vector<Match>> _positions;
        size_t _id;
        size_t _lines_found;
        size_t _global_data_offset;
        size_t _last_line_offset;
        //when present, overlap region was used as last found match ending and
        //its last char used written here.
        //so, if new region found pattern including this char we will discard last match in
        //previous region. if new found region starts after last used overlap char - then we should
        //keep it.
        std::optional<size_t> _tail_chars_used;
    };


    using task_order_channel_t = boost::fibers::buffered_channel<ParseOrder>;
    using task_result_channel_t = boost::fibers::buffered_channel<TaskResult>;

    class WorkerComm
    {
    public:
        WorkerComm(size_t worker_id,
                   task_order_channel_t &tasks_channel,
                   task_result_channel_t &output_channel)
        :_id(worker_id),_input_channel(&tasks_channel), _output_channel(&output_channel){}

        size_t get_worker_id() const
        {
            return _id;
        }

        task_order_channel_t & get_input_channel()
        {
            return *_input_channel;
        }
        task_result_channel_t & get_output_channel()
        {
            return *_output_channel;
        }

    private:
        size_t _id;
        task_order_channel_t *_input_channel;
        task_result_channel_t *_output_channel;
    };

    class Worker {
    public:
        Worker(size_t id):_id(id),
        _input_channel(new  task_order_channel_t{4}),
        _output_channel(new task_result_channel_t {4})
        {
        }

        Worker(Worker&& worker) = delete;
        Worker(const Worker& worker) = delete;

        size_t get_id()
        {
            return _id;
        }
        WorkerComm get_comm()
        {
            return WorkerComm(_id,*_input_channel.get(),*_output_channel.get());
        }

        void Run(const std::string & pattern) {
            std::scoped_lock lock(_destroy_mutex);
            while (true) {
                ParseOrder order;
                if (_input_channel->pop(order) == boost::fibers::channel_op_status::closed
                || order.exit_requested()) {
                    close_channels();
                    break;
                }

                ParseResult result = parse(pattern, order);
                say_order_done(order, result);
            }
        }
        ~Worker()
        {
            std::scoped_lock lock(_destroy_mutex);
            close_channels();
        }

    private:

        std::mutex _destroy_mutex;
        void close_channels() {
            if (!_output_channel->is_closed())
                _output_channel->close();
            if (!_input_channel->is_closed())
                _input_channel->close();
        }

        void say_order_done(ParseOrder &order, ParseResult parse_result) {
            if (_output_channel->is_closed())
                return;
            _output_channel->push(TaskResult::make_task_result(order, parse_result));
        }

        std::unique_ptr<task_order_channel_t> _input_channel;
        std::unique_ptr<task_result_channel_t> _output_channel;
        const size_t _id;

    };

    enum class ScanResult
    {
        NothingFound,
        LfFound,
        PatternFound,
    };

    bool operator<(const TaskResult& left,const TaskResult& right);
    bool operator==(const TaskResult& left,const TaskResult& right);
    inline ScanResult test_for_pattern(const std::string& pattern,
                                       size_t pattern_length,
                                       std::span<const char> data,size_t &offset);

}
#endif //CSP_WORKER_H

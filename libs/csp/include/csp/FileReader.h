#ifndef CSP_EXAMPLE_FILEREADER_H
#define CSP_EXAMPLE_FILEREADER_H
#include <vector>
#include <span>
#include <memory>
#include <stdexcept>
#include <fstream>

namespace csp {

    struct SliceWithOffset
    {
    public:
        SliceWithOffset(size_t global_offset,std::span<char> span)
                :_global_offset(global_offset),_span(span){}

        std::span<char> get_span() {return _span;}
        //returns offset of the start of span from the start of the data.
        std::size_t get_global_offset(){return _global_offset;}

    private:
        std::size_t _global_offset;
        std::span<char> _span;
    };

    class FileReader
    {
    public:
        FileReader(std::ifstream& input,size_t buffer_size,size_t pattern_size):
                _input(input), _maximum_memory_size(buffer_size), _pattern_size(pattern_size),
                _memory(new char[buffer_size]),
                _actual_data_offset(0),
                _stream_ended(false),
                _first_read(true)
        {

        }
        std::span<char> get_memory()
        {
            return std::span(_memory.get(),_maximum_memory_size);
        }

    void update_global_offset()
    {
        if (_first_read) {
            _actual_data_offset = 0;
            return;
        }

        if (_stream_ended)
            return;

        _actual_data_offset += _data_memory_size - get_tail_size();
    }
        //read another buffer_size chunk of data and split it to elementary spans of work.
        bool read_update()
        {
            if (_stream_ended)
                return false;

            update_global_offset();

            auto memory = get_memory();
            //copied tail data to the start of memory and resized memory span to free mem.
            if (!_first_read)
                memory = move_tail(memory, get_tail_size());

            _input.read(memory.data(),memory.size());
            size_t red_count =_input.gcount();

            if (red_count!=memory.size())
                _stream_ended = true;

            _data_memory_size = red_count;

            if (!_first_read)
                _data_memory_size+=get_tail_size();
            _first_read = false;

            return true;
        }

        std::span<char> move_tail(std::span<char> memory,size_t tail_size) {
            //here we checking did our previous read operation filled whole buffer.
            //if so then we move tail data of pattern_zie-1 to start of the buffer and then
            //read new data after it.
            //instead of changing file pointer, we copy tail data to beginning of memory buffer
            auto tail = memory.subspan(memory.size() - get_tail_size());
            std::memcpy(memory.data(), tail.data(), tail_size);
            memory = memory.subspan(tail_size);
            return memory;
        }

        std::deque<SliceWithOffset> get_slices(size_t slice_size)
        {
            std::deque<SliceWithOffset> result;
            //whole buffer memory
            auto memory = get_memory().subspan(0,_data_memory_size);
            auto left_bytes = _data_memory_size;
            auto current_data_offset = _actual_data_offset;
            while(memory.size()>2*slice_size)
            {
                result.emplace_back(current_data_offset, memory.subspan(0, slice_size));
                //every previous slice "overlap" every next slice by pattern_size-1 bytes.
                memory=memory.subspan(slice_size-_pattern_size+1);
                current_data_offset+= slice_size - _pattern_size + 1;
            }
            if (memory.size()!=0)
                result.emplace_back(current_data_offset, memory);

            return result;
        }
    private:
        inline size_t get_tail_size()
        {
            return _pattern_size - 1;
        }
        std::ifstream& _input;
        std::unique_ptr<char[]> _memory;
        bool _stream_ended;
        bool _first_read;
        size_t _data_memory_size;
        size_t _maximum_memory_size,_pattern_size;
        //offset of the start memory in the data stream.
        size_t _actual_data_offset;
    };
}

#endif //CSP_EXAMPLE_FILEREADER_H

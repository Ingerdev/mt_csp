#include "csp/Worker.h"
namespace csp {

    bool operator<(const TaskResult& left,const TaskResult& right)
    {
        return left.get_id()<right.get_id();
    }
    bool operator==(const TaskResult& left,const TaskResult& right)
    {
        return left.get_id()==right.get_id();
    }


    //scan input data for pattern (from 0 to pattern.length or less if mismatch/CR found earlier).
    //if match return ScanResult::PatternFound
    //if CR found inbetween - updates offset to LF position and return ScanResult::LfFound.
    //if no match and no LF found return ScanResult::NothingFound
    //offset changes only when LF found.
    ScanResult test_for_pattern(const std::string& pattern,size_t pattern_length,std::span<const char> data,size_t &offset)
    {
        static const char LF = '\n';
        size_t c = 0;
        for (; c != pattern_length; ++c) {
            char data_char =data[offset+c];

            if (data_char == LF) {
                offset = offset+c;
                return ScanResult::LfFound;
            }

            char pattern_char = pattern[c];
            if (pattern_char == '?')
                continue;
            if (pattern_char != data_char) {
                return ScanResult::NothingFound;
            }
        }
        return ScanResult::PatternFound;

    }

    ParseResult parse(const std::string &pattern, const ParseOrder &order) {

        auto data = order.get_data().value();
        auto data_length = data.size();
        auto plen = pattern.length();

        ParseResult result;
        if (plen > data_length)
            return result;

        size_t maximum_scan_length = data_length - plen + 1;
        //global offset relating to data start.
        size_t offset = 0;
        //current found lines
        size_t line = 0;
        //line start offset position
        size_t line_offset = 0;
        size_t last_match_end_offset = 0;

        while (offset < maximum_scan_length) {
            switch(test_for_pattern(pattern,plen,data,offset))
            {
                case ScanResult::NothingFound:
                    ++offset;
                    continue;
                case ScanResult::LfFound:
                    //we found that the last plen-1 chunk of the data fragment have CR somewhere
                    //we must not register this line as it will be registered by next data fragment processor.
                    if (offset >maximum_scan_length)
                        break;
                    ++line;
                    line_offset = ++offset;
                    break;
                case ScanResult::PatternFound:
                    result.Matches.emplace_back(line,
                                                offset - line_offset,
                                                std::string(data.data()+offset,plen));
                    offset+=plen;
                    last_match_end_offset = data_length - offset;
                    break;
                default:
                    throw ::NotImplementedException();
            }
        }
        result.LinesFound = line;
        result.LastLineOffset = line_offset;
        if (last_match_end_offset<plen)
            result.TailCharsUsed = plen - last_match_end_offset;
        return result;
    }

}

#ifndef CSP_EXAMPLE_UTILS_H
#define CSP_EXAMPLE_UTILS_H
#include <memory>
namespace csp
{

    inline size_t calculate_optimal_splits_count(size_t
                                          pattern_size,
                                          size_t data_size) {
        const size_t PAGE_SIZE = 4096; //todo: calculate in runtime
        auto data_pattern_ratio = data_size/pattern_size;
        //very low rations:
        if (data_pattern_ratio == 0)
            throw std::invalid_argument("File contain not enough data for even single part");
        if (data_pattern_ratio<3)
            return 1;
        if (data_pattern_ratio<5)
            return 3;

        //ratio increases,memory footprint goes in action.
        auto data_page_ratio = data_size/PAGE_SIZE;
        if (data_page_ratio < 5)
            return 3; //~ 7kb per slice

        if (data_page_ratio < 10)
            return 5; //~ 10kb per slice max

        if (data_page_ratio <100)
            return 10; // ~40kb per slice max

        return data_size/ (100*1024); //~100kb per slice max
    }

    inline size_t calculate_optimal_threads_count(size_t splits) {

        if (splits < 3)
            return 1;
        if (splits < 10)
            return 3;
        return std::thread::hardware_concurrency();
    }
}
#endif //CSP_EXAMPLE_UTILS_H

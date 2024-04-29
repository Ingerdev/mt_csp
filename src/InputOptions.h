#ifndef CSP_INPUTOPTIONS_H
#define CSP_INPUTOPTIONS_H
#include <iostream>
#include "boost/program_options.hpp"


namespace options {
    namespace po = boost::program_options;

    class InputOptions {
    public:
        InputOptions(int ac, char *av[]) {
            _malformed = true;
            po::options_description desc("Allowed options");
            desc.add_options()
                    ("help", "produce help message")
                    (std::format("{},p",PATTERN_PARAMETER_NAME).c_str(), po::value<std::string>(&_pattern),
                     "search pattern")
                    (std::format("{},i",INPUT_FILE_PARAMETER_NAME).c_str(), po::value<std::string>(&_input_file),
                     "input file name")
                    (std::format("{},o",OUTPUT_FILE_PARAMETER_NAME).c_str(), po::value<std::string>(&_output_file),
                     "output file name. Absent value triggers output to console.")
                    (std::format("{},t",THREADS_PARAMETER_NAME).c_str(), po::value<size_t>(&_read_threads)->default_value(static_cast<size_t>(0)),
                     "number of threads that should read and parse file. Absent value means automatic detection.");

            po::variables_map vm;
            po::store(po::command_line_parser(ac, av).
                    options(desc).run(), vm);
            po::notify(vm);

            if (vm.count("help")|| ac == 1) {
                std::cout << "Usage: csp [options]\n";
                std::cout << desc;
                return;
            }

            if (vm.count(PATTERN_PARAMETER_NAME))
                _pattern = vm[PATTERN_PARAMETER_NAME].as<std::string>();
            else {
                std::cout << std::format("{} parameter didnt provided.", PATTERN_PARAMETER_NAME) << std::endl;
                return;
            }

            if (vm.count(INPUT_FILE_PARAMETER_NAME))
                _input_file = vm[INPUT_FILE_PARAMETER_NAME].as<std::string>();
            else{
                std::cout << std::format("{} parameter didnt provided.",INPUT_FILE_PARAMETER_NAME) << std::endl;
                return;
            }

            //if output file not set, we keep name empty to decide in future where to save.
            if (vm.count(OUTPUT_FILE_PARAMETER_NAME))
                _output_file = vm[OUTPUT_FILE_PARAMETER_NAME].as<std::string>();
            else{
                _output_file = std::string();
            }

            if (vm.count(THREADS_PARAMETER_NAME))
                _read_threads = vm[THREADS_PARAMETER_NAME].as<std::size_t>();


            _malformed = false;
        }

        bool OptionsAreValid() {
            return !_malformed;
        };

        const std::string& Pattern()
        {
            return _pattern;
        }
        const std::string& InputFile()
        {
            return _input_file;
        }

        const std::string& OutputFile()
        {
            return _output_file;
        }

        size_t ThreadsCount()
        {
            return _read_threads;
        }
    private:
        const char* PATTERN_PARAMETER_NAME= "pattern";
        const char* INPUT_FILE_PARAMETER_NAME = "input_file";
        const char* OUTPUT_FILE_PARAMETER_NAME = "output_file";
        const char* THREADS_PARAMETER_NAME = "threads";

        std::string _pattern,_input_file,_output_file;
        size_t _read_threads;
        bool _malformed;
    };
}
#endif //CSP_INPUTOPTIONS_H

#include <stdio.h>
#include <fstream>
#include <iostream>
#include "csp/Accountant.h"
#include "InputOptions.h"

int main(int argc,char* argv[]) {

    try {
        auto opts = options::InputOptions(argc,argv);
        if (!opts.OptionsAreValid())
            return -1;

        auto accountant = csp::Accountant(opts.Pattern(),
                opts.InputFile(), opts.OutputFile(),opts.ThreadsCount());

        accountant.run();
    } catch (const std::exception &e) {
        std::cout << e.what();
    }

}


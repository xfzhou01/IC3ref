/*********************************************************************
Copyright (c) 2013, Aaron Bradley

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/

#include <iostream>
#include <string>
#include <time.h>
#include <getopt.h>

extern "C" {
#include "aiger.h"
}
#include "IC3.h"
#include "Model.h"

int main(int argc, char **argv) {
    unsigned int propertyIndex = 0;
    bool basic = false, random = false;
    int verbose = 0;
    bool use_mab = false; // mab
    float alpha = 1.0f; // default value

    int opt;
    while ((opt = getopt(argc, argv, "-:alpha:")) != -1) {
        if (strcmp(argv[optind-1], "-alpha") == 0 && optarg) {
            alpha = atof(optarg);
        }
        else if (string(argv[optind-1]) == "-v")
            // option: verbosity
            verbose = 2;
        else if (string(argv[optind-1]) == "-s")
            // option: print statistics
            verbose = max(1, verbose);
        else if (string(argv[optind-1]) == "-r") {
            // option: randomize the run, which is useful in performance
            // testing; default behavior is deterministic
            srand(time(NULL));
            random = true;
        }
        else if (string(argv[optind-1]) == "-b")
            // option: use basic generalization
            basic = true;
        else if (string(argv[optind-1]) == "-mab")
            // option: enable MAB
            use_mab = true;
        else
            // optional argument: set property index
            propertyIndex = (unsigned) atoi(argv[optind-1]);
    }

    // read AIGER model
    aiger * aig = aiger_init();
    const char * msg = aiger_read_from_file(aig, stdin);
    if (msg) {
        cout << msg << endl;
        return 0;
    }
    // create the Model from the obtained aig
    Model * model = modelFromAiger(aig, propertyIndex);
    aiger_reset(aig);
    if (!model) return 0;

    // model check it
    bool rv = IC3::check(*model, verbose, basic, random, use_mab, alpha);
    // print 0/1 according to AIGER standard
    cout << !rv << endl;

    delete model;

    return 1;
}

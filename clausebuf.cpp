/*********************************************************************
Copyright (c) 2013, Hongce Zhang

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

#include "clausebuf.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

bool ClauseBuf::from_file(const char *fname) {
    ifstream fin(fname);
    if (!fin.is_open()){
        cout << "cannot find file: " << fname << endl;
        return false;
    }
        
    int n_clause = 0;
    {
        char header[128];
        fin.getline(header, 128);
        stringstream buf(header);
        string tmp_unsat;
        buf>>tmp_unsat;
        buf>>n_clause;
    }
    if (n_clause <= 0) {
        cout << "[ERROR] n_clause derived <= 0" << endl;
        return false;
    }
        
    
    char buffer[2048];
    for (int idx = 0; idx < n_clause; ++ idx) {
        clauses.push_back(std::vector<int>());
        fin.getline(buffer, 2048);
        stringstream str(buffer);
        int literal;
        while(str >> literal) {
            clauses.back().push_back(literal);
        }
        if (clauses.back().empty()) {
            cout << "[ERROR] no literal in clause, current idx = " << idx << endl;
            return false;
        }
             
    }
    return true;
}

bool ClauseBuf::from_ckp_string(std::string &ckp_frame)
{
    // strip
    auto start = ckp_frame.find_first_not_of(" \t\n\r\f\v");
    auto end = ckp_frame.find_last_not_of(" \t\n\r\f\v");
    ckp_frame = ckp_frame.substr(start, end - start + 1);;

    bool pushed_new_clause = false;
    int literal_tmp = 0;
    for (int i = 0; i < ckp_frame.size(); i++)
    {
        if(ckp_frame[i] == '\n' && !pushed_new_clause) {
            pushed_new_clause = true;
            this->clauses.push_back(std::vector<int>());
        } 
        else if (isdigit(ckp_frame[i])) {
            if (this->clauses.size() == 0) {
                this->clauses.push_back(std::vector<int>());
            }
            pushed_new_clause = false;
            literal_tmp *= 10;
            literal_tmp += (int)(ckp_frame[i] - '0');
        }
        else if (ckp_frame[i] == ' ') {
            this->clauses.back().push_back(literal_tmp);
            literal_tmp = 0;
        }
    }
    return true;
}

void ClauseBuf::dump() const {
    for (const auto & cls : clauses) {
        for (int lit : cls)
            cout << lit << " ";
        cout << endl;
    }
}


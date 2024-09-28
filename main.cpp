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
#include <string.h>

#include <future>
#include <chrono>


extern "C" {
#include "aiger.h"
}
#include "IC3.h"
#include "Model.h"
#include "clausebuf.h"


void print_help_info() {
  std::cout << "------- Help info -------" << std::endl;
  std::cout << "    -v: verbose = 2" << std::endl;
  std::cout << "    -v3: verbose = 3" << std::endl;
  std::cout << "    -s: print statistics" << std::endl;
  std::cout << "    -r: random" << std::endl;
  std::cout << "    -c <file>: checkpoint file" << std::endl;
  std::cout << "    -f <file>: frame file" << std::endl;
  std::cout << "    -d <file>: dump the inductive invariants to <file> (inv.cnf) by default" << std::endl;
  std::cout << "    -dc <file>: dump the check points to <file> (checkpoint.ckp) by default" << std::endl;
  std::cout << "    -e: in dump invariant file, whether to show variable string or literal number" << std::endl;
  std::cout << "    -i: load aig from file " << std::endl; 
  std::cout << "    -b: use basic generalization" << std::endl;
  std::cout << "    -t <int value>: the IC3 execution time limit" << std::endl;
}

bool is_string_contain_dash(const char *s) {
  int len_str = strlen(s);
  if (len_str == 0) {
    return false;
  } 
  for (int i = 0; i < len_str; i++)
  {
    if (s[i] == '-') {
      return true;
    }
  }
  return false;
}

void extract_frame_segement(const std::string& filename, std::vector<std::string>& parts) {
    std::ifstream file(filename); 
    std::string contents = "";
    if (file) {
        std::stringstream buffer;
        buffer << file.rdbuf();  
        contents = buffer.str();  
    } else {
        std::cout << "cannot open file: " << filename  << std::endl;
    }
    if (contents.size() == 0) {
      cout << "[ERROR] empty checkpoint file name " << filename << endl; 
      assert(false);
    }
    size_t pos = 0;
    size_t end;
    std::string delimiter = "F";
    while ((end = contents.find(delimiter, pos)) != std::string::npos) {
        parts.push_back(contents.substr(pos, end - pos));
        pos = end + delimiter.length();
    }
    parts.push_back(contents.substr(pos));
}

void write_frame_segement(const std::vector<std::vector<std::vector<int>>> &frames_cp,
std::string &out_file_path) {
  // sanity check
  bool valid = frames_cp.back().back().back() == -1;
  assert(valid);
  // write to file
  std::string res_str = "";
  for (auto &ff : frames_cp) {
    for (auto &cls : ff) {
      for (auto lit : cls) {
        if (lit == -1) {
          break;
        }
        res_str += std::to_string(lit);
        res_str += " ";
      }
      res_str += "\n";
    }
    res_str += "F";
    res_str += "\n";
  }
  std::ofstream outFile(out_file_path);
  if (outFile.is_open()) {
        outFile << res_str;
        outFile.close();
    } else {
        std::cerr << "Failed to open the file." << std::endl;
    }
}





int main(int argc, char ** argv) {
  std::cout << "***** IC3Ref *****" << std::endl;
  unsigned int propertyIndex = 0;
  bool basic = false, random = false;
  int verbose = 0;
  bool dump = false;
  bool dump_name = false;
  const char * fname = NULL;
  ClauseBuf clsbuf;
  std::vector<ClauseBuf> checkpoint_clsbuf;
  std::string fname_out = "inv.cnf";
  std::string checkpoint_fname_in = "";
  std::string checkpoint_fname_out = "checkpoint.ckp";

  bool has_time_limit = false;
  bool has_checkpoint = false;

  int max_execution_time_seconds = -1;

  for (int i = 1; i < argc; ++i) {
    if (string(argv[i]) == "-h") {
      print_help_info();
      exit(0);
    }
    if (string(argv[i]) == "-v") {
      // option: verbosity
      verbose = 2;
    }
    else if (string(argv[i]) == "-v3") {
      verbose = 3;
    }
    else if (string(argv[i]) == "-v4") {
      verbose = 4;
    } else if (string(argv[i]) == "-c") {
      has_checkpoint = true;
      if (i+1 >= argc) {
        std::cout << "[INFO] missing checkpoint file name for `-c`" << endl;
        return 0;
      } else if (is_string_contain_dash(argv[i+1])) {
        std::cout << "[INFO] missing checkpoint file name for `-c`"  << endl;
        return 0;
      } else {
        checkpoint_fname_in = argv[++i];
      }
      std::vector<std::string> frame_str_parts;
      extract_frame_segement(checkpoint_fname_in, frame_str_parts);
      for (auto &frame_str : frame_str_parts) {
        ClauseBuf frame_buf_ckp_load;
        frame_buf_ckp_load.from_ckp_string(frame_str);
        if (!frame_buf_ckp_load.is_empty()) {
        checkpoint_clsbuf.push_back(frame_buf_ckp_load);
        }
      }
    }
    else if (string(argv[i]) == "-s") {
      // option: print statistics
      verbose = max(1, verbose);
    }  
    else if (string(argv[i]) == "-r") {
      // option: randomize the run, which is useful in performance
      // testing; default behavior is deterministic
      srand(time(NULL));
      random = true;
    }
    else if (string(argv[i]) == "-d") {
      // option: dump result to inv.cnf
      dump = true;
      if (i+1 >= argc) {
        std::cout << "[INFO] missing dump target file name for `-d`" << endl;
        std::cout << "[INFO] use default as inv.cnf" << endl;
      } else if (is_string_contain_dash(argv[i+1])) {
        std::cout << "[INFO] missing dump target file name for `-d`"  << endl;
        std::cout << "[INFO] use default as inv.cnf" << endl;
      } else {
        fname_out = argv[++i];
      }
    }
    else if (string(argv[i]) == "-dc") {
      // option: dump ckp results to checkpoint.ckp
      dump = true;
      if (i+1 >= argc) {
        std::cout << "[INFO] missing dump target file name for `-dc`" << endl;
        std::cout << "[INFO] use default as checkpoint.ckp" << endl;
      } else if (is_string_contain_dash(argv[i+1])) {
        std::cout << "[INFO] missing dump target file name for `-dc`"  << endl;
        std::cout << "[INFO] use default as checkpoint.ckp" << endl;
      } else {
        checkpoint_fname_out = argv[++i];
      }
    }

    else if (string(argv[i]) == "-e") {
      dump_name = true;
    }
    else if (string(argv[i]) == "-f") {
      // option: load frame from file
      if (i+1 >= argc) {
        std::cout << "missing frame buf file name for `-f`" << endl;
        return 0;
      }
      bool res = clsbuf.from_file(argv[++i]);
      if (!res) {
        std::cout << "Unable to read from " << argv[i] << endl;
        return 2;
      }
      std::cout << "Load " << clsbuf.clauses.size() << " clauses." << endl;
      if (dump)
        clsbuf.dump();
    } else if (string(argv[i]) == "-i") {
      // option: load aig from file
      if (i+1 >= argc) {
        std::cout << "missing aig name for `-i`" << endl;
        return 0;
      }
      fname = argv[++i];
    } else if (string(argv[i]) == "-t") {
      if (i+1 >= argc) {
        std::cout << "[INFO] missing max execution time for IC3 algorithm, ignore" << endl;
      } else if (is_string_contain_dash(argv[i+1])) {
        std::cout << "[INFO] missing max execution time for IC3 algorithm, ignore" << endl;
      } else {
        max_execution_time_seconds = std::stoi(argv[++i]);
        has_time_limit = true;
        std::cout << "[INFO] set run time limit " << max_execution_time_seconds << " seconds" << endl;
      }
    }
    else if (string(argv[i]) == "-b")
      // option: use basic generalization
      basic = true;
    else
      // optional argument: set property index
      propertyIndex = (unsigned) atoi(argv[i]);
  }

  // read AIGER model
  aiger * aig = aiger_init();
  const char * msg;
  if (fname) {
    msg = aiger_open_and_read_from_file(aig, fname);
  } else {
    msg = aiger_read_from_file(aig, stdin);
  }
  cout << "[INFO] finished read aiger" << endl;
  if (msg) {
    cout << msg << endl;
    return 1;
  }
  // create the Model from the obtained aig
  Model * model = modelFromAiger(aig, propertyIndex);
  cout << "[INFO] finished construct model" << endl;
  aiger_reset(aig);
  if (!model) { 
    cout << "[INFO] cannot create model" << endl;
    return 2;
  }

  // model check it


  bool rv;
  std::vector<std::map<int, int>> lvcp;
  std::vector<std::vector<std::vector<int>>> frames_cp;
  if (!has_time_limit) {
    rv = IC3::check(*model, clsbuf,checkpoint_clsbuf,
    verbose, basic, random, dump, dump_name, fname_out.c_str(), &lvcp, &frames_cp);
  } else {
    auto future = std::async(std::launch::async, IC3::check, 
    std::ref(*model), clsbuf,std::ref(checkpoint_clsbuf),
    verbose, basic, random, dump, dump_name, fname_out.c_str(), 
    &lvcp, &frames_cp);
    if (future.wait_for(std::chrono::seconds(max_execution_time_seconds)) == std::future_status::ready) {
        rv = future.get(); 
    } else {
        std::cout << "[INFO] IC3 got timeout at " << max_execution_time_seconds <<" seconds" << std::endl;
        std::cout << "the CTI encountered at stuck point" << std::endl;
        auto &m_tmp = lvcp[lvcp.size() - 2];
        std::cout << ". CTI stat begin:" << std::endl;
        for (auto &p_tmp : m_tmp) {
          std::cout << ". -- VAR " << p_tmp.first << " -- CNT " << p_tmp.second << std::endl;
        }
        std::cout << ". CTI stat end" << std::endl;
        write_frame_segement(frames_cp, checkpoint_fname_out);
        rv = false;
        std::exit(rv);
        return rv;
    }
  }
  cout << "[INFO] finished IC3 check" << endl;
  // print 0/1 according to AIGER standard
  if (rv) {
    std::cout << "[INFO] property proved" << std::endl;
  } else {
    std::cout << "[INFO] find cex or timeout" << std::endl;
  }
  cout << !rv << endl;

  delete model;

  return rv;
}

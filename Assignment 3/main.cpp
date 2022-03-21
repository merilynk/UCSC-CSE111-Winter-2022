// $Id: main.cpp,v 1.13 2021-02-01 18:58:18-08 - - $

#include <cstdlib>
#include <exception>
#include <iostream>
#include <fstream>
#include <string>
#include <libgen.h>
#include <unistd.h>
#include <regex>

using namespace std;

#include "listmap.h"
#include "xpair.h"
#include "util.h"

using str_str_map = listmap<string,string>;
using str_str_pair = str_str_map::value_type;

void scan_options (int argc, char** argv) {
   opterr = 0;
   for (;;) {
      int option = getopt (argc, argv, "@:");
      if (option == EOF) break;
      switch (option) {
         case '@':
            debugflags::setflags (optarg);
            break;
         default:
            complain() << "-" << char (optopt) << ": invalid option"
                       << endl;
            break;
      }
   }
}
   
const string cin_name = "-";
str_str_map lmap;  // initialize a listmap

void parsefile(const string &infile_name, istream &infile) {

   // regex expressions
   regex comment_regex {R"(^\s*(#.*)?$)"};
   regex key_value_regex {R"(^\s*(.*?)\s*=\s*(.*?)\s*$)"};
   regex trimmed_regex {R"(^\s*([^=]+?)\s*$)"};

   smatch result;  // used for regex

   int line_num = 0;  // keep track of line number for printing outputs
   
   for (;;) {

      line_num ++;

      // get line from infile
      string line;
      getline (infile, line);
      if (infile.eof()) return;

      // trim whitespace at the start and the end of the string
      int start = line.find_first_not_of("\t");
      int end = line.find_last_not_of("\t");
      if (line.empty()) line = "";
      else line = line.substr(start, end - start + 1);

      // print the command inputed to console
      cout << infile_name << ": " << line_num << ": " << line << endl;

      // blank line
      if (line.size() == 0) {
         // ignore
      }

      // #
      else if (regex_search (line, result, comment_regex)) {
         // ignore
      }

      // key =, = value, key = value, =
      else if (regex_search (line, result, key_value_regex)) {

         // =
         if (result[1] == "" && result[2] == "") {
            // loop through the map starting from the end
            str_str_map::iterator it;
            for (it = lmap.begin(); it != lmap.end(); ++it) {
               cout << it->first << " = " << it->second << endl;
            }
         }

         // key =
         else if (result[2] == "") {
            // find the key in the map
            string key = result[1];
            str_str_map::iterator it = lmap.find(result[1]);

            // if the key is found, erase the key
            if (it != lmap.end()) {
               lmap.erase(it);
            }
         }

         // = value
         else if (result[1] == "") {
            // loop through the map starting from the end
            // if the value is the same, print out the key-value pair
            str_str_map::iterator it;
            for (it = lmap.begin(); it != lmap.end(); ++it) {
               if (result[2] == it->second) {
                  cout << it->first << " = " << it->second << endl;
               }
            }
         }

         // key = value
         else {
            string key = result[1];
            string value = result[2];
            cout << key << " = " << value << endl;

            // create a pair
            str_str_pair kv_pair(key, value);

            // insert the pair into the map
            lmap.insert(kv_pair);
         }
      }

      // key
      else if (regex_search (line, result, trimmed_regex)) {

         string key = result[1];

         // find the key in the map
         str_str_map::iterator it = lmap.find(result[1]);

         // if key wasn't found print "key: key not found"
         if (it == lmap.end()) {
            cout << key << ": key not found" << endl;
         }
         // else get the value mapped to the key and print
         else {
            cout << key << " = " << (*it).second << endl;
         }
      }
   }
   
}

int main (int argc, char** argv) {
   sys_info::execname (argv[0]);
   scan_options (argc, argv);

   if (argc >= 2) {
      for (char** argp = &argv[optind]; argp != &argv[argc]; ++argp) {
         string filename = *argp;
         if (filename == cin_name) {
            parsefile(cin_name, cin);
            continue;
         }
         ifstream infile;
         infile.open(filename);
         if (infile.is_open() == false) {
            syscall_error(filename);
            continue;
         }
         parsefile(filename, infile);
         infile.close();
      }
   }
   else {
      parsefile(cin_name, cin);
   }
   
   lmap.~listmap();
   return EXIT_SUCCESS;
}


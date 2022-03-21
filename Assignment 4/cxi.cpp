// $Id: cxi.cpp,v 1.6 2021-11-08 00:01:44-08 - - $

#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <vector>
using namespace std;

#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

#include "debug.h"
#include "logstream.h"
#include "protocol.h"
#include "socket.h"

logstream outlog (cout);
struct cxi_exit: public exception {};

string command_to_string (cxi_command command) {
   switch (command) {
      case cxi_command::ERROR  : return "ERROR"  ;
      case cxi_command::EXIT   : return "EXIT"   ;
      case cxi_command::GET    : return "GET"    ;
      case cxi_command::HELP   : return "HELP"   ;
      case cxi_command::LS     : return "LS"     ;
      case cxi_command::PUT    : return "PUT"    ;
      case cxi_command::RM     : return "RM"     ;
      case cxi_command::FILEOUT: return "FILEOUT";
      case cxi_command::LSOUT  : return "LSOUT"  ;
      case cxi_command::ACK    : return "ACK"    ;
      case cxi_command::NAK    : return "NAK"    ;
      default                  : return "????"   ;
   };
}

unordered_map<string,cxi_command> command_map {
   {"get", cxi_command::GET},
   {"put", cxi_command::PUT},
   {"rm", cxi_command::RM},
   {"exit", cxi_command::EXIT},
   {"help", cxi_command::HELP},
   {"ls"  , cxi_command::LS  },
   {"error", cxi_command::ERROR},
};

static const char help[] = R"||(
exit         - Exit the program.  Equivalent to EOF.
get filename - Copy remote file to local host.
help         - Print help summary.
ls           - List names of files on remote server.
put filename - Copy local file to remote host.
rm filename  - Remove file from remote server.
)||";

void cxi_get (client_socket& server, string filename) {
   cxi_header header;
   header.command = cxi_command::GET;
   //cout << command_to_string(header.command) << endl;
   if (filename.size() > 58) {
      cout << 
         "File names longer than 58 characters are prohibited." 
         << endl;
      throw cxi_exit();
   }
   for (size_t i = 0; i < filename.size(); i++) {
      header.filename[i] = filename[i];
   }
   //cout << "file to get: " << header.filename << endl;
   DEBUGF ('h', "sending header " << header << endl);
   send_packet (server, &header, sizeof header);
   recv_packet (server, &header, sizeof header);
   DEBUGF ('h', "received header " << header << endl);
   if (header.command != cxi_command::FILEOUT) {
      cout <<  "sent GET, server did not return FILEOUT" << endl;
      cout << "server returned " << header << endl;
   } else {
      size_t host_nbytes = ntohl (header.nbytes);
      auto buffer = make_unique<char[]> (host_nbytes + 1);
      recv_packet (server, buffer.get(), host_nbytes);
      DEBUGF ('h', "received " << host_nbytes << " bytes");
      buffer[host_nbytes] = '\0';
      ofstream recieved_file (header.filename, ofstream::binary);
      recieved_file.seekp(0);
      recieved_file.write(buffer.get(), host_nbytes);
      cout << filename << ": sucessfully got file" << endl;
   }
}

void cxi_put (client_socket& server, string filename) {
   cxi_header header;
   header.command = cxi_command::PUT;
   //cout << command_to_string(header.command) << endl;
   if (filename.size() > 58) {
      cout << 
         "File names longer than 58 characters are prohibited." 
         << endl;
      throw cxi_exit();
   }
   for (size_t i = 0; i < filename.size(); i++) {
      header.filename[i] = filename[i];
   }
   //cout << "file to put: " << header.filename << endl;
   ifstream file_to_send (header.filename, ifstream::binary);
   if (file_to_send) {
      file_to_send.seekg(0, file_to_send.end);
      int size = file_to_send.tellg();
      file_to_send.seekg(0, file_to_send.beg);
      char * buffer = new char [size];
      file_to_send.read(buffer, size);
      //cout << "nbytes of header: " << size << endl;
      //cout << "size of header: " << sizeof header << endl;
      header.nbytes = ntohl(size);
      DEBUGF ('h', "sending header " << header << endl);
      send_packet (server, &header, sizeof header);
      send_packet (server, buffer, size);
      recv_packet (server, &header, sizeof header);
      DEBUGF ('h', "received header " << header << endl);
      file_to_send.close();
      delete[] buffer;
   } else {
      cout << filename 
         << ": "
         << strerror (errno) << endl;
      header.nbytes = ntohl(errno);
      return;
   }

   if (header.command == cxi_command::NAK) {
      cout << "sent PUT, server did not return ACK" << endl;
      cout << "server returned " << header << endl;
      cout << strerror(ntohl(header.nbytes)) << endl;
   } else if (header.command == cxi_command::ACK) {
      cout << filename << ": sucessfully put file" << endl;
   }
}


void cxi_rm (client_socket& server, string filename) {
   cxi_header header;
   header.command = cxi_command::RM;
   //cout << command_to_string(header.command) << endl;
   if (filename.size() > 58) {
      cout << 
         "File names longer than 58 characters are prohibited." 
         << endl;
      throw cxi_exit();
   }
   for (size_t i = 0; i < filename.size(); i++) {
      header.filename[i] = filename[i];
   }
   //cout << "file to rm: " << header.filename << endl;
   header.nbytes = ntohl(0);
   DEBUGF ('h', "sending header " << header << endl);
   send_packet (server, &header, sizeof header);
   recv_packet (server, &header, sizeof header);
   DEBUGF ('h', "received header " << header << endl);
   if (header.command == cxi_command::NAK) {
      cout << "sent RM, server did not return ACK" << endl;
      cout << "server returned " << header << endl;
   } else if (header.command == cxi_command::ACK) {
      cout << filename << ": sucessfully removed file" << endl;
   }
}

void cxi_help() {
   cout << help;
}

void cxi_ls (client_socket& server) {
   cxi_header header;
   header.command = cxi_command::LS;
   DEBUGF ('h', "sending header " << header << endl);
   send_packet (server, &header, sizeof header);
   recv_packet (server, &header, sizeof header);
   DEBUGF ('h', "received header " << header << endl);
   if (header.command != cxi_command::LSOUT) {
      outlog << "sent LS, server did not return LSOUT" << endl;
      outlog << "server returned " << header << endl;
   }else {
      size_t host_nbytes = ntohl (header.nbytes);
      auto buffer = make_unique<char[]> (host_nbytes + 1);
      recv_packet (server, buffer.get(), host_nbytes);
      DEBUGF ('h', "received " << host_nbytes << " bytes");
      buffer[host_nbytes] = '\0';
      cout << buffer.get();
   }
}


void usage() {
   cerr << "Usage: " << outlog.execname() << " host port" << endl;
   throw cxi_exit();
}

pair<string,in_port_t> scan_options (int argc, char** argv) {
   for (;;) {
      int opt = getopt (argc, argv, "@:");
      if (opt == EOF) break;
      switch (opt) {
         case '@': debugflags::setflags (optarg);
                   break;
      }
   }
   if (argc - optind != 2) usage();
   string host = argv[optind];
   in_port_t port = get_cxi_server_port (argv[optind + 1]);
   return {host, port};
}

int main (int argc, char** argv) {
   outlog.execname (basename (argv[0]));
   outlog << to_string (hostinfo()) << endl;
   try {
      auto host_port = scan_options (argc, argv);
      string host = host_port.first;
      in_port_t port = host_port.second;
      outlog << "connecting to " << host << " port " << port << endl;
      client_socket server (host, port);
      outlog << "connected to " << to_string (server) << endl;
      for (;;) {
         string line;
         string contents;
         vector <string> command;
         getline (cin, line);
         //cout << "line entered " << line << endl;
         stringstream lineSS (line);
         while (getline(lineSS, contents, ' ')) {
            command.push_back(contents);
         }
         if (cin.eof()) throw cxi_exit();
         const auto& itor = command_map.find (command[0]);
         cxi_command cmd = itor == command_map.end()
                         ? cxi_command::ERROR : itor->second;
         switch (cmd) {
            case cxi_command::EXIT:
               throw cxi_exit();
               break;
            case cxi_command::HELP:
               cxi_help();
               break;
            case cxi_command::LS:
               cxi_ls (server);
               break;
            case cxi_command::GET:
               cxi_get(server, command[1]);
               break;
            case cxi_command::PUT:
               cxi_put(server, command[1]);
               break;
            case cxi_command::RM:
               cxi_rm(server, command[1]);
               break;
            default:
               outlog << line << ": invalid command" << endl;
               break;
         }
      }
   }catch (socket_error& error) {
      outlog << error.what() << endl;
   }catch (cxi_exit& error) {
      DEBUGF ('x', "caught cxi_exit");
   }
   return 0;
}


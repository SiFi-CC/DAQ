#include <string.h>
#include "CommandLine.h"
#include <stdio.h>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

void help_command_line(const char* my_name)
{
  std::cerr << "\nUsage: " << my_name << "  [options]\n"
            << "    Positional arguments: None\n\n"
            << "Valid options are:\n"
            << "  -t <type>     Snapshot type, seconds or events.\n"
            << "  -i <interval> Snapshot interval.\n"
            << std::endl;
  return;
}


//bool isNumber(std::string s);
//----------------------------------------------------------------------
bool isNumber(const char* c)
{
  for ( std::size_t i=0; i<strlen(c); i++){
    if(!isdigit(c[i])) return false;
  }
  return true;
}

//----------------------------------------------------------------------
int check_arguments(ARGUMENTS& arguments){
    if(arguments.snapshot_interval < 0){
        fprintf(stderr, "Snapshot interval is equal or less than zero.\n");
        return 1;
    }

  return 0; //success
}

//----------------------------------------------------------------------
void PrintLocation()
{
  static int count=0;
  std::cout << "At location" << count++ << std::endl;
}
//----------------------------------------------------------------------
int analyze_command_line (int argc, char **argv, ARGUMENTS& arguments)
{
  // Inialise the arguments to be consistently meaningless
  arguments.snapshot_type="";
  arguments.snapshot_interval=0;

  // Now loop over all the arguments
  // There are a minimum of seven arguments:
  // -i, -m, -o, and corresponding files, and
  // program name.
  if(argc < 5 || std::string(argv[1]) == "--help") {
    help_command_line(argv[0]);
    return 1;
  }

  for(int i=1; i<argc; /* incrementing of i done in the loop! */){
    if(argv[i][0] != '-'){
      std::cerr << "ERROR: Wrong argument " << argv[i] << std::endl;
      help_command_line(argv[0]);
      return 1;
    }
    
   if(strlen(&argv[i][1]) != 1){ 
     std::cerr << "ERROR: All options must be single characters, "
               << "separated with a space and prefixed with a '-'"
               << std::endl;
      help_command_line(argv[0]);
      return 1;
   }

   switch(argv[i][1]){
   case 't':
     if(i+1 < argc){
       arguments.snapshot_type = argv[i+1];
       i+=2;
     }
     else{
       std::cerr << "ERROR: No argument for snapshot type specified\n";
       help_command_line(argv[0]);   return 1;
     }
     break;

     //----------
   case 'i':
     if(i+1 < argc){
       if(isNumber(argv[i+1])){
         arguments.snapshot_interval = atoi(argv[i+1]);
         i+=2;
       }
       else{
         std::cerr << "ERROR: Argument " << argv[i+1]
                   << " for option -i is not a number\n";
         help_command_line(argv[0]);   return 1;
       }
     }
     break;

     //----------
   default:
     std::cerr << "ERROR: Argument " << argv[i] << " not recognized\n";
     help_command_line(argv[0]);   return 1;
   } // End switch block
  } // End for loop over all args

  // Everything looks ok, so we now check the arguments
  return check_arguments(arguments);
}

//----------------------------------------------------------------------
void print_arguments(const ARGUMENTS& args){
  std::cout << "ARGUMENTS struct:"
            << "\n    snapshot type:\t\t" << args.snapshot_type
            << "\n    snapshot interval:\t" << args.snapshot_interval
            << std::endl;
}


#ifndef COMMANDLINE_H
#define COMMANDLINE_H
#include <string>
/** @file */
/// arguments
struct ARGUMENTS {
    /// type
    std::string snapshot_type;
    /// interval
    int snapshot_interval;
};// ARGUMENTS;
/// help command line
void help_command_line(const char* my_name);
/// print arguments
void print_arguments(const ARGUMENTS& args);
/// is number
bool isNumber(const char* c);
/// check arguments
int check_arguments(ARGUMENTS& arguments);
/// analyze command line
int analyze_command_line (int argc, char **argv, ARGUMENTS& arguments);

#endif //COMMANDLINE_H

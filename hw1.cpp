#include <stdio.h>
#include <vector>
#include <string>
#include <cstring>
#include <getopt.h>

#include "lsof.h"

int GetOpt(int argc, char* argv[], 
        std::vector<std::string> &commandFilter,
        std::vector<std::string> &typeFilter,
        std::vector<std::string> &filenamesFilter)
{
    int flags, opt;
    int nsecs, tfnd;
    std::string filter;
    nsecs = 0;
    tfnd = 0;
    while ((opt = getopt(argc, argv, "c:t:f:h")) != -1) {
        switch (opt) {
        case 'c':
            commandFilter.push_back(optarg);
            break;
        case 't':
            if (strcmp(optarg, "REG") == 0 ||
                strcmp(optarg, "CHR") == 0 ||
                strcmp(optarg, "DIR") == 0 || 
                strcmp(optarg, "FIFO") == 0 || 
                strcmp(optarg, "SOCK") == 0 ||
                strcmp(optarg, "unknown") == 0)
                {
                    typeFilter.push_back(optarg);
                }
            else
            {
                fprintf(stderr, "Invalid TYPE option.\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'f':
            filenamesFilter.push_back(optarg);
            break;
        case 'h':
            printf("====help======\n");
            printf("-c REGEX: a regular expression (REGEX) filter for filtering command line. For example -c sh would match bash, zsh, and share.\n");
            printf("-t TYPE: a TYPE filter. Valid TYPE includes REG, CHR, DIR, FIFO, SOCK, and unknown. TYPEs other than the listed should be considered invalid. \n");
            printf("-f REGEX: a regular expression (REGEX) filter for filtering filenames.\n");
            printf("==============\n");
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-c command_filter] [-t type_filter] [-f filenames_filter]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

int main(int argc, char* argv[]){
    std::vector<std::string> commandFilter, typeFilter, filenamesFilter;
    int err = GetOpt(argc, argv, commandFilter, typeFilter, filenamesFilter);
    if (err == -1) return err;
    Lsof lsof(commandFilter, typeFilter, filenamesFilter);
    lsof.run();
    lsof.show();
    // printf("size %ld %ld %ld\n", commandFilter.size(), typeFilter.size(), filenamesFilter.size());
    return 0;
}
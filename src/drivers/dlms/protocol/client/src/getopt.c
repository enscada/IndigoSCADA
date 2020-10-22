/* *****************************************************************
*
* Copyright 2016 Microsoft
*
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
******************************************************************/

#include "../include/getopt.h"
#include <string.h>

char* optarg = 0;
int optind = 1;

int getopt(int argc, char *const argv[], const char *optstring)
{
	int opt;
    const char *p;

    if ((optind >= argc) || (argv[optind][0] != '-') || (argv[optind][0] == 0))
    {
        return -1;
    }
    opt = argv[optind][1];
    p = strchr(optstring, opt);

    if (p == 0)
    {
        return '?';
    }
    ++optind;
    if (p[1] == ':')
    {
        if (optind >= argc)
        {
            optarg = (char*)p;
            return '?';
        }
        optarg = argv[optind];
        ++optind;
    }
    return opt;
}

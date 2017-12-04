/**
 * @souravra_assignment3
 * @author  sourav ranu <souravra@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/global.h"
#include "../include/connection_manager.h"





/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
  /*Start Here*/
    sscanf(argv[1], "%" SCNu16, &CONTROL_PORT);
    running_app = TRUE;
    periodic_timer = {1, 0};
    init(); // Initialize connection manager; This will block]
    return 0;
}

void lprint(const char* format, ...) {
    va_list args;

    va_start(args, format);
    FILE* file = fopen("log_pa3.txt", "a");
    vfprintf(file, format, args);
    fclose(file);
    va_end(args);
}

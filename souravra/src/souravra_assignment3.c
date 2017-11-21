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

    int stdout_copy = dup(STDOUT_FILENO);
    fflush(stdout);
    int nf = fileno(fopen("pa3_red.log","w"));
    dup2(nf, STDOUT_FILENO);

    /* do something with stdout */
    printf("test]\n");
    init(); // Initialize connection manager; This will block
    fflush(stdout);
    dup2(stdout_copy, STDOUT_FILENO);
    close(stdout_copy);
	return 0;
}

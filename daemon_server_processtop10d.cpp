/* daemon_server_processtop10d.cpp

Its a data producer daemon.
The daemon consults and stores the 10 processes that most consumes CPU in the server

Copyright (c) 2019, irbaran. 
All rights reserved.

Permission to use, copy, modify, and distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright
notice and this permission notice appear in all copies.

The software is provided "as is", without warranty of any kind, express or
implied, including but not limited to the warranties of merchantability,
fitness for a particular purpose and noninfringement of third party rights. 
In no event shall the authors or copyright holders be liable for any claim,
damages or other liability, whether in an action of contract, tort or
otherwise, arising from, out of or in connection with the software or the use
or other dealings in the software.

Except as contained in this notice, the name of a copyright holder shall not
be used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization of the copyright holder.
 */

#include <stdio.h> 
#include <string.h>
#include <stdlib.h> 
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>  
#include <netinet/in.h> 
#include <fcntl.h>
using namespace std;

//Define functions
static void server_processtop10d(void);

//////////////////////////
// daemon server_processtop10d
// It consults and stores the 10 processes that most consumes server CPU
/////////////////////////
static void server_processtop10d(void)
{
	FILE *fp_top10= NULL;
	FILE *fp_lock= NULL;	
	string top_cmd = "top -b -n1 -Em -o+%CPU -s -w85 -c -1";   // command top options:
                                                               // -b  --Batch-mode operation
                                                               // -n1 --number-of-iterations equals 1
                                                               // -Em --show memory in megabytes
                                                               // -o+%CPU ---sort processes by CPU percentage use
                                                               // -s --enable secure-mode operation
                                                               // -w85 --define width of colums to show
                                                               // -c --show only program-name
                                                               // -1 --show cpu info in a single line
	string sed_qualifier = "sed -n '7,17 p'";                  // Use sed to cut desired lines from file
	string filename = "server_proctop10_filed.txt";            // File to store server processes information
	string cli_cmd = top_cmd + " | " + sed_qualifier + " > " + filename;  // Format string system command
	char *cli_cmd_c = const_cast<char*>(cli_cmd.c_str());                 // cast string to char*
	int ret;                                                   // funtion return
	const char *filename_lock = "server_proctop10_filed.lock"; //Filename lock
	
	// process pid_processtop10 that will become daemon
	pid_t pid_processtop10 = 0;

	/* Fork off the parent process */
	pid_processtop10 = fork();

	/* check fork */
	if (pid_processtop10 < 0)
	{
		/* When an error occurred */
		cout << "Error server_processtop10d: fork from parent failed!\n";
		exit(EXIT_FAILURE);
	}

	/* Let the parent process terminate */
	if (pid_processtop10 > 0)
		exit(EXIT_SUCCESS);

	/* Child process session become process leader */
	if (setsid() < 0)
	{
		/* An error occurred */
		cout << "Error server_processtop10d: Became child process as leader failed!\n";
		exit(EXIT_FAILURE);
	}
	
	/* Fork off for the second time to ensure that the daemon will not re-acquire a tty*/
	pid_processtop10 = fork();

	/* check fork */
	if (pid_processtop10 < 0)
	{
		/* When an error occurred */
		cout << "Error server_processtop10d: 2nd fork from parent failed!\n";
		exit(EXIT_FAILURE);
	}

	/* Let the parent (first child) terminate */
	if (pid_processtop10 > 0)
		exit(EXIT_SUCCESS);

	//unmask the file mode
	umask(0);
	
	/* Change the working directory to the root/tmp directory */
	chdir("/tmp");

	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	
	cout << "daemon server_processtop10d initialized!\n";

	while (1) {
		//Create a lock file for synchronism while deamon is writing 
		fp_lock = fopen (filename_lock, "a+"); 
		fclose(fp_lock);
		
		/* Ask server SO about processes most consume CPU*/
		ret = system(cli_cmd_c);
		
		//Remove lock file after file update
		remove(filename_lock);			
		
		if ( ret == 0 )		
		{
			// Open a log file in case of errros from top command.
			fp_top10 = fopen ("Log_server_processtop10d.txt", "a+");
			time_t my_time = time(NULL);
			fprintf(fp_top10, "server_processtop10d system command ERROR - %s\n", ctime(&my_time));
			fflush(fp_top10);
			fclose(fp_top10);
		}
		sleep(5); /* time to updated file (IPC using shared memory method)*/
	}
	exit(EXIT_SUCCESS);	
}

//////////////////////////
//Main function
/////////////////////////
int main(int argc, char* argv[])
{
	//Init daemon
	server_processtop10d();
	
	exit(EXIT_SUCCESS);
}
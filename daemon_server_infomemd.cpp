/* daemon_server_infomemd.cpp

Its a data producer daemon.
The daemon process consults and stores the server memory status (available and used)

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
static void server_infomemd(void);

//////////////////////////
// daemon server_infomemd
// It consults and stores the server memory status (available and used)
/////////////////////////
static void server_infomemd(void)
{
	FILE *fp_infomemd= NULL;
	FILE *fp_lock= NULL;
	string free_cmd = "free -m -t";                        // command free options:
                                                           // -m  --show data in megabytes
                                                           // -t  --show total memory (RAM + swap)
	string filename = "server_freemem_filed.txt";          // File to store server memory information
	string cli_cmd = free_cmd + " > " + filename;          // Format string system command
	char *cli_cmd_c = const_cast<char*>(cli_cmd.c_str());  // cast string to char*
	int ret;                                               // funtion return
	const char *filename_lock = "server_freemem_filed.lock"; //Filename lock

	// process pid_infomem that will become daemon
	pid_t pid_infomem = 0;

	/* Fork off from parent process */
	pid_infomem = fork();

	/* check fork */
	if (pid_infomem < 0)
	{
		/* When an error occurred */
		cout << "Error server_infomemd: fork from parent failed!\n";
		exit(EXIT_FAILURE);
	}

	/* Let the parent process terminate */
	if (pid_infomem > 0)
		exit(EXIT_SUCCESS);

	/* Child process session become process leader */
	if (setsid() < 0)
	{
		/* When an error occurred */
		cout << "Error server_infomemd: Became child process as leader failed!\n";
		exit(EXIT_FAILURE);
	}

	/* Fork off for the second time to ensure that the daemon will not re-acquire a tty*/
	pid_infomem = fork();

	/* check fork */
	if (pid_infomem < 0)
	{
		/* When an error occurred */
		cout << "Error server_infomemd: 2nd fork failed!\n";
		exit(EXIT_FAILURE);
	}

	/* Let the parent (first child) terminate */
	if (pid_infomem > 0)
		exit(EXIT_SUCCESS);

	//unmask the file mode
	umask(0);

	/* Change the working directory to the root/tmp directory */
	chdir("/tmp");

	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	cout << "daemon server_infomemd initialized!\n";

	while (1) {
		
		//Create a lock file for synchronism while deamon is writing 
		fp_lock = fopen (filename_lock, "a+"); 
		fclose(fp_lock);
		
		/* Ask server SO about memory use*/
		ret = system(cli_cmd_c);
		
		//Remove lock file after file update
		remove(filename_lock);	
		
		if ( ret != 0 )
		{
			// Open a log file in case of errros.
			fp_infomemd = fopen ("Log_server_infomemd.txt", "a+");
			time_t my_time = time(NULL);
			fprintf(fp_infomemd, "server_processtop10d system command ERROR - %s\n", ctime(&my_time));
			fflush(fp_infomemd);
			fclose(fp_infomemd);
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
	server_infomemd();
	
	exit(EXIT_SUCCESS);
}
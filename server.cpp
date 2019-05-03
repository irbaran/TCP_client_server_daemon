/* server.cpp

Its a server side code which receives multiple TCP socket messages from multiple clients.
The server handles multiple socket connections with select and fd_set functinality on Linux.

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
#include <errno.h> 
#include <iostream>
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h> 
#include <sys/time.h> 
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

//define for socket
#define TRUE 1 
#define FALSE 0 
#define SOCKET_PORT 7000 

//define for socket services
#define PROCESS_TOP10_SEL 1
#define SERVER_INFOMEM_SEL 2

//Main status errors
#define RUNSTS_ERROR        0    // Run main status error
#define RUNSTS_OK           1    // Run main status ok

//functions
int	init_daemons(void);
int consume_deamon_data(char *filename, char (&file_buffer)[1025]);
int init_socket(struct sockaddr_in sock_addr, int max_clients);
int switch_cmd_convert(char *buffer);

//Constants
const char *server_processtop10 = "-server_processtop10";
const char *server_infomem = "-server_infomem";


//////////////////////////
//Initialize daemon services server_processtop10d and server_infomemd
/////////////////////////
int	init_daemons(void)
{
	//deamons variables
	const char *server_infomemd_cmd = "./server_infomemd.exe";           //daemon to inform server memory
	const char *server_processtop10d_cmd = "./server_processtop10d.exe"; //daemon to inform server processes

	int error_sts = RUNSTS_OK;     // internal status error
	
	if ( system(server_infomemd_cmd) != 0 )
	{
		cout << "Error when initialize daemon server_infomemd !\n";
		error_sts = RUNSTS_ERROR;
	}
	else
		cout << "daemon server_infomemd initialized!\n";

	//Initialize daemon server_processtop10d_cmd 
	if ( system(server_processtop10d_cmd) != 0 )
	{
		cout << "Error when initialize daemon server_processtop10d_cmd !\n";
		error_sts = RUNSTS_ERROR;
	}
	else
		cout << "daemon server_processtop10d_cmd initialized!\n";

	return error_sts;
}

//////////////////////////
// Function consume_deamon_data()
// It reads and consumes data files produced by daemons
/////////////////////////
int consume_deamon_data(char *filename, char (&file_buffer)[1025] )
{
	int error_sts = RUNSTS_OK;              // internal status error
	int file;                               // file descriptor
	int file_read_size;                     // Number of bytes of file read
	int count_fread = 0;                    // Counter of file read attempts
	const int num_fileread_retries = 5;     // Number of file read retries
	int file_lock;                          // file lock descriptor
	char filename_lock[45] = {0};           // File name for respectively lock
	
	//Check filename argument
	if ( filename[0] != '\0' )
	{
		//Get filename lock from argment filename
		strncpy(filename_lock, filename, strlen(filename) - 3);
		strcat(filename_lock, "lock\0");

		do
		{
			//Check if filename is locked by producer writings
			file_lock = open(filename_lock,O_RDONLY);
			if (file_lock < 0)
			{
				//if filename is available, open and read it.
				file = open(filename,O_RDONLY);
				if(file < 0)
				{
					error_sts = RUNSTS_ERROR;
					if (count_fread == num_fileread_retries)
					{
						cout << "Error: Unable to open file after tries" << filename << endl;
						file_buffer[0] = '\0';
					}
				} 
				else
				{
					//Read information from file
					file_read_size = read(file,file_buffer,sizeof(file_buffer));
					if (file_read_size <= 0) 
					{
						error_sts = RUNSTS_ERROR;
						if (count_fread == num_fileread_retries)
						{
							cout << "Error: Invalid data read from file after tries" << filename << endl;
							strcpy(file_buffer, "Error: Invalid data read from file\n\0");
						}
					}
					else
					{
						error_sts = RUNSTS_OK;
					}
					close(file);
				}
			}
			else
			{
				close(file_lock);
				error_sts = RUNSTS_ERROR;
				if (count_fread == num_fileread_retries)
				{
					cout << "Error: filename is locked after tries" << filename << endl;
					strcpy(file_buffer, "Error: filename is lock at server\n\0");
				}				
				
			}
		}
		while ( (error_sts == RUNSTS_ERROR) && (count_fread++ <= num_fileread_retries) );		
	}
	else
	{
		cout << "Error consume_deamon_data(): empty filename argument !\n";
		error_sts = RUNSTS_ERROR;
	}

	return error_sts;
}

//////////////////////////
// Function init_socket()
// It initializes and configures server connection and return the socket
/////////////////////////
int init_socket(struct sockaddr_in sock_addr, int max_clients)
{ 
	int error_sts = RUNSTS_OK;      // internal function status error
	
	//Socket variables
	int main_socket;                // Main socket
	int ret_tmp;                    // Temporary return
	int sock_opt = TRUE;            // Socket option 

	//create a main socket 
	main_socket = socket(AF_INET , SOCK_STREAM , 0);
	if( main_socket == 0) 
	{ 
		perror("Error: main socket failed");
		error_sts = RUNSTS_ERROR;
		return error_sts;
	}

	//set main socket in order to allow multiple connections
	ret_tmp = setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt, sizeof(sock_opt));
	if( ret_tmp  < 0 ) 
	{ 
		perror("Error: set main socket option failed");
		error_sts = RUNSTS_ERROR;
		return error_sts; 
	}

	//bind the socket to localhost port 7000 
	ret_tmp = bind(main_socket, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
	if (ret_tmp < 0) 
	{ 
		perror("Error: bind main socket failed");
		error_sts = RUNSTS_ERROR;
		return error_sts; 
	}
	printf("Listener on port %d \n", SOCKET_PORT); 

	//try to specify maximum pending connections for the main socket 
	ret_tmp = listen(main_socket, max_clients); 
	if (ret_tmp  < 0) 
	{
		perror("Error: listen main socket failed");
		error_sts = RUNSTS_ERROR;
		return error_sts;  
	} 
	cout << "Waiting for connections ...\n";
	
	return main_socket;
}

//////////////////////////
// Function switch_cmd_convert()
// It converts receved client command buffer string to an integer for switch selection
/////////////////////////
int switch_cmd_convert(char *buffer)
{
	char *buffer_aux = buffer;		// Auxiliary buffer 
	char *char_pointer;				// Pointer for each character of buffer	
	int len = 0;					// Buffer cumputed length	
	
	// Cut off CR and LP characters from buffer
	for (char_pointer = buffer_aux; *char_pointer != 0; char_pointer++)
	{
		if ((*char_pointer == '\r') || (*char_pointer == '\n'))
		{
			//Ends string if found a line feed (LF = \n = 0x0A) or Carriage Return (CR = '\n' = 0x0D) 
			buffer_aux[len] = '\0';
			break;
		}
		len++;
	}
	
	//Return an integer based on string command received
	if ( strcmp(buffer_aux, server_processtop10) == 0 ) 
		return PROCESS_TOP10_SEL;
	else if	(strcmp(buffer_aux, server_infomem) == 0 ) 
		return SERVER_INFOMEM_SEL;
	else
		return 0;
}


//////////////////////////
// server main function
/////////////////////////
int main(int argc , char *argv[]) 
{ 
	int main_error_sts = RUNSTS_OK; // Main function status error
	int main_error_ststmp;          // Main temporary status error
	
	//Socket variables
	int main_socket;                // Main socket
	int main_socket_addrlen;        // Main socket address length
	int new_socket;                 // New socket accept
	int ret_tmp;                    // Temporary return
	int max_clients = 100;          // Maximum of client sockets
	int client_socket[max_clients]; // Client income sockets
	int sock_activity;              // Main socket activity
	int client;                     // client iterator
	int read_msg;                   // Read message value from clients
	int sock_dcpt;                  // socket descriptor
	int max_sd;                     // Maximum of socket descriptor
	struct sockaddr_in sock_addr;   // Socket address input structure
	char curr_buffer[1025] = {0};   // Current buffer from socket. 1 kbyte size.
	string echo_client_info;		// Echo client communication information
	bool invalid_req;				// Invalid request information
	const char *init_server_msg = "Server OK and listening\r\n";  //Initial greeting message to clients
	
	//file related variables
	char file_buffer[1025] = {0};   // File buffer read. 1 kbyte size.
	char filename_sel[40] = {0};    // File name selected
	
	//Initialize daemons 
	main_error_sts = init_daemons();
	
	//type of socket created 
	sock_addr.sin_family = AF_INET;             //IPv4 protocol
	sock_addr.sin_addr.s_addr = INADDR_ANY;     //localhost
	sock_addr.sin_port = htons( SOCKET_PORT );

	//Initialize main server socket	
	main_socket = init_socket(sock_addr, max_clients);	
	if (main_socket == 0) 
	{
		perror("Error: main socket failed");
		main_error_sts = RUNSTS_ERROR;
		return main_error_sts; 
	}
	
	//set of socket descriptors 
	fd_set read_fd_sock; 
	
	//initialise all client_socket[] to 0 so not checked 
	for (client = 0; client < max_clients; client++) 
	{ 
		client_socket[client] = 0; 
	} 
	
	//Gets Main socket address length
	main_socket_addrlen = sizeof(sock_addr); 
	
	while(TRUE) 
	{ 
		//clear the socket set 
		FD_ZERO(&read_fd_sock); 
	
		//add main socket to set 
		FD_SET(main_socket, &read_fd_sock); 
		max_sd = main_socket; 
			
		//add income child sockets to set 
		for ( client = 0 ; client < max_clients ; client++) 
		{ 
			//socket descriptor 
			sock_dcpt = client_socket[client]; 
				
			//if valid socket descriptor then add it to income client list 
			if(sock_dcpt > 0) 
				FD_SET( sock_dcpt , &read_fd_sock); 
				
			//file descriptor maximum number. Necessary for select function
			if(sock_dcpt > max_sd) 
				max_sd = sock_dcpt; 
		} 
	
		//wait indefinitely for activity on one of the sockets (timeout is NULL). Stay listen.
		sock_activity = select( max_sd + 1 , &read_fd_sock , NULL , NULL , NULL); 
	
		if ((sock_activity < 0) && (errno!=EINTR)) 
		{ 
			printf("Error: select error found");
			main_error_sts = RUNSTS_ERROR;
		} 
			
		// If something happened on the main socket, 
		// its an new client connection. Accept and include it to array of clients.
		if (FD_ISSET(main_socket, &read_fd_sock)) 
		{ 
			new_socket = accept(main_socket, (struct sockaddr *)&sock_addr, (socklen_t*)&main_socket_addrlen);
			if ( new_socket < 0 ) 
			{ 
				perror("Error: accept socket"); 
				main_error_sts = RUNSTS_ERROR;
				return main_error_sts; 
			} 
			
			//inform user of socket number - used in send and receive commands 
			printf("New client connection, socket fd is %d, ip is : %s, port : %d \n",
				new_socket , inet_ntoa(sock_addr.sin_addr) , ntohs(sock_addr.sin_port)); 
		
			//send for a new connection client an intial greeting message 
			ret_tmp = send(new_socket, init_server_msg, strlen(init_server_msg), 0);
			if( ret_tmp != strlen(init_server_msg) )
			{ 
				perror("Error: during send greeting message");
				main_error_sts = RUNSTS_ERROR;
			} 
				
			cout << "Welcome message sent successfully for new client\n"; 
				
			//add new socket to array of sockets 
			for (client = 0; client < max_clients; client++) 
			{ 
				//if position is empty 
				if( client_socket[client] == 0 ) 
				{ 
					client_socket[client] = new_socket; 
					printf("Adding to list of sockets as client %d\n" , client); 
					break; 
				} 
			} 
		} 
			
		//Also keeps checking for any receive operation on some other socket
		for (client = 0; client < max_clients; client++) 
		{ 
			sock_dcpt = client_socket[client];
			
			// check if something happened on each client file descritor socket
			if (FD_ISSET( sock_dcpt , &read_fd_sock)) 
			{ 
				//read the incoming message 
				read_msg = read( sock_dcpt , curr_buffer, 1024);
		
				//Check if it a close client section request. Log it and close. 
				if (read_msg <= 0) 
				{ 
					//Some client disconnected, get his details and show 
					getpeername(sock_dcpt, (struct sockaddr*)&sock_addr, (socklen_t*)&main_socket_addrlen); 
					printf("Client disconnected, ip %s, port %d \n" , 
						inet_ntoa(sock_addr.sin_addr) , ntohs(sock_addr.sin_port)); 
						
					//Close the client socket and mark as 0 in list for reuse 
					close( sock_dcpt ); 
					client_socket[client] = 0; 
				}
				//if it is not a close message, treat income message request and respond to client
				else
				{ 
					//Clean arrays
					invalid_req = false;
					memset(filename_sel, 0, sizeof(filename_sel));
					memset(file_buffer, 0, sizeof(file_buffer));
					
					if( curr_buffer[0] == '-' )
					{ 
						//Capture client command request 
						switch( switch_cmd_convert(curr_buffer) )
						{
							//case "-server_processtop10":
							case PROCESS_TOP10_SEL:
								strcpy(filename_sel, "/tmp/server_proctop10_filed.txt\0");
								break;
							//case "-server_infomem":
							case SERVER_INFOMEM_SEL:
								strcpy(filename_sel, "/tmp/server_freemem_filed.txt\0");
								break;
							//next cases - next options of client requests
							// ...
							default:    // Invalid argument
								invalid_req = true;
								break;
						}
					}
					else
					{
						invalid_req = true;
					}
					
					//When client request is invalid, log and inform client
					if (invalid_req) 
					{
						//prepare invalid message to inform client
						strcpy(file_buffer, "Invalid client request, use commands -server_processtop10 or -server_infomem\n\0");
						
						//Log in server the client with invalid request 
						getpeername(sock_dcpt ,(struct sockaddr*)&sock_addr, 
							(socklen_t*)&main_socket_addrlen);
						printf("Invalid request from client connected with ip %s and port %d .\n" , 
							inet_ntoa(sock_addr.sin_addr) , ntohs(sock_addr.sin_port));
					}
					// When client request is valid, return information produced by deamons.
					else
					{						
						main_error_ststmp = consume_deamon_data(filename_sel, file_buffer);
						main_error_sts = main_error_sts && main_error_ststmp;
					}
					
					// Send the requested information to client
					if (file_buffer[0] != '\0')
					{
						ret_tmp = send(sock_dcpt , file_buffer , strlen(file_buffer) , 0 );
						if( ret_tmp != strlen(file_buffer ) ) 
						{ 
							perror("Error: during send requested information to client");
							main_error_sts = RUNSTS_ERROR;
						}
					}
					else
					{
						perror("Error: file_buffer failed");
						main_error_sts = RUNSTS_ERROR;						
					}
				} 
			} 
		} 
	} 

	return main_error_sts;
} 



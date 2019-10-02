/******************************************************************************************************************************************************
Header Files:
stdio.h -> defines variable types, macros, functions to perform input and output; string.h -> defines functions for manipulating arrays of characters; stdlib.h -> defines functions for performing general functions; unistd.h -> provides access to the POSIX operating system API; sys/types.h -> defines the socket functions; sys/socket.h -> defines the function and API used to create socket; netinet/in.h -> to store the address information; arpa/inet.h -> header makes available the type in_port_t, in_addr structure and the type in_addr_t as defined in the description of <netinet/in.h>; netdb.h -> defines the hostent structure; pthread.h -> The POSIX thread libraries are a standards based thread API which allows one to spawn a new concurrent process flow.
*******************************************************************************************************************************************************/
#include <stdio.h>					
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>		
#include <sys/socket.h>		
#include <netinet/in.h>		
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

/********************************************************************************************************************************************************
Error function:
in-built function that interprets the error number and outputs the error description STDERR (standard error free). We are using this function at various point in our program wherever at the worse situation, the program nedds to get terminated, so we included exit(1) in this function.
********************************************************************************************************************************************************/
void error(const char *msg)
{
	perror(msg);		
	exit(1);
}

void *group_handler(void *file);		
int main(int argc, char *argv[])				//command line arguments to hold the filename, portnumber.
{
	int client_socketfd, server_socketetfd;		        //file descriptors for server and client to store socket information
	int client_length, portno;             			//integer to hold the length of client address and port number
	char buffer[1000], hostname[100], msg_response[1000];	//buffer to send the message, to store host name, server response
	char host[INET_ADDRSTRLEN];				//Length of the string form for IP address
		

	struct sockaddr_in server_addr;				//specifies the address for the server socket in structure sockaddr_in
	pthread_t read_thread;					//thread to read the other server response

	//terminates the program if argc have few arguments
	if(argc < 3)
	{
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(1);
	}

	strcpy(hostname, argv[1]);				//copy the string given as second argument in argv to a 100 bytes buffer
	portno = atoi(argv[2]);					//converts the port number in string format to integer value
	client_socketfd = socket(AF_INET,SOCK_STREAM,0);	//creating socket. AF_INET -> internet family, domain of the socket (internet socket).
								//SOCK_STREAM -> type of the socket (tcp), 0 -> default protocol (tcp).	

	//terminates the code if socket connection failed
	if(client_socketfd < 0)
		error("Error opening the socket! \n");

	printf("Successfully established socket connection! \n");
	
	//clearing the structure by making it zero
	memset(&server_addr, '0', sizeof(server_addr));

	//after clean up, fill the fields of the structure with what we need
	server_addr.sin_family = AF_INET;			//specifies the address of the family which should be used for socket.
	server_addr.sin_port = htons(portno);			//converts the port number to the dataformat that the structure can understand.
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");	//specifies the ip address of the server.

	//terminates the program if the socket connection is not accepted by server
	if(connect(client_socketfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
		error("Connection not established");

	//function to convert IPv4 and IPv6 addresses from binary to text form
	inet_ntop(AF_INET, (struct sockaddr *) &server_addr, host, INET_ADDRSTRLEN);
	printf("connected to %s, start chatting\n", host);

	//pthread to connect multiple users in a group
	pthread_create(&read_thread, NULL, group_handler, &client_socketfd);

	//while loop to send a message in a group chat
	while(1) 
	{
		bzero(buffer, 1000);				//clears the buffer before sending any message
		fgets(buffer, 1000, stdin);			//read user input and stores that in the buffer
		strcpy(msg_response, hostname);			//copies the hostname to the string names msg_response
		//appends a copy of string pointed by s2 (including the terminating null character) to the end of the string pointed to by s1
		strcat(msg_response, ":");			
		strcat(msg_response, buffer);

		//writes the message on to the buffer
		client_length = write(client_socketfd, msg_response, strlen(msg_response));

		//terminates the program if message sending fails
		if(client_length < 0) 
			error("Error sending message \n");
	
		//fill the buffer and msg_response with zeros to free the buffer
		memset(buffer,'0', sizeof(buffer));
		memset(msg_response,'0', sizeof(msg_response));
	}
	pthread_join(read_thread, NULL);
	close(client_socketfd);					//close the socket connection

}

void *group_handler(void *file)
{
	int server_socketfd = *((int *)file);
	int message_length;
	char group_chat[2000];
	
       //connection happened, the users broadcasts their message from the buffer. It is an infine loop until the user disconnects the socket connection
	while(1) 
	{
		//recieves the response from the server
		message_length = recv(server_socketfd, group_chat, sizeof(group_chat), 0);

		//terminates the code if the response is not recieved
		if(message_length < 0)
			error("Message not recieved \n");

		group_chat[message_length] = '\0';
		//writes the null-terminated string the stream. The terminating null byte shall not be written.
		fputs(group_chat, stdout);	
		//frees the buffer by fill it with zeros	
		memset(group_chat,'0', sizeof(group_chat));
	}
}


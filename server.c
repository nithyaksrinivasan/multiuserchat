/*****************************************************************************************************************************************************
Header Files:
stdio.h -> defines variable types, macros, functions to perform input and output; string.h -> defines functions for manipulating arrays of characters; stdlib.h -> defines functions for performing general functions; unistd.h -> provides access to the POSIX operating system API; sys/types.h -> defines the socket functions; sys/socket.h -> defines the function and API used to create socket; netinet/in.h -> to store the address information; arpa/inet.h -> header makes available the type in_port_t, in_addr structure and the type in_addr_t as defined in the description of <netinet/in.h>; netdb.h -> defines the hostent structure; pthread.h -> The POSIX thread libraries are a standards based thread API which allows one to spawn a new concurrent process flow.
*****************************************************************************************************************************************************/
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

/*****************************************************************************************************************************************************
Error function:
/in-built function that interprets the error number and outputs the error description STDERR (standard error free). We are using this function at various point in our program wherever at the worse situation, the program nedds to get terminated, so we included exit(1) in this function.
*****************************************************************************************************************************************************/
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

/****************************************************************************************************************************************************
Structure to store client details. We used file descriptors to store the socket informations. We used buffer to send the messages and we used another buffer to get the name of the host.
****************************************************************************************************************************************************/
struct user_information 
{
	int socket_id;
	char host[INET_ADDRSTRLEN];
	char hostname[50];
};

void *msg_handler(void *socketfd);

int numberOfClients[200];					//integer array to store number of users
int size = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;		//mutex intializer for statically alloacted variables

//Function to broadcast the message sent by one client to all the users in the group
void broadcast_msg(char *chat,int recent_user)
{
	int write_info;						//integer variable to write a message
	pthread_mutex_lock(&mutex);				//mutex object referenced by mutex in locked state

	for(int i = 0; i < size; i++) 
	{
		if(numberOfClients[i] != recent_user) 
		{
			write_info = send(numberOfClients[i], chat, strlen(chat), 0);
			
			if(write_info < 0) 
			{
				error("Message cannot send");
				continue;
			}
		}
	}
	pthread_mutex_unlock(&mutex);				//release the mutex object referenced by mutex
		
}

//Main Function
int main(int argc,char *argv[])				//command line arguments to hold the filename, portnumber.
{
	int socket_fd, new_socket_fd, portno;		
	char host[INET_ADDRSTRLEN];
		
	struct user_information user;
	struct sockaddr_in server_addr,client_addr;	//specifies the address for the server socket and client socket.

	socklen_t client_length;			//32-bit data type used to specify the length of client address.
	pthread_t read_thread;

	//terminates the program if argc have few arguments
	if(argc < 2)
	{
		fprintf(stderr, "Port Number is not provided. Program Terminated! \n");
		exit(1);
	}
	
	portno = atoi(argv[1]);				//converts the port number in string format to integer value
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);	//creating socket. AF_INET -> internet family, domain of the socket (internet socket).
							//SOCK_STREAM -> type of the socket (tcp), 0 -> default protocol (tcp).	

	//terminates the code if socket connection failed
	if(socket_fd < 0)
		error("Error opening the Socket");

	//clearing the structure by fill it with zero
	memset(server_addr.sin_zero, '0', sizeof(server_addr.sin_zero));

	////after clean up, fill the fields of the structure with what we need
	server_addr.sin_family = AF_INET;		//specifies the address of the family which should be used for socket.
	server_addr.sin_port = htons(portno);		//converts the port number to the dataformat that the structure can understand.
	server_addr.sin_addr.s_addr = INADDR_ANY;	//specifies the ip address.
	client_length = sizeof(client_addr);		//size of the client address
	
	//bind function -> file descriptor, typecast server address from sockaddr_in into sockaddr, size
	if(bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
		error("Binding Failed!");
	
	listen(socket_fd, 5);				//listen for sockets with file descriptor, number of connection it can listen parameters.

	//print statements on server side
	printf("********************  ********************  ******************** \n");
	printf("*****Project Discussion ***** \n");
	printf("***** Chat Room is Open ***** \n");
	printf("Waiting for users to enter! \n");
	printf("********************  ********************  ******************** \n");

/***************************************************************************************************************************************************
The communication starts between client and server and continues untill the connection gets exit. The accept statement is inside the while loop and the following code executes as an infinite loop. After a connection is established, call pthread to create a new process. The child process will close sockfd and dostuffs, passing the new socket file descriptor as an argument. When the two processes have completed their conversation, this process simply exits. The parent process closes newsockfd. Because all of this code is in an infinite loop, it will return to the accept statement to wait for the next connection.
****************************************************************************************************************************************************/
	while(1) 
	{
		//accepts the connection and stores in a  -> file descriptor, typecast the client address
		new_socket_fd = accept(socket_fd, (struct sockaddr *) &client_addr, &client_length);	
		
		//terminates the program if the connction is not accepted	
		if(new_socket_fd < 0)
			error("Socket connection cannot be established \n");

		//return with the mutex object referenced by mutex in locked state with the calling thread as its owner.
		pthread_mutex_lock(&mutex);	
		
		//function to convert IPv4 and IPv6 addresses from binary to text form
		inet_ntop(AF_INET, (struct sockaddr *) &client_addr, host, INET_ADDRSTRLEN);
		
		//gets the name of the client host
		printf("Enter your name: \n");
		scanf("%s", user.hostname);
		printf("%s entered to the chat group with address %s \n", user.hostname, host);

		user.socket_id = new_socket_fd;
		strcpy(user.host, host);					//copy the string arguement of second parameter to first
		numberOfClients[size] = new_socket_fd;
		size++;
		pthread_create(&read_thread, NULL, msg_handler, &user);		//creates pthread to handle multiple clients
		pthread_mutex_unlock(&mutex);					//release the mutex object referenced by mutex
		
	}
	
	return 0;
}

//thread handler function to handle multiple users
void *msg_handler(void *socket)
{
	struct user_information user = *((struct user_information *)socket);
	char user_msg[1000];
	int msg_length;
	
	//server recieves the socket connection form the users
	while((msg_length = recv(user.socket_id,user_msg,1000,0)) > 0) 
	{
		user_msg[msg_length] = '\0';
		broadcast_msg(user_msg, user.socket_id);
		memset(user_msg, '0', sizeof(user_msg));
	}
	
	pthread_mutex_lock(&mutex);

	printf("%s left the chat room with address %s \n", user.hostname, user.host);
		
	for(int i = 0; i < size; i++) 
	{
		if(numberOfClients[i] == user.socket_id) 
		{
			int j = i;
			while(j < size-1) 
			{
				numberOfClients[j] = numberOfClients[j+1];
				j++;
			}
		}
		
	}
	size--;
	pthread_mutex_unlock(&mutex);
	
}

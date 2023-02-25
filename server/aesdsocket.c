/*
 * @File name (aesdsocket.c)
 * @File Description: (implementation of IPC communication using sockets)
 * @Author Name (AYSWARIYA KANNAN)
 * @Date (02/24/2023)
 * @Attributions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <arpa/inet.h>

/**********************/
#define MAX_BACKLOG (10)
#define BUFFER_SIZE (100)

/*** GLOBALS *******************************/
char *server_port = "9000";					 // given port for communication
char *file_data = "/var/tmp/aesdsocketdata"; // file to save input string
char *output_buffer = NULL;

int socket_fd = 0; // socket file descriptor
int accept_fd = 0; // cleint accept file descriptor
void socket_connect(void);


/*SIGNAL HANDLER*/
/*
 * @function	:  Signal handler function for handling SIGINT,SIGTERM and SIGKILL
 *
 * @param		:  int signal_no : signal number
 * @return		:  NULL
 *
 */
void signal_handler(int signal_no)
{
	if (signal_no == SIGINT || signal_no == SIGTERM || signal_no == SIGKILL)
	{
		printf("signal detected to exit\n");
		syslog(LOG_DEBUG, "Caught the signal, exiting...");
		unlink(file_data); // delete the file
		free(output_buffer);
		close(accept_fd);
		close(socket_fd);
	}
	exit(EXIT_SUCCESS);
}


/*MAIN FUNCTION*/
/*
 * @function	: main fucntion for Socket based communication
 *
 * @param		: argc -refers to the number of arguments passed, and argv[] is a pointer array which points to each argument passed
 * @return		: return 0 on success
 *
 */
int main(int argc, char *argv[])
{

	openlog(NULL, 0, LOG_USER); // open log to the default location var/log/syslog with LOG_USER

	syslog(LOG_DEBUG, "syslog opened."); // indicating logging
	// to associate signal handler with corresponding signals using signal() API
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGKILL, signal_handler);

	// if any command line arguments given to specify to run as deamon
	if ((argc > 1) && (!strcmp("-d", (char *)argv[1])))
	{

		printf("Run in daemon mode.\n");
		syslog(LOG_DEBUG, "AESDSOCKET Application in daemon mode");

		// to set as daemon
		int temp_daemon = daemon(0, 0);
		if (temp_daemon == -1)
		{
			printf("Couldn't process into deamon mode\n");
			syslog(LOG_ERR, "failed to enter deamon mode %s", strerror(errno));
		}
	}
	socket_connect();
	closelog();
	return 0;
}
/*SOCKET COMMUNICATION FUNCTION*/
/*
 * @function	:  To handle socket communication
 *
 * @param		:   struct addrinfo *res holds the local address for binding,
 * 					int socket_fd- socket file descriptor and int accept_fd -file descriptor of client
 * @return		:  NULL
 *
 */

void socket_connect()
{

	// setting the initial paramters
	struct addrinfo hints;		  // for getaddrinfo parameters
	struct addrinfo *res;		  // to get the address
	struct sockaddr client_add;	  // to get client address
	socklen_t client_size;		  // size of sockaddr
	char buff[BUFFER_SIZE] = {0}; // buffer to receive input string
	memset(buff, 0, BUFFER_SIZE);
	char addr_ip[INET_ADDRSTRLEN]; // to save the ip address
	int data_count = 0;			   // for counting the data packet bytes

	// Step-1 Initilaizing the  sockaddr using getaddrinfo

	// clear the hints first
	memset(&hints, 0, sizeof(hints));

	// set all the hint parameters then
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	printf("Assigning address for socket\n");
	// getting address in res uding getaddrinfo
	int temp = getaddrinfo(NULL, server_port, &hints, &res);
	if (temp != 0)
	{
		printf("Error while allocating address for socket\n");
		syslog(LOG_ERR, "Error while setting socket address= %s. Exiting.", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Step-2 Opening socket
	printf("Opening socket\n");
	socket_fd = socket(PF_INET6, SOCK_STREAM, 0); // IPv6 with type SOCK_STREAM and 0 protocol
	if (socket_fd == -1)
	{
		printf("Error: Socket file descriptor not created\n");
		syslog(LOG_ERR, "Error while setting socket= %s. Exiting.", strerror(errno));
		freeaddrinfo(res);
		exit(EXIT_FAILURE);
	}
	// Step-3 Binding to address
	printf("Binding socket descriptor to address\n");
	int temp2 = bind(socket_fd, res->ai_addr, res->ai_addrlen);
	if (temp2 == -1)
	{
		printf("Error: Binding with address failed\n");
		syslog(LOG_ERR, "Error while binding socket= %s. Exiting.", strerror(errno));
		freeaddrinfo(res);
		exit(EXIT_FAILURE);
	}

	// All recived data should be stored in the PATH file_data
	int file_fd = creat(file_data, 0644);
	if (file_fd == -1)
	{
		printf("Error while creating file \n");
		syslog(LOG_ERR, "Error: File could not be created!= %s. Exiting...", strerror(errno));
		exit(EXIT_FAILURE);
	}
	close(file_fd);

	// free res when no longer needed
	freeaddrinfo(res);

	// Step-4 Listening and Accepting packets in a loop
	// Loop for listening and accepting data from client
	while (1)
	{

		output_buffer = (char *)malloc(BUFFER_SIZE); // mallocing output buffer to send data to file
		if (output_buffer == NULL)
		{
			printf("buffer not created\n");
			exit(EXIT_FAILURE);
		}
		memset(output_buffer, 0, BUFFER_SIZE);

		// Listening for client
		int temp_listen = listen(socket_fd, MAX_BACKLOG);
		if (temp_listen == -1)
		{
			printf("Error while listening \n");
			syslog(LOG_ERR, "Error: Listening failed =%s. Exiting ", strerror(errno));
			exit(EXIT_FAILURE);
		}

		client_size = sizeof(struct sockaddr);

		// Accepting connection
		accept_fd = accept(socket_fd, (struct sockaddr *)&client_add, &client_size);
		if (accept_fd == -1)
		{
			printf("Error while accepting \n");
			syslog(LOG_ERR, "Error: Accepting failed =%s. Exiting ", strerror(errno));
			exit(EXIT_FAILURE);
		}
		// to get the client address in a reaadble format

		struct sockaddr_in *addr_read = (struct sockaddr_in *)&client_add;
		inet_ntop(AF_INET, &(addr_read->sin_addr), addr_ip, INET_ADDRSTRLEN);

		// Logging the client connection and address
		syslog(LOG_DEBUG, "Connection succesful. Accepting connection from %s", addr_ip);
		printf("Connection succesful.Accepting connection from %s\n", addr_ip);

		int i;
		bool receive = false; // set the completion of packets
		while (receive == false)
		{

			int temp_recv = recv(accept_fd, buff, BUFFER_SIZE, 0);
			if (temp_recv < 0)
			{
				printf("Error while receving data packets\n");
				syslog(LOG_ERR, "Error: Receiving failed =%s. Exiting ", strerror(errno));
				exit(EXIT_FAILURE);
			}
			else if (temp_recv == 0)
			{ // if no bytes received it will return 0
				break;
			}

			for (i = 0; i < BUFFER_SIZE; i++)
			{
				if (buff[i] == 10)
				{
					receive = true;
					i++;
					printf("data packet receiving completed\n");
					syslog(LOG_DEBUG, "data packet received");
					break;
				}
			}
			data_count += i;
			output_buffer = (char *)realloc(output_buffer, (data_count + 1));
			if (output_buffer == NULL)
			{
				printf("buffer not created\n");
				exit(EXIT_FAILURE);
			}

			strncat(output_buffer, buff, i);
			memset(buff, 0, BUFFER_SIZE);
		}
		char *s = output_buffer; // to print the input characters
		while (*s != '\n')
		{
			printf("%c", *s);
			s++;
		}
		printf("\n");
		// writing data to the file given in the path
		file_fd = open(file_data, O_APPEND | O_WRONLY);
		if (file_fd == -1)
		{
			printf("Error: Opening file\n");
			syslog(LOG_ERR, "Error: Opening file %s. Exiting ", strerror(errno));
			exit(EXIT_FAILURE);
		}

		int temp_write = write(file_fd, output_buffer, strlen(output_buffer));
		if (temp_write == -1)
		{
			printf("Error: File failed to be written \n");
			syslog(LOG_ERR, "Error:  File failed to be written= %s. Exiting ", strerror(errno));
			exit(EXIT_FAILURE);
		}
		else if (temp_write != strlen(output_buffer))
		{
			printf("Error: File not completely written\n");
			syslog(LOG_ERR, "Error: File not completely written=%s. Exiting ", strerror(errno));
			exit(EXIT_FAILURE);
		}

		close(file_fd);

		// sending received data to the client back
		memset(buff, 0, BUFFER_SIZE);
		file_fd = open(file_data, O_RDONLY);
		if (file_fd == -1)
		{
			printf("Error: Opening file\n");
			syslog(LOG_ERR, "Error: Opening file %s. Exiting ", strerror(errno));
			exit(EXIT_FAILURE);
		}

		char *send_buffer;
		send_buffer = (char *)malloc(sizeof(char) * (data_count + 1));
		memset(send_buffer, 0, data_count + 1);

		int temp_read = read(file_fd, send_buffer, data_count + 1);

		if (temp_read == -1)
		{
			printf("Error: reading failed\n");
			syslog(LOG_ERR, "Error:  read from file failed = %s. Exiting ", strerror(errno));
			exit(EXIT_FAILURE);
		}

		printf("sending\n");
		int temp_send = send(accept_fd, send_buffer, strlen(send_buffer), 0);
		if (temp_send == -1)
		{
			printf("Error: sending failed\n");
			syslog(LOG_ERR, "Error:  sending to client failed= %s. Exiting ", strerror(errno));
			exit(EXIT_FAILURE);
		}

		close(file_fd);
		free(output_buffer);
		free(send_buffer);
		// closing the connection
		syslog(LOG_DEBUG, "Closing connection from %s", addr_ip);
		printf("Closed connection from %s\n", addr_ip);
	}
	close(accept_fd);
	close(socket_fd);
}

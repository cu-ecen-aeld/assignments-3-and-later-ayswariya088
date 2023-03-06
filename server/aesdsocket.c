/**********************************************************************************************************************************
 * @File name (aesdsocket.c)
 * @File Description: (implementation of IPC communication using sockets for Assignment 6 P1)
 * @Author Name (AYSWARIYA KANNAN)
 * @Date (02/24/2023)
 * @Attributions :https://www.binarytides.com/socket-programming-c-linux-tutorial/
 * 				  https://www.tutorialspoint.com/c_standard_library/c_function_strerror.htm
 *                https://stackoverflow.com/questions/1276294/getting-ipv4-address-from-a-sockaddr-structure
 * 				  https://www.geeksforgeeks.org/socket-programming-cc/
 **************************************************************************************************************************/


#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include "queue.h"

#define PORT_NO             9000

//Thread to print time stamps
pthread_t timer_thread=(pthread_t)NULL;

//Boolean flag to indicate if the process needs to be termintaed
bool g_terminateFlag=false;

int sockfd=-1, clientfd=-1; //file descriptors of client and the server's socket
int fd=-1; //file descriptor for /var/tmp/aesdsocketdata

int daemonize = 0; //Used to check if the "-d" flag is set. If set, the process is daemonised

//Function that is used to daemonize this process if -d argument was passed
static void daemonize_process();

//Mutex that is used to synchronise data reads and writes
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct thread_data_t
{
    pthread_t thread_id; //thread id 
    int clientfd; //client file descriptor
    int fd; //File descriptor to read and write
    bool thread_complete;

    // This macro does the magic to point to other nodes
    TAILQ_ENTRY(thread_data_t) next;
    
};

typedef struct thread_data_t thread_data;
thread_data *data = NULL;

// This macro creates the data type for the head of the queue
// for nodes of type 'struct node'
TAILQ_HEAD(head_s, thread_data_t) head;

//Function to terminate gracefully
void graceful_exit();

/*
* Brief - Signal handler for SIGINT and SIGTERM
*/
void signal_handler(int signo)
{
    //Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received
    syslog(LOG_USER,"Caught signal, exiting\n");

    if(sockfd > 0)
    {
        shutdown(sockfd,SHUT_RDWR);
        close(sockfd);
    }

    g_terminateFlag = true;

}

/*
* Brief- Thread function that interacts with the client
*/
static void * threadFunc(void *arg)
{
    thread_data * data = (thread_data *) arg;

    //Paramters used in recieving data
    char temp;
    unsigned long idx=0;
    int n;
    char *temp_buff = NULL; //pointer used to store recieved data through socket

    //Initialise a temp buffer to read data
    temp_buff = (char *)malloc(sizeof(char));

    //Lock the mutex
    int ret = pthread_mutex_lock(&mutex);
    if(ret!=0)
        perror("pthread_mutex_lock");

    //Start receiving the packets from socket
    while(1)
    {

        n = recv(data->clientfd,&temp,1,0);
        
        if(n < 1)
        {
            //Error in recv()
            perror("recv failed");
            exit(-1);

        }

        temp_buff[idx] = temp;
        idx++;
        temp_buff = (char *)realloc(temp_buff,sizeof(char) * (idx + 1));

        if(temp == '\n')
        {
            //write buffer to the file of len 'idx'
            // printf("Recieved string = %s\n",temp_buff);
            n = write(data->fd,temp_buff,idx);
            idx = 0;
            free(temp_buff);
            temp_buff = NULL;
            break;
        }

    }

    free(temp_buff);

    //read the entire contents of the file and transfer it over the socket
    lseek(fd,0,SEEK_SET);

    while(1)
    {
        n = read(data->fd,&temp,1);
        
        if(n==0)
        {
            //EOF file reached
            break;
        }
        write(data->clientfd,&temp,1);

    }

    ret = pthread_mutex_unlock(&mutex);

    if(ret!=0)
        perror("pthread_mutex_lock");

    
    data->thread_complete = true;
    
    pthread_exit(NULL);

}

static void *timer_threadFunc(void * arg)
{
    char stamp[] = "timestamp:";
    char new_line = '\n';
    time_t rawtime;
    struct tm *info;
    char buffer[80];

    while(1)
    {
        //sleep for 10 seconds
        for(int i = 0; i<10; i++)
        {
            sleep(1);

            if(g_terminateFlag)
            {
                break;
            }
        }
        

        if(g_terminateFlag)
        {
            break;
        }

        //Get the time flag
        time(&rawtime);
        info = localtime(&rawtime);

        //Lock the mutex
        int ret = pthread_mutex_lock(&mutex);

        //set the global exit flag and return if there's an error
        if(ret!=0)
        {
            perror("pthread_mutex_lock");
            g_terminateFlag = true;
            pthread_exit(NULL);
        }
        
        if(ret!=0)
            perror("pthread_mutex_lock");   

        //Write the timestamp to the file
        write(fd,stamp,strlen(stamp));

        strftime(buffer,80,"%a, %d %b %Y %T %z", info);
        write(fd,buffer,strlen(buffer));
        write(fd,&new_line,1);

        
        ret = pthread_mutex_unlock(&mutex); //Unlock the mutex

        //set the global exit flag and return if there's an error
        if(ret!=0)
        {
            perror("pthread_mutex_lock");
            g_terminateFlag = true;
            pthread_exit(NULL);
        }
                     

    }
    

     pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    //Parameters related to signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM,signal_handler);

    //Setup syslog
    openlog("aesdsocket",0,LOG_USER);
    //Check if process needs to be daemonised
    if(argc > 2)
    {
        printf("Invalid arguments to main\nUsage: ./main <-d>\n");
        exit(-1);
    }

    if(argc == 2)
    {
        if((strcmp(argv[1],"-d")) == 0)
        {
            daemonize = 1;
        }

        else
        {
            printf("Invalid arguments to main\nUsage: ./main <-d>\n");
            exit(-1);
        }
    }

    //Linked list setup:
    // Initialize the head before use
    TAILQ_INIT(&head);

    //Start the servers socket and bind
    sockfd = socket(AF_INET,SOCK_STREAM, 0); //open a socket with IPV4, stream type (tcp) and the protocol is TCP (0)
    printf("sockfd = %d\n",sockfd);
    if(sockfd < 0)
    {
        perror("Error opening socket");
        exit(-1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    memset((void *)&serv_addr,0,sizeof(serv_addr)); //Set serv_addr to 0
    
    //Set the server characteristics 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; //This is an IP address that is used when we don't bind a socket to any specific IP. 
                                            //When we don't know the IP address of our machine, we can use the special IP address INADDR_ANY.
    serv_addr.sin_port = htons(PORT_NO); //host to network short

    int ret = bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(ret < 0)
     {
         perror("Binding failed");
         exit(-1);
     }

    if(daemonize == 1)
     {
         //-d argument was passed to main, now the process has to daemonise itself
         daemonize_process();
        // daemon(0,0);
     }

     //file needed for append (/var/tmp/aesdsocketdata), creating this file if it doesn’t exist
     fd = open("/var/tmp/aesdsocketdata",O_RDWR | O_TRUNC | O_CREAT, 0666);

     if(fd == -1)
     {
         perror("Error opening file");
         exit(-1);
     }

    //Thread to print the timestamp every 10 seconds
    bool timer_thread_created = false;

    while(1)
    {
        
        //start listening on the created socket
        listen(sockfd, 5);

        clilen = sizeof(cli_addr);
     
        clientfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if(g_terminateFlag)
        {
            if(data != NULL)
            {
                free(data);
            }

            graceful_exit();
        }

        if(!timer_thread_created)
        {
            pthread_create(&timer_thread,NULL,timer_threadFunc,NULL);
            timer_thread_created = true;
        }

        if(clientfd < 0)
        {
            perror("Error on accept");
            exit(-1);
        }

        //Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client
        syslog(LOG_USER,"Accepted connection from %s\n",inet_ntoa(cli_addr.sin_addr));

        //Malloc the data that needs to be passed to the thread function
        data = (thread_data *)malloc(sizeof(thread_data));

        //Initialise the data that needs to be passed to the head function
        data->fd = fd;
        data->clientfd = clientfd;
        data->thread_complete = false;

        //Create the thread
        int err = pthread_create(&(data->thread_id),NULL,threadFunc,(void *)data);
        if(err !=0)
        {
            free(data);
        }

        //Insert into the linked list
        TAILQ_INSERT_TAIL(&head,data,next);

        //Iterate loop. temp holds the previous node that needs to be freed.
        void * res;
        int s;
        thread_data * temp = NULL;
        
        TAILQ_FOREACH(data, &head, next)
        {
            if(temp != NULL)
            {
                free(temp);
                temp = NULL;
            }
            if(data->thread_complete == true)
            {
                s = pthread_join(data->thread_id,&res);
                if(s!=0)
                {
                    perror("pthread_join error");
                    
                    graceful_exit();
                }

                //Shutdown the client fd
                shutdown(data->clientfd,SHUT_RDWR);

                //Close the connection with the client and start listening for incoming connections again
                syslog(LOG_USER,"Closed connection from %s\n",inet_ntoa(cli_addr.sin_addr));

                close(data->clientfd);
                TAILQ_REMOVE(&head,data,next);

                temp = data;
            }
        
        }

        //If any node is still pending to be free'd, free it
        if(temp!=NULL)
        {
            free(temp);
            temp = NULL;
        }

        data = NULL;
    }

    //Unreachable code
    return 0;

}

/*
* Brief - Used to gracefully exit the process
*/
void graceful_exit()
{
    // thread_data *temp;

    free(data);
    
    if(fd > 0)
    {
        //deleting the file /var/tmp/aesdsocketdata
        remove("/var/tmp/aesdsocketdata");
        close(fd);
    }

    if(clientfd > 0)
    {
        close(clientfd);
    }

    if(sockfd > 0)
    {
        close(sockfd);
    }

    void * res;
    thread_data *data;
    thread_data *temp = NULL; //Temp holds the previous node that needs to be freed
    TAILQ_FOREACH(data, &head, next)
    {
        if(temp != NULL)
        {
            free(temp);
            temp = NULL;
        }
        int s = pthread_join(data->thread_id,&res);

        if(s!=0)
        {
            perror("pthread_join in cleanup");
        }

        //Shutdown the client fd
        shutdown(data->clientfd,SHUT_RDWR);

        close(data->clientfd);
        TAILQ_REMOVE(&head,data,next);

        // free(data_temp);
        temp = data;
    }

    if(temp!=NULL)
    {
        free(temp);
        temp = NULL;
    }


    if(timer_thread)
    {
       int s = pthread_join(timer_thread,&res);

       if(s!=0)
        perror("Timer thread join:");
    }

    exit(0);

}

static void daemonize_process()
{
    pid_t pid;

    pid = fork();

    if(pid<0)
    {
        perror("fork");
        exit(-1);
    }

    if(pid > 0)
    {
        //Parent Process: has to exit
        printf("Parent Exiting!. Child PID = %d\n",pid);
        exit(0);
    }

    //Child process executes from here on, i.e pid = 0

    umask(0);

    //Create a new session and set the child as group leader
    pid = setsid();
    
    if(pid < 0)
    {
        perror("setsid");
        exit(-1);
    }

    //Change working directory to root directory
    chdir("/");

    //close all file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

}

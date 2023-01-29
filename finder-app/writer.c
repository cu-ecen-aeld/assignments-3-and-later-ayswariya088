/*
 * @File name (writer.c)
 * @File Description (The code will write the given string in the desired file location given that the file exists)
 * @Author Name (AYSWARIYA KANNAN)
 * @Date (1/28/2022)
 * @Attributions :
 *
 */
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <syslog.h> //for system logs
#include <errno.h>

#define WRTFILE (1)
#define WRTSTR  (2)
#define N       (3)

int main(int argc, char** argv){ //argc and argv are the command line arguments passed to the main function

/*argc is the no of arguments given by argv i.e, it is one greater than the number of arguments since
*name of the program will be the first argument*/
 openlog(NULL,0,LOG_USER); //setting the logging from LOG_USER facility

if(argc!=N){

   
    printf("Error: Invalid No of Arguments.\n\rPlease enter the path of the directory and the string to be written.\n\r");
    syslog(LOG_ERR,"Invalid No. Of Arguments"); //log msg to the LOG_ERR priority level
    exit(1);
}
else {

FILE *writefile=fopen(argv[WRTFILE],"wb"); //opening the file with access of writing in binary mode
    if(writefile==NULL){            //if the file doesn't exists it will return NULL
        printf("Error: %s\n",strerror(errno));
        syslog(LOG_ERR,"Unable to open file. %s",strerror(errno)); //log msg to LOG_ERR priority level
        exit(1);
                    }
    else{
        fputs(argv[WRTSTR],writefile); //writing the string
        syslog(LOG_DEBUG,"Writing %s to %s",argv[2],argv[1]); //send the log message to LOG_DEBUG

        if(fclose(writefile)!=0){ //if any error while closing the file it will throw a non-zero error number 
          syslog(LOG_ERR,"Error while closing %s",strerror( errno ));//log msg to LOG_ERR priority level
          exit(1);
        }
    }

}
closelog(); //closing all logs
return 0;

}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <syslog.h>
#include <errno.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 9000
#define BUFFERSIZE 20480 //20kB

//Global Variables
char serverfile_loc[]= "/tmp/var/aesdsocketdata";
char serverfile_path[]= "/tmp/var/";
FILE *serverfile = NULL;
struct sockaddr_in serveraddr, clientaddr;
int serverfd, clientfd;
int backlog = 5;

//flag is 1 when running, 0 when to shutdown
int runflag = 1;

static void sig_handler(int sigflag){

    if(sigflag == SIGINT)
    {
        syslog(LOG_INFO, "SIGINT Flagged");
    }
    else if(sigflag == SIGTERM)
    {
        syslog(LOG_INFO, "SIGTERM Flagged");
    }

    syslog(LOG_INFO, "Caught signal, exiting");
    
    //close file descriptors
    close(serverfd);
    close(clientfd);
    remove(serverfile_loc);

    //close syslogging
    syslog(LOG_INFO, "Finished Logging for aesdsocket");
    closelog();

    //exiting ~gracefully~
    printf("Exiting program, completed shutdown. /n");
    exit(EXIT_SUCCESS);
    
}

int main(int argc, char **argv){

    int rcbind;
    int daemonize;
    ssize_t recvfd = 0;
    int rclisten;
    int sendret;
    int filesetup;
    char buffer_recv[BUFFERSIZE];
    char buffer_send[BUFFERSIZE];
    char buffer_packet[BUFFERSIZE];


    openlog(NULL, 0, LOG_USER);
    syslog(LOG_INFO, "Start Logging for aesdsocket\n");
    printf("Started Socket Program \n");

    //daemonize
    if (argc > 2)
    {
        printf("Too many arguments. Try again. \n");
        exit(EXIT_FAILURE);
    }
    else if (argc==2 && strcmp(argv[1],"-d")==0)
    {
        printf("Started in daemon mode \n");
        daemonize = daemon(0,0);
        if (daemonize==-1)
        {
            syslog(LOG_ERR, "Failed starting daemon mode: %s", strerror(errno));
            printf("Failed to start in daemon mode \n");
            exit(EXIT_FAILURE);
        }
         
    }

    //setup signalling for SIGTERM and SIGINT
    if(signal(SIGINT,sig_handler)==SIG_ERR)
    {   
        syslog(LOG_ERR, "SIGINIT failed");
        printf("Sigint failed \n");
        exit(EXIT_FAILURE);
    }
    else if(signal(SIGTERM,sig_handler)==SIG_ERR)
    {
        syslog(LOG_ERR, "SIGTERM failed");
        printf("Sigterm failed \n");
        exit(EXIT_FAILURE);
    }

    //creating socket and setting up sockaddr_in to cast (IPv4, "old method")
    printf("Creating socket \n");
    serverfd = socket(PF_INET, SOCK_STREAM, 0);
    if(serverfd < 0)
    {
        syslog(LOG_ERR, "Socket Error: %s", strerror(errno));
        printf("Socket Creation Failed \n");
        exit(EXIT_FAILURE);
    }

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port= htons(PORT);
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    memset(serveraddr.sin_zero, '\0', sizeof serveraddr.sin_zero);  
    
    //check to if in use BEFORE binding
    int reuse=1;
    int checkreuse = setsockopt (serverfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse) );
    if(checkreuse == -1)
    {
        syslog(LOG_WARNING, "Failed to set socket to previously used address: %s", strerror(errno));
        printf("Warning: Failed to reuse socket address");
    }

    //binding (assigning address to socket)
    printf("Binding address to socket \n");
    rcbind = bind(serverfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (rcbind == -1){
        syslog(LOG_ERR, "Bind failure: %s", strerror(errno));
        printf("Bind failed \n");
        exit(EXIT_FAILURE);
    }

    //create directory if does not exist
    filesetup=mkdir(serverfile_path,0777);
    if ((filesetup < 0) && (errno!=EEXIST))
    {  
        syslog(LOG_ERR, "Failed to create directory %s", strerror(errno));
        printf("Failed to create directory %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    //checking if file already exists. If it does, delete before starting loop (assignment specific, may delete in future)
    if(access(serverfile_loc, F_OK) == 0)
    {
        printf("File already exists. Deleting before proceeding.");
        if(remove(serverfile_loc)!=0)
        {
            syslog(LOG_ERR, "Failed to delete file %s", strerror(errno));
            printf("Failed to delete file %s. Proceeding with program, may be errors. \n", strerror(errno));
        }
        else
        {
            printf("File deleted succesfully. \n");
        }
    }


    // While loop that listens for new sockets before entering inner loop to receive data
    while(runflag){
        
        //listening and accepting new clients
        printf("Listening... \n");
        rclisten=listen(serverfd, backlog);
        if(rclisten==-1)
        {
            syslog(LOG_ERR, "Failed to listen on socket: %s", strerror(errno));
            printf("Failed to listen in on socket \n");
            close(serverfd);
            exit(EXIT_FAILURE);
        }

        socklen_t clientaddrlen = sizeof(clientaddr);
        
        clientfd = accept(serverfd, (struct sockaddr *) &clientaddr, &clientaddrlen);
        if (clientfd < 0){

            syslog(LOG_ERR, "Failed to accept client: %s", strerror(errno));
            printf("Failed to accept client \n");
            exit(EXIT_FAILURE);
        }
        else
        {
            syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(clientaddr.sin_addr));
            printf("Accepted client \n");
        }

        
        buffer_recv[0]='\0';
        buffer_send[0]='\0';
        buffer_packet[0]='\0';
        size_t filesize = 0;
        ssize_t packetsize = 0;

        //while loop to receive data from client, write to txt file, and then send back
        while ((recvfd = recv(clientfd, buffer_recv, BUFFERSIZE - 1, 0)) > 0)
        {
            //check if full packet was received until you find a newline character
            if(buffer_recv[recvfd-1]!='\n')
            {
                //printf("Received packet of size %zu \n", recvfd);
                //printf("%s",buffer_recv);
                printf("Did not receive full packet.\n");
                for(ssize_t i=0; i < (recvfd); i++)
                {
                buffer_packet[i+packetsize] = buffer_recv[i];
                }
                
                packetsize += recvfd;
                
                //buffer_recv[0]='\0';  
            }
            else //end of packet found, go into this part
            {
                //printf("Received packet of size %zu \n", recvfd);
                //printf("Received full packet of size %zu \n", (recvfd + packetsize));
                printf("Received full packet \n");
                
                for(ssize_t i=0; i < (recvfd); i++)
                {
                    buffer_packet[i+packetsize] = buffer_recv[i];
                }

                packetsize += recvfd;

                buffer_packet[packetsize] = '\0';

                //open file to only append
                printf("Opening file \n");
                serverfile = fopen(serverfile_loc, "a");
                if (serverfile < 0)
                {  
                    syslog(LOG_ERR, "Failed to open file to write: %s", strerror(errno));
                    printf("Failed to open file \n");
                    exit(EXIT_FAILURE);
                }

                //write buffer value to serverfile before resetting buffer and closing file
                fprintf(serverfile,"%s", buffer_packet);
                fclose(serverfile);
            

                //open file to only rea
                serverfile = fopen(serverfile_loc, "r");
                filesize = fread(buffer_send, 1, BUFFERSIZE, serverfile);
                syslog(LOG_INFO, "Success: Read file");
                printf("Success, read file.\n");
                fclose(serverfile);

                //Sending file to client
                syslog(LOG_INFO, "Sending file to client");
                printf("Sending file to client \n");
                //printf("Sending: %s", buffer_send);
                sendret = send(clientfd,buffer_send,filesize,0);
                if(sendret == -1)
                {
                    syslog(LOG_ERR, "Failed to send file to client: %s", strerror(errno));
                    printf("Failed to send file to client \n");
                    exit(EXIT_FAILURE);
                }
                
                packetsize = 0;
            
            }             

        }
        if(recvfd == 0)
        {
            syslog(LOG_INFO, "Connection closed from client");
            printf("Connection closed from client \n");
        }
        else if (recvfd == -1)
        {
            printf("Failed to receive data packet \n");
            syslog(LOG_ERR, "Data packet receive error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        //closing client
        printf("Closing client \n");
        close(clientfd);
        syslog(LOG_INFO, "Closed Client Connection from %s", inet_ntoa(clientaddr.sin_addr));
    }

    syslog(LOG_DEBUG, "Reached end of main, error in code.");
    printf("Error in code, reached end of main \n");
    return 0;    
}
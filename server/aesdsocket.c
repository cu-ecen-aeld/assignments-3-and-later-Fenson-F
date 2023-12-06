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

#include <pthread.h>
#include <sys/queue.h>
#include <time.h>
#include <stdbool.h>

#define PORT 9000
#define BUFFERSIZE 20480 //20kB

//Global Variables
char serverfile_loc[]= "/tmp/var/aesdsocketdata";
char serverfile_path[]= "/tmp/var/";
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

FILE *serverfile = NULL;
struct sockaddr_in serveraddr, clientaddr;
int serverfd; 
int backlog = 5; //number of clients

volatile sig_atomic_t tstatus= 0;
pthread_t timerthread = {0};
pthread_t jointhread = {0};
SLIST_HEAD(llisthead , thread_data) head;
struct thread_data *newclientthread = NULL;

//flag is 1 when running, 0 when to shutdown
int runflag = 1;

//thread data
struct thread_data{
    int clientfd;
    pthread_t thread;
    int complete;
    SLIST_ENTRY(thread_data) node;
};

void delete_thread_data (struct thread_data *ptr){
    if(ptr != NULL){
        free(ptr);
    }
}

void* thread_timer_func(void* timerparam){
    
    char output[200];
    int ret=0;
    time_t t;
    struct tm *buff;

    while(runflag)
    {
        //sleep for the first 10 seconds to avoid tripping up the autotest
        sleep(10);

        t=time(NULL);
        buff = localtime(&t);
        if(buff == NULL){
            syslog(LOG_ERR, "Failure with getting local time: %s", strerror(errno));
            printf("Timer failure with getting local time \n");
            break;
        }
        
        ret = strftime(output,sizeof(output),"timestamp:%a, %d %b %Y %T %z \n", buff);

        if(ret==0){
            syslog(LOG_ERR, "Failure with timer string: %s", strerror(errno));
            printf("Timer string failure \n");
            break;
        }

        printf("Timer output: %s \n", output);

        pthread_mutex_lock(&mutex);

        serverfile = fopen(serverfile_loc, "a");
        if (serverfile < 0)
        {  
            syslog(LOG_ERR, "Failed to open file to write timer: %s \n", strerror(errno));
            printf("Failed to open file in timer \n");
            pthread_mutex_unlock(&mutex);
            break;
        }

        //write buffer value to serverfile before resetting buffer and closing file
        fprintf(serverfile,"%s", output);
        
        fclose(serverfile);
        pthread_mutex_unlock(&mutex);
        
    }

    return timerparam;
}

void* thread_join_func(void* threadparam){

    struct thread_data *currentthread = NULL;
    struct thread_data **removethread= NULL;
    void *retthread = NULL;

    while(runflag)
    {   
        //wait 5 seconds to allow some threads to complete
        sleep(5);

        currentthread = NULL;
        removethread = NULL;
        int retjoin = 1;
        int joinsize=0;

        //check each item in the list for any completed and store
        SLIST_FOREACH(currentthread, &head, node){
            if(currentthread->complete == 1) {
                retjoin = pthread_join(currentthread->thread, &retthread);
                if (retjoin != 0){
                    printf("Thread did not join succesfully\n");
                }
                else {
                    printf("Joined thread from client %d \n", currentthread->clientfd);
                    joinsize++;
                    removethread = (struct thread_data**) realloc (removethread, sizeof(struct thread_data)*joinsize);
                    removethread[joinsize-1] = currentthread;
                    
                }
            }
        }

        //delete outside of list in order to avoid memory leaks
        printf("Joining completed threads \n");
        for(int i=0; i<joinsize; i++){
            SLIST_REMOVE(&head, removethread[i], thread_data, node);
            delete_thread_data(removethread[i]);
        }

        if(removethread!=NULL)
        {
            free(removethread);
        }
    }

    return threadparam;

}

void *thread_client_func(void* threadparam){
    
    char buffer_recv[BUFFERSIZE];
    char buffer_send[BUFFERSIZE];
    char buffer_packet[BUFFERSIZE];
    ssize_t recvfd = 0;
    int filesize = 0;
    ssize_t packetsize = 0;
    int sendret=0;

    buffer_recv[0]='\0';
    buffer_send[0]='\0';
    buffer_packet[0]='\0';
    struct thread_data *clientthreaddata = (struct thread_data *)threadparam;

    //while loop to receive data from client, write to txt file, and then send back
    while ((recvfd = recv(clientthreaddata->clientfd, buffer_recv, BUFFERSIZE - 1, 0)) > 0)
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
            pthread_mutex_lock(&mutex);

            printf("Opening file \n");
            serverfile = fopen(serverfile_loc, "a");
            if (serverfile < 0)
            {  
                syslog(LOG_ERR, "Failed to open file to write: %s", strerror(errno));
                printf("Failed to open file \n");
                goto conn_fail;
            }

            //write buffer value to serverfile before resetting buffer and closing file
            fprintf(serverfile,"%s", buffer_packet);
            fclose(serverfile);

            //open file to read only
            serverfile = fopen(serverfile_loc, "r");
            filesize = fread(buffer_send, 1, BUFFERSIZE, serverfile);
            syslog(LOG_INFO, "Success: Read file");
            printf("Success, read file.\n");
            fclose(serverfile);

            pthread_mutex_unlock(&mutex);


            //Sending file to client
            syslog(LOG_INFO, "Sending file to client");
            printf("Sending file to client \n");
            //printf("Sending: %s", buffer_send);
            sendret = send(clientthreaddata->clientfd,buffer_send,filesize,0);
            if(sendret == -1)
            {
                syslog(LOG_ERR, "Failed to send file to client: %s", strerror(errno));
                printf("Failed to send file to client \n");
            }
                
            packetsize = 0;
            
        }             

    }

    clientthreaddata->complete = 1;

    return threadparam;

    conn_fail:
        pthread_mutex_unlock (&mutex);
        clientthreaddata->complete = 1;

        return threadparam;
}


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

    struct thread_data *threaddelptr = NULL;

    while(!SLIST_EMPTY(&head)){
        threaddelptr = SLIST_FIRST(&head);
        SLIST_REMOVE(&head, threaddelptr, thread_data, node);
        delete_thread_data(threaddelptr);
    }

    //close file descriptors and threads
    pthread_cancel(timerthread);
    pthread_cancel(jointhread);
    delete_thread_data (newclientthread);
    
    close(serverfd);
    remove(serverfile_loc);
    pthread_mutex_destroy(&mutex);    

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
    
    int rclisten;
    int headret;
    int tailret;
    int filesetup;
    int joinret;
    SLIST_INIT(&head);

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
    
    //Use the head of the linked list to write the timer thread
    headret = pthread_create(&timerthread, NULL, thread_timer_func, NULL);
    if(headret != 0 ){
        syslog(LOG_ERR, "Failed to create timer thread%s",strerror(errno));
        printf("Failed to create timer thread%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    //start join thread seperately
    joinret  = pthread_create(&jointhread, NULL, thread_join_func, NULL);
    if(joinret != 0 ){
        syslog(LOG_ERR, "Failed to create join thread%s",strerror(errno));
        printf("Failed to create join thread%s\n", strerror(errno));
        exit(EXIT_FAILURE);
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
        
        int newclient = accept(serverfd, (struct sockaddr *) &clientaddr, &clientaddrlen);
        if (newclient < 0){

            syslog(LOG_ERR, "Failed to accept client: %s", strerror(errno));
            printf("Failed to accept client \n");
            exit(EXIT_FAILURE);
        }
        else
        {
            syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(clientaddr.sin_addr));
            printf("Accepted client \n");
        }

        //create thread to handle the connection
        newclientthread = (struct thread_data*) malloc (sizeof(struct thread_data));
        if(newclientthread == NULL) {
            printf("Failed to create new client thread /n");
        }
        else {
            newclientthread->clientfd=newclient;
            newclientthread->complete=0;
        }
        //add the new connection as the new head to avoid having to keep track of the tail
        SLIST_INSERT_HEAD(&head, newclientthread, node);

        tailret  = pthread_create(&newclientthread->thread, NULL, thread_client_func, (void*) newclientthread);
        if(tailret != 0 ){
            syslog(LOG_ERR, "Failed to create client thread %s", strerror(errno));
            printf("Failed to create client thread %s\n", strerror(errno));
            SLIST_REMOVE(&head, newclientthread, thread_data, node);
            delete_thread_data(newclientthread);
            continue;
            
        }

        newclientthread = NULL;
        
    }

    syslog(LOG_DEBUG, "Reached end of main, error in code.");
    printf("Error in code, reached end of main \n");
    exit(EXIT_FAILURE);  

}
/*=====================================================================================================================================*/

/*=====================================================================================================================================*/
/*=======================================--------------------------------------------------------======================================*/
/*==================================---------------------------Tal Randi------------------------------=================================*/
/*==================================------------------------------Ex4--------------------------------==================================*/
/*=======================================--------------------------------------------------------======================================*/
/*=====================================================================================================================================*/

/*=====================================================================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAXIMUM_WAITING 10
#define BUFFSIZE 4096

//Node struct
typedef struct MSG_QNode 
{ 
    char* message;
    int msg_size; 
    int send_to;
    struct MSG_QNode* next; 
}MSG_QNode; 

//Queue struct
typedef struct MSG_Queue 
{ 
    int qsize;
    struct MSG_QNode* head; 
    struct MSG_QNode* tail; 
}MSG_Queue; 

//Global queue
MSG_Queue* messages_queue;

//This function catch the SIGINT signal, and clean the memory before the program will done
void clean_memory(MSG_Queue* queue)
{
    if(queue)
    {
        MSG_QNode* p;
        for(int i = 0; i < queue->qsize ; i++)
        {
            p = queue->head;
            if(p)
            {
                queue->head = queue->head->next;
                free(p);
            }
        }
        free(queue);
    }
}

//SIGINT signal handler
void program_closing(int signal) 
{
    if(signal == SIGINT)
        clean_memory(messages_queue);
    exit(EXIT_SUCCESS);
}


//This function gets a new message and insert it to the tail of the quque
void insert_message(MSG_Queue* queue, char* msg, int size, int to)
{
    MSG_QNode* new_msg = (MSG_QNode*)malloc(sizeof(MSG_QNode));
    if(!new_msg)
    {
        perror("error: Malloc failure\n");
        exit(EXIT_FAILURE);
    }
    new_msg->message = msg;
    new_msg->msg_size = size;
    new_msg->send_to = to;
    new_msg->next = NULL;

    //Empty queue
    if(queue->qsize == 0)
    {
        queue->head = new_msg;
        queue->tail = new_msg;
    }
    else
    {
        queue->tail->next = new_msg;
        queue->tail = new_msg;
    }
    queue->qsize++;
}

int main(int argc, char* argv[])
{
    //Validation checks
    if(argc != 2)
    {
        fprintf(stderr,"Usage: server <port>\n");
        exit(EXIT_FAILURE);
    }
    if(atoi(argv[1]) <= 0)
    {
        fprintf(stderr,"Usage: server <port>\n");
        exit(EXIT_FAILURE);        
    }
    signal(SIGINT, program_closing);

    int welcome_socket_fd, newsock_fd;
    struct sockaddr_in server_address;

    //Defines a server and it's welcome socket
    welcome_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(welcome_socket_fd < 0)
    {
        perror("Socket failure");
        exit(EXIT_FAILURE);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[1]));

    if(bind(welcome_socket_fd,(struct sockaddr *)&server_address,sizeof(server_address)) < 0)
    {
        perror("Error on binding");
        exit(EXIT_FAILURE);
    }    
    listen(welcome_socket_fd,MAXIMUM_WAITING);
    //--------------------Server is ready----------------------//
    
    fd_set original_fd;
    fd_set read_fd;
    fd_set write_fd;
    char buff[BUFFSIZE] = {0};
    char buff_copy[BUFFSIZE] = {0};
    //Initialize new queue
    messages_queue = (MSG_Queue*)malloc(sizeof(MSG_Queue));
    if(!messages_queue)
    {
        perror("error: Malloc failure\n");
        exit(EXIT_FAILURE);
    }
    messages_queue->head = NULL;
    messages_queue->tail = NULL;
    messages_queue->qsize = 0;

    //Varaible initialization
    MSG_QNode* pointer;
    int rc = 0;
    int i = 0;
    int send_to = 0;
    int counter = 0;
    int biggest_fd = welcome_socket_fd;    
    FD_ZERO(&original_fd);
    FD_SET(welcome_socket_fd,&original_fd);

    //Infinite loop - gets and serve clients
    while(1)
    {
        read_fd = original_fd;

        if(messages_queue->qsize == 0)
            FD_ZERO(&write_fd);
        else
            write_fd = original_fd;
        
        if(select(biggest_fd+1,&read_fd,&write_fd,NULL,NULL) == -1)
        {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }
        //Accept new client
        if(FD_ISSET(welcome_socket_fd,&read_fd))
        {
            newsock_fd = accept(welcome_socket_fd,NULL,NULL);
            //Insert new fd to fd set
            if(newsock_fd > 0)
            {
                FD_SET(newsock_fd,&original_fd);
                if(newsock_fd > biggest_fd)
                    biggest_fd = newsock_fd;
            }
        }
        //Reading loop
        for(i = welcome_socket_fd+1 ; i <= biggest_fd ; i++)
        {
            //FD "i" wrote something
            if(FD_ISSET(i, &read_fd))
            {
                printf("Server is ready to read from socket %d\n", i);
                for(int reset = 0 ; reset < BUFFSIZE ; reset++)
                {
                    buff[reset] = '\0';
                    buff_copy[reset] = '\0';
                }
                rc = read(i,&buff,BUFFSIZE);
                //Close i fd's
                if(rc == 0)
                {
                    FD_CLR(i,&original_fd);
                    close(i);
                }
                //Read some text
                else if(rc > 0)
                {
                    sprintf(buff_copy, "guest%d: %s",i,buff);
                    for(send_to = welcome_socket_fd+1 ; send_to <= biggest_fd ; send_to++)
                    {
                        if(FD_ISSET(send_to, &original_fd))
                        {
                            //The message will send to all other clients
                            if(send_to != i)
                                insert_message(messages_queue,buff_copy,strlen(buff_copy),send_to);      
                        }
                    }
                }
                //Read failed
                else
                {
                    perror("Read failed");
                    exit(EXIT_FAILURE);
                }
            }
        }
        //Writing loop
        if(messages_queue->qsize > 0)
        {
            pointer = messages_queue->head;
            while(pointer && counter < messages_queue->qsize)
            {
                //Sending the head message to "sent_to" fd
                if(FD_ISSET(pointer->send_to, &write_fd))
                {
                    printf("Server is ready to write to socket %d\n", pointer->send_to);
                    write(pointer->send_to,pointer->message,pointer->msg_size);
                    messages_queue->head = messages_queue->head->next;
                    free(pointer);
                    messages_queue->qsize--;
                }
                //Moving the head to be the last node
                else
                {
                    messages_queue->tail->next = messages_queue->head;
                    messages_queue->head = messages_queue->head->next;
                    messages_queue->tail = messages_queue->tail->next;
                    messages_queue->tail->next = NULL;
                }
                counter++;
                pointer = messages_queue->head;
            }
            counter = 0;
        } 
    }
    free(messages_queue);
    return 0;
}

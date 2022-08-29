#include "helper.cpp"
#include<string.h>

int count_client = 0;         //Counting the number of client
SOCKET client_socket[100];    //Storing client socket
int start_of_uid = 101;       //initialization of id

void del_str(char str[],int n)
{
    int i=0,j;
    i = i+n;
    for(j=0; str[i]!='\0'; i++,j++)
    {
        str[j]=str[i];
    }
    str[j]='\0';
}

void thread_to_send(struct thread_data td)
{
    SOCKET s = td.s;

    char msg_send[15] = "Your id is ";
    char id[5];

    //Converting the integer user id into string
    sprintf(id, "%d", td.uid);

    //Preparing the message with id
    strcat(msg_send, id);

    //Server is sending the message to client
    bool success = send_to_socket(client_socket[count_client-1], msg_send);
    if(!success) printf("Send failed.\n\n");

}

void thread_to_recv(struct thread_data td)
{
    SOCKET s = td.s;
    int sender = td.uid; //Sender user id
    char msg_recv[1000];
    strcpy(msg_recv,"");

    while(strcmp(msg_recv,"bye"))
    {
        int msg_len = receive_from_socket(s, msg_recv, 1000);

        //Duplicating the message
        char dup_msg_recv[strlen(msg_recv)];
        strcpy(dup_msg_recv, msg_recv);

        //Extracting the receiver id as a string
        char *token;
        token = strtok(msg_recv, " ");

        //converting the string receiver id into integer
        int uid;
        char str[10];
        strcpy(str,token);
        uid = atoi(str);

        //Removing the receiver id from message
        int uid_len = strspn(dup_msg_recv, token);
        del_str(dup_msg_recv, uid_len);

        //Converting the integer sender id into string
        char sender_str[10];
        sprintf(sender_str, "%d", sender);

        //Adding the sender id with message
        strcat(sender_str,">");
        strcat(sender_str, dup_msg_recv);

        //Sending message to receiver
        bool success = send_to_socket(client_socket[uid-start_of_uid], sender_str);
        if(!success) printf("Send failed.\n\n");

    }
}


int main(int argc, char *argv[])
{
    bool success = init_networking();
    if(!success) return 0;

    SOCKET server_socket = create_socket();
    if(!server_socket) return 0;

    success = bind_socket(server_socket, 0, 8888);
    if(!success) return 0;

    listen_for_connections(server_socket, 1);

    while(1)
    {
        //Accept an incoming connection
        puts("Waiting for incoming connections...");

        client_socket[count_client] = accept_connection(server_socket);

        puts("Connection accepted");

        //create sending and receiving threads
        struct thread_data td;
        td.s = client_socket[count_client];
        td.uid = start_of_uid + count_client;

        create_socket_thread(thread_to_send, td);
        create_socket_thread(thread_to_recv, td);

        count_client++;
    }

    //go to sleep
    sleep_for_ever();
    WSACleanup();
    return 0;
}





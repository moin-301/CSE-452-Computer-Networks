#include "network_helper.cpp"
#include "http_helper.cpp"

int main(int argc, char *argv[])
{
    while(1)
    {
        bool success = init_networking();
        if(!success) return 0;

        SOCKET client_socket = create_socket();
        if(!client_socket) return 0;

        success = connect_to_server(client_socket, "127.0.0.1", 8888);
        if(!success) return 0;


        char msg_send[1000], msg_recv[1000];

        //Sending request to server
        input_http_request(msg_send);

        success = send_to_socket(client_socket, msg_send);
        if(!success) printf("Sending Failed.");

        int msg_len = receive_from_socket(client_socket, msg_recv, 1000);

        if(msg_len>0)
        {
            printf("<--- \n\n %s \n\n -->", msg_recv);
        }

        //Closing client socket after getting response of the server
        closesocket(client_socket);
    }

    WSACleanup();
    return 0;
}









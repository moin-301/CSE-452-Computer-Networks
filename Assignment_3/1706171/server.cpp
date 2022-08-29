#include "helper.cpp"

struct mac_address_entry
{
    int port;    // port number (only used in switch)
    // you can define additional parameters if needed to send to threads
    SOCKET s;    // Storing the client socket to send frame
    char mac[7]; // eg- ab:bc:cd:ef:10:12
};

mac_address_entry table[255]; // take a large table
int table_size = 0;

thread_data clients[255];     // Store all clients here
int client_size = 0;          // Initial Client size or for indexing the clients araay
int switch_port = 0;          // Port number of switch


void *command_listener(void *arg)
{
    char command[100];

    while (true)
    {
        gets(command);

        if (strcmp(command, "exit") == 0) // exiting
            exit(0);
        // Add your code here
        else if(strcmp(command, "sh mac address-table") == 0) /// sh mac address-table
        {
            puts("Printing MAC table of the switch");

            if(table_size == 0)  printf("No Entry in the MAC table.");
            else
            {
                puts("Port |\t\tMAC");
                for(int i=0; i<table_size; i++) /// Printing the table array
                {
                    printf("  %d  |\t", table[i].port);
                    print_mac(table[i].mac);
                    printf("\n");
                }
            }


        }
        else if(strcmp(command, "clear mac address-table") == 0) /// clear mac address-table
        {
            /*for(int i=0;i<table_size;i++)
            {
                table[i].port = 0;
                strcpy(table[i].mac, "");
            }*/
            table_size = 0;
            //printf("No entry in the MAC address table.");
        }

        printf("\n\n\n");
    }
}

// return receiver socket based on given mac addres.
SOCKET get_receivers_socket(mac_address_entry *table, int table_size, char *receiver_mac)
{
    int i;
    for (i = 0; i < table_size; i++)
        if (!strcmp(table[i].mac, receiver_mac))
            return table[i].s;

    return 0;
}

void thread_to_recv(thread_data data)
{
    puts("\n\n\n");

    // take a ethernet frame pointer and allocate memory. Allocation memory is important here.
    ethernet_frame *frame = (ethernet_frame *)malloc(sizeof(ethernet_frame));

    while (true)
    {
        int msg_len = receive_ethernet_frame(data.s, frame);
        if (msg_len < 0)
            break; // once send or receive fails, please return;
        if (msg_len > 0)
        {
            // Add your code here

            char sender_mac[7],receiver_mac[7];
            strcpy(sender_mac,frame->mac_source);           /// Extracting sender mac from ethernet-frame
            strcpy(receiver_mac,frame->mac_destination);    /// Extracting receiver mac from ethernet-frame

            if(!get_receivers_socket(table, table_size, sender_mac)) /// Finding if the receiver is in the table
            {
                /// MAC of Receiver is not present in the table. So, storing the receiver MAC in table
                strcpy(table[table_size].mac, sender_mac);
                table[table_size].port = data.port;
                table[table_size].s = data.s;
                table_size++;
            }


            if(is_boadcast_mac(receiver_mac)) /// The destination is broadcasting.
            {
                printf("Getting a frame from MAC: ");
                print_mac(sender_mac);
                printf(" to MAC: ");
                print_mac(receiver_mac);
                printf("\nBroadcasting the frame.");

                for(int i=0; i<client_size; i++)
                {
                    if(clients[i].port != data.port)
                    {
                        send_ethernet_frame(clients[i].s, frame);
                    }
                }
            }
            else if(!is_boadcast_mac(receiver_mac))
            {
                /// Destination is not broadcasting, rather it is a valid MAC
                printf("Getting a frame from MAC: ");
                print_mac(sender_mac);
                printf(" to MAC: ");
                print_mac(receiver_mac);
                printf("\nSending a frame to MAC: ");
                print_mac(receiver_mac);
                printf("\n");

                /// If the table contains the destination MAC, then ethernet-frame will be sent to the destination
                send_ethernet_frame(get_receivers_socket(table, table_size, receiver_mac), frame);


            }
            else if(get_receivers_socket(table, table_size, receiver_mac) == 0)
            {
                /// The table does not contain the destination MAC, so ethernet-frame will be broadcasted
                printf("Getting a frame from MAC: ");
                print_mac(sender_mac);
                printf(" to MAC: ");
                print_mac(receiver_mac);
                printf("\nBroadcasting the frame.");

                for(int i=0; i<client_size; i++)
                {
                    if(clients[i].port != data.port)
                    {
                        send_ethernet_frame(clients[i].s, frame);
                    }
                }

            }/*
            else
            {
                printf("Getting a frame from MAC: ");
                print_mac(sender_mac);
                printf(" to MAC: ");
                print_mac(receiver_mac);
                printf("\nSending a frame to MAC: ");
                print_mac(receiver_mac);
                printf("\n");
                send_ethernet_frame(get_receivers_socket(table, table_size, receiver_mac), frame);
            }*/
            puts("\n");
        }
    }

    free(frame); // free up the allocated memoru before exiting from this function.
}

int main(int argc, char *argv[])
{
    bool success = init_networking();
    if (!success)
        return 0;

    SOCKET server_socket = create_socket();
    if (!server_socket)
        return 0;

    success = bind_socket(server_socket, 0, 8888);
    if (!success)
        return 0;

    // Opening command thread
    pthread_t command_thread;
    int arg;
    pthread_create(&command_thread, NULL, command_listener, &arg);

    listen_for_connections(server_socket, 1);

    SOCKET client_socket;

    while (1)
    {
        // Accept and incoming connection
        puts("Waiting for incoming connections...");

        client_socket = accept_connection(server_socket);

        puts("\n\n\nConnection accepted");

        // create sending and receiving threads
        struct thread_data td;
        td.s = client_socket;   /// Assigning the client_socket in the thread_data(socket) variable
        td.port = switch_port;  /// Assigning the switch-port in the thread_data(port) variable


        /// Storing each client socket and their respective port in clients array
        clients[client_size].s = client_socket;
        clients[client_size].port = switch_port;
        client_size++;
        switch_port++;


        // create_socket_thread(thread_to_send, td);
        create_socket_thread(thread_to_recv, td);

    }

    // go to sleep
    sleep_for_ever();

    // finally process cleanup job
    closesocket(server_socket);
    closesocket(client_socket);
    WSACleanup();
    return 0;
}

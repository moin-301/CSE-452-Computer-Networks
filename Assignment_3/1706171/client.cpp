#include "helper.cpp"

char my_ip[5];   // eg- 192.178.255.255   (one extra for storing null char at last)
// (only used in client part)
char my_mac[7];  // eg- ab:bc:cd:ef:10:12 (one extra for storing null char at last)

struct arp_table_entry
{
    char ip[5];  // eg- 192.168.255.255
    char mac[7]; // eg- ab:bc:cd:ef:10:12
};

arp_table_entry arp_table[255]; // take a big table
int arp_table_size = 0;

// find the index of the arp table entry for the specified ip
int find_index_using_ip(arp_table_entry *arp_table, int table_size, char *ip)
{
    int i = 0;
    for (i = 0; i < table_size; i++)
    {
        if (strcmp(arp_table[i].ip, ip) == 0)
            return i; // find a match so return this ip
    }
    return -1; // for not finding any entry for the specified ip
}


void thread_to_send(thread_data data)
{
    Sleep(100);

    puts("\n\n\n");

    // take a ethernet frame pointer and allocate memory. Allocation memory is important here.
    ethernet_frame *frame = (ethernet_frame *)malloc(sizeof(ethernet_frame));

    // Add your code here

    char receiver_ip[5];
    char command[100];

    while (true)
    {
        gets(command);

        if (strcmp(command, "exit") == 0) // exiting
            exit(0);
        // Add your code here

        else if (strcmp(command, "arp -a") == 0)    /// showing the elements in the arp_table
        {
            puts("Printing ARP table.");
            if(arp_table_size == 0) printf("There are no entry in ARP Table.\n");
            else
            {
                for(int i=0; i<arp_table_size; i++)
                {
                printf("Entry: %d =>", i+1);
                print_ip_and_mac(arp_table[i].ip, arp_table[i].mac);
                printf("\n");
                }
            }

        }
        else if (strcmp(command, "arp -d") == 0)    /// Clearing the arp_table
        {
            arp_table_size = 0;
            printf("Deleted All the entries.\n");
        }

        char extracted_command[20];
        char extracted_ip[20];
        char *token;

        /// Extracting "find_mac" part from the command
        token = strtok(command, " ");
        strcpy(extracted_command, token);

        /// Extracting [IP] part from the command
        token = strtok(NULL, " ");
        strcpy(extracted_ip, token);


        /// Checking if the command is "find_mac"
        if (strcmp(extracted_command, "find_mac") == 0)
        {
            /// Converting the IP
            extract_ip(receiver_ip, extracted_ip);

            /// Checking if the table contains the IP
            int index = find_index_using_ip(arp_table, arp_table_size, receiver_ip);
            if ( index >= 0) ///The table contains the IP
            {
                printf("\n\n");
                printf("ARP entry found.\n");
                printf("Entry: %d => ", index+1);
                print_ip_and_mac(arp_table[index].ip, arp_table[index].mac);
                printf("\n");
            }
            else
            {
                printf("Entry does not found. Broadcasting...\n");
                char broadcast_mac[7];
                generate_broadcast_mac(broadcast_mac);

                strcpy(frame->mac_destination, broadcast_mac);
                strcpy(frame->mac_source, my_mac);

                strcpy(frame->payload.sender_hardware_address, my_mac);
                strcpy(frame->payload.sender_protocol_address, my_ip);
                strcpy(frame->payload.target_protocol_address, receiver_ip);

                frame->payload.operation = 0;

                send_ethernet_frame(data.s, frame);
            }
        }


        puts("\n\n");
    }

    free(frame); // free up the allocated memory before exiting from this function.
}

void thread_to_recv(thread_data data)
{
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
            int request_or_reply;
            request_or_reply = frame->payload.operation; /// 0 for request, 1 for reply

            if (request_or_reply) /// This is reply
            {
                printf("Get an ARP reply from IP: ");
                print_ip(frame->payload.sender_protocol_address);


                printf("\nARP entry added with ");
                print_ip_and_mac(frame->payload.sender_protocol_address, frame->payload.sender_hardware_address);
                printf("\n");

                /// Storing the MAC & IP in arp_table
                strcpy(arp_table[arp_table_size].ip, frame->payload.sender_protocol_address);
                strcpy(arp_table[arp_table_size].mac, frame->payload.sender_hardware_address);
                arp_table_size++;

                printf("ARP resolved and MAC is: ");
                print_mac(frame->payload.sender_hardware_address);
                printf("\n");

            }
            else
            {
                /// This is request
                printf("Get an ARP request from IP: ");
                print_ip(frame->payload.sender_protocol_address);
                printf(" for ");
                print_ip(frame->payload.target_protocol_address);

                /// Checking if the arp_table contains the IP
                int index = find_index_using_ip(arp_table, arp_table_size, frame->payload.sender_protocol_address);
                if(index == -1) /// The arp_table doesn't contain the IP
                {
                    strcpy(arp_table[arp_table_size].ip, frame->payload.sender_protocol_address);
                    strcpy(arp_table[arp_table_size].mac, frame->payload.sender_hardware_address);
                    arp_table_size++;

                    printf("\nARP entry added with ");
                    print_ip_and_mac(frame->payload.sender_protocol_address, frame->payload.sender_hardware_address);
                    printf("\n");

                }
                else    /// The arp_table contains the IP
                {
                    printf("\n\n");
                    printf("ARP entry found.\n");
                    printf("Entry: %d => ", index+1);
                    print_ip_and_mac(arp_table[index].ip, arp_table[index].mac);
                    printf("\n");
                }

                /// The broadcasted IP matches with my_ip
                if(!strcmp(my_ip, frame->payload.target_protocol_address))
                {

                    /// Defining an ethernet-frame for replying the ARP request
                    ethernet_frame *reply_frame = (ethernet_frame *)malloc(sizeof(ethernet_frame));

                    strcpy(reply_frame->mac_destination, frame->payload.sender_hardware_address);
                    strcpy(reply_frame->mac_source, my_mac);

                    strcpy(reply_frame->payload.sender_hardware_address, my_mac);
                    strcpy(reply_frame->payload.sender_protocol_address, my_ip);

                    strcpy(reply_frame->payload.target_protocol_address, frame->payload.sender_protocol_address);
                    strcpy(reply_frame->payload.target_hardware_address, frame->payload.sender_hardware_address);

                    reply_frame->payload.operation = 1;

                    send_ethernet_frame(data.s, reply_frame);

                    free(reply_frame);  /// free up the allocated memory before exiting from this function.

                    printf("Sending ARP Reply to ");
                    print_ip(frame->payload.sender_protocol_address);
                }
                /// The broadcasted IP doesn't match with my_ip, so dropping the ARP request
                else
                    printf("Drop ARP Request.\n");
                }

            puts("\n\n");
        }
    }

    free(frame); // free up the allocated memory before exiting from this function.
}

int main(int argc, char *argv[])
{
    bool success = init_networking();
    if (!success)
        return 0;

    SOCKET client_socket = create_socket();
    if (!client_socket)
        return 0;

    char server_ip[16];
    strcpy(server_ip, "127.0.0.1");
    success = connect_to_server(client_socket, server_ip, 8888);
    if (!success)
        return 0;

    // create sending and receiving threads
    thread_data data;

    // Add your code here
    data.s = client_socket;

    generate_ip(my_ip);
    generate_mac(my_mac);

    printf("Generated IP:\t");
    print_ip(my_ip);

    printf("\nGenerated MAC:\t");
    print_mac(my_mac);
    printf("\n\n");


    create_socket_thread(thread_to_send, data);
    create_socket_thread(thread_to_recv, data);

    // go to sleep
    sleep_for_ever();

    // finally process cleanup job
    closesocket(client_socket);
    WSACleanup();
    return 0;
}

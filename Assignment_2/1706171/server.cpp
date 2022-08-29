#include "network_helper.cpp"
#include "http_helper.cpp"
#include "api_helper.cpp"

SOCKET client_sockets[100];

void extracting_path(char source[], char dest[])
{
    int i;
    for(i=1; i<strlen(source); i++)
    {
        dest[i-1] = source[i];
    }
    dest[i-1] = '\0';
}
void extracting_id_from_path(char id[], char path[])
{
    int i;
    for(i=5; i<strlen(path); i++)
    {
        id[i-5] = path[i];
    }
    id[i-5] = '\0';
}
void extracting_id_from_DELETE(char id[], char path[])
{
    int i;
    for(i=7; i<strlen(path); i++)
    {
        id[i-7] = path[i];
    }
    id[i-7] = '\0';
}
/*void removing_whitespace(char str[])
{
    int i,j,len;
    //Calculating length of the array
    len = strlen(str);

    //Checks for space character in array if its there then ignores it and swap str[i] to str[i+1];
    for(i = 0; i < len; i++)
    {
        if(str[i] == ' ')
        {
            for(j=i; j<len; j++)
            {
                str[j]=str[j+1];
            }
            len--;
        }
    }
}*/
void response_for_api(char uid[], char name[], char email[], char Response[])
{
    char resp[100];
    strcpy(resp,"id=");
    strcat(resp,uid);
    strcat(resp,"&name=");
    strcat(resp,name);
    strcat(resp,"&email=");
    strcat(resp,email);
    sprintf(Response,"HTTP/1.1 200 OK\nContent-length:%d\n\n%s",strlen(resp),resp);
}
void thread_to_recv_and_send(struct thread_data td)
{
    SOCKET s = td.s;

    char msg_recv[1000], msg_send[1000];

    int msg_len = receive_from_socket(s, msg_recv, 1000);


    char req_header[1000], req_body[1000];
    char response[1000], file_contents[1000];

    get_http_header_body(msg_recv, req_header, req_body);

    char * request = req_header;
    char type[100], path[100], proto[100], name_part[100], email_part[100], id_part[10], name[50], email[50];

    request = get_token(request, " ", type);
    request = get_token(request, " ", path);
    request = get_token(request, " ", proto);

    //For GET type request
    if(strcmp(type,"GET")==0 && strcmp(proto, "HTTP/1.1")==0)
    {

        //Checking for root address
        if(strcmp(path,"/")==0)
        {
            //preparing response
            int found = read_html_file("index.html", file_contents);
            if(found)
            {
                int con_len = strlen(file_contents);
                sprintf(response, "HTTP/1.1 200 OK\nContent-length:%d\n\n%s",con_len,file_contents);
            }
            else
            {
                sprintf(response, "HTTP/1.1 404 NOT FOUND\nContent-length:0");
            }
        }
        //Checking for api without id in header
        else if(strcmp(path, "/api")==0)
        {
            char id_field[10], id[10];

            char * body = req_body;

            body = get_token(body, "=", id_field);
            body = get_token(body, "=", id);

            int integer_id = atoi(id);

            //Preparing response

            //Parsing data from api.db database
            int found = get_record(integer_id, name, email);
            if (found)
            {
                response_for_api(id, name, email, response);
            }
            else
            {
                sprintf(response, "HTTP/1.1 404 NOT FOUND\nContent-length:0");
            }


        }

        //Checking for api with id in header
        else if(strncmp(path,"/api/",5)==0)
        {
            char id[10];

            //Extracting id from path -- /api/id
            extracting_id_from_path(id, path);

            int integer_id = atoi(id);

            //Parsing data from api.db database
            int found = get_record(integer_id, name, email);
            if(found)
            {
                response_for_api(id, name, email, response);
            }
            else
            {
                sprintf(response, "HTTP/1.1 404 NOT FOUND\nContent-length:0");
            }
        }

        //Checking for valid address of a file
        else if(path[0] == '/' && strlen(path)>1)
        {
            //Removing the first '/' from the path
            char extracted_path[strlen(path)-1];
            extracting_path(path, extracted_path);

            //Preparing Response
            int found = read_html_file(extracted_path, file_contents);
            if (found)
            {
                int con_len = strlen(file_contents);
                sprintf(response, "HTTP/1.1 200 OK\nContent-length:%d\n\n%s",con_len,file_contents);
            }
            else
            {
                sprintf(response, "HTTP/1.1 404 NOT FOUND\nContent-length:0");
            }
        }

        //For any other malformed path
        else
        {
            sprintf(response, "HTTP/1.1 400 BAD REQUEST\nContent-length:0");

        }
    }

    //For POST type request
    else if(strcmp(type, "POST")==0 && strcmp(proto, "HTTP/1.1")==0)
    {
        if(strcmp(path,"/api")==0)
        {
            //Extracting name and email with field
            char * p = req_body;
            p = get_token(p, "&", name_part);
            p = get_token(p, "&", email_part);

            //Extracting name
            p = name_part;
            char name_field[10];
            p = get_token(p, "=", name_field);
            p = get_token(p, "=", name); //second token
            //removing_whitespace(name_field);
            //removing_whitespace(name);

            //Extracting email
            p = email_part;
            char email_field[30];
            p = get_token(p, "=", email_field);
            p = get_token(p, "=", email); //second token
            //removing_whitespace(email_field);
            //removing_whitespace(email);

            //Creating Record in api.db
            if(strcmp(name_field,"name")==0 && strcmp(email_field,"email")==0)
            {
                int status = create_record(name, email);
                if(status)
                {
                    sprintf(response, "HTTP/1.1 201 CONTENT CREATED\nContent-length:0");
                }
            }
            else
            {
                sprintf(response, "HTTP/1.1 400 BAD REQUEST\nContent-length:0");
            }

        }
        //Bad request if path is not /api
        else
        {
            sprintf(response, "HTTP/1.1 400 BAD REQUEST\nContent-length:0");
        }

    }

    //For PUT type request
    else if(strcmp(type, "PUT")==0 && strcmp(proto, "HTTP/1.1")==0)
    {
        if(strcmp(path,"/api")==0)
        {
            char * p = req_body;

            //Extracting id, name, email from body with field
            p = get_token(p, "&", id_part);
            p = get_token(p, "&", name_part);
            p = get_token(p, "&", email_part);

            //Extracting id
            p = id_part;
            char id_field[5], id[5];
            p = get_token(p, "=", id_field);
            p = get_token(p, "=", id); //second token
            //removing_whitespace(id_field);
            //removing_whitespace(id);
            int integer_id = atoi(id);

            //Extracting name
            p = name_part;
            char name_field[10];
            p = get_token(p, "=", name_field);
            p = get_token(p, "=", name); //second token
            //removing_whitespace(name_field);
            //removing_whitespace(name);

            //Extracting email
            p = email_part;
            char email_field[30];
            p = get_token(p, "=", email_field);
            p = get_token(p, "=", email); //second token
            //removing_whitespace(email_field);
            //removing_whitespace(email);

            //Updating Record into api.db
            if(strcmp(id_field,"id")==0 && strcmp(name_field,"name")==0 && strcmp(email_field,"email")==0)
            {
                int status = update_record(integer_id, name, email);
                if(status)
                {
                    sprintf(response, "HTTP/1.1 201 CONTENT CREATED\nContent-length:0");
                }
                else
                {
                    sprintf(response, "HTTP/1.1 404 NOT FOUND\nContent-length:0");
                }
            }
            else
            {
                sprintf(response, "HTTP/1.1 400 BAD REQUEST\nContent-length:0");
            }

        }
        //Bad request if path is not /api
        else
        {
            sprintf(response, "HTTP/1.1 400 BAD REQUEST\nContent-length:0");
        }

    }

    //For DELETE type request
    else if(strcmp(type, "DELETE")==0 && strcmp(proto, "HTTP/1.1")==0)
    {
        if(strcmp(path,"/api")==0)
        {
            char * p = req_body;

            //Extracting id from body
            char id_field[5],id[5];

            p = get_token(p, "=", id_field);
            p = get_token(p, "=", id); //second token

            //removing_whitespace(id_field);
            //removing_whitespace(id);

            int int_id = atoi(id);

            if(strcmp(id_field,"id")==0)
            {
                int status = delete_record(int_id);
                if(status)
                {
                    sprintf(response, "HTTP/1.1 204 NO CONTENT\nContent-length:0");
                }
                else
                {
                    sprintf(response, "HTTP/1.1 404 NOT FOUND\nContent-length:0");
                }
            }
            //if body has not id field
            else
            {
                sprintf(response, "HTTP/1.1 400 BAD REQUEST\nContent-length:0");
            }

        }
        else if (strncmp(path,"/api/",5)==0)
        {
            char id[10];

            //Extracting id from path -- /api/id
            extracting_id_from_path(id, path);

            int int_id = atoi(id);

            int status = delete_record(int_id);
            if(status)
            {
                sprintf(response, "HTTP/1.1 204 NO CONTENT\nContent-length:0");
            }
            else
            {
                sprintf(response, "HTTP/1.1 404 NOT FOUND\nContent-length:0");
            }

        }
        //Bad request if path is not /api
        else
        {
            sprintf(response, "HTTP/1.1 400 BAD REQUEST\nContent-length:0");
        }

    }
    //Bad request if type or protocol is malformed
    else
    {
        sprintf(response, "HTTP/1.1 400 BAD REQUEST\nContent-length:0");
    }

    bool success = send_to_socket(s, response);
    if(!success) printf("Send failed.\n\n");

    closesocket(s);

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

    int client_id=100;
    while(1)
    {

        //Accept and incoming connection
        puts("Waiting for incoming connections...");

        SOCKET s = accept_connection(server_socket);
        client_id++;
        client_sockets[client_id - 100] = s; //store the socket in global variable

        puts("Connection accepted");

        struct thread_data td;
        td.s = s;
        td.client_id = client_id;

        create_socket_thread(thread_to_recv_and_send, td);
    }


    //go to sleep
    sleep_for_ever();

    //finally process cleanup job
    closesocket(server_socket);
    //closesocket(s);
    WSACleanup();
    return 0;
}




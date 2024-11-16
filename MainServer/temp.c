Information Technology University Mail Izza Arooj<bscs22006 @itu.edu.pk>
    server
        Ayesha Siddiqa<bscs22106 @itu.edu.pk>
            Sat, Aug 31, 2024 at 4 : 50 PM To : bscs22006 @itu.edu.pk
// Server side
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
                                                typedef long ssize_t; // Only needed on Unix-like systems
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8080

#define BUFFER_LEN 1024
                                                char RECV_BUFFER[BUFFER_LEN] = {0};
char *SEND_BUFFER = NULL;
int server_socket;
struct sockaddr_in server_addr;
pthread_t clients[10];

void init_and_connect()
{

#ifdef _WIN32
    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInit != 0)
    {
        fprintf(stderr, "WSAStartup failed with error: %d\n", wsaInit);
        exit(EXIT_FAILURE);
    }
#endif

    // ... creating socket ...
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("\nFailed to create socket\n");
        exit(EXIT_FAILURE);
    }

    // ... specify port ...
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // ... bind the socket ...
    int connection = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (connection < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // ... listen on port ...
    if (listen(server_socket, 3) < 0)
    {
        perror("listening failed");
        exit(EXIT_FAILURE);
    }
}

int my_exit()
{
    if (SEND_BUFFER[0] == 'e' && SEND_BUFFER[1] == 'x' && SEND_BUFFER[2] == 'i' && SEND_BUFFER[3] == 't')
        return 1;
    return 0;
}

/*
~ format of client's msg

! if server recvs 0 from recv() func it means client closed connection

^ ---------------( operation ids )---------------
& all these operation will be in in while(1)
& i.e. if a user is loged in it will exit on its own


~ ** ( 0 ) ** create my account
? get the op id only
 client will give request to server to give hum an id ..
 server will check for the available id and assign/send it that

~ ** ( 1 ) ** log me in
? get the op id and the cliend_id
 check if the id that is loging in is present in id_log or not
 then keep it keep loged in ..

~ ** ( 2 ) ** view my files
? it doesn't recv any 2nd param
 check if loged in ..
 open directory and send names of the files along with sizes

~ ** ( 3 ) ** upload file
? recv file-size(in bytes) and file-name(with ext.)
 check if loged in ..
 get size and name
 get the conents of file in chunks ..

~ ** ( 4 ) ** download file
? recv only file-name(with ext.)
 check if loged in ..
 get the name
 send in chunks acc to size ..

~ ** ( 5 ) ** delete account
? get the op id only
 check if loged in ..
 server deletes the directory of that client

~ ** ( 6 ) ** log out my account
? get the op id only
 check if loged in ..
 server sets the client_id to -1 again

*/

const char *ids_log = "/home/leu/Desktop/semester-5/OS/labs/lab-01-cloud/id_log.txt";
pthread_mutex_t lock;

int *load_all_ids(int *size)
{
    FILE *file = fopen(ids_log, "r");
    if (file == NULL)
    {
        printf("id_log file not found !");
        exit(1);
    }

    int int_size;
    fscanf(file, "%d", &int_size);
    *size = int_size;

    int *ids = (int *)malloc(int_size * sizeof(int));

    for (int i = 0; i < int_size; i++)
    {
        if (fscanf(file, "%d", &ids[i]) != 1)
        {
            printf("Error reading the file!");
            fclose(file);
            free(ids);
            exit(1);
        }
    }
    fclose(file);
    return ids;
}

void print_all_ids()
{
    int size;
    int *ids = load_all_ids(&size);
    printf("size : %d", size);
    printf("\n");
    for (int i = 0; i < size; i++)
    {
        printf("\t%d", ids[i]);
    }
    printf("\n");
}

void check(int byteCount, int client_socket)
{
    if (byteCount < 0)
    {
        perror("Connection closed !");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
}
void syntax_error()
{
    printf("Invalid syntax !");
    // exit(EXIT_FAILURE);
    return;
}

int get_op_id(char arr[])
{
    int i = 0;
    char op_id[2];
    while (arr[i] != '-')
    {
        op_id[i] = arr[i];
        i++;
        if (i == BUFFER_LEN)
            syntax_error();
    }
    return atoi(op_id);
}

int id_received_from_client(char arr[])
{
    int i = 0;
    while (arr[i] != '-')
        i++;
    i++;
    char c_received_id[3] = {'\0'};
    int idx = 0;
    while (arr[i] != '\n' && arr[i] != '\0' && arr[i] != '-')
    {
        c_received_id[idx++] = arr[i++];
        if (i == BUFFER_LEN)
            syntax_error();
    }
    return atoi(c_received_id);
}

int check_for_available_id(int *client_ids, int c_ids_size)
{
    for (int val = 0; val < c_ids_size; val++)
    {
        int bool = 0;
        for (int i = 0; i < c_ids_size; i++)
        {
            if (client_ids[i] == val)
            {
                bool = 1;
            }
        }
        if (bool != 1)
        {
            return val; // any id in between is available
        }
    }
    return c_ids_size; // no id is available
}

void append_number_to_file(int new_number)
{
    FILE *file = fopen(ids_log, "r+");
    if (file == NULL)
    {
        printf("id_log file not found!\n");
        exit(1);
    }

    int size;
    fscanf(file, "%d", &size); // Read the current size

    // Read the rest of the file to preserve formatting
    char remaining_content[1024];
    fgets(remaining_content, sizeof(remaining_content), file);

    // Move the file pointer to the end to append the new number
    fseek(file, 0, SEEK_END);
    fprintf(file, "\n%d", new_number); // Append the new number

    // Move the file pointer back to the beginning and update the size
    fseek(file, 0, SEEK_SET);
    fprintf(file, "%d", size + 1); // Increment the size and update it

    // Write the rest of the content back if necessary (to preserve formatting)
    fputs(remaining_content, file);

    fclose(file);
}

char *get_file_name(char arr[])
{
    int i = 0;
    while (arr[i] != '-')
        i++;
    i++;
    char f_name[10] = {'\0'};
    int idx = 0;
    while (arr[i] != '\n' && arr[i] != '\0' && arr[i] != '-')
    {
        f_name[idx++] = arr[i++];
        if (i == BUFFER_LEN)
            syntax_error();
    }
    char *str = f_name;
    return str;
}

long int get_file_size(char *file_name)
{
    printf("%s", file_name);
    fflush(stdout);
    // opening the file in read mode
    FILE *fp = fopen(file_name, "r");

    // checking if the file exist or not
    if (fp == NULL)
    {
        printf("File Not Found!\n");
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    // calculating the size of the file
    long int res = ftell(fp);

    // closing the file
    fclose(fp);

    return res;
}

int remove_directory(const char *path)
{
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d)
    {
        struct dirent *p;

        r = 0;
        while (!r && (p = readdir(d)))
        {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);

            if (buf)
            {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf))
                {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = remove_directory(buf);
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r)
        r = rmdir(path);

    return r;
}

void *handle_client(void *params)
{

    int client_socket = *(int *)params;
    int client_id = -1;
    int loged_in[100] = {-1};
    int lidx = 0;
    char c_client_id[8] = "client";

    //---------------------------------
    // ----- get the op id -----
    while (1)
    {

        char OPERATION_ID[BUFFER_LEN] = {0};
        check(read(client_socket, OPERATION_ID, BUFFER_LEN), client_socket);
        int op_id = get_op_id(OPERATION_ID);

        //~....................
        //~ op id for sign up
        if (op_id == 0)
        {

            //?---------------------------------
            //? lock th mutex
            pthread_mutex_lock(&lock);

            //?---------------------------------
            //? load the ids form file
            int c_ids_size;
            int *client_ids = load_all_ids(&c_ids_size);

            //?---------------------------------
            //? get the available id ..
            char new_id[2] = "0";
            int id_to_give =
                sprintf(new_id, "%d", check_for_available_id(client_ids, c_ids_size));

            //?---------------------------------
            //? send new id to client
            char msg[BUFFER_LEN] = "\nYou have been successfully signed up.\nYour sign in ID is ";
            (msg, new_id, 2);
            send(client_socket, msg, BUFFER_LEN, 0);

            //?---------------------------------
            //? save that id in thstrncate id_log
            append_number_to_file(atoi(new_id));

            //?---------------------------------
            //? unlock the mutex
            pthread_mutex_unlock(&lock);
        }
        //~....................
        //~ op id for sign in
        else if (op_id == 1)
        {

            if (client_id == -1)
            {

                //?---------------------------------
                //? lock th mutex
                pthread_mutex_lock(&lock);

                //?---------------------------------
                //? load the ids form file
                int c_ids_size;
                int *client_ids = load_all_ids(&c_ids_size);

                //?---------------------------------
                //? unlock the mutex
                pthread_mutex_unlock(&lock);

                //?---------------------------------
                //? get the id received from client
                int recv_id = id_received_from_client(OPERATION_ID);

                //?---------------------------------
                // ? check if client has signed up
                int isPresent = 0;
                for (int i = 0; i < c_ids_size; i++)
                {
                    if (recv_id == client_ids[i])
                    {
                        isPresent = 1;
                        break;
                    }
                }
                //?---------------------------------
                //? the case if client has signed up
                if (isPresent)
                {
                    client_id = recv_id;
                    char c[3];
                    sprintf(c, "%d", client_id);
                    printf("\nClient : %s\n", c);
                    strcat(c_client_id, c);
                    printf("\nc_client_id%s\n", c_client_id);
                    char msg[BUFFER_LEN] = "\nYou have been successfully loged in !";
                    send(client_socket, msg, BUFFER_LEN, 0);
                    loged_in[lidx++] = client_id;
                }
                //?---------------------------------
                //? case if client has not signed up
                else
                {
                    char msg[BUFFER_LEN] = "\nYou don't have an account !\nKindly signup first !";
                    send(client_socket, msg, BUFFER_LEN, 0);
                }
            }
            //?---------------------------------
            //? the case if client has signed in
            else
            {
                char msg[BUFFER_LEN] = "\nYou are already signed in !";
                send(client_socket, msg, BUFFER_LEN, 0);
            }

            /*

            logic for multiple logn in of a client

            // lock th mutex
            pthread_mutex_lock(&lock);

            // load the ids form file
            int c_ids_size;
            int* client_ids = load_all_ids(&c_ids_size);

            //unlock the mutex
            pthread_mutex_unlock(&lock);

            // get the id received from client
            int recv_id = id_received_from_client(OPERATION_ID);

            // check if client is already loged in
            int isLogedIn = 0;
            for (int i = 0; i < lidx; i++) {
            if (recv_id == loged_in[i]) {
            isLogedIn = 1;
            break;
            }
            }
            if (isLogedIn == 1) {
            char msg[BUFFER_LEN] = "\nYou are already signed in !";
            send(client_socket, msg, BUFFER_LEN, 0);
            }
            else {
            // check if client has signed up
            int isPresent = 0;
            for (int i = 0; i < c_ids_size; i++) {
            if (recv_id == client_ids[i]) {
            isPresent = 1;
            break;
            }
            }
            if (isPresent) {
            client_id = recv_id;
            char msg[BUFFER_LEN] = "\nYou have successfully loged in !";
            send(client_socket, msg, BUFFER_LEN, 0);
            loged_in[lidx++] = client_id;
            }
            else {
            char msg[BUFFER_LEN] = "\nYou don't have an account !\nKindly signup first !";
            send(client_socket, msg, BUFFER_LEN, 0);
            }
            }
            */
        }
        //~....................
        //~ op id for view
        else if (op_id == 2)
        {
        }
        //~....................
        //~ op id for upload
        else if (op_id == 3)
        {

            if (client_id != -1)
            {

                //?---------------------------------
                //? recv size of file from client
                char size_buffer[BUFFER_LEN];
                read(client_socket, size_buffer, BUFFER_LEN);
                int size = atoi(size_buffer);

                //?---------------------------------
                //? recv contents of file from file
                char *contents = (char *)malloc(size * sizeof(char));
                read(client_socket, contents, size);

                //?---------------------------------
                //? getting the file name
                char *f_name = get_file_name(OPERATION_ID);
                char file_name[64];
                memcpy(file_name, f_name, strlen(f_name));
                file_name[strlen(f_name)] = '\0';

                //?----------------------------------
                //? making path for the client's file
                char path[100] = "/home/leu/Desktop/semester-5/OS/labs/lab-01-cloud/data/";
                strcat(path, c_client_id);

                //?---------------------------------
                //? creating the directory for client
                struct stat s;
                int err = stat(path, &s);
                if (-1 == err)
                {
                    if (mkdir(path, 0777) == -1)
                    {
                        printf("unable to create directory !");
                        break;
                    }
                }

                //?---------------------------------
                //? writing the file
                strcat(path, "/");
                strcat(path, file_name);

                FILE *file = fopen(path, "w");
                if (file == NULL)
                {
                    printf("File Not Found!\n");
                    break;
                }
                fwrite((char *)contents, size, sizeof(char), file);
                fclose(file);

                //?---------------------------------
                //? sending the success message
                char msg[BUFFER_LEN] = "\nFile Uploaded successfully !";
                send(client_socket, msg, BUFFER_LEN, 0);
            }
            else
            {
                char msg[BUFFER_LEN] = "\nYou are not signed in !\nKindly sign in first! ";
                send(client_socket, msg, BUFFER_LEN, 0);
            }
        }
        //~....................
        //~ op id for download
        else if (op_id == 4)
        {

            if (client_id != -1)
            {

                //?---------------------------------
                //? getting the file name
                char *f_name = get_file_name(OPERATION_ID);
                char file_name[64];
                memcpy(file_name, f_name, strlen(f_name));
                file_name[strlen(f_name)] = '\0';

                //?---------------------------------
                //? making path to the client's file
                char path[100] = "/home/leu/Desktop/semester-5/OS/labs/lab-01-cloud/data/";
                strcat(path, c_client_id);
                strcat(path, "/");
                strcat(path, file_name);

                //?---------------------------------
                //? sending size to the client
                size_t size = get_file_size(path);
                char c_size[BUFFER_LEN];
                sprintf(c_size, "%d", (int)size);
                send(client_socket, c_size, BUFFER_LEN, 0);

                //?---------------------------------
                //? reading the contents of the file
                char *contents = (char *)malloc(size * sizeof(char));

                FILE *file = fopen(path, "r");
                if (file == NULL)
                {
                    printf("File Not Found!\n");
                    break;
                }
                int i = 0;
                char c;
                while ((c = fgetc(file)) != EOF)
                {
                    contents[i++] = c;
                }
                contents[i] = '\0';
                fclose(file);

                //?---------------------------------
                //? sending file to the client
                send(client_socket, contents, size, 0);

                //?---------------------------------
                //? sending the scucess message
                char msg[BUFFER_LEN] = "\nFile downloaded successfully !";
                send(client_socket, msg, BUFFER_LEN, 0);
            }
            else
            {
                char msg[BUFFER_LEN] = "\nYou are not signed in !\nKindly sign in first! ";
                send(client_socket, msg, BUFFER_LEN, 0);
            }
        }
        //~....................
        //~ op id for del acc.
        else if (op_id == 5)
        {

            if (client_id != -1)
            {

                //?---------------------------------
                //? making path to the client's file
                char path[100] = "/home/leu/Desktop/semester-5/OS/labs/lab-01-cloud/data/";
                strcat(path, c_client_id);

                //?---------------------------------
                //? deleting the contents recursively
                remove_directory(path);

                //?---------------------------------
                //? sending the scucess message
                char msg[BUFFER_LEN] = "\nContent deleted successfuly !";
                send(client_socket, msg, BUFFER_LEN, 0);
            }
            else
            {
                char msg[BUFFER_LEN] = "\nYou are not signed in !\nKindly sign in first! ";
                send(client_socket, msg, BUFFER_LEN, 0);
            }
        }
        //~....................
        //~ op id for log out
        else if (op_id == 6)
        {

            //?---------------------------------
            //? check if client has not signed in
            if (client_id == -1)
            {
                char msg[BUFFER_LEN] = "\nYou are not signed in !\nKindly sign in first! ";
                send(client_socket, msg, BUFFER_LEN, 0);
            }

            //?---------------------------------
            //? logout and reset the id
            else
            {
                client_id = -1;
                c_client_id[0] = '\0';
                strcpy(c_client_id, "client\0");
                char msg[BUFFER_LEN] = "\nYou have been successfully loged out !";
                send(client_socket, msg, BUFFER_LEN, 0);
            }
        }
        //~....................
        //~ op id for closing connection
        else if (op_id == 7)
        {

            char msg[BUFFER_LEN] = "\nClosing the connection ...";
            send(client_socket, msg, BUFFER_LEN, 0);
            // Close the client socket
            close(client_socket);

            // Terminate the thread
            pthread_exit(NULL);
            return NULL;
        }
        //~....................
        else
        {
            syntax_error();
        }

        // clear the op_id
        OPERATION_ID[0] = '\0';
    }

    // Close the client socket
    close(client_socket);

    // Terminate the thread
    pthread_exit(NULL);
    return NULL;
}

int main()
{

    init_and_connect();
    int addr_len = sizeof(server_addr);

    int accept_socket;

    int i = 0;

    while (1)
    {
        printf("\nWaiting for client to connect...\n");

        accept_socket = accept(server_socket, (struct sockaddr *)&server_addr, &addr_len);
        if (accept_socket < 0)
        {
            perror("failed to connect !");
            exit(EXIT_FAILURE);
        }
        printf("\nClient conneted !%d\n", accept_socket);

        if (pthread_create(&clients[i++], NULL, handle_client, (void *)&accept_socket) != 0)
        {
            printf("\nFailed to create thread for client%d\n", accept_socket);
            exit(EXIT_FAILURE);
        }
        printf("\nCreated thread for client : %d", accept_socket);

        printf("\n");
    }

#ifdef _WIN32
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif

    return 0;
}

int main1()
{

    // print_all_ids();

    char new_id[2] = "0";
    // itoa(val, new_id, 10);

    int size;
    int *ids = load_all_ids(&size);
    sprintf(new_id, "%d", check_for_available_id(ids, size));
    printf("%s", new_id);

    return 0;
}

//************* ( MAIN FOR TEXTING ) **********/
// int main() {

// init_and_connect();
// int addr_len = sizeof(server_addr);

// size_t size = BUFFER_LEN;

// int accept_socket;
// printf("Waiting for client to connect...\n");

// accept_socket = accept(server_socket, (struct sockaddr*)&server_addr, &addr_len);
// if (accept_socket < 0) {
// perror("failed to connect !");
// exit(EXIT_FAILURE);
// }
// printf("Client conneted !\n\n");

// while (1) {
// long byte_count;
// #ifdef _WIN32
// byte_count = recv(accept_socket, RECV_BUFFER, BUFFER_LEN - 1, 0); // Windows equivalent of read
// #else
// byte_count = read(accept_socket, RECV_BUFFER, BUFFER_LEN - 1); // subtract 1 for the null
// #endif
// printf("\tCLIENT : \t%s", RECV_BUFFER);

// printf("\tSERVER : \t");
// getline(&SEND_BUFFER, &size, stdin);

// if (my_exit()) break;

// send(accept_socket, SEND_BUFFER, BUFFER_LEN - 1, 0);
// }

// #ifdef _WIN32
// closesocket(accept_socket);
// closesocket(server_socket);
// WSACleanup();
// #else
// close(accept_socket);
// close(server_socket);
// #endif

// return 0;
// }

/*
to run in windows :
 gcc -o sout server.c -lws2_32

to run in linux :
 gcc -o sout server.c

*/

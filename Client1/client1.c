// Client side C program to demonstrate Socket
// programming
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h> //directory entry
#include <sys/stat.h> //mkdir
#include <unistd.h>   //chdir
#include <openssl/sha.h>

#include "encrypt.h"


#define PORT 8080
#define bufferSize 1024
int client_fd = -1;

#define true 1
#define false 0

typedef int bool;

void print_hash(unsigned char *digest, size_t length)
{
    printf("%s: ", "SHA-256");
    for (size_t i = 0; i < length; i++)
    {
        printf("%02X", digest[i]);
    }
    printf("%s","\n");
}

unsigned char* Sha256Hash(const char *Name)
{
    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);   
    size_t bytes;
    SHA256_Update(&sha256_ctx, Name, strlen(Name));
    unsigned char *sha256_digest = malloc(SHA256_DIGEST_LENGTH);
    SHA256_Final(sha256_digest, &sha256_ctx);  
    print_hash(sha256_digest, SHA256_DIGEST_LENGTH);
    return sha256_digest;
}
void signalHandler(int sig)
{
    if (client_fd != -1)
    {
        close(client_fd);
    }
    printf("Client terminated by signal Ctrl C%d\n", sig);
    fflush(stdout);
    exit(0);
}
void binaryToHex(unsigned char *data, size_t length, unsigned char *hexStr)
{
    for (size_t i = 0; i < length; ++i)
    {
        sprintf(hexStr + i * 2, "%02X", data[i]);
        
    }
    hexStr[length * 2] = '\0'; 
}

void takingUserNameAurPassword(int client_fd,char * msg){
    char buffer[bufferSize];
    int bytesRecv = recv(client_fd, buffer, sizeof(buffer), 0);
    buffer[bytesRecv] = '\0';
    if (bytesRecv > 0 && strcmp(buffer, msg) == 0)
    {
        printf("Enter %s:",msg);
        fflush(stdout);
        char *str = malloc(bufferSize);
        if (fgets(str, bufferSize, stdin) != NULL)
        {
           

            if(strncmp(msg,"Password",8)==0){
                size_t len = strlen(str);
                if (len > 0 && str[len - 1] == '\n')
                {
                    str[len - 1] = '\0'; 
                }
                unsigned char *hashedPassword=Sha256Hash(str);
                char hexStrPassword[SHA256_DIGEST_LENGTH * 2 + 1];
              
                binaryToHex(hashedPassword, SHA256_DIGEST_LENGTH, hexStrPassword);
               
                // printf("\nHashed Password:%s\n", hexStrPassword);
                // fflush(stdout);
                // char*hexStrPassword="A665A45920422F9D417E4867EFDC4FB8A04A1F3FFF1FA07E998E86F7F7A27AE3";
                send(client_fd, hexStrPassword, strlen(hexStrPassword), 0);
            }
            else
            {
                send(client_fd, str, strlen(str)-1, 0);
            }
           
        }
        free(str);
    }
}
bool whoOwnsThisFile(char * fileName){
    char cwd[200];
    if (getcwd(cwd, 200) != NULL)
    {
        DIR *dir;
        struct dirent *entry;
        struct stat fileStatistics;
        dir = opendir(cwd);
        bool hasFiles = false;
        if (dir == NULL)
        {
            perror("No such Dir");
        }
        else
        {
            while ((entry = readdir(dir)) != NULL)
            {
                if (strcmp(entry->d_name,fileName)==0)
                {
                    return true;
                }
            }
            return false;
        }
    }
}
bool isFileExist(char* fileName){
    FILE *fp;
   
    fp = fopen(fileName, "rb");
    if(fp==NULL){
        return 0;
    }
    else{
        return 1;
    }
}
int getSizeOfFile(char* fileName){
    FILE *fp;
    int size;
    fp = fopen(fileName, "rb");
    if(fp!=NULL){

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fclose(fp);
    }
    return size;
}
long long int getSizeOfDir(const char *dirPath)
{
    long long int totalSize = 0;
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;

    if ((dir = opendir(dirPath)) == NULL)
    {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char filePath[bufferSize];

        snprintf(filePath, sizeof(filePath), "%s/%s",
                 dirPath, entry->d_name);

        if (stat(filePath, &statbuf) == 0)
        {
            if (S_ISDIR(statbuf.st_mode))
            {
                totalSize += getSizeOfDir(filePath);
            }
            else if (S_ISREG(statbuf.st_mode))
            {
                totalSize += statbuf.st_size;
            }
        }
        else
        {
            perror("stat");
        }
    }

    closedir(dir);

    return totalSize;
}

void uploadFile(int client_fd,char *fileName){

   
    char *path = "/home/izza/OperatingSystem/Lab1/Client1/";
    char fullPath[100];
    strncpy(fullPath, path, sizeof(fullPath));
    strncat(fullPath, fileName, sizeof(fullPath) - strlen(fullPath) - 1);
    fullPath[strlen(fullPath)] = '\0';

    // xorEncryptDecrypt(fullPath, "1234");

    {
        int bytesRecv;
        FILE *fp = fopen(fullPath, "rb");
        if (fp == NULL)
        {
            perror("File open error");
           
        }
    while (!feof(fp))
    {
        char buffer[bufferSize]={0};  //here
        int bytesRead = fread(buffer, 1, sizeof(buffer), fp);
        if (bytesRead > 0)
        {
            send(client_fd, buffer,bytesRead , 0);
            buffer[bytesRead] = '\0';

            bytesRecv = recv(client_fd, buffer, bufferSize, 0);
            if(bytesRecv>0 && strncmp(buffer,"ACK",3)!=0){
                break;
            }
        }
    }
    fclose(fp);
    char *msg = "EOF_Signal";
    send(client_fd, msg, strlen(msg), 0);
    char buffer[bufferSize]={0};  //here
    bytesRecv=recv(client_fd, buffer, sizeof(buffer), 0);
    if(bytesRecv>0){
        if(strncmp(buffer,"ACK",3)==0){
            printf("\n%s\n", "File Uploaded successfully");
            fflush(stdout);
        }
    }
   }
  
}
char* separateBySpace(char buffer[bufferSize],int *i){
    char* Name=malloc(100);
    while (buffer[*i] != ' '||buffer[*i]!='\0'||buffer[*i]!='\n')
    {
        char strTemp[2] = {buffer[*i], '\0'};
        strncat(Name, strTemp, sizeof(strTemp) - 1);
       *i++;
    }
    return Name;
}
void takingCommand(char **cmdType,char **fileName){
    printf("%s\n","Enter Command:");

    char *buffer = malloc(bufferSize);
    if (fgets(buffer, bufferSize, stdin) != NULL){
        buffer[strlen(buffer)-1] = '\0';
       *cmdType = strtok(buffer, " ");
       if (strncmp(*cmdType, "VIEW", 4) != 0 &&strncmp(*cmdType, "view", 4) != 0)
       {
           *fileName = strtok(NULL, " ");
       }
    }
}
void sendingCommand(int client_fd,char* cmdType,char* fileName){
    int fSize;
        char *fileSize = malloc(100);
   
    if (fileName != NULL)
    {
        fSize = getSizeOfFile(fileName);
        sprintf(fileSize, "%d", fSize);
    }

    send(client_fd, cmdType, strlen(cmdType), 0);
    
    char buffer[bufferSize] = {0}; // here
    int bytesRecv=recv(client_fd, buffer, sizeof(buffer), 0);
    buffer[bytesRecv] = '\0';
   
    if (strncmp(buffer, "ACK", 3) == 0)
    {
        if (cmdType != NULL && (strncmp(cmdType, "VIEW", 4) != 0 && strncmp(cmdType, "view", 4) != 0 && strncmp(cmdType, "LOGOUT", 6) != 0 && strncmp(cmdType, "logout", 6) != 0))
        {
            send(client_fd, fileName, strlen(fileName), 0);
            bytesRecv = recv(client_fd, buffer, sizeof(buffer), 0);
            buffer[bytesRecv] = '\0';
            if (strncmp(cmdType, "uploaddir", 9) == 0)
            {
                long long int Size = getSizeOfDir(fileName);
                sprintf(fileSize, "%lld", Size);
                send(client_fd, fileSize, strlen(fileSize), 0);
                bytesRecv = recv(client_fd, buffer, sizeof(buffer), 0);
                buffer[bytesRecv] = '\0';
                if (strncmp(buffer, "ACK", 3) == 0)
                {
                }
            }
            else if (strncmp(buffer, "ACK", 3) == 0 && (strncmp(cmdType, "UPLOAD", 6) == 0 || strncmp(cmdType, "upload", 6) == 0))
            {

                send(client_fd, fileSize, strlen(fileSize), 0);
                bytesRecv = recv(client_fd, buffer, sizeof(buffer), 0);
                buffer[bytesRecv] = '\0';
                if (strncmp(buffer, "ACK", 3) == 0)
                {
                }
            }
        }
        }
}
void sendAckPacket(int new_socket)
{
    char *ackMsg = "ACK";
    send(new_socket, ackMsg, strlen(ackMsg), 0);
}
void downloadFile(int client_fd,char* fileName){
    char buffer[bufferSize] = {0};
    sendAckPacket(client_fd);
    recv(client_fd, buffer, bufferSize, 0);
    if (strncmp(buffer, "FAILURE FILE_NOT_FOUND",22)==0)
    {
        printf("\n%s\n", "FAILURE FILE NOT FOUND");
        fflush(stdout);
    }
    else if(strncmp(buffer,"SUCCESS",7)==0){
        sendAckPacket(client_fd);
    
        FILE *fp = fopen(fileName, "wb");
        if(fp==NULL){

        }
        else{
            int bytesRecv = 1;
            while (bytesRecv > 0)
            {
                char buffer[bufferSize]={0};
                bytesRecv = recv(client_fd, buffer, bufferSize, 0);
               
                    if (strncmp(buffer, "EOF_Signal", 10) == 0)
                    {
                        printf("\n%s\n", "FILE DOWNLOADED SUCCESSFULLY");
                        break;
                    }
                sendAckPacket(client_fd);
                if(strncmp(buffer,"EOF_Signal",10)!=0){
                    fwrite(buffer, 1, bytesRecv, fp);
                    fflush(fp);
                    printf("%s\n", buffer);
                }
            
            }
            // xorEncryptDecrypt(fileName, "1234");
        }
        fclose(fp);
    }

}
void isThatWorked(int val, char *errorMsg, char *successMsg)
{
    if (val < 0)
    {
        perror(errorMsg);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("%s\n", successMsg);
    }
}

void recvBuffer(int client_fd,char* whatData){
    char buffer[bufferSize];
    int bytesRecv = recv(client_fd, buffer, bufferSize, 0);
    if(bytesRecv>0){
        buffer[bytesRecv] = '\0';
        printf("%s:%s\n", whatData, buffer);
    }
}
void viewClientFolder(int client_fd){
    char buffer[bufferSize];
    recv(client_fd, buffer, bufferSize, 0);
    if(strncmp(buffer,"SUCCESS",7)==0){
        sendAckPacket(client_fd);
        while (1)
        {
            recv(client_fd,buffer,bufferSize,0);
            if(strncmp(buffer,"MORE",4)==0){
                printf("\n%s\n", "_______________________");

                recvBuffer(client_fd, "Inode");
                sendAckPacket(client_fd);
                recvBuffer(client_fd, "Type");
                sendAckPacket(client_fd);
                recvBuffer(client_fd, "Name");
                sendAckPacket(client_fd);
                recvBuffer(client_fd, "Size In Bytes");
                sendAckPacket(client_fd);
                recvBuffer(client_fd, "Creation Time");
                sendAckPacket(client_fd);
                recvBuffer(client_fd, "Modification Time");
                sendAckPacket(client_fd);
                printf("\n%s\n", "_______________________");
            }
            else if (strncmp(buffer, "ALL METADATA SENT",17)==0)
            {
                break;
            }
        }
    }
    else if(strncmp(buffer,"FAILURE",7)==0){
        sendAckPacket(client_fd);
        printf("\n%s\n", "FAILURE NO CLIENT DATA");
        fflush(stdout);
    }
}
char* signUpOrIn(int client_fd){
    printf("\n_____________%s_____________\n", "SIGNUP OR SIGNIN");
    fflush(stdout);
    char *str = malloc(bufferSize);
    fflush(stdin);
    if (fgets(str, bufferSize, stdin) != NULL)
    {
        str[strlen(str) - 1] = '\0';
        send(client_fd, str, strlen(str), 0);
        char buffer[bufferSize] = {0};
        if (strncmp(str, "SIGNUP", 6) == 0 || strncmp(str, "signup", 6) == 0)
        {
            while (1)
            {
                takingUserNameAurPassword(client_fd, "Username");
                char buffer[bufferSize] = {0};
                recv(client_fd, buffer, bufferSize, 0);
                if (strncmp(buffer, "USERNAME ALREADY EXIST", 22) != 0)
                {
                    sendAckPacket(client_fd);
                    break;
                }
                else
                {
                    sendAckPacket(client_fd);
                    printf("\n%s\n", buffer);
                    fflush(stdout);
                }
            }
            takingUserNameAurPassword(client_fd, "Password");
            return "signup";
        }
        else if (strncmp(str, "SIGNIN", 6) == 0 || strncmp(str, "signin", 6) == 0)
        {
            // while(true)
            {
                takingUserNameAurPassword(client_fd, "Username");
                takingUserNameAurPassword(client_fd, "Password");
                recv(client_fd, buffer, bufferSize, 0);
                if (strncmp(buffer, "LOGIN SUCCESSFULLY",18)==0){
                    printf("\n%s\n", buffer);
                    fflush(stdout);
                    return "signin";
                    // break;
                }
                else if (strncmp(buffer, "NOT LOGIN SUCCESSFULLY",22)==0){
                    printf("\n%s\n", buffer);
                    fflush(stdout);
                    // continue;
                    return "not";
                }
            }
        }
    }
}
void logOutClient()
{
    // char *msg = "LOGOUT";
    // send(client_fd, msg, strlen(msg), 0);
    char buffer[bufferSize] = {0};
    int bytesRecv = recv(client_fd, buffer, bufferSize, 0);
    if (bytesRecv > 0)
    {
        close(client_fd);
    }
}
void uploadFolder(int client_fd, const char *folderPath)
{
    DIR *dir;
    struct dirent *entry;
    struct stat fileStats;

    dir = opendir(folderPath);
    if (dir == NULL)
    {
        perror("Error opening directory");
        return;
    }
    send(client_fd, "DIR", 3, 0);
    char buffer[bufferSize] = {0};
    recv(client_fd, buffer, bufferSize, 0);
    send(client_fd, folderPath, strlen(folderPath) + 1, 0);


    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue; 
        }

        char filePath[200];
        snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);

        if (stat(filePath, &fileStats) == 0)
        {
            if (S_ISDIR(fileStats.st_mode))
            {
                uploadFolder(client_fd, filePath);
            }
            else if (S_ISREG(fileStats.st_mode))
            {
                send(client_fd, "FILE", 4, 0);
                recv(client_fd, buffer, bufferSize, 0);
                send(client_fd, filePath, strlen(filePath) + 1, 0);
                uploadFile(client_fd, filePath);

               
            }
        }
        else
        {
            perror("Error getting file information");
        }
    }

    closedir(dir);

}

int main(int argc, char const *argv[])
{

    int status, client_fd;
    struct sockaddr_in serv_addr;
    // signal(SIGINT, signalHandler);

    isThatWorked((client_fd = socket(AF_INET, SOCK_STREAM, 0)), "Socket", "Socket Created");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    isThatWorked(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr), "Invalid Address", "Address Converted");

    isThatWorked((status = connect(client_fd, (struct sockaddr *)&serv_addr,
                                   sizeof(serv_addr))),
                 "connection Failed", "Connected To Server");

    char * state=signUpOrIn(client_fd);
    if (strncmp(state, "signup", 6) == 0)
    {
        printf("\n%s\n", "Please SignIn to continue");
        fflush(stdout);
        close(client_fd);
        return 0;
    }
    else if (strncmp(state, "not", 3) == 0)
    {
        close(client_fd);
        return 0;
    }
    else if (strncmp(state, "signin", 6) == 0)
    {
        while (1)
        {
            char *cmdType = NULL;
            char *fileName = NULL;
            takingCommand(&cmdType, &fileName);
           
            if (!isFileExist(fileName) && (strncmp(cmdType, "UPLOAD", 6) == 0 || strncmp(cmdType, "upload", 6) == 0))
            {
                printf("\n%s\n", "This File Does Not Exist");
                fflush(stdout);
                continue;
            }
            if ((strncmp(cmdType, "UPLOAD", 6) == 0 || strncmp(cmdType, "upload", 6) == 0) && !whoOwnsThisFile(fileName))
            {
                printf("\n%s\n", "Permission Denied");
                fflush(stdout);
                continue;
            }
            sendingCommand(client_fd, cmdType, fileName);
           
            char buffer[bufferSize] = {0}; // here
            if (strncmp(cmdType, "uploaddir", 9) == 0)
            {

                // sendAckPacket(client_fd);
                int bytesRecv = recv(client_fd, buffer, bufferSize, 0);
                printf("%d:%s", bytesRecv, buffer);
                fflush(stdout);
                if (bytesRecv > 0 && strncmp(buffer, "ACK", 3) == 0)
                {
                    printf("%s\n", "Have Space You can Upload");
                    uploadFolder(client_fd, fileName);
                    send(client_fd, "END", 3, 0);
                }
                if (bytesRecv > 0 && strncmp(buffer, "$FAILURE$LOW_SPACE$", 19) == 0)
                {
                    printf("\nYou have no Space to Upload File\n");
                    fflush(stdout);
                }
            }
            else if (strncmp(cmdType, "UPLOAD", 6) == 0 || strncmp(cmdType, "upload", 6) == 0)
            {
                sendAckPacket(client_fd);
                int bytesRecv = recv(client_fd, buffer, bufferSize, 0);
                if (bytesRecv > 0 && strncmp(buffer, "ACK", 3) == 0)
                {
                    printf("%s\n", "Have Space You can Upload");
                    uploadFile(client_fd, fileName);
                }
                else if (bytesRecv > 0 && strncmp(buffer, "$FAILURE$LOW_SPACE$", 19) == 0)
                {
                    printf("\nYou have no Space to Upload File\n");
                    fflush(stdout);
                }
            }
            else if (strncmp(cmdType, "DOWNLOAD", 8) == 0 || strncmp(cmdType, "download", 8) == 0)
            {
                downloadFile(client_fd, fileName);
            }
            else if (strncmp(cmdType, "VIEW", 4) == 0 || strncmp(cmdType, "view", 4) == 0)
            {
                viewClientFolder(client_fd);
            }
            else if (strncmp(cmdType, "LOGOUT", 6) == 0 || strncmp(cmdType, "logout", 6) == 0)
            {
                logOutClient();
                printf("\n%s\n", "LOGOUT SUCCESSFULLY");
                fflush(stdout);
                break;
            }
            else{
             continue;
        }
    }
   
    }

   
    close(client_fd);
  

    return 0;
}

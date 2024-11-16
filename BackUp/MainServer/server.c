#include <stdio.h>      //fstream
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   //mkdir
#include <unistd.h>     //chdir

#include <dirent.h>   //directory entry

#include <arpa/inet.h>
#include <pthread.h>

#include "loadconfig.h"

#define configurationFilePath "/home/izza/OperatingSystem/Lab1/MainServer/server.config"
#define bufferSize 1024
#define PORT 8080
#define MAXCLIENTS 100
#define true 1
#define false 0


typedef int bool;
typedef struct
{
    int clientSocketId;
    char *clientName;
    char *password;
    bool isAuthenticated;
} clientSession;




bool isAbruptlyDisconnected(int bytesRecv,int *client_fd){
    if (bytesRecv == 0)
    {
        close(*client_fd);
        *client_fd = -1;
        perror("Abruptly Disconnected");
        printf("\n%s\n", "Client Disconnected Successfully");
        fflush(stdout);
        return 1;
    }
    else {
        return 0;
    }
}
char* getUsernameAurPassword(int *client_fd,char *msg){

   send(*client_fd, msg, strlen(msg), 0);
   unsigned char *buffer = malloc(bufferSize);
    int bytesRecv = recv(*client_fd, buffer, bufferSize, 0);
    buffer[bytesRecv] = '\0';
    if (bytesRecv > 0)
    {
        buffer[bytesRecv] = '\0';
        if(strncmp(msg,"Password",8)==0){
            printf("\n%s:", msg);
            fflush(stdout);
            size_t sha256OutputBytes = 64;
            for (size_t i = 0; i < sha256OutputBytes; i++)
            {
                printf("%c", buffer[i]);
                fflush(stdout);
            }
        }
        else
        {
            printf("\n%s:%s\n",msg, buffer);
            fflush(stdout);
        }
        return buffer;
    }
    if(isAbruptlyDisconnected(bytesRecv, client_fd)){
        return EXIT_SUCCESS;
    }
    free(buffer);
}
void addUserOnDB(char *userName, char *password, int spaceForClient, PathConfig pathStruct)
{
    FILE *db_fd = fopen(pathStruct.databasePath, "ab");
    if(db_fd==NULL){
        printf("%s\n","database is not opening");
        fflush(stdout);
    }
    fwrite(userName, 1, strlen(userName), db_fd);
    fwrite(" ", 1, 1, db_fd);
    fwrite(password, 1, strlen(password), db_fd);
    fwrite(" ", 1, 1, db_fd);
    char spaceStr[20];
    snprintf(spaceStr, sizeof(spaceStr), "%d", spaceForClient);
    fwrite(spaceStr, 1, strlen(spaceStr), db_fd);
    fwrite("\n", 1, 1, db_fd);
    fclose(db_fd);
}
bool findUser(char* userName,char* password,char* dbName){
    FILE *db_fd = fopen(dbName, "rb");
    char buffer[bufferSize]={0};
    int i = 0;
    bool isUserNameExist = 0;
    bool isUserExist = 0;
    while (fgets(buffer, sizeof(buffer), db_fd) != NULL)
    {
        if(buffer[0]=='\n' || buffer==NULL){
            return isUserNameExist;
           
        }
        char * name=strtok(buffer, " ");
        if (strcmp(name, userName) == 0 && password==NULL)
        {
            isUserNameExist = 1;
            return isUserNameExist;
        }
        char *pass= strtok(NULL, " ");
        pass[strlen(pass)] = '\0';
        if (password != NULL && strcmp(pass, password) == 0 && strcmp(name, userName) == 0)
        {
            isUserExist = 1;
            return isUserExist;
        }
    }
   
}
void sendAckPacket(int new_socket){
    char *ackMsg = "ACK";
    send(new_socket, ackMsg, strlen(ackMsg), 0);
}
void recvdCommand(int *new_socket,char **cmdType,char **fileName,char** fileSize){
    *cmdType = malloc(bufferSize);
    if(*cmdType==NULL){
        perror("\nFailed to allocate memory\n");
    }
    int bytesRecv=recv(*new_socket, *cmdType, bufferSize, 0);
    if(bytesRecv>0){
       (*cmdType)[bytesRecv] = '\0';
        sendAckPacket(*new_socket);
    }
    if(isAbruptlyDisconnected(bytesRecv,new_socket)){
        return;
    }
    if (strncmp(*cmdType, "VIEW", 4) == 0 || strncmp(*cmdType, "view", 4) == 0 || strncmp(*cmdType, "LOGOUT", 6) == 0 || strncmp(*cmdType, "logout", 6) == 0)
    {
        printf("\n--------%s---------\n", *cmdType);
        fflush(stdout);
    }
    if (strncmp(*cmdType, "VIEW", 4) != 0 && strncmp(*cmdType, "view", 4) != 0 && strncmp(*cmdType, "LOGOUT", 6) != 0 &&
    strncmp(*cmdType, "logout", 6) != 0)
    {
        *fileName = malloc(bufferSize);
        if (*fileName == NULL)
        {
            perror("\nFailed to allocate memory\n");
        }
        *fileSize = malloc(bufferSize);
        if (*fileSize == NULL)
        {
            perror("\n No filesize Reached\n");
        }

        bytesRecv = recv(*new_socket, *fileName, bufferSize, 0);
        if(isAbruptlyDisconnected(bytesRecv,new_socket)){
            return;
        }
        if (bytesRecv > 0)
        {
            (*fileName)[bytesRecv] = '\0';
            sendAckPacket(*new_socket);
        }
        if (strncmp(*cmdType, "DOWNLOAD", 8) == 0 || strncmp(*cmdType, "download", 8) == 0)
        {
            printf("\n--------%s %s---------\n", *cmdType, *fileName);
        }
        if (strncmp(*cmdType, "UPLOAD", 6) == 0 || strncmp(*cmdType, "upload", 6) == 0 || strncmp(*cmdType, "uploaddir", 9) == 0)
        {
            bytesRecv = recv(*new_socket, *fileSize, bufferSize, 0);
            if(isAbruptlyDisconnected(bytesRecv,new_socket)){
                return;
            }
            if (bytesRecv > 0)
            {
                (*fileSize)[bytesRecv] = '\0';
                printf("\n--------%s %s %s---------\n", *cmdType, *fileName, *fileSize);
                fflush(stdout);
                sendAckPacket(*new_socket);
            }
        }
    }
}
char *makeDirForUser(int new_socket, char *userName,char *fileName, bool isClientExist)
{
    char cwd[200];
    if (getcwd(cwd, 200) != NULL)
    {
       
        char *curr = strtok(cwd, "/");
        while(curr!=NULL){
            if(strcmp(curr,"Storage")==0){
                break;
            }

            curr = strtok(NULL, "/");
        }

       if(curr==NULL){
            if (chdir("Storage") < 0)
            {
                perror("failed to change Storage Dir\n");
            }
        }
    }
    if (!isClientExist)
    {
        if (mkdir(userName, 0755) < 0)
        {
            
        }
        
    }
    getcwd(cwd, 200);
    char *filePath=malloc(bufferSize);
    snprintf(filePath, bufferSize, "%s/%s",cwd,userName);
    // printf("%s:%s\n","Current Directory", filePath);
    // fflush(stdout);
    return filePath;
}
void recvdFileContent(char *file_name,int *new_socket){
    
    FILE *fp = fopen(file_name, "wb");
    int bytesRecv = 1;
    if(fp!=NULL){
        // printf("\n\n%s\n\n", "File is going to receive");
        // fflush(stdout);
    }
    if(fp==NULL){
        printf("There is no such file", file_name);
        fflush(stdout);
    }
    while (bytesRecv > 0)
    {

        char buffer[bufferSize] = {0};
        bytesRecv = recv(*new_socket, buffer, sizeof(buffer), 0);
        if(isAbruptlyDisconnected(bytesRecv,new_socket)){
            break;
        }

        if (strcmp(buffer, "EOF_Signal") == 0)
        {
            fclose(fp);
            sendAckPacket(*new_socket);;
            break;
        }
        sendAckPacket(*new_socket);;
        fwrite(buffer, 1, bytesRecv, fp);
        fflush(fp);
        // printf("%s", buffer);
        // fflush(stdout);
    }
    fflush(stdout);
}

void recvUploadFolder(){

}
long calculateDirSize(char *clientDirPath,char* fileName,bool* isAlreadyFoundFile,int *prevSize)
{
    DIR *dir;
    long consumedSpace = 0;
    struct dirent *entry;
    struct stat fileStatistics;
    dir = opendir(clientDirPath);
    printf(clientDirPath);
    if (dir == NULL)
    {
        perror("\n failed to open Directory");
    }
    while ((entry = readdir(dir)) != NULL)
    {
        char *fullPath = (char *)malloc(bufferSize);
        if (entry->d_name[0] == '.')
        {
            continue;
        }
        if(strcmp(entry->d_name,fileName)==0){
            *isAlreadyFoundFile = true;
            snprintf(fullPath, bufferSize, "%s/%s", clientDirPath, entry->d_name);

            if (stat(fullPath, &fileStatistics) == 0)
            {

                *prevSize= fileStatistics.st_size;
            }
        }
        snprintf(fullPath, bufferSize, "%s/%s", clientDirPath, entry->d_name);

        if (stat(fullPath, &fileStatistics) == 0)
        {

            consumedSpace += fileStatistics.st_size;
        }
    }
    closedir(dir);
    return consumedSpace;
}
int convertKBToBytes(char *quota)
{
    return atol(quota) * 1024; 
}
int updateUserSpace(char *filename, char *username, int newSpace)
{
    FILE *file = fopen(filename, "r+");
    if (!file)
    {
        perror("Failed to open file");
        return -1;
    }

    char line[bufferSize];
    int found = 0;
    int oldSpace = 0;
    int space;
    while (fgets(line, sizeof(line), file))
    {
        
        char currentUsername[bufferSize];
        char password[bufferSize];

        sscanf(line, "%s %s %d", currentUsername, password, &space);
        oldSpace = space;
        printf("\nOldSpace :%d\n", oldSpace);
        fflush(stdout);
        if (strcmp(currentUsername, username) == 0)
        {
           
            fseek(file, -strlen(line), SEEK_CUR);
            fprintf(file, "%s %s %d\n", currentUsername, password, newSpace);
            fflush(file);
            found = 1;
            break;
        }
    }

    fclose(file);

    if (found)
    {
        printf("\nUpdated space for user %s from %d to %d\n", username, oldSpace, newSpace);
        fflush(stdout);
    }
    else
    {
        printf("User not found: %s\n", username);
    }

    return 0;
}
bool checkSpaceForClient(char *clientDirPath, int currFileSize,char* fileName,int* newSpace)
{
    bool isAlreadyFoundFile;
    int prevFileSize;
    long consumedSpace = calculateDirSize(clientDirPath, fileName, &isAlreadyFoundFile,&prevFileSize);
    char *spaceForClient = loadSpaceFromConfig(configurationFilePath);
    int bytesSize=convertKBToBytes(spaceForClient);
    if(isAlreadyFoundFile){
        if (consumedSpace - prevFileSize + currFileSize <= bytesSize)
        {
            *newSpace = bytesSize - consumedSpace - currFileSize +prevFileSize ;
            return true;
        }
        else{
            return false;
        }
    }

    if (consumedSpace + currFileSize <= bytesSize)
    {
        *newSpace = bytesSize-consumedSpace - currFileSize;

        return true;
    }
    else
    {
        return false;
    }
}

void uploadFolder(int *new_socket, char *userName, char *folderName, bool isClientExist, char* fileSize)
{
    char *filePath = makeDirForUser(*new_socket, userName, folderName, isClientExist);
    int fileSize_ = atoi(fileSize);
    int newSpace=0;
    if (checkSpaceForClient(filePath, fileSize_, folderName, &newSpace))
    {
        sendAckPacket(*new_socket);
        while(true)
        {
            char buffer[bufferSize] = {0};
            int  bytesRecv = recv(*new_socket, buffer, bufferSize, 0);
            buffer[bytesRecv] = '\0';
            if (strncmp(buffer, "END", 3) == 0){
                break;
            }

            if (isAbruptlyDisconnected(bytesRecv, new_socket))
            {
                return;
            }
            sendAckPacket(*new_socket);
            if (strncmp(buffer, "DIR", 3) == 0){
                bytesRecv = recv(*new_socket, buffer, bufferSize, 0);
                buffer[bytesRecv] = '\0';
                char *fullPath = (char *)malloc(bufferSize);
                snprintf(fullPath, bufferSize, "%s/%s", filePath, buffer);
                if (mkdir(fullPath, 0777) == -1)
                {
                    perror("mkdir");
                
                }
                //make dir for fullpath

            }
            if (strncmp(buffer, "FILE", 4) == 0)
            {
                bytesRecv = recv(*new_socket, buffer, bufferSize, 0);
                buffer[bytesRecv] = '\0';

                char *fullPath = (char *)malloc(bufferSize);
                snprintf(fullPath, bufferSize, "%s/%s", filePath, buffer);
                recvdFileContent(fullPath, new_socket);
            }

        }
    }
    else
    {
        char *msg = "$FAILURE$LOW_SPACE$";
        send(*new_socket, msg, strlen(msg), 0);
    }
}
void downloadFile(int *new_socket,char* userName,char* fileName,PathConfig pathStruct){
    char *fullPath = malloc(bufferSize);
    snprintf(fullPath, bufferSize, "%s/%s/%s", pathStruct.cloudStoragePath, userName, fileName);
    printf("%s\n", fullPath);
    fflush(stdout);
    FILE *fp = fopen(fullPath, "rb");
    char buffer[bufferSize] = {0};
    int bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
    if(isAbruptlyDisconnected(bytesRecv,new_socket)){
        return;
    }
    if (fp == NULL)
    {
        char buffer[bufferSize]={0};
        perror("File does not exit!!!");
        char *res = "FAILURE FILE_NOT_FOUND";
        send(*new_socket, res, strlen(res), 0);
    }
    else{
        char buffer[bufferSize]={0};
        char *res = "SUCCESS";
        send(*new_socket, res, strlen(res), 0);
      int bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
      if (isAbruptlyDisconnected(bytesRecv, new_socket))
      {
          return;
      }

        while(!feof(fp)){
            char buffer[bufferSize];
            int bytesRead=fread(buffer, 1, bufferSize, fp);
            if(bytesRead>0){
                send(*new_socket, buffer, bytesRead, 0);
            }
            if(bytesRead==0){
                break;
            }
           int bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
           if (isAbruptlyDisconnected(bytesRecv, new_socket))
           {
               return;
           }
           if(strncmp(buffer,"ACK",3)==0){
               continue;
           }
        }
        fclose(fp);
        char *msg = "EOF_Signal";
        send(*new_socket, msg, strlen(msg), 0);
        printf("FILE IS TRANSFERRED\n");
        fflush(stdout);
    }

}

void uploadFile(int *new_socket, char *userName, char *fileName, char *fileSize,bool isClientExist,PathConfig pathStruct)
{
    char *filePath = makeDirForUser(*new_socket, userName, fileName, isClientExist);
    int fileSize_ = atoi(fileSize);
    int newSpace = 0;
    if (checkSpaceForClient(filePath, fileSize_, fileName, &newSpace))
    {
        char buffer[bufferSize] = {0};
        int bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
        if (isAbruptlyDisconnected(bytesRecv, new_socket))
        {
            return;
        }
        sendAckPacket(*new_socket);;
        char *fullPath = (char *)malloc(bufferSize);
        snprintf(fullPath, bufferSize, "%s/%s", filePath, fileName);
        recvdFileContent(fullPath, new_socket);
        updateUserSpace(pathStruct.databasePath, userName, newSpace);
    }
    else
    {
        char *msg = "$FAILURE$LOW_SPACE$";
        send(*new_socket, msg, strlen(msg), 0);
    }

}
bool isClientDirHaveFiles(char *clientDirPath)
{
    DIR *dir;
    struct dirent *entry;
    struct stat fileStatistics;
    dir = opendir(clientDirPath);
    bool hasFiles = false;
    if (dir == NULL)
    {
        perror("No such Dir");
    }
    else{
        while((entry=readdir(dir))!=NULL){
            if(entry->d_name[0]=='.'){
                continue;
            }
            else{
                hasFiles = true;
                break;
            }
        }
        return hasFiles;
    }
}
void viewClientFolder(int *new_socket,char *clientDirPath)
{
    DIR *dir;
   
    struct dirent *entry;
    struct stat fileStatistics;
    dir = opendir(clientDirPath);
    printf(clientDirPath);
    fflush(stdout);
    if (dir == NULL)
    {
        perror("\n failed to open Directory");
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] != '.'){
            char *msg = "MORE";
            send(*new_socket, msg, strlen(msg), 0);
        }
        else if(entry->d_name[0]=='.'){
            continue;
        }
        char *fullPath = (char *)malloc(bufferSize);
        snprintf(fullPath, bufferSize, "%s/%s", clientDirPath, entry->d_name);

        if (stat(fullPath, &fileStatistics) == 0)
        {
            char inoStr[32];
            char typeStr[32];
            char ctimeStr[64];
            char mtimeStr[64];
            char fileSizeStr[100];

            snprintf(inoStr, sizeof(inoStr), "%ld", (long)entry->d_ino);
            if(entry->d_type==8){
                snprintf(typeStr, sizeof(typeStr), "%s","Regular file");
            }
            if (entry->d_type == 4)
            {
                snprintf(typeStr, sizeof(typeStr), "%s", "Directory");
            }

            snprintf(fileSizeStr, sizeof(fileSizeStr), "%lld", (long long)fileStatistics.st_size);

            snprintf(ctimeStr, sizeof(ctimeStr), "%s", ctime(&fileStatistics.st_ctime));
            snprintf(mtimeStr, sizeof(mtimeStr), "%s", ctime(&fileStatistics.st_mtime));

            ctimeStr[strcspn(ctimeStr, "\n")] = 0;
            mtimeStr[strcspn(mtimeStr, "\n")] = 0;
            char buffer[bufferSize];
            send(*new_socket, inoStr, strlen(inoStr), 0);

          int bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
          if (isAbruptlyDisconnected(bytesRecv, new_socket))
          {
              break;
          }
            send(*new_socket, typeStr, strlen(typeStr), 0);
           bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
           if (isAbruptlyDisconnected(bytesRecv, new_socket))
           {
               break;
           }
            send(*new_socket, entry->d_name, strlen(entry->d_name), 0);
           bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
           if (isAbruptlyDisconnected(bytesRecv, new_socket))
           {
               break;
           }
            send(*new_socket, fileSizeStr, strlen(fileSizeStr), 0);
            bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
          if (isAbruptlyDisconnected(bytesRecv, new_socket))
          {
              break;
          }

            send(*new_socket, ctimeStr, strlen(ctimeStr), 0);
          bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
          if (isAbruptlyDisconnected(bytesRecv, new_socket))
          {
              break;
          }

            send(*new_socket, mtimeStr, strlen(mtimeStr), 0);
          bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
          if (isAbruptlyDisconnected(bytesRecv, new_socket))
          {
              break;
          }
        }
        else
        {
            perror("Failed to get file statistics");
        }
    }
    char *msg = "ALL METADATA SENT";
    send(*new_socket, msg, strlen(msg), 0);

    closedir(dir);
}
char* trimTelnetString(char *str)
{
    char *end = str + strlen(str) - 1;

    // Remove trailing newline and carriage return characters
    while (end >= str && (*end == '\n' || *end == '\r'))
    {
        *end = '\0';
        end--;
    }
    return str;
}
void logOutClient(int *new_socket){
    if(*new_socket!=-1){
        sendAckPacket(*new_socket);;
        close(*new_socket);
        printf("\n%s\n", "Client LogOut");
        fflush(stdout);
        return;
    }
}
//thread func have specific style
void signUpHandle(int *new_socket, clientSession *cs, PathConfig pathStruct)
{
    char *userName;
    bool isClientExist = false;
    while (true)
    {
        userName = getUsernameAurPassword(new_socket, "Username");
        if (userName == NULL)
        {
            printf("Failed to get username\n");
            close(*new_socket);
            *new_socket = -1;
            return;
        }
        bool isUserNameExist = findUser(userName, NULL, pathStruct.databasePath);
        if (isUserNameExist)
        {
            char *msg = "USERNAME ALREADY EXIST";
            send(*new_socket, msg, strlen(msg), 0);
            char buffer[bufferSize] = {0};
            int bytesRecv = recv(*new_socket, buffer, bufferSize, 0);
            if (isAbruptlyDisconnected(bytesRecv, new_socket))
            {
                break;
            }
            continue;
        }
        else
        {
            char buffer[bufferSize] = {0};
            sendAckPacket(*new_socket);
            int bytesRecv = recv(*new_socket, buffer, bufferSize, 0);
            if (isAbruptlyDisconnected(bytesRecv, new_socket))
            {
                break;
            }
            break;
        }
    }
    cs->clientName = userName;
    char *password = getUsernameAurPassword(new_socket, "Password");
    if (password == NULL)
    {
        printf("\n%s\n", "Failed to get Password\n");
        fflush(stdout);
        close(*new_socket);
        *new_socket = -1;
        return;
    }
    cs->password = password;
    isClientExist = findUser(userName, NULL, pathStruct.databasePath);
    if (!isClientExist)
    {
        char *spaceForClient = loadSpaceFromConfig(configurationFilePath);
        int bytesSize = convertKBToBytes(spaceForClient);
        addUserOnDB(cs->clientName, cs->password, bytesSize,pathStruct);
        cs->isAuthenticated = true;
        makeDirForUser(*new_socket, userName, NULL, isClientExist);
    }
    if (isClientExist)
    {
        cs->isAuthenticated = true;
        cs->clientName = userName;
        cs->password = password;
    }
}
void signInHandle(int*new_socket,clientSession* cs,PathConfig pathStruct){
    // while(true)
    {
        char *userName;
        char *password;
        bool isClientExist = false;
        userName = getUsernameAurPassword(new_socket, "Username");
        if (userName == NULL)
        {
            printf("\n%s\n", "Failed to get UserName");
            fflush(stdout);
            close(*new_socket);
            *new_socket = -1;
            return;
        }
        userName=trimTelnetString(userName);

        password = getUsernameAurPassword(new_socket, "Password");
        if (password == NULL)
        {
            printf("\n%s\n", "Failed to get Password\n");
            fflush(stdout);
            close(*new_socket);
            *new_socket = -1;
            return;
        }
        password = trimTelnetString(password);
        isClientExist = findUser(userName, password, pathStruct.databasePath);
        if (isClientExist)
        {
            char *msg = "LOGIN SUCCESSFULLY";
            send(*new_socket, msg, strlen(msg), 0);
            cs->isAuthenticated = true;
            cs->clientName = userName;
            cs->password = password;
          
        }
        else
        {
            char *msg = "NOT LOGIN SUCCESSFULLY";
            send(*new_socket, msg, strlen(msg), 0);
           
            return;
        }
    }
}
void *clientHandler(void *_session){
    clientSession *cs = (clientSession*)_session;
    int temp_new_socket = cs->clientSocketId;
    int *new_socket = &temp_new_socket;
    char *userName=cs->clientName;
    char *password=NULL;
    char *cmdType = NULL;
    char *fileName = NULL;
    bool isClientExist = false;
    PathConfig pathStruct;
    loadPathsFromConfig(configurationFilePath, &pathStruct);
    while (true)
    {
        if(*new_socket==-1){
            break;
        }
        if(cs->isAuthenticated){
           isClientExist = true;
        }
        if (!cs->isAuthenticated)
        {
            char buffer[bufferSize] = {0};
            int bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
         if(isAbruptlyDisconnected(bytesRecv, new_socket)){
                break;
            }

            if (strncmp(buffer, "SIGNUP", 6 )==0||strncmp(buffer,"signup",6) == 0)
            {
                signUpHandle(new_socket,cs,pathStruct);
                break;
            }
            else if (strncmp(buffer, "SIGNIN", 6) == 0 || strncmp(buffer, "signin", 6) == 0)
            {
                signInHandle(new_socket, cs,pathStruct);
            }
        }
        char *fileSize = NULL;
        recvdCommand(new_socket, &cmdType, &fileName, &fileSize);
        
        if(strncmp(cmdType,"uploaddir",9)==0){
            uploadFolder(new_socket, cs->clientName, fileName,isClientExist,fileSize);
        }
        else if (strncmp(cmdType, "UPLOAD", 6) == 0 || strncmp(cmdType, "upload",6) == 0)
        {
            uploadFile(new_socket, cs->clientName, fileName, fileSize, isClientExist, pathStruct);
        }
        else if (strncmp(cmdType, "DOWNLOAD",8) == 0 || strncmp(cmdType, "download",8) == 0)
        {
            downloadFile(new_socket, cs->clientName, fileName,pathStruct);
        }
        else if (strncmp(cmdType, "LOGOUT", 6) == 0 || strncmp(cmdType, "logout", 6) == 0)
        {
            logOutClient(new_socket);
            break;
        }
        else if (strncmp(cmdType, "VIEW",4) == 0 || strncmp(cmdType, "view", 4) == 0)
        {
            bool isFound = findUser(cs->clientName, cs->password, pathStruct.databasePath);
            if (isFound)
            {
                char buffer[bufferSize];
                char *fullPath = malloc(bufferSize);
                snprintf(fullPath, bufferSize, "%s/%s", pathStruct.cloudStoragePath, cs->clientName);
                if (isClientDirHaveFiles(fullPath))
                {
                    char *msg = "SUCCESS";
                    send(*new_socket, msg, strlen(msg), 0);
                    int bytesRecv = recv(*new_socket, buffer, bufferSize, 0);
                    if(isAbruptlyDisconnected(bytesRecv,new_socket)){
                        break;
                    }
                    viewClientFolder(new_socket, fullPath);
                }
                else
                {
                    char *msg = "FAILURE";
                    send(*new_socket, msg, strlen(msg), 0);
                  int bytesRecv=recv(*new_socket, buffer, bufferSize, 0);
                  if (isAbruptlyDisconnected(bytesRecv, new_socket))
                  {
                      break;
                  }
                }
            }
        }
    }
    free(cmdType);
    free(fileName);
    return NULL;
}
void isThatWorked(int val,char * errorMsg,char * successMsg){
    if(val<0){
       perror(errorMsg);
       exit(EXIT_FAILURE);
    }else{
        printf("%s\n", successMsg);
    }
}
int main(int argc, char const *argv[])
{
    int serverSockfd;
    isThatWorked((serverSockfd = socket(AF_INET, SOCK_STREAM, 0)), "Socket failed", "Socket Created");
    int opt = 1;
    if (setsockopt(serverSockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr=INADDR_ANY;
    address.sin_port = htons(PORT);
    
    isThatWorked(bind(serverSockfd, (struct sockaddr *)&address, sizeof(address)), "Bind Failed", "Bind successfully");

    isThatWorked(listen(serverSockfd, MAXCLIENTS), "Not Listening", "Listening For Incoming Connections");
    
    //_____________________
  
    int *new_socket=malloc(sizeof(int));

    while (1)
    {
        printf("%s","Waiting for connections\n");
        fflush(stdout);
        ((*new_socket = accept(serverSockfd, (struct sockaddr *)&address, (socklen_t *)&addrlen)));
        clientSession *cSession = malloc(sizeof(clientSession));

        if(*new_socket>0){
            printf("%s %d\n","Accepted a Client Connection having id",*new_socket);
            cSession->clientSocketId = *new_socket;
            cSession->isAuthenticated = false;
        }
        // clientHandler(cSession);
        pthread_t t1;
        int val=pthread_create(&t1, NULL, clientHandler, cSession);
        if(val!=0){
            perror("THread is not successfully Created!!!");
        }
        else{
        printf("%s", "New Thread \n");
        fflush(stdout);
        }
    }

    close(serverSockfd);

    return 0;
}


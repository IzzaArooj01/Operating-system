#include <stdio.h>      //fstream
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   //mkdir
#include <unistd.h>     //chdir
#include <time.h>

#include <dirent.h>   //directory entry

#include <arpa/inet.h>
#include <pthread.h>

#include "loadconfig.h"
#include "queue.h"





bool clientFoundGlobally(char * clientName, int *c)
{
    // sem_wait(&client_lock);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
       
        
        if (clients[i].clientIntro.clientName!=NULL &&strcmp(clients[i].clientIntro.clientName, clientName) == 0 && !clients[i].isFree)
        {
            *c = i;
            // sem_post(&client_lock);
            return true;
        }
    }
    // sem_post(&client_lock);
    return false;
}
bool fileQueueFoundGlobally(int i,int* f,char* fileName){
    if (fileName == NULL)
    {
        return false;
    }

    for (int j = 0; j < MAX_FILES; j++)
    {
        if (clients[i].files[j].fName !=NULL && strcmp(clients[i].files[j].fName, fileName) == 0)
        {
            *f = j;
            return true;
        }
    }
    return false;
}

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
    sem_wait(&db_lock);
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
    sem_post(&db_lock);
}
bool findUser(char* userName,char* password,char* dbName){

    sem_wait(&db_lock);
    FILE *db_fd = fopen(dbName, "rb");
    char buffer[bufferSize]={0};
    int i = 0;
    bool isUserNameExist = 0;
    bool isUserExist = 0;
    while (fgets(buffer, sizeof(buffer), db_fd) != NULL)
    {
        if(buffer[0]=='\n' || buffer==NULL){
            sem_post(&db_lock);
            return isUserNameExist;
        }
        char * name=strtok(buffer, " ");
        if (strcmp(name, userName) == 0 && password==NULL)
        {
            isUserNameExist = 1;
            sem_post(&db_lock);
            return isUserNameExist;
        }
        char *pass= strtok(NULL, " ");
        pass[strlen(pass)] = '\0';
        if (password != NULL && strcmp(pass, password) == 0 && strcmp(name, userName) == 0)
        {
            isUserExist = 1;
            sem_post(&db_lock);
            return isUserExist;
        }
    }
    sem_post(&db_lock);
}
void sendAckPacket(int newSocket){
    char *ackMsg = "ACK";
    send(newSocket, ackMsg, strlen(ackMsg), 0);
}
void recvdCommand(int *newSocket,char **cmdType,char **fileName,char** fileSize){
    *cmdType = malloc(bufferSize);
    if(*cmdType==NULL){
        perror("\nFailed to allocate memory\n");
    }
    int bytesRecv=recv(*newSocket, *cmdType, bufferSize, 0);
    if(bytesRecv>0){
       (*cmdType)[bytesRecv] = '\0';
        sendAckPacket(*newSocket);
    }
    if(isAbruptlyDisconnected(bytesRecv,newSocket)){
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

        bytesRecv = recv(*newSocket, *fileName, bufferSize, 0);
        if(isAbruptlyDisconnected(bytesRecv,newSocket)){
            return;
        }
        if (bytesRecv > 0)
        {
            (*fileName)[bytesRecv] = '\0';
            sendAckPacket(*newSocket);
        }
        if (strncmp(*cmdType, "DOWNLOAD", 8) == 0 || strncmp(*cmdType, "download", 8) == 0)
        {
            printf("\n--------%s %s---------\n", *cmdType, *fileName);
        }
        if (strncmp(*cmdType, "UPLOAD", 6) == 0 || strncmp(*cmdType, "upload", 6) == 0 || strncmp(*cmdType, "uploaddir", 9) == 0)
        {
            bytesRecv = recv(*newSocket, *fileSize, bufferSize, 0);
            if(isAbruptlyDisconnected(bytesRecv,newSocket)){
                return;
            }
            if (bytesRecv > 0)
            {
                (*fileSize)[bytesRecv] = '\0';
                printf("\n--------%s %s %s---------\n", *cmdType, *fileName, *fileSize);
                fflush(stdout);
                sendAckPacket(*newSocket);
            }
        }
    }
}
char *makeDirForUser(int newSocket, char *userName,bool isClientExist)
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
void recvdFileContent(char *file_name,int *newSocket){
    
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
        bytesRecv = recv(*newSocket, buffer, sizeof(buffer), 0);
        if(isAbruptlyDisconnected(bytesRecv,newSocket)){
            break;
        }

        if (strcmp(buffer, "EOF_Signal") == 0)
        {
            fclose(fp);
            sendAckPacket(*newSocket);;
            break;
        }
        sendAckPacket(*newSocket);;
        fwrite(buffer, 1, bytesRecv, fp);
        fflush(fp);
        // printf("%s", buffer);
        // fflush(stdout);
    }
    fflush(stdout);
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
int countDigits(int n)
{
    if (n == 0)
    {
        return 1;
    }
    if (n < 0)
    {
        n = -n;
    }

    int count = 0;
    while (n > 0)
    {
        n /= 10;
        count++;
    }
    return count;
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
        printf("%s %s %d", currentUsername, password, &space);
        fflush(stdout);
        oldSpace = space;

        // prependZeros(&newSpace,oldSpace);
        if (strcmp(currentUsername, username) == 0)
        {
            char* sp= loadSpaceFromConfig(configurationFilePath);
            int bytesSize = convertKBToBytes(sp);
            int a = countDigits(bytesSize); 
            fseek(file, -strlen(line), SEEK_CUR);
            fprintf(file, "%s %s %0*d\n", currentUsername, password, a, newSpace);
            fflush(file);
            found = 1;
            break;
        }
    }
    // rewind(file);
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
            *newSpace = bytesSize - (consumedSpace + currFileSize - prevFileSize);
            printf("File alreday exist total spcae:%d Consumed space:%d Available Space :%d\n", bytesSize, consumedSpace - prevFileSize + currFileSize, *newSpace);
            fflush(stdout);
            return true;
        }
        else{
            return false;
        }
    }

    if (consumedSpace + currFileSize <= bytesSize)
    {
        *newSpace = bytesSize-consumedSpace - currFileSize;
        printf("total spcae:%d Consumed space:%d Available Space :%d\n", bytesSize, consumedSpace + currFileSize, *newSpace);
        fflush(stdout);

        return true;
    }
    else
    {
        return false;
    }
}

void uploadFolder(int *newSocket, char *userName, char *folderName, bool isClientExist, char* fileSize)
{
    char *filePath = makeDirForUser(*newSocket, userName,  isClientExist);
    int fileSize_ = atoi(fileSize);
    int newSpace=0;
    if (checkSpaceForClient(filePath, fileSize_, folderName, &newSpace))
    {
        sendAckPacket(*newSocket);
        while(true)
        {
            char buffer[bufferSize] = {0};
            int  bytesRecv = recv(*newSocket, buffer, bufferSize, 0);
            buffer[bytesRecv] = '\0';
            if (strncmp(buffer, "END", 3) == 0){
                break;
            }

            if (isAbruptlyDisconnected(bytesRecv, newSocket))
            {
                return;
            }
            sendAckPacket(*newSocket);
            if (strncmp(buffer, "DIR", 3) == 0){
                bytesRecv = recv(*newSocket, buffer, bufferSize, 0);
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
                bytesRecv = recv(*newSocket, buffer, bufferSize, 0);
                buffer[bytesRecv] = '\0';

                char *fullPath = (char *)malloc(bufferSize);
                snprintf(fullPath, bufferSize, "%s/%s", filePath, buffer);
                recvdFileContent(fullPath, newSocket);
            }

        }
    }
    else
    {
        char *msg = "$FAILURE$LOW_SPACE$";
        send(*newSocket, msg, strlen(msg), 0);
    }
}
void downloadFile(int *newSocket,char* userName,char* fileName){
    char *fullPath = malloc(bufferSize);
    snprintf(fullPath, bufferSize, "%s/%s/%s", pathStruct.cloudStoragePath, userName, fileName);
    printf("%s\n", fullPath);
    fflush(stdout);
    FILE *fp = fopen(fullPath, "rb");
    char buffer[bufferSize] = {0};
    int bytesRecv=recv(*newSocket, buffer, bufferSize, 0);
    if(isAbruptlyDisconnected(bytesRecv,newSocket)){
        return;
    }
    if (fp == NULL)
    {
        char buffer[bufferSize]={0};
        perror("File does not exit!!!");
        char *res = "FAILURE FILE_NOT_FOUND";
        send(*newSocket, res, strlen(res), 0);
    }
    else{
        char buffer[bufferSize]={0};
        char *res = "SUCCESS";
        send(*newSocket, res, strlen(res), 0);
      int bytesRecv=recv(*newSocket, buffer, bufferSize, 0);
      if (isAbruptlyDisconnected(bytesRecv, newSocket))
      {
          return;
      }

        while(!feof(fp)){
            char buffer[bufferSize];
            int bytesRead=fread(buffer, 1, bufferSize, fp);
            if(bytesRead>0){
                send(*newSocket, buffer, bytesRead, 0);
            }
            if(bytesRead==0){
                break;
            }
           int bytesRecv=recv(*newSocket, buffer, bufferSize, 0);
           if (isAbruptlyDisconnected(bytesRecv, newSocket))
           {
               return;
           }
           if(strncmp(buffer,"ACK",3)==0){
               continue;
           }
        }
        fclose(fp);
        char *msg = "EOF_Signal";
        send(*newSocket, msg, strlen(msg), 0);
        printf("FILE IS TRANSFERRED\n");
        fflush(stdout);
    }

}

void uploadFile(int *newSocket, char *userName, char *fileName, char *fileSize,bool isClientExist)
{
    char *filePath = makeDirForUser(*newSocket, userName, isClientExist);
    int fileSize_ = atoi(fileSize);
    int newSpace = 0;
    if (checkSpaceForClient(filePath, fileSize_, fileName, &newSpace))
    {
        char buffer[bufferSize] = {0};
        int bytesRecv=recv(*newSocket, buffer, bufferSize, 0);
        if (isAbruptlyDisconnected(bytesRecv, newSocket))
        {
            return;
        }
        sendAckPacket(*newSocket);;
        char *fullPath = (char *)malloc(bufferSize);
        snprintf(fullPath, bufferSize, "%s/%s", filePath, fileName);
        recvdFileContent(fullPath, newSocket);
        updateUserSpace(pathStruct.databasePath, userName, newSpace);
    }
    else
    {
        char *msg = "$FAILURE$LOW_SPACE$";
        send(*newSocket, msg, strlen(msg), 0);
    }

}
void readFromOneAndWriteToOther(char *sourceFileName, char *destFileName)
{
    FILE *sourceFile, *destFile;
    char ch;
    sourceFile = fopen(sourceFileName, "r");
    if (sourceFile == NULL)
    {
        printf("Error: Could not open source file %s\n", sourceFileName);
        exit(1); 
    }
    destFile = fopen(destFileName, "wb");
    if (destFile == NULL)
    {
        printf("Error: Could not create destination file %s\n", destFileName);
        fclose(sourceFile); 
        exit(1);
    }
    while ((ch = fgetc(sourceFile)) != EOF)
    {
        fputc(ch, destFile);
    }
    printf("File copied successfully.\n");
    fclose(sourceFile);
    fclose(destFile);
}
void FromTempFileToOriginal(int *newSocket, char *userName,char* fileName, char *fileSize, bool isClientExist)
{
    char *filePath = makeDirForUser(*newSocket, userName, isClientExist);
    int fileSize_ = atoi(fileSize);
    int newSpace = 0;
    if (checkSpaceForClient(filePath, fileSize_, fileName, &newSpace))
    {
        char *sourcePath = (char *)malloc(bufferSize);
        char *destPath = (char *)malloc(bufferSize);
        char *tempFileName = generateTempFile(userName, fileName,*newSocket);
        snprintf(sourcePath, bufferSize, "%s/%s", pathStruct.tempStoragePath, tempFileName);
        snprintf(destPath, bufferSize, "%s/%s", filePath, fileName);
        printf("source path:%s\n", sourcePath);
        printf("dest path:%s\n", destPath);
        fflush(stdout);
        readFromOneAndWriteToOther(sourcePath, destPath);
        updateUserSpace(pathStruct.databasePath, userName, newSpace);
    }
}
void uploadTempFile(int *newSocket, char *userName, char *fileName, char *fileSize, bool isClientExist)
{
    char *filePath = makeDirForUser(*newSocket, userName, isClientExist);
    int fileSize_ = atoi(fileSize);
    int newSpace = 0;
    if (checkSpaceForClient(filePath, fileSize_, fileName, &newSpace))
    {
        char buffer[bufferSize] = {0};
        int bytesRecv = recv(*newSocket, buffer, bufferSize, 0);
        if (isAbruptlyDisconnected(bytesRecv, newSocket))
        {
            return;
        }
        sendAckPacket(*newSocket);
        char *fullPath = (char *)malloc(bufferSize);
        char* tempFileName=generateTempFile(userName, fileName,*newSocket);
        snprintf(fullPath, bufferSize, "%s/%s", pathStruct.tempStoragePath, tempFileName);
        recvdFileContent(fullPath, newSocket);
       // updateUserSpace(pathStruct.databasePath, userName, newSpace);
    }
    else
    {
        char *msg = "$FAILURE$LOW_SPACE$";
        send(*newSocket, msg, strlen(msg), 0);
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
void viewClientFolder(int *newSocket,char *clientDirPath)
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
            send(*newSocket, msg, strlen(msg), 0);
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
            send(*newSocket, inoStr, strlen(inoStr), 0);

          int bytesRecv=recv(*newSocket, buffer, bufferSize, 0);
          if (isAbruptlyDisconnected(bytesRecv, newSocket))
          {
              break;
          }
            send(*newSocket, typeStr, strlen(typeStr), 0);
           bytesRecv=recv(*newSocket, buffer, bufferSize, 0);
           if (isAbruptlyDisconnected(bytesRecv, newSocket))
           {
               break;
           }
            send(*newSocket, entry->d_name, strlen(entry->d_name), 0);
           bytesRecv=recv(*newSocket, buffer, bufferSize, 0);
           if (isAbruptlyDisconnected(bytesRecv, newSocket))
           {
               break;
           }
            send(*newSocket, fileSizeStr, strlen(fileSizeStr), 0);
            bytesRecv=recv(*newSocket, buffer, bufferSize, 0);
          if (isAbruptlyDisconnected(bytesRecv, newSocket))
          {
              break;
          }

            send(*newSocket, ctimeStr, strlen(ctimeStr), 0);
          bytesRecv=recv(*newSocket, buffer, bufferSize, 0);
          if (isAbruptlyDisconnected(bytesRecv, newSocket))
          {
              break;
          }

            send(*newSocket, mtimeStr, strlen(mtimeStr), 0);
          bytesRecv=recv(*newSocket, buffer, bufferSize, 0);
          if (isAbruptlyDisconnected(bytesRecv, newSocket))
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
    send(*newSocket, msg, strlen(msg), 0);

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
void logOutClient(int *newSocket){
    if(*newSocket!=-1){
        sendAckPacket(*newSocket);;
        close(*newSocket);
        printf("\n%s\n", "Client LogOut");
        fflush(stdout);
        return;
    }
}
//thread func have specific style
void signUpHandle(int *newSocket, clientSession *cs)
{
    char *userName;
    bool isClientExist = false;
    while (true)
    {
        userName = getUsernameAurPassword(newSocket, "Username");
        if (userName == NULL)
        {
            printf("Failed to get username\n");
            close(*newSocket);
            *newSocket = -1;
            return;
        }
        bool isUserNameExist = findUser(userName, NULL, pathStruct.databasePath);
        if (isUserNameExist)
        {
            char *msg = "USERNAME ALREADY EXIST";
            send(*newSocket, msg, strlen(msg), 0);
            char buffer[bufferSize] = {0};
            int bytesRecv = recv(*newSocket, buffer, bufferSize, 0);
            if (isAbruptlyDisconnected(bytesRecv, newSocket))
            {
                break;
            }
            continue;
        }
        else
        {
            char buffer[bufferSize] = {0};
            sendAckPacket(*newSocket);
            int bytesRecv = recv(*newSocket, buffer, bufferSize, 0);
            if (isAbruptlyDisconnected(bytesRecv, newSocket))
            {
                break;
            }
            break;
        }
    }
    cs->clientName = userName;
    char *password = getUsernameAurPassword(newSocket, "Password");
    if (password == NULL)
    {
        printf("\n%s\n", "Failed to get Password\n");
        fflush(stdout);
        close(*newSocket);
        *newSocket = -1;
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
        makeDirForUser(*newSocket, userName,  isClientExist);
    }
    if (isClientExist)
    {
        cs->isAuthenticated = true;
        cs->clientName = userName;
        cs->password = password;
    }
}
void signInHandle(int*newSocket,clientSession* cs){
    // while(true)
    {
        char *userName;
        char *password;
        bool isClientExist = false;
        userName = getUsernameAurPassword(newSocket, "Username");
        if (userName == NULL)
        {
            printf("\n%s\n", "Failed to get UserName");
            fflush(stdout);
            close(*newSocket);
            *newSocket = -1;
            return;
        }
        userName=trimTelnetString(userName);

        password = getUsernameAurPassword(newSocket, "Password");
        if (password == NULL)
        {
            printf("\n%s\n", "Failed to get Password\n");
            fflush(stdout);
            close(*newSocket);
            *newSocket = -1;
            return;
        }
        password = trimTelnetString(password);
        isClientExist = findUser(userName, password, pathStruct.databasePath);
        if (isClientExist)
        {
            char *msg = "LOGIN SUCCESSFULLY";
            send(*newSocket, msg, strlen(msg), 0);
            cs->isAuthenticated = true;
            cs->clientName = userName;
            cs->password = password;
          
        }
        else
        {
            char *msg = "NOT LOGIN SUCCESSFULLY";
            send(*newSocket, msg, strlen(msg), 0);
           
            return;
        }
    }
}


void operationDeduce(char * cmdType,char* operation){
    if (strncmp(cmdType, "UPLOAD", 6) == 0 || strncmp(cmdType, "upload", 6) == 0)
    {
        *operation = 'W';
    }
    if (strncmp(cmdType, "DOWNLOAD", 8) == 0 || strncmp(cmdType, "download", 8) == 0){
        *operation = 'R';
    }
}

void addQueueOfClient(queueNode * node)
{
    int cInd = -1;
    int fInd = -1;
    char *tempFileName=NULL;
   
    printf("\n_____________\nClient Lock within addQueueOfClient\n");
    fflush(stdout);
    if (clientFoundGlobally(node->client.clientName, &cInd))
    {
        printf("%s", "client found\n");
        fflush(stdout);
        if (fileQueueFoundGlobally(cInd, &fInd, node->fileName))
        {
            if (node->operation == 'W')
            {
                printf("%s", "gen tempFile if c found\n");
                fflush(stdout);
                tempFileName=generateTempFile(node->client.clientName, node->fileName,node->client.clientSocketId);
                uploadTempFile(&node->client.clientSocketId, node->client.clientName, node->fileName, node->fileSize, node->client.isClientExist);
                printf("%s", "\nuploaded tempfile if c found\n");
                fflush(stdout);
            }
            enqueueFileOperation(&clients[cInd].files[fInd], node, tempFileName);
            sem_wait(&clientCountLock);
            activeClientCountForCop++;
            sem_post(&clientCountLock);
        }
        else if (!fileQueueFoundGlobally(cInd, &fInd, node->fileName))
        {

            // strcpy(clients[cInd].files[clients[cInd].fileCount].fName, node->fileName);
            clients[cInd].files[clients[cInd].fileCount].fName = node->fileName;
            sem_init(&clients[cInd].files[clients[cInd].fileCount].lock, 0, 1);
            if (fileQueueFoundGlobally(cInd, &fInd, node->fileName))
            {
                if (node->operation == 'W')
                {
                    tempFileName = generateTempFile(node->client.clientName, node->fileName,node->client.clientSocketId);
                    uploadTempFile(&node->client.clientSocketId, node->client.clientName, node->fileName, node->fileSize, node->client.isClientExist);
                }
                if (clients[cInd].fileCount==fInd){
                    printf("%s", "going to enque");
                    fflush(stdout);
                   
                    enqueueFileOperation(&clients[cInd].files[fInd], node, tempFileName);
                    clients[cInd].fileCount++;
                    sem_wait(&clientCountLock);
                    activeClientCountForCop++;
                    sem_post(&clientCountLock);
                }
                // *tempQueue = clients[cInd].files[fInd].queue;
            }
        }
       
    }
    else 
    {
        printf("%s", "client Not  found\n");
        fflush(stdout);
       
        for (int cInd = 0; cInd < MAX_CLIENTS; cInd++)
        {
            if (clients[cInd].isFree)
            {
                printf("%s", "found free slot\n");
                fflush(stdout);

                clients[cInd].clientIntro.clientName = node->client.clientName;
                clients[cInd].clientIntro.clientSocketId = node->client.clientSocketId;
                clients[cInd].clientIntro.password = node->client.password;
                clients[cInd].clientIntro.isAuthenticated = node->client.isAuthenticated;

                clients[cInd].isFree = 0;
               
                clients[cInd].files[clients[cInd].fileCount].fName = node->fileName;

                // strcpy(clients[cInd].files[clients[cInd].fileCount].fName, node->fileName);
                sem_init(&clients[cInd].files[clients[cInd].fileCount].lock, 0, 1);
                printf("%s", "make client\n");
                fflush(stdout);
                if (fileQueueFoundGlobally(cInd, &fInd, node->fileName))
                {
                  
                    printf("%s", "file queue found\n");
                    fflush(stdout);
                    if (node->operation == 'W')
                    {
                       
                        tempFileName = generateTempFile(node->client.clientName, node->fileName,node->client.clientSocketId);
                        printf("%s", "generated temp file\n");
                        fflush(stdout);
                        uploadTempFile(&node->client.clientSocketId, node->client.clientName, node->fileName, node->fileSize, node->client.isClientExist);
                        printf("%s", "\nupload on  generated temp file\n");
                        fflush(stdout);
                    }
                    if (clients[cInd].fileCount==fInd){
                    printf("%s LOck :%d", "going to enque",clients[cInd].files[fInd].lock);
                    fflush(stdout);
                    }
                    
                    enqueueFileOperation(&clients[cInd].files[fInd], node, tempFileName);
                    printf("%s", "enqued file operation\n");
                    fflush(stdout);
                    sem_wait(&clientCountLock);
                    activeClientCountForCop++;
                    sem_post(&clientCountLock);
                    // *tempQueue = clients[cInd].files[fInd].queue;
                }
                clients[cInd].fileCount++;
                break;
            }
        }
        
    }
   
}
void printNode(queueNode* node){
    if (node == NULL)
    {
        printf("The node is NULL.\n");
        return;
    }

    // Print client session details
    printf("\n____________Node Status____________\n");
    printf("Client Socket ID: %d\n", node->client.clientSocketId);
    printf("Client Name: %s\n", node->client.clientName ? node->client.clientName : "(null)");
    printf("Password: %s\n", node->client.password ? node->client.password : "(null)");
    printf("Is Authenticated: %s\n", node->client.isAuthenticated ? "true" : "false");
    printf("Is Client Exist: %s\n", node->client.isClientExist ? "true" : "false");

    // Print queueNode details
   
    printf("Operation: %c\n", node->operation);
    // if(node->fileName!=NULL){
    //     printf("File Name: %d\n", strlen(node->fileName));
    // }
    printf("File Name: %s\n", node->fileName ? node->fileName : "(null)");
    printf("File Size: %s\n", node->fileSize ? node->fileSize : "(null)");
    printf("Temporary File for Chunks: %s\n", node->tempFileForChunks ? node->tempFileForChunks : "(null)");
    printf("Timestamp: %s", ctime(&node->timestamp)); // ctime() includes a newline
    printf("Is Queued: %s\n", node->isQueued ? "true" : "false");
    printf("\n_____________________________________\n");
    fflush(stdout);
}
int getClientIndRelativeToSocketId(int *newSocket, bool indOrCop) // 0 for ind And 1 for coop
{
    int getClientInd = -1;
    pthread_mutex_lock(&arrIdLock);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        printf("\n^^^^^^^^^%d  %d %d ^^^^^^^^^^^^\n", arrIds[i].availToP, arrIds[i].cSocketId, *newSocket);
        fflush(stdout);
        if (arrIds[i].availToP == 0 && arrIds[i].cSocketId == *newSocket)
        {
            getClientInd = i;
            if (indOrCop)
            {
                arrIds[i].availToP =1;
                printf("\n\n!!!!!!!!!!!!!!!!!! Triggered arrIds[clientInd].availToP id :%d\n", arrIds[getClientInd].availToP);
                fflush(stdout);
            }
            break;
        }
    }
    pthread_mutex_unlock(&arrIdLock);
    return getClientInd;
}
void *readerThread(void *arg)
{
    FileQueue *queue = (FileQueue *)arg;
    while (1)
    {
        sem_wait(&queue->lock);
       
        if (queue->size>0 && queue->front->operation == 'R')
        {
            queueNode *node = dequeueFileOperation(queue);
            printf("\n__________________Inside Reader______________\n");
            fflush(stdout);
            printNode(node);
            sem_post(&queue->lock);
            sem_wait(&accessLock);
            sem_wait(&readerCLock);
            rc++;
            if (rc == 1)
            {
                sem_wait(&writerLock);
            }
            sem_post(&readerCLock);
            sem_post(&accessLock);
            downloadFile(&node->client.clientSocketId, node->client.clientName, node->fileName);

            sem_wait(&readerCLock);
            rc--;
            if (rc == 0)
            {
                sem_post(&writerLock);
            }
            sem_post(&readerCLock);
            int clientIdx = getClientIndRelativeToSocketId(&node->client.clientSocketId,1);
            sem_post(&clientSemaphores[clientIdx]);
        }
        else
        {
            sem_post(&queue->lock);
        }
    }
    return NULL;
}

void *writerThread(void *arg)
{
    FileQueue *queue = (FileQueue *)arg;
    while (1)
    {
            sem_wait(&queue->lock);
           
            if (queue->size>0 && queue->front->operation == 'W')
            {

                printf("\n__________________Inside Writer______________\n");
                fflush(stdout);
                
                queueNode *node = dequeueFileOperation(queue);
                printNode(node);
                sem_post(&queue->lock);
                sem_wait(&writerCLock);
                wc++;
                if (wc == 1)
                {
                    sem_wait(&readerLock);
                    sem_wait(&accessLock);
                }
                sem_post(&writerCLock);

                sem_wait(&writerLock);
                char *tempFileName = generateTempFile(node->client.clientName, node->fileName,node->client.clientSocketId);
                FromTempFileToOriginal(&node->client.clientSocketId, node->client.clientName, node->fileName, node->fileSize, node->client.isClientExist);
                
                // writedata(item.filename, item.data);
                sem_post(&writerLock);
                

                sem_wait(&writerCLock);
                wc--;
                if (wc == 0)
                {
                    sem_post(&readerLock);
                    sem_post(&accessLock);
                }
                sem_post(&writerCLock);
                int clientIdx = getClientIndRelativeToSocketId(&node->client.clientSocketId,1);
                sem_post(&clientSemaphores[clientIdx]);
            }
            else
            {
                sem_post(&queue->lock);
            }
        
    }
    return NULL;
}

void *fileHandlerThread(void *arg)
{
    while (1)
    {
        sem_wait(&client_lock);
        sem_wait(&clientCountLock);
        while (activeClientCountForCop > 0)
        {
            printf("active client for  %d\n", activeClientCountForCop);
            fflush(stdout);
            sem_post(&clientCountLock);

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if(!clients[i].isFree){
                    printf("have client for i %d\n",i);
                    fflush(stdout);
                    for (int j = 0; j < clients[i].fileCount;j++){
                        printf("check size>0 for j %d\n",j);
                        fflush(stdout);

                        if (clients[i].files[j].size > 0)
                        {
                            // Create reader and writer threads for each non-empty file queue
                            printf("reader and writer thread for jind:%d %s %d created file %s\n",j, clients[i].clientIntro.clientName,clients[i].clientIntro.clientSocketId,clients[i].files[j].fName);
                            fflush(stdout);
                            pthread_create(&readerThreadId, NULL, readerThread, &clients[i].files[j]);
                            pthread_create(&writerThreadId, NULL, writerThread, &clients[i].files[j]);
                            sem_wait(&clientCountLock);
                            activeClientCountForCop = activeClientCountForCop - clients[i].files[j].size;
                            sem_post(&clientCountLock);

                            //____________________________________________
                            //in case of Global Queue
                            //____________________________________________
                            // sem_wait(&tempQueue->lock);
                            // sem_wait(&globalQueue.lock);
                            // while(tempQueue->size>0)
                            // {
                            //     queueNode *node = dequeueFileOperation(&clients[i].files[j]);
                            //     enqueGlobalQueue(&globalQueue,node);
                            // }
                            // sem_post(&tempQueue->lock);
                            // sem_post(&globalQueue.lock);

                        
                        }
                    }
                }
            }
        }
        sem_post(&client_lock);
        sem_post(&clientCountLock);
    }
}

void* processingIndRequestQueue(void* arg){
    while(true){
        queueNode *node;
        pthread_mutex_lock(&independentQueue.requestsQLock);
    
        while (independentQueue.size > 0)
        {
            node = dequeue(&independentQueue);
            int clientInd = getClientIndRelativeToSocketId(&node->client.clientSocketId,0);
            sem_post(&clientSemaphores[clientInd]);
        }
        
        pthread_mutex_unlock(&independentQueue.requestsQLock); 
    }
}
void printArrIds()
{
    pthread_mutex_lock(&arrIdLock);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        printf("%d , %d\n", arrIds[i].availToP, arrIds[i].cSocketId);
        fflush(stdout);
    }
    pthread_mutex_unlock(&arrIdLock);
}
void printQueueOfClient(){
    for (int i = 0;i<MAX_CLIENTS;i++){
        if(!clients[i].isFree){
            printf("\n------Name:%s  id:%d------\n", clients[i].clientIntro.clientName, clients[i].clientIntro.clientSocketId);
            fflush(stdout);
            for (int j = 0; j < clients[i].fileCount; j++)
            {
                printf("filename:%s\n", clients[i].files[j].fName);
                fflush(stdout);
                printQueue(&clients[i].files[j]);
            }
        }
    }
}
void* processingCopRequestQueue(void* arg){
    while(true){
       
        pthread_mutex_lock(&cooperativeQueue.requestsQLock);
        sem_wait(&client_lock);
        while (cooperativeQueue.size > 0)
        {
            printf("Coopertaive size before going into Multilevel Queue :%d \n",cooperativeQueue.size);
            fflush(stdout);
            queueNode *node = dequeue(&cooperativeQueue);
            printf("Coopertaive node will go into Multilevel Queue\n");
            fflush(stdout);
            printNode(node);
            addQueueOfClient(node);
            printf(" node is added into Multilevel Queue\n");
            fflush(stdout);
        }
        pthread_mutex_unlock(&cooperativeQueue.requestsQLock);
        sem_post(&client_lock);
    }
}
void *timeMonitor(void *args)
{
    while (1)
    {
        sleep(SPLIT_INTERVAL);
        printf("sleep finished\n");
        fflush(stdout);
        
        pthread_mutex_lock(&requestQueue.requestsQLock);
        while(requestQueue.size>0)
        {
            printf("%s", "request queue lock qcuired of timer thraed \n");
            fflush(stdout);
            pthread_mutex_lock(&independentQueue.requestsQLock);
            pthread_mutex_lock(&cooperativeQueue.requestsQLock);
            divisionOfRequestQueue();
            printf("after threshold its splits\n");
            fflush(stdout);
            pthread_mutex_unlock(&independentQueue.requestsQLock);
            pthread_mutex_unlock(&cooperativeQueue.requestsQLock);
            printf("%s\n", "After division");
            fflush(stdout);

            printRequestQueue(&independentQueue, "Independent Queue");
            printRequestQueue(&cooperativeQueue, "Cooperative Queue");
            printf("%s\n", "division Ended");
            fflush(stdout);
        }
        pthread_mutex_unlock(&requestQueue.requestsQLock);
    }
}

void *clientHandler(void *_session)
{
    clientSession *cs = (clientSession *)_session;
    int temp_newSocket = cs->clientSocketId;
    int *newSocket = &temp_newSocket;
    char *userName = cs->clientName;
    char *password = NULL;
    char *cmdType = NULL;
    char *fileName = NULL;
    bool isClientExist = false;
   
    loadPathsFromConfig(configurationFilePath, &pathStruct);
    while (true)
    {
        if (*newSocket == -1)
        {
            break;
        }
        if (cs->isAuthenticated)
        {
            isClientExist = true;
        }
        if (!cs->isAuthenticated)
        {
            char buffer[bufferSize] = {0};
            int bytesRecv = recv(*newSocket, buffer, bufferSize, 0);
            if (isAbruptlyDisconnected(bytesRecv, newSocket))
            {
                break;
            }

            if (strncmp(buffer, "SIGNUP", 6) == 0 || strncmp(buffer, "signup", 6) == 0)
            {
                signUpHandle(newSocket, cs);
                break;
            }
            else if (strncmp(buffer, "SIGNIN", 6) == 0 || strncmp(buffer, "signin", 6) == 0)
            {
                signInHandle(newSocket, cs);
            }
        }
        char *fileSize = NULL;
        recvdCommand(newSocket, &cmdType, &fileName, &fileSize);
        char operation = '\0';
        operationDeduce(cmdType, &operation);
        printf("%s\n", "operation deduced");
        fflush(stdout);

        pthread_mutex_lock(&arrIdLock);
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (arrIds[i].availToP == -1)
            {
                arrIds[i].cSocketId = *newSocket;
                pthread_t thread_id = pthread_self();
                arrIds[i].threadId = thread_id;
                arrIds[i].availToP = 0;
                break;
            }
        }
        pthread_mutex_unlock(&arrIdLock);

        printf("%s\n", "update aviable to process");
        fflush(stdout);
        if(operation=='\0'){
            if (strncmp(cmdType, "LOGOUT", 6) == 0 || strncmp(cmdType, "logout", 6) == 0)
            {
                logOutClient(newSocket);
                break;
            }
            else if (strncmp(cmdType, "VIEW", 4) == 0 || strncmp(cmdType, "view", 4) == 0)
            {
                printf("%s\n", "inside view");
                fflush(stdout);
                bool isFound = findUser(cs->clientName, cs->password, pathStruct.databasePath);
                if (isFound)
                {
                    printf("%s\n", "found");
                    fflush(stdout);
                    char buffer[bufferSize];
                    char *fullPath = malloc(bufferSize);
                    snprintf(fullPath, bufferSize, "%s/%s", pathStruct.cloudStoragePath, cs->clientName);
                    if (isClientDirHaveFiles(fullPath))
                    {
                        char *msg = "SUCCESS";
                        send(*newSocket, msg, strlen(msg), 0);
                        int bytesRecv = recv(*newSocket, buffer, bufferSize, 0);
                        if (isAbruptlyDisconnected(bytesRecv, newSocket))
                        {
                            break;
                        }
                        viewClientFolder(newSocket, fullPath);
                    }
                    else
                    {
                        char *msg = "FAILURE";
                        send(*newSocket, msg, strlen(msg), 0);
                        int bytesRecv = recv(*newSocket, buffer, bufferSize, 0);
                        if (isAbruptlyDisconnected(bytesRecv, newSocket))
                        {
                            break;
                        }
                    }
                }
            }
        }

        if (operation != '\0')
        {
            printf("Socket id :%d ----->", *newSocket);
            fflush(stdout);
            printf("%s", "requests \n");
            fflush(stdout);
            char *tempFileForChunks = NULL;
            
            queueNode *newNode = createQueueNode(*cs, operation, fileName, tempFileForChunks,isClientExist,fileSize);
            printf("Socket id :%d ----->", *newSocket);
            fflush(stdout);
            printf("%s", "new node created \n");
            fflush(stdout);
            if (newNode == NULL)
            {
                printf("%s", "node is null");
                fflush(stdout);
            }
            enqueue(&requestQueue, newNode);
            printf("Socket id :%d ----->\n", *newSocket);
            fflush(stdout);
            printRequestQueue(&requestQueue, "Request Queue");
            printRequestQueue(&independentQueue, "Independent Queue");
            printRequestQueue(&cooperativeQueue, "Cooperative Queue");
            printQueueOfClient();

           
           // while (1)
            
            bool found = false;
            int clientInd=getClientIndRelativeToSocketId(newSocket,0);
            printf("\n\n!!!!!!!!!!!!!!!!!!client id :%d\n", clientInd);
            fflush(stdout);
            sem_wait(&clientSemaphores[clientInd]);
            printf("\n\n!!!!!!!!!!!!!!!!!!arrIds[clientInd].availToP id :%d\n", arrIds[clientInd].availToP);
            fflush(stdout);
            pthread_mutex_lock(&arrIdLock);

            if (!arrIds[clientInd].availToP)
            {
                //independent
                arrIds[clientInd].availToP = -1;
                pthread_mutex_unlock(&arrIdLock);
                printf("Socket id :%d ----->%s\n", *newSocket, "Ind Q processed");
                fflush(stdout);

                if (strncmp(cmdType, "UPLOAD", 6) == 0 || strncmp(cmdType, "upload", 6) == 0)
                {
                    printf("%s\n", "starts uploading");
                    fflush(stdout);
                    uploadFile(newSocket, cs->clientName, fileName, fileSize, isClientExist);
                    printf("%s\n", "uploaded");
                    fflush(stdout);
                }
                else if (strncmp(cmdType, "DOWNLOAD", 8) == 0 || strncmp(cmdType, "download", 8) == 0)
                {
                    printf("%s\n", "starts downloading");
                    fflush(stdout);
                    downloadFile(newSocket, cs->clientName, fileName);
                    printf("%s\n", "downloaded");
                    fflush(stdout);
                }
                // break;
            }
            else{
                //coopertaive
                arrIds[clientInd].availToP = -1;
                pthread_mutex_unlock(&arrIdLock);
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
void init(){
    for (int i = 0; i < MAX_CLIENTS;i++){
        arrIds[i].availToP = -1;
    }
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].isFree = 1;
        clients[i].fileCount = 0;
        clients[i].clientIntro.clientName = NULL;
        for (int j = 0; j < MAX_FILES;j++){
            clients[i].files[j].size = 0;
            clients[i].files[j].fName = NULL;
        }
    }
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        sem_init(&clientSemaphores[i], 0, 0); 
    }
    sem_init(&db_lock, 0, 1);
    sem_init(&clientCountLock, 0, 1);
    sem_init(&client_lock, 0, 1);
    sem_init(&readerCLock, 0, 1);
    sem_init(&writerCLock, 0, 1);
    sem_init(&readerLock, 0, 1);
    sem_init(&writerLock, 0, 1);
    sem_init(&accessLock, 0, 1);

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

    isThatWorked(listen(serverSockfd, MAX_CLIENTS), "Not Listening", "Listening For Incoming Connections");
    
    //_____________________
  
    int *newSocket=malloc(sizeof(int));
    init();

    pthread_create(&timerThread, NULL, timeMonitor, NULL);
    pthread_create(&f1, NULL, fileHandlerThread,0);
    pthread_create(&f2, NULL, processingIndRequestQueue, 0);
    pthread_create(&f3, NULL, processingCopRequestQueue, 0);

    while (1)
    {
        printf("%s","Waiting for connections\n");
        fflush(stdout);
        ((*newSocket = accept(serverSockfd, (struct sockaddr *)&address, (socklen_t *)&addrlen)));
        clientSession *cSession = malloc(sizeof(clientSession));

        if(*newSocket>0){
            printf("%s %d\n","Accepted a Client Connection having id",*newSocket);
            cSession->clientSocketId = *newSocket;
            cSession->isAuthenticated = false;
        }
        // clientHandler(cSession);
        pthread_t t1;
        int val=pthread_create(&t1, NULL, clientHandler, cSession);
        if(val!=0)
        {
            perror("THread is not successfully Created!!!");
        }
        else
        {
            printf("%s", "Client Hnadler Thread Created For socket id:%d\n",*newSocket);
            fflush(stdout);
        }
    }
    pthread_join(timerThread, NULL);
    pthread_join(f1, NULL);
    pthread_join(f2, NULL);
    pthread_join(f3, NULL);
    for (int i = 0; i < MAX_CLIENTS;i++)
    {
        if(arrIds[i].cSocketId>0){
            
            pthread_join(arrIds[i].threadId, NULL);
        }
    }
    close(serverSockfd);

    return 0;
}


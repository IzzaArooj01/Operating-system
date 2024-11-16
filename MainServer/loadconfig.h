#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#define MAX_PATH_LEN 200
typedef struct
{
    char databasePath[MAX_PATH_LEN];
    char cloudStoragePath[MAX_PATH_LEN];
    char tempStoragePath[MAX_PATH_LEN];
    char output_path[MAX_PATH_LEN];
} PathConfig;
char* readConfigFile(char * fileName){
    FILE *file = fopen(fileName, "r");
    if (!file)
    {
        perror("Failed to open config file");
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char *)malloc(length + 1);
    fread(data, 1, length, file);
    fclose(file);
    data[length] = '\0';
    return data;
}
int loadPathsFromConfig(char *filename, PathConfig *config) {

    char *data =readConfigFile(filename);
    cJSON *json = cJSON_Parse(data);
    free(data); 
    if (!json)
    {
        printf("JSON parse error: %s\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON *paths = cJSON_GetObjectItem(json, "paths");
    if (paths)
    {
        cJSON *databasePath = cJSON_GetObjectItem(paths, "databasePath");
        cJSON *cloudStoragePath = cJSON_GetObjectItem(paths, "cloudStoragePath");
        cJSON *tempStoragePath = cJSON_GetObjectItem(paths, "tempStoragePath");

        if (databasePath)
            strncpy(config->databasePath, databasePath->valuestring, MAX_PATH_LEN);
        if (cloudStoragePath)
            strncpy(config->cloudStoragePath, cloudStoragePath->valuestring, MAX_PATH_LEN);
        if (tempStoragePath)
            strncpy(config->tempStoragePath, tempStoragePath->valuestring, MAX_PATH_LEN);
    }

    cJSON_Delete(json); 
    return 0;
}
char* loadSpaceFromConfig(char* fileName)
{
    char* data=readConfigFile(fileName);
    char *space=(char*)malloc(MAX_PATH_LEN);
    cJSON *json = cJSON_Parse(data);
   
    if (!json)
    {
        printf("JSON parse error: %s\n", cJSON_GetErrorPtr());
    }

    cJSON *spaceItem = cJSON_GetObjectItem(json, "storageQuota");
    if (space)
    {
        strncpy(space, spaceItem->valuestring, MAX_PATH_LEN);
    }
    return space;
}
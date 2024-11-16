#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#define configurationFilePath "/home/izza/OperatingSystem/Lab1/MainServer/server.config"
#define bufferSize 1024
#define PORT 8080
#define MAX_CLIENTS 5
#define true 1
#define false 0
#define MAX_FILES 5
#define MAX_FILENAME_LENGTH 255
#define MAX_REQUESTS 4
#define THRESHOLD 0.5
#define SPLIT_INTERVAL 15
#define connected
PathConfig pathStruct;

pthread_t timerThread;
pthread_t f1;
pthread_t f2;
pthread_t f3;
pthread_t readerThreadId;
pthread_t writerThreadId;

sem_t readerLock;
sem_t accessLock;
sem_t writerLock;
sem_t readerCLock;
sem_t writerCLock;
int rc = 0;
int wc = 0;

typedef int bool;
typedef struct
{
    int clientSocketId;
    char *clientName;
    char *password;
    bool isAuthenticated;
    bool isClientExist;
} clientSession;

typedef struct queueNode
{
    clientSession client;
    char operation; // W for "upload" or R for "download"
    char *fileName;
    char* fileSize;
    char *tempFileForChunks;
    time_t timestamp;
    bool isQueued;
    struct queueNode *next;
} queueNode;

typedef struct requestsQueue
{
    queueNode *front;
    queueNode *rear;
    int size;
    pthread_mutex_t requestsQLock;
    pthread_cond_t queueNotFull;  // Signals when there's space to enqueue
    pthread_cond_t queueNotEmpty; // Signals when there are items to dequeue
} requestsQueue;

requestsQueue requestQueue;
requestsQueue independentQueue;
requestsQueue cooperativeQueue;

typedef struct availableToProcessIds
{
    int cSocketId;
    bool availToP;
    pthread_t threadId;
} availableToProcessIds;
availableToProcessIds arrIds[MAX_CLIENTS];
pthread_mutex_t arrIdLock;

// done till here

typedef struct FileQueue
{
    queueNode *front;
    queueNode *rear;
    int size;
    sem_t lock; // Lock for queue operations
    char *fName;
} FileQueue;


// Structure for managing client connections
typedef struct clientDetails
{
    int isFree;
    clientSession clientIntro;
    FileQueue files[MAX_FILES]; // Array of files for this client
    int fileCount;              // Number of files for the client
} clientDetails;
clientDetails clients[MAX_CLIENTS];
int activeClientCountForCop = 0; // Count of clients
sem_t clientCountLock;

sem_t client_lock; // Lock for client management

sem_t clientSemaphores[MAX_CLIENTS]; // Array of semaphores for clients
sem_t db_lock;

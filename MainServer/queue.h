
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"

void enqueue(requestsQueue *queue, queueNode *node);
queueNode *createQueueNode(clientSession client, char operation, const char *fileName, const char *tempFileForChunks,bool isClientExist,char* fileSize)
{
    queueNode *newNode = (queueNode *)malloc(sizeof(queueNode));
    if (!newNode)
    {
        perror("Failed to allocate memory for new node");
        return NULL;
    }
    newNode->client = client;
    newNode->operation = operation;
    newNode->fileName = strdup(fileName);
    newNode->tempFileForChunks = NULL;
    newNode->client.isClientExist = isClientExist;
    newNode->fileSize = fileSize;
    newNode->timestamp = time(NULL);
    newNode->isQueued = false;
    newNode->next = NULL;
    return newNode;
}
queueNode *dequeue(requestsQueue *queue)
{

    // while (queue->size == 0)
    // {
    //     pthread_cond_wait(&queue->queueNotEmpty, &queue->requestsQLock);
    // }
    queueNode *dequeuedNode = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL)
    {
        queue->rear = NULL;
    }
    queue->size--;
    // pthread_cond_signal(&queue->queueNotFull);
    return dequeuedNode;
}
void printRequestQueue(requestsQueue *queue, char *name)
{

    pthread_mutex_lock(&queue->requestsQLock);

    queueNode *current = queue->front;
    int count = 0;

    printf("%s contents:\n", name);
    while (current != NULL)
    {
        printf("Client Name: %s,socket id: %d, Operation: %c, File name :%s\n", current->client.clientName, current->client.clientSocketId, current->operation, current->fileName);
        current = current->next;
        count++;
    }

    if (count == 0)
    {
        printf("The queue is empty.\n");
    }

    pthread_mutex_unlock(&queue->requestsQLock);
}
void enqueueIOrC(requestsQueue *queue, queueNode *node)
{
    // pthread_mutex_lock(&queue->requestsQLock);
    if (node == NULL)
    {
        printf("%s", "node is not found  \n");
        fflush(stdout);
    }

    // node->timestamp = time(NULL);
    if (queue->rear == NULL)
    {
        queue->front = queue->rear = node;
    }
    else
    {
        queue->rear->next = node;
        queue->rear = node;
    }
    queue->size++;
    // pthread_mutex_unlock(&queue->requestsQLock);
}
void divisionOfRequestQueue()
{
    queueNode *node;
    while (requestQueue.size > 0)
    {
        node = dequeue(&requestQueue);
        if (node->isQueued)
        {
            continue;
        }

        if (node == NULL)
        {
            return;
        }
        bool isIndependent = true;

        queueNode *temp = requestQueue.front;
        int c = 0;
        while (temp != NULL)
        {
            if (strcmp(temp->client.clientName, node->client.clientName) == 0 && strcmp(temp->fileName, node->fileName) == 0)
            {
                isIndependent = false;
                c++;
                if (c == 1)
                {
                    node->isQueued = true;
                    enqueueIOrC(&cooperativeQueue, node);
                    printf(" added %d node on Cooperative  Queue\n", node->client.clientSocketId);
                    fflush(stdout);
                    
                }
                temp->isQueued = true;
                enqueueIOrC(&cooperativeQueue, temp);
                printf(" added %d node on Cooperative  Queue\n", temp->client.clientSocketId);
                fflush(stdout);
              

                break;
            }
            temp = temp->next;
        }
        if (isIndependent)
        {
            enqueueIOrC(&independentQueue, node);
            printf(" added %d node on Independent Queue\n", node->client.clientSocketId);
            fflush(stdout);
           
        }
    }
    
}

void enqueue(requestsQueue *queue, queueNode *node)
{
    pthread_mutex_lock(&queue->requestsQLock);
    printf("%s", "request queue lock qcuired of client handler thraed \n");
    fflush(stdout);

    if (node == NULL)
    {
        printf("%s", "node is not found  \n");
    }
    // if (queue->size == 1)
    // {
    //     startDivisionTimer(); // Start timer to check threshold
    // }
    // if(queue->size==1){
    //     resetTimer();
    // }

    if (queue->size >= 1)
    {

        time_t t1 = node->timestamp;
        time_t t2 = queue->front->timestamp;
        printf("%ld  %ld\n", (long)t1, (long)t2);
        fflush(stdout);
        if (difftime(t1, t2) > (double)THRESHOLD)
        {
            printf("Socket id :%d ----->", node->client.clientSocketId);
            fflush(stdout);
            printf("%s", "threshold reached\n");
            fflush(stdout);
            divisionOfRequestQueue();
        }
        else
        {
            // resetTimer();
        }
    }

    // node->timestamp = time(NULL);
    if (queue->rear == NULL)
    {
        queue->front = queue->rear = node;
    }
    else
    {
        queue->rear->next = node;
        queue->rear = node;
    }
    queue->size++;
    printf("Socket id :%d -----> %s\n", node->client.clientSocketId, "Enqued node");
    fflush(stdout);
    // pthread_cond_signal(&queue->queueNotEmpty);
    pthread_mutex_unlock(&queue->requestsQLock);
}
char *generateTempFile(char *clientName, char *fileName,int cSocketId)
{
    int idLength = snprintf(NULL, 0, "%d", cSocketId);
    size_t resultSize = strlen(clientName) + strlen(fileName) + idLength+3;
    char *result = (char *)malloc(resultSize);
    snprintf(result, resultSize, "%s_%d_%s", clientName, cSocketId,fileName);
    return result;
}

void enqueueFileOperation(FileQueue *fileQueue, queueNode* node,char* tempFile)
{
    queueNode *newNode = (queueNode *)malloc(sizeof(queueNode));
    newNode->client = node->client;
    newNode->operation = node->operation;
    newNode->fileName = node->fileName;
    newNode->fileSize = node->fileSize;
    // newNode->tempFileForChunks = generateTempFile(client.clientName, fileName);
    newNode->tempFileForChunks = tempFile;
    newNode->next = NULL;

    sem_wait(&fileQueue->lock);
    printf("lock acquired File opt\n");
    fflush(stdout);
    if (fileQueue->rear == NULL)
    {
        fileQueue->front = fileQueue->rear = newNode;
    }
    else
    {
        fileQueue->rear->next = newNode;
        fileQueue->rear = newNode;
    }
    fileQueue->size++;
    sem_post(&fileQueue->lock);
    printf("lock released File opt\n");
    fflush(stdout);
}

queueNode* dequeueFileOperation(FileQueue *fileQueue)
{
    // sem_wait(&fileQueue->lock);
    printf("on deque lock acquired File opt\n");
    fflush(stdout);

    if (fileQueue->front == NULL)
    {
        // sem_post(&fileQueue->lock);
        return NULL; // Indicate that the queue was empty
    }

    queueNode *temp = fileQueue->front;
    fileQueue->front = fileQueue->front->next;  
    if (fileQueue->front == NULL)
    {
        fileQueue->rear = NULL;
    }
    fileQueue->size--;
    printf("Dequeued node, updated queue size: %d\n", fileQueue->size);
    fflush(stdout);

    // sem_post(&fileQueue->lock);
    printf("on deque lock released File opt\n");
    fflush(stdout);
    return temp; 
}

int getQueueSize(FileQueue *queue)
{
    int count = 0;
    sem_wait(&queue->lock);

    queueNode *current = queue->front;
    while (current != NULL)
    {
        count++;
        current = current->next;
    }

    sem_post(&queue->lock);
    return count;
}

void printQueue(FileQueue *queue)
{
    sem_wait(&queue->lock);

    queueNode *current = queue->front;
    int count = 0;

    printf("Queue contents:\n");
    while (current != NULL)
    {
        printf("Client Name: %s, Operation: %c\n", current->client.clientName, current->operation);
        current = current->next;
        count++;
    }

    if (count == 0)
    {
        printf("The queue is empty.\n");
    }

    sem_post(&queue->lock);
}

void enqueGlobalQueue(FileQueue *queue, queueNode *node)
{
   
    if (node == NULL)
    {
        printf("%s", "node is not found  \n");
    }
    if (queue->rear == NULL)
    {
        queue->front = queue->rear = node;
    }
    else
    {
        queue->rear->next = node;
        queue->rear = node;
    }
    queue->size++;
    printf("Socket id :%d -----> %s\n", node->client.clientSocketId, "Enqued node");
    fflush(stdout);
   
}
queueNode *dequeueGlobalQueue(FileQueue *queue)
{
    sem_wait(&queue->lock);

    if (queue->front == NULL)
    {
        sem_post(&queue->lock);
        return NULL; // Indicate that the queue was empty
    }

    queueNode *temp = queue->front;
    queue->front = queue->front->next;

    if (queue->front == NULL)
    {
        queue->rear = NULL;
    }
    sem_post(&queue->lock);
    return temp;
}
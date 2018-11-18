#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

char buffer[2] = {'a', 'b'};
int numWritersWaiting = 0;
pthread_mutex_t mutex;
pthread_cond_t swappable;
pthread_cond_t writable;
int swapcount = 0;


pthread_mutex_t printMutex;
long syncPrintTimestampedString(char * string) {
    static long timestamp = 0;
    pthread_mutex_lock(&printMutex);
    timestamp++;
    printf("%ld: %s\n", timestamp, string);
    pthread_mutex_unlock(&printMutex);
    return timestamp;    
}


void reader(int * numWriters) {
    long timeout = 30;
    long count = 0;
    printf("Reader started. Number of writers: %d\n", *numWriters);
    while (swapcount < timeout) {
        //here is where the reader fills it's buffer and waits until writers are waiting to swap
        //first, fill buffer 0    
        char stringBuffer[128] = {0};    
        sprintf(stringBuffer, "Reading into buffer %c", buffer[0]);
        count = syncPrintTimestampedString(stringBuffer);

        //then, wait until the writers are done
        pthread_mutex_lock(&mutex);
        while (numWritersWaiting < *numWriters) {
            pthread_cond_wait(&swappable, &mutex);
        }

        //when all the threads are done writing, we regain the lock (via condition variable)
        //and swap the buffer
        char temp = buffer[0];
        buffer[0] = buffer[1];
        buffer[1] = temp;
        numWritersWaiting = 0;
        swapcount++;
        sprintf(stringBuffer, "TR Swapped buffer, %c is now in position 0", buffer[0]);
        count = syncPrintTimestampedString(stringBuffer);

        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&writable); //wake all the writers waiting for the next 
    }
    char stringBuffer[128] = {0};
    sprintf(stringBuffer, "TR Reader quit.\n");
    syncPrintTimestampedString(stringBuffer);
    exit(0);
}

void writer(int * writerId) {
    printf("Writer %d started\n", *writerId);
    long timeout = 3;
    long count = 0;
    char oldBuffer = 'b'; //initial value of buffer[1], not subject to memory access time
    char fileName[10] = {'f','i','l','e',0,0,0,0,0,0};
    fileName[4] = ((char)*writerId) + 65;
    FILE *fp = fopen(fileName, "w");
    if (fp == NULL) {
        puts("Failed to open file!");
        return;
    }
    while (swapcount < timeout) {
        //wait for the first swap
        pthread_mutex_lock(&mutex);
        numWritersWaiting += 1;
        pthread_cond_signal(&swappable); //tell the reader that at least one writer is waiting
        
        //wait until the buffer has swapped
        while (buffer[1] == oldBuffer) {
            pthread_cond_wait(&writable, &mutex);
        }
        oldBuffer = buffer[1]; //update our "old buffer" value so we know when it's changed again
        pthread_mutex_unlock(&mutex);

        //here is where the writers dole out their buffer and wait for a swap when finished
        //don't need to lock the writers buffer since we're just reading from it and writing to 
        //separate destinations
        char stringBuffer[128] = {0};
        sprintf(stringBuffer, "T%d Writing from buffer %c", *writerId, buffer[1]);
        count = syncPrintTimestampedString(stringBuffer);

        //write some data to our thingadoo for correctness checking
        fputc(buffer[1], fp);
    }    
    fclose(fp);
    char stringBuffer[128] = {0};
    sprintf(stringBuffer, "T%d Writer quitting.\n", *writerId);
    syncPrintTimestampedString(stringBuffer);

    //this is just a hack to avoid the deadlock when the writer quits first
    exit(0);
}

int main(int argc, char *argv[]) {
    int numThreads = 1;
    pthread_t readerThread;
    pthread_t writerThreads[numThreads];
    
    //initialize mutex and condition variables
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&swappable, NULL);
    pthread_cond_init(&writable, NULL);

    //mutex for locking stdout
    pthread_mutex_init(&printMutex, NULL);

    //initialize synchronization state
    numWritersWaiting = numThreads; //initially all the writers are waiting for the first buffer

    //create the threads
    pthread_create(&readerThread, NULL, (void *)reader, &numThreads);
    int ids[numThreads];
    for (int i = 0; i < numThreads; i++) {
        ids[i] = i;
        printf("Starting writer thread %d\n", i);
        pthread_create(&writerThreads[i], NULL, (void *)writer, &ids[i]);        
    }

    //destroy the threads
    pthread_join(readerThread, NULL);
    for (int i = 0; i < numThreads; i++) {
        pthread_join(writerThreads[i], (void **)NULL);
    }
    return 0;
}

/* Notes:

Critical section operations
- swapping buffer: only Reader thread can do this. Writers must be finished.
- incrementing count of Writers waiting: each Writer thread must increment once when done




*/

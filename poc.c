#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

char buffer[2] = {'a', 'b'};
int numWritersWaiting = 0;
int numThreadsObservedSwap = 0;
bool readerFinished = false;
pthread_mutex_t mutex;
pthread_cond_t swappable;
pthread_cond_t writable;
pthread_cond_t observedSwap;


pthread_mutex_t printMutex;
void syncPrintTimestampedString(char * string) {
    static long timestamp = 0;
    pthread_mutex_lock(&printMutex);
    timestamp++;
    printf("%ld: %s\n", timestamp, string);
    pthread_mutex_unlock(&printMutex);    
}


void reader(int * numWriters) {
    printf("Number of writers: %d\n", *numWriters);
    while (1) {
        //here is where the reader fills it's buffer and waits until writers are waiting to swap
        //first, fill buffer 0    
        char stringBuffer[128] = {0};    
        sprintf(stringBuffer, "Reading into buffer %c", buffer[0]);
        syncPrintTimestampedString(stringBuffer);

        //then, wait until the writers are done
        pthread_mutex_lock(&mutex);
        while (numWritersWaiting < *numWriters) {
            pthread_cond_wait(&swappable, &mutex);
        }

        while (numThreadsObservedSwap < *numWriters) {
            pthread_cond_wait(&observedSwap, &mutex);
        }

        //when all the threads are done writing, we regain the lock (via condition variable)
        //and swap the buffer
        char temp = buffer[0];
        buffer[0] = buffer[1];
        buffer[1] = temp;
        numThreadsObservedSwap = 0;
        sprintf(stringBuffer, "Swapped buffer, %c is now in position 0", buffer[0]);
        syncPrintTimestampedString(stringBuffer);

        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&writable); //wake all the writers waiting for the next step
    }
}

void writer(int * writerId) {
    char oldBuffer = 'b'; //initial value of buffer[1], not subject to memory access time
    char fileName[3] = {0};
    fileName[0] = ((char)*writerId) + 65;
    FILE *fp = fopen(fileName, "w");
    if (fp == NULL) {
        return;
    }
    while (1) {
        //wait for the first swap
        pthread_mutex_lock(&mutex);
        numWritersWaiting += 1;
        pthread_cond_signal(&swappable); //tell the reader that at least one writer is waiting
        
        //wait until the buffer has swapped
        while (buffer[1] == oldBuffer) {
            pthread_cond_wait(&writable, &mutex);
        }
        numWritersWaiting -= 1;
        numThreadsObservedSwap += 1;
        oldBuffer = buffer[1]; //update our "old buffer" value so we know when it's changed again
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&observedSwap);

        //here is where the writers dole out their buffer and wait for a swap when finished
        //don't need to lock the writers buffer since we're just reading from it and writing to 
        //separate destinations
        char stringBuffer[128] = {0};
        sprintf(stringBuffer, "Writing from buffer %c", buffer[1]);
        syncPrintTimestampedString(stringBuffer);

        //write some data to our thingadoo for correctness checking
        fputc(buffer[1], fp);
        fflush(fp);
    }    
}

int main(int argc, char *argv[]) {
    int numThreads = 2;
    pthread_t readerThread;
    pthread_t writerThreads[numThreads];
    
    //initialize mutex and condition variables
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&swappable, NULL);
    pthread_cond_init(&writable, NULL);
    pthread_cond_init(&observedSwap, NULL);

    //mutex for locking stdout
    pthread_mutex_init(&printMutex, NULL);

    //initialize synchronization state
    numWritersWaiting = numThreads; //initially all the writers are waiting for the first buffer
    numThreadsObservedSwap = numThreads;

    //create the threads
    pthread_create(&readerThread, NULL, (void *)reader, &numThreads);
    for (int i = 0; i < numThreads; i++) {
        pthread_create(&writerThreads[i], NULL, (void *)writer, &i);        
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

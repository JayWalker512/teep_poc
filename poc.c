#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

char buffer; //uninitialized because we 'fill' the buffer before starting threads
int numWritersWaiting = 0;
pthread_mutex_t mutex;
pthread_cond_t writable;
int numThreads = 16;
long timeout = 8192;
long swapcount = 0;

pthread_mutex_t printMutex;
long syncPrintTimestampedString(char * string) {
    static long timestamp = 0;
    pthread_mutex_lock(&printMutex);
    timestamp++;
    printf("%ld: %s\n", timestamp, string);
    pthread_mutex_unlock(&printMutex);
    return timestamp;    
}

/* This is a placeholder function used to swap the contents of the buffer between
'a' and 'b' to give an easy means of correctness checking. If currentBuffer = 'a' the
function returns 'b', otherwise 'a'. */ 
char fillBuffer(char currentBuffer) {
    if (currentBuffer == 'a') {
        return 'b';
    }
    return 'a';
}

void writer(int * writerId) {
    printf("Writer %d started\n", *writerId);

    char fileName[10] = {'f','i','l','e',0,0,0,0,0,0};
    fileName[4] = ((char)*writerId) + 65;
    FILE *fp = fopen(fileName, "w");
    if (fp == NULL) {
        puts("Failed to open file!");
        return;
    }

    bool refilledBuffer = false;
    while (swapcount < timeout) {
        //write some data to this threads file for correctness checking
        fputc(buffer, fp); //this simulates the output of Tee

        //give each thread some busy work to do
        for (int i = 0; i < 10000; i++) {} //Because Tee output wouldn't happen instantly
        
        pthread_mutex_lock(&mutex);
        numWritersWaiting += 1;
        if (numWritersWaiting == numThreads) {
            buffer = fillBuffer(buffer);
            numWritersWaiting = 0;
	    	refilledBuffer = true; 	
            swapcount++; //just to add an exit condition for the POC
        } else {
            pthread_cond_wait(&writable, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        if (refilledBuffer) { //avoid spurious wakeup by releasing lock before broadcast
            pthread_cond_broadcast(&writable);
            refilledBuffer = false;
        }
    }   
     
    fclose(fp);
    char stringBuffer[128] = {0};
    sprintf(stringBuffer, "T%d Writer quitting.\n", *writerId);
    syncPrintTimestampedString(stringBuffer);
}

int main(int argc, char *argv[]) {
    pthread_t writerThreads[numThreads];
    
    //initialize mutex and condition variables
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&writable, NULL);

    //mutex for locking stdout
    pthread_mutex_init(&printMutex, NULL);

    //initialize synchronization state
    numWritersWaiting = 0; //initially all the writers are waiting for the first buffer

    //create the threads
    int ids[numThreads];
    
    //read in the first buffer before even starting all the threads
    buffer = 'a';
    for (int i = 0; i < numThreads; i++) {
        ids[i] = i;
        printf("Starting writer thread %d\n", i);
        pthread_create(&writerThreads[i], NULL, (void *)writer, &ids[i]);        
    }

    //destroy the threads
    for (int i = 0; i < numThreads; i++) {
        pthread_join(writerThreads[i], (void **)NULL);
    }
    return 0;
}

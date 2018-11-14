#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

char buffer[2] = {'a', 'b'};


void reader() {
    while (1) {puts("reading");}
}

void writer() {
    while (1) {puts("writing");}    
}

int main(int argc, char *argv[]) {
    int numThreads = 8;
    pthread_t readerThread;
    pthread_t writerThreads[numThreads];

    pthread_create(&readerThread, NULL, (void *)reader, NULL);
    for (int i = 0; i < numThreads; i++) {
        pthread_create(&writerThreads[i], NULL, (void *)writer, NULL);        
    }

    pthread_join(readerThread, NULL);
    for (int i = 0; i < numThreads; i++) {
        pthread_join(writerThreads[i], (void **)NULL);
    }
    return 0;
}

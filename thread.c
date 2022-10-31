#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <mqueue.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "definitions.c"
#include <pthread.h>

//global variables, each thread will modify its index, no synch issue
int** frequencies;
Words* allWords;

struct arg{ //arg to be passed to the thread
    int index;
    char* filename;
};

static void* myThread(void* data ){ //function to be executed in the thread
    int index = ((struct arg*) data)->index;
    char* filename = ((struct arg*) data)->filename;

    
    Words words;
    initWords( &words, 2);
    readWords( filename, &words);
    //pass the unique words and frequencies to global variables
    initWords(&(allWords[index]), 2);
    frequencies[index] = countFrequencies(&words, &(allWords[index]), frequencies[index]);

    freeWords( &words);
    pthread_exit(NULL);
}

int main(int arc, char* argv[]){
    int N = atoi(argv[2]);

    frequencies = malloc(sizeof(int*) * N);
    allWords = malloc(sizeof(Words) * N);

    struct arg t_args[N]; //args to be passed to threads

    char filenames[N][1024];
    for(int i = 0; i < N; i++){
        strcpy(filenames[i], argv[i+3]);
    }
    char* outfile = argv[1];
    
    pthread_t thr_ids[N];
    //create threads
    for ( int i = 0; i < N; i++) {
        t_args[i].index = i;
        t_args[i].filename = filenames[i];
        int fail = pthread_create(&(thr_ids[i]), NULL, myThread, (void*) &(t_args[i]));
        if( fail){
            printf("ERROR with code: %d\n", fail);
            exit(1);
        }
    }
    for(int i = 0; i < N; i++){
        int fail = pthread_join( thr_ids[i], NULL);
        if( fail){
            printf("Joining thread failed\n");
            exit(1);
        }
    }
    
    //put together all words

    Words concatWords;
    initWords(&concatWords, 2);
    for(int i = 0; i < N; i++){
        for(int j = 0; j < allWords[i].valid; j++){
            insertWord(&concatWords, allWords[i].words[j]);
        }
    }
    int allFrequencies[concatWords.valid];
    int freqIndex = 0;
    for(int j = 0; j < N; j++){
        for(int k = 0; k < allWords[j].valid; k++){
            allFrequencies[freqIndex] = frequencies[j][k];
            freqIndex++;
        }
    }
    
    Words uniqueWords;
    initWords(&uniqueWords, 2);
    for(int i = 0; i < concatWords.valid; i++){
        if( wordExists(&uniqueWords, concatWords.words[i]) == -1){
            insertWord(&uniqueWords, concatWords.words[i]);
        }
    }
    int uniqueFreqs[uniqueWords.valid];
    
    for(int i = 0; i < uniqueWords.valid; i++){
        uniqueFreqs[i] = 0;
        for(int j = 0; j < concatWords.valid; j++){
            if(strcmp(uniqueWords.words[i], concatWords.words[j]) == 0){
                uniqueFreqs[i] = uniqueFreqs[i] + allFrequencies[j];
            }
        }
    }
    char* temp;
    //sort with strcmp
    for(int i=0; i<uniqueWords.valid; i++){
        for(int j=0; j<uniqueWords.valid-1-i; j++){
        if(strcmp(uniqueWords.words[j], uniqueWords.words[j+1]) > 0){
            temp = uniqueWords.words[j];
            uniqueWords.words[j] = uniqueWords.words[j+1];
            uniqueWords.words[j+1] = temp;
            int tmpFreq = uniqueFreqs[j];
            uniqueFreqs[j] = uniqueFreqs[j + 1];
            uniqueFreqs[j+1] = tmpFreq;
        }
        }
    }

    FILE* fptr;
    fptr = fopen(outfile,"w");

    if ( fptr == NULL) {
        printf("Error");
        exit(1);
    }
    for(int i = 0; i < uniqueWords.valid; i++){
        if( i == uniqueWords.valid -1)
            fprintf(fptr,"%s %d", uniqueWords.words[i], uniqueFreqs[i]);
        else
            fprintf(fptr,"%s %d\n", uniqueWords.words[i], uniqueFreqs[i]);
    }
    fclose(fptr);
    freeWords(&uniqueWords);
    freeWords(&concatWords);
    for(int i = 0; i < N; i++){
        freeWords( &(allWords[i]));
        free(frequencies[i]);
    }
    free(frequencies);
    free(allWords);
    return 0;
}
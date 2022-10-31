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

int main(int arc, char* argv[]) {
    if ( arc > 3) {
        
        int N = atoi(argv[3]); //number of input files
        char filenames[N][1024]; // name of the file cannot be more than 1024 
        for(int i = 0; i < N; i++){
            strcpy(filenames[i], argv[i+4]); //filenames in a global array
        }


        int msgsize = atoi(argv[1]);

        char* outfile = argv[2]; //name of the output file


        mqd_t mq;

        mq = mq_open( "/my_queue___eren_arda", O_RDWR | O_CREAT, 0666, NULL);

        if( mq == -1){ //handle openerror
            perror("mq_open()");
            return EXIT_FAILURE;
        }
        
        
        
        for(int i = 0; i < N; i++) {
            //printf("for loop %d\n", i);
            int child = fork(); // create child process
            if( child == 0){ //in child process
                Words words;
                initWords(&words, 2);
                readWords( filenames[i], &words); //read all words from the file

                int* frequencies;
                Words uniqueWords; 
                initWords(&uniqueWords, 2);
                frequencies = countFrequencies(&words, &uniqueWords, frequencies); //find unique words and their frequencies


                struct tuple tuples[uniqueWords.valid]; //initialize word/freq pair
                for(int i = 0; i < uniqueWords.valid; i++){
                    tuples[i].frequency = frequencies[i];
                    tuples[i].wordLength = strlen(uniqueWords.words[i]);
                    strcpy( tuples[i].word, uniqueWords.words[i]);
                }

                
                int insertedCount = 0;
                

                while( insertedCount < uniqueWords.valid){
                    int byteCount = 0;
                    char message[msgsize];
                    //continue to append to the current message if new word/freq tuple fits the message's remaining bytes
                    while( insertedCount < uniqueWords.valid && byteCount + sizeof(int) * 2 + strlen(tuples[insertedCount].word) + 1 < msgsize){

                        
                        char wordLength[4] = "";
                        sprintf(wordLength, "%d",(int) strlen(tuples[insertedCount].word));
                        
                        char count[4];
                        //sprintf(count, "%d", tuples[insertedCount].frequency);
                        int tempFreq = tuples[insertedCount].frequency;
                        count[3] = (tempFreq >> 24) & 0xFF;
                        count[2] = (tempFreq >> 16) & 0xFF;
                        count[1] = (tempFreq >> 8) & 0xFF;
                        count[0] = (tempFreq >> 0) & 0xFF;
                        
                        //insert word length (4 bytes)
                        for ( int i = 0; i < 4; i++) {
                            message[byteCount] = wordLength[i];
                            byteCount++;
                        }
                        //insert word (length of the word + null character)
                        for ( int i = 0; i < strlen(tuples[insertedCount].word) + 1; i++) {
                            message[byteCount] = tuples[insertedCount].word[i];
                            byteCount++;
                        }
                        //insert frequency (4 bytes)
                        for ( int i = 0; i < 4; i++) {
                            message[byteCount] = count[i];
                            byteCount++;
                        }
                        
                        insertedCount++;
                    
                    }

                    //padding to reach msgsize
                    for(int i = byteCount; i < msgsize; i++){
                        message[i] = '\0';
                    }

                    
                    int canBeSent = mq_send(mq, message, msgsize, 0);


                    if( canBeSent == -1){
                        perror("mq_send failed");
                        exit(1);
                    }

                }

                free(frequencies);
                freeWords(&uniqueWords);
                freeWords(&words);
                
                exit(0); //exit child process
            }
        }
        //IN PARENT PROCESS

        struct mq_attr attr;
        mq_getattr(mq, &attr);
        int maxMsgSize = (int)attr.mq_msgsize;
        char buffer[maxMsgSize]; //initialize buffer to read the message


        Words allWords;
        int* frequencies = malloc( sizeof(int) * 2);
        int valid = 0;
        int size = 2;

        initWords(&allWords, 2);

        //concurrently get messages from children
        for(int i = 0; i < N; i++) {
            wait(NULL);
            mq_getattr(mq, &attr);
            int noOfMessages = (int) attr.mq_curmsgs;
            int messageCount = 0;


            while(messageCount < noOfMessages){ //continue until message queue is empty
                // printf("Current no of messages on queue: %d\n", t);
                int canReceive = mq_receive(mq, buffer, maxMsgSize, NULL);
                if( canReceive == -1){
                    perror("mq_receive failed");
                    exit(1);
                }
                // printf("\n\n-------RECEIVED--------\n\n");
                for(int i = 0; i < msgsize;){
                    char length[4];
                    memcpy(length, &buffer[i], 4); //retrieve wordlength
                    int len = atoi(length);
                    if(len != 0){
                        // printf("Length is: %d\n", len);

                        char word[70];
                        memcpy(word, &buffer[i+4], len+1); //retrieve word
                        // printf("Word is: %s\n", word);

                        char freq[4];
                        memcpy(freq, &buffer[i+4+len+1], 4); //retrieve frequency
                        int count = *(int *)freq;
                       
                        //printf("Count is: %d\n", count);

                        insertWord(&allWords, word); 
                        if(valid == size){ //dynamically grow the frequencies array
                            size = size * 2;
                            frequencies = realloc(frequencies, size * sizeof(int));
                        }
                        frequencies[valid] = count;
                        valid++;
                        
                       
                    }
                    
                    i = i + 4 + len + 1 + 4; //UPDATE BYTE POINTER
                        
                    }
                    
                    messageCount++; //number of messages retrieved from the queue in a single batch
            }
        }

        FILE* fptr;
        fptr = fopen(outfile,"w");

        if ( fptr == NULL) {
            printf("Error");
            exit(1);
        }

        Words uniqueWords;
        initWords(&uniqueWords, 2);
        for(int i = 0; i < allWords.valid; i++){
            if( wordExists(&uniqueWords, allWords.words[i]) == -1){
                insertWord(&uniqueWords, allWords.words[i]);
            }
        }
        int uniqueFreqs[uniqueWords.valid];
        
        for(int i = 0; i < uniqueWords.valid; i++){
            uniqueFreqs[i] = 0;
            for(int j = 0; j < allWords.valid; j++){
                if(strcmp(uniqueWords.words[i], allWords.words[j]) == 0){
                    uniqueFreqs[i] = uniqueFreqs[i] + frequencies[j];
                }
            }
        }

        char* temp;
        //sort the word/freq pairs with strcmp
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
        //printf("Valid : %d\n", uniqueWords.valid);
        for(int i = 0; i < uniqueWords.valid; i++){
            if( i == uniqueWords.valid -1)
                fprintf(fptr,"%s %d", uniqueWords.words[i], uniqueFreqs[i]);
            else
                fprintf(fptr,"%s %d\n", uniqueWords.words[i], uniqueFreqs[i]);
        }


        fclose(fptr);
        free(frequencies);
        freeWords(&uniqueWords);
        freeWords(&allWords);
        mq_close(mq);
        return 0;

    }
    return -1;
}
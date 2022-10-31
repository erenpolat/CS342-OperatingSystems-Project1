#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <mqueue.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>


typedef struct Words{
    char** words;
    int valid;
    int size;
} Words;

void initWords( Words* array, int initSize ) {
    array->words = malloc(initSize * sizeof(char*));
    array->valid = 0;
    array->size = initSize;
}

void insertWord(Words* array, char* word){
    
    if( array->size <= array->valid){
        array->size = array->size * 2;
        array->words = realloc(array->words, array->size * sizeof(char*));
    }
    array->words[array->valid] = malloc(sizeof(char) * (strlen(word) + 1 ));
    strcpy( array->words[array->valid], word);
    array->valid += 1;
}

void freeWords( Words* array){
    for(int i = 0; i < array->valid; i++){
        free((array->words[i]));
        array->words[i] = 0;
    }
}

void printWords( Words* array){
    printf("Words in the array are: \n");
    for(int i = 0; i < array->valid; i++){
        printf("Word %d: %s\n", i, array->words[i]);
        

    }
   
}
void readWords(char* filename, Words* words){
    FILE* ptr;
    char ch;
    ptr = fopen(filename,"r");
    if ( ptr == NULL) {
        printf("couldn't open");
    }
    char curWord[70] = "";

    do {
        ch = fgetc(ptr);
        if ( ch != '\n' && ch != '\t' && ch != ' ' && ch != EOF) {
            ch = toupper(ch);
            strncat(curWord, &ch, 1);     
        } else {
            if( strlen(curWord) > 0){
                insertWord( words, curWord);
            }
            strcpy(curWord, "");            
        }

        
    } while ( ch != EOF);
    fclose(ptr);
}
int wordExists(Words* uniqueWords, char* word){
    for(int i = 0; i < uniqueWords->valid; i++){
        if( strcmp(uniqueWords->words[i], word) == 0)
            return i;
    }
    return -1;
}

int* countFrequencies(Words* allWords, Words* uniqueWords, int* frequencies){
    
    for(int i = 0; i < allWords->valid; i++){
        int index = wordExists(uniqueWords, allWords->words[i]);
        if( index == -1){
            insertWord( uniqueWords, allWords->words[i]);
        }
    }
    
    frequencies = malloc( sizeof(int) * uniqueWords->valid);
    for(int i = 0; i < uniqueWords->valid; i++){
        frequencies[i] = 0;
    }
    for(int i = 0; i < uniqueWords->valid; i++){
        for(int j = 0; j < allWords->valid; j++){
            if( strcmp( uniqueWords->words[i], allWords->words[j]) == 0)
                frequencies[i] = frequencies[i] + 1;
        }
    }
    return frequencies;
}

struct tuple{
    int frequency;
    int wordLength;
    char word[70];
};
typedef struct Tuples{
    struct tuple* tuples;
    int valid;
    int size;
}Tuples;

void insertTuple(  Tuples* myTuples, struct tuple* myTuple){
    if( myTuples->size <= myTuples->valid){
        myTuples->size = myTuples->size * 2;
        myTuples->tuples = realloc(myTuples->tuples, myTuples->size * sizeof(struct tuple*));
    };
    strcpy( myTuples->tuples[myTuples->valid].word, myTuple->word);
    myTuples->tuples[myTuples->valid].frequency = myTuple->frequency;
    myTuples->tuples[myTuples->valid].wordLength = myTuple->wordLength;
    myTuples->valid += 1;

}
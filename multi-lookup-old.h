#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>s
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "util.h"


#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MAX_REQUESTER_THREADS 5
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define INPUTFS "%1024s"


//items in the array (files)
typedef struct sharedArrayItemStruct
{
    void *contains;
} sharedArrayItem;

//actual array with the array element having indicies that correspond to each thred
typedef struct sharedArrayStruct
{
    //the actual array
    //char* array
    sharedArrayItem* array;
    int head;
    int tail;
    int maxSize;
} sharedArray;


typedef struct dataFileStruct
{
  int size;
  FILE *array[5];
};

//pointed to by contains
typedef struct reqThreadStruct
{
  long threadNumber;
  int servicedFilesCount;
  struct dataFileStruct dataFiles;
  sharedArray* s_array;
};

typedef struct resolveThreadStruct
{
  FILE *resultsFile;
  sharedArray* s_array;
};


int sharedArrayInit(sharedArray* s_array, int size);

int main(int argc, char* argv[]);

void* createReqThread(void* threadArg);

void* createResolveThread (void* threadArg);

int addArrayItem(sharedArray* s_array, void* newItem);

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
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
#define BUFFERSIZE 1024


//items in the array (files)
// typedef struct sharedArrayItemStruct
// {
//     void *contains;
// } sharedArrayItem;

//actual array with the array element having indicies that correspond to each thred
typedef struct sharedArrayStruct
{
    //the actual array
    //char* array -- contains hostnames
    char* hostNames[BUFFERSIZE];
    int head;
    int tail;
    //int maxSize;
};




//pointed to by contains
typedef struct reqThreadStruct
{
  long threadNumber;
  int servicedFilesCount;
  char* dataFiles[5];
  int dataFilesSize;
  struct sharedArrayStruct* s_array;
};

typedef struct resolveThreadStruct
{
  FILE *resultsFile;
  struct sharedArrayStruct* s_array;
};


int sharedArrayInit(struct sharedArrayStruct* s_array);

int main(int argc, char* argv[]);

void* createReqThread(void* threadArg);

void* createResolveThread (void* threadArg);

int addArrayItem(struct sharedArrayStruct* s_array, char* newItem);

<<<<<<< HEAD
#include <pthread.h>

#define USAGE "<inputFilePath> <outputFilePath>"
//%1024s maximizes the length of the string to be scanned in 1024 characters, so it will always fit in an 1025 byte long buffer (1024 + 1 for the 0-terminator).
#define INPUTFS "%1024s"
=======
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>s
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "util.h"

>>>>>>> e0c095eaf3ff75abe7fbdffccfc2edff5f7852db

#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MAX_REQUESTER_THREADS 5
<<<<<<< HEAD
#define MAX_NAME_LENGTHS 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

#define MIN_ARGUMENT (MAX_INPUT_FILES - 4) // 6
#define MAX_ARGUMENT MAX_INPUT_FILES // 10
#define SBUFFSIZE 1025


/* Test for extra credit */



typedef int bool; //for readability
#define true 1
#define false 0
bool requests_exist = true;

#ifdef SYS_gettid
pid_t gettid(void) {
    return syscall(SYS_gettid);
}
#endif

typedef struct sharedArray {
    char ** names; //equal to *names[], strings character arrays so char**
    int capacity;
    int first;
    int end;
    pthread_mutex_t *bufferMutex;
    pthread_mutex_t *outMutex;
} sharedArray;


// initialize queue and pointers
sharedArray create_sharedArray(int capacity){
    sharedArray q;
    q.names = malloc(sizeof(char*) * capacity);
    q.capacity = capacity;
    q.first = 0;
    q.end = 0;
    return q;
}

void *addReqToArray(void *input_file);
void *resolve_DNS(void *output_file);
int sharedArray_is_full(sharedArray *q);
int sharedArray_count_full_slots(sharedArray*q);
int sharedArray_is_empty(sharedArray *q);
int mod(int numerator, int denominator);
int sharedArray_push(sharedArray *q, char *name);
void *sharedArray_pop(sharedArray *q);
void sharedArray_cleanup(sharedArray *q);
//int queue_init(safe_q* q, int size);

/*
//Threads
struct thread{
    queue* buffer;
    FILE* thread_file;
    pthread_mutex_t* buffmutex;
    pthread_mutex_t* outmutex;
}; */
=======
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define INPUTFS "%1024s"


//items in the array
typedef struct sharedArrayItem
{
    void *contains;
} sharedArrayItem;

//actual array
typedef struct sharedArrayStruct
{
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

typedef struct reqThreadStruct
{
  long threadNumber;
  int servicedFilesCount;
  struct dataFileStruct dataFiles;
  sharedArray* s_array;
};


int sharedArrayInit(sharedArray* s_array, int size);

int main(int argc, char* argv[]);

void* createReqThread(void* threadArg);

int addArrayItem(sharedArray* s_array, void* newItem);
>>>>>>> e0c095eaf3ff75abe7fbdffccfc2edff5f7852db

#include <pthread.h>

#define USAGE "<inputFilePath> <outputFilePath>"
//%1024s maximizes the length of the string to be scanned in 1024 characters, so it will always fit in an 1025 byte long buffer (1024 + 1 for the 0-terminator).
#define INPUTFS "%1024s"

#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MAX_REQUESTER_THREADS 5
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

typedef struct safe_q {
    char ** names; //equal to *names[], strings character arrays so char**
    int capacity;
    int first;
    int end;
    pthread_mutex_t *bufferMutex;
    pthread_mutex_t *outMutex;
} safe_q;


// initialize queue and pointers
safe_q create_safe_q(int capacity){
    safe_q q;
    q.names = malloc(sizeof(char*) * capacity);
    q.capacity = capacity;
    q.first = 0;
    q.end = 0;
    return q;
}

void *addReqToArray(void *input_file);
void *resolve_DNS(void *output_file);
int safe_q_is_full(safe_q *q);
int safe_q_count_full_slots(safe_q *q);
int safe_q_is_empty(safe_q *q);
int mod(int numerator, int denominator);
int safe_q_push(safe_q *q, char *name);
void *safe_q_pop(safe_q *q);
void safe_q_cleanup(safe_q *q);

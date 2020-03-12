#include "util.h"
#include <unistd.h> // Header defines miscellaneous symbolic constants and types, and declares miscellaneous functions.
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> // shall provide a declaration for errno and give positive values for the following symbolic constants
#include <sys/types.h> // gettid()
#include <sys/syscall.h> // gettid()
#include "multi-new.h"
#include <sys/time.h>

/* Test for extra creait */
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// declare  mutex object
pthread_mutex_t shared_array_input_lock;
pthread_mutex_t shared_array_output_lock;
safe_q shared_array;
bool finishedEnding = false;

int main(int argc, char *argv[])
{
    /* For calculating time interval*/
    struct timeval start, end;

    gettimeofday(&start, NULL);
    //local variables
    int num_names = argc -5;
    int num_requester_threads = atoi(argv[1]);
    int num_resolver_threads = atoi(argv[2]);
    //initialize mutex object
    pthread_mutex_init(&shared_array_input_lock, NULL);
    pthread_mutex_init(&shared_array_output_lock, NULL);
    // check amount of arguments
    if(argc < MIN_ARGUMENT){
        fprintf(stderr, "I need more arguments %d\n", (argc - 5));
        fprintf(stderr, "USAGE: \n %s %s \n", argv[0], USAGE);
        return EXIT_FAILURE;
    }
    else if (argc > MAX_ARGUMENT){
        fprintf(stderr, "Wat too much!! %d \n", (argc - 5));
        fprintf(stderr, "USAGE: \n %s %s \n", argv[0], USAGE);
        return EXIT_FAILURE;
    }
    else if (num_requester_threads > MAX_REQUESTER_THREADS){
        fprintf(stderr, "Requeseter threads are too many! %d\n", num_requester_threads);
        return EXIT_FAILURE;
    }
    else if (num_resolver_threads > MAX_RESOLVER_THREADS){
        fprintf(stderr, "Resolver threads are too many! %d\n", num_resolver_threads);
        return EXIT_FAILURE;
    }

    // get the number of processor cores and create that many resolvers
    int availCPU = sysconf( _SC_NPROCESSORS_ONLN );

    FILE *outfile_test = NULL;
    FILE *inputfps[num_names];
    // check for bogus output file path
    if(!(outfile_test = fopen(argv[argc - 1], "w"))){
        fprintf(stderr, "Bogus output file path...exiting\n");
        fprintf(stderr, "Usage: \n %s %s \n", argv[0], USAGE);
        return EXIT_FAILURE;
    }
    //Open input files or try at least
    for(int i = 1; i < (argc -1); i++){
        inputfps[i - 1] = fopen(argv[i], "r");
    }
    fclose(outfile_test);

    printf("TID os this thread: %d\n", gettid());
    printf("Number for requester thread = %d\n", num_requester_threads);
    printf("Number for resolver threads = %d\n", num_resolver_threads);

    // initialize the shared_array. Must be initialize before use
    shared_array = create_safe_q(50);

    //Create requester thread pool for each input file
    int rc_req;
    pthread_t requester_threads[num_requester_threads];

    for (int t = 0; t < num_requester_threads; t++){
        printf("In main: creating requester thread %d\n", t);
        rc_req = pthread_create(&(requester_threads[t]), NULL, addReqToArray, (void*)argv[t + 5]); //plus five to avoid the first argument, which is the executable..
        if(rc_req){
            printf("ERROR; return code from pthread_create() is %d\n", rc_req);
            exit(EXIT_FAILURE);
        }
    }

    /* Create resolver thread pool for resolving DNS */
    int rc_res;
    pthread_t resolver_threads[num_resolver_threads];

    for(int t = 0; t < num_resolver_threads; t ++){
        printf("In main: creating resolver thread %d\n", t);
        rc_res = pthread_create(&(resolver_threads[t]), NULL, resolve_DNS, (void*)argv[num_resolver_threads]); ////////////////// argc = 10 because
        if(rc_res){
            printf("ERROR; return code from pthread_create() is %d\n", rc_res);
            exit(EXIT_FAILURE);
        }
    }

    /* Wait for requester threads to finish*/
    for (int i = 0; i < num_requester_threads; i++){
        // Shall suspend execution of the calling thread until the target thread terminates
        pthread_join(requester_threads[i], NULL);
    }
    printf("All of the requester threads done!\n");

    //set the flag true after creating thread pools
    finishedEnding = true;

    /* Wait for resolver threads to finish */
    for (int i = 0; i < num_resolver_threads; i++){
        pthread_join(resolver_threads[i], NULL);
    }
    printf("All of the resolver threads done\n");

    /* clean up the shared array*/
    safe_q_cleanup(&shared_array);
    pthread_mutex_destroy(&shared_array_input_lock);
    pthread_mutex_destroy(&shared_array_output_lock);

    gettimeofday(&end, NULL);

    printf("Total run time: %ld\n", ((end.tv_sec - start.tv_sec)/1000000L + end.tv_usec) - start.tv_usec);
    return 0;
}

/*Function that returns a void* and that takes a void* argument*/
void *addReqToArray(void *input_file){
    char hostname[SBUFFSIZE]; //hostname

    pid_t tid = gettid();

    FILE *tidopen = fopen("serviced.txt", "a");
    fprintf(tidopen, "Thread %d serviced %s\n", tid, input_file);
    if(!tidopen){
        perror("Error to open file!");
        return NULL;
    }

    //printf("Thread %d serviced %s\n", tid, input_file);

    /* open file with file pointer*/
    FILE *inputfp = fopen(input_file, "r");
    if(!inputfp){
        perror("Error to open file!");
        return NULL;
    }

    //fscanf(FILE *stream, const char *format, ...) reads formatted input from a stream
    while(fscanf(inputfp, INPUTFS, hostname) > 0){
        pthread_mutex_lock(&shared_array_input_lock);

        //This will be assigned each domain name individually and then be pushed onto the queue.
        // Allocate space for the max domain name size
        char *push_in = (char *)malloc(sizeof(char) * SBUFFSIZE);

        //char *strncpy(char *dest, const char *src, size_t n) copies up to n characters from the string pointed to, by src to dest. In a case where the length of src is less than that of n, the remainder of dest will be padded with null bytes.
        strncpy(push_in, hostname, SBUFFSIZE);
        while(safe_q_is_full(&shared_array)){
            fprintf(stderr, "queue_is_full reports that the queue is full\n");
            /* while the queue is full, we put the thread to sleep for a random period of time between 0 and 100 microseconds.
            Unlock before they sleep so that another thread whether its resolver or requester can take the lock.
            Optimally it would be a resolver so that they could empty out the queue a bit and the requester thread could break out of this while loop.*/
            pthread_mutex_unlock(&shared_array_input_lock);
            usleep(rand() % 101);
            pthread_mutex_lock(&shared_array_input_lock); // lock shared array or try
        }
        safe_q_push(&shared_array, push_in);
        pthread_mutex_unlock(&shared_array_input_lock);
    }
    fclose(inputfp);
    return NULL;
}

void *resolve_DNS(void *output_file){

    /* test for extra credit */

    //create IP array with size 100 for dnslookup fxn
    //char IPstr[100];
    char IPstr[INET6_ADDRSTRLEN];
    char IPPstr[INET_ADDRSTRLEN];
    /* Lock up so other resolver threads can't fuck it up while we pulling data out of shared array.
    We want to ensure they stay alive until all resolver thread are complete, otherwise, requester threads will get stuck with a full shared array */
    FILE *outputfp = fopen("result.txt", "a");
    if(!outputfp){
        perror("Error to open output file");
        return NULL;
    }

    while(!finishedEnding || !safe_q_is_empty(&shared_array)){
        //Pull domains out of queue, look up and shit them in the result.txt file
        char *output_in = safe_q_pop(&shared_array);
        //pthread_mutex_unlock(&shared_array_output_lock);
        if(output_in == NULL){
            pthread_mutex_unlock(&shared_array_input_lock);
            usleep(rand() % 101);
            pthread_mutex_lock(&shared_array_input_lock);
            //continue;
        }
        else{
            /* Look up hostname and get IP*/
            //if(dnslookup(output_in, IPstr, sizeof(IPstr)) == UTIL_SUCCESS)
            //    strncpy(IPstr, "", sizeof(IPstr));
            dnslookup(output_in, IPstr, sizeof(IPstr));
            dnslookup(output_in, IPPstr, sizeof(IPPstr));

            pthread_mutex_lock(&shared_array_output_lock);

            /* write the domain name, IP addr to the result.txt */
            //fprintf(outputfp, "%s, %s\n", output_in, IPstr);
            fprintf(outputfp, "%s, %s, %s\n", output_in, IPstr, IPPstr);

            /* print to terminal to test  */
            printf("Resolveing %s to be %s, %s\n", output_in, IPstr, IPPstr);

            pthread_mutex_unlock(&shared_array_output_lock);
        }
        free(output_in);
    }

    pthread_mutex_unlock(&shared_array_input_lock);
    fclose(outputfp);

    return NULL;
}

int safe_q_count_full_slots(safe_q *q){
    return mod(q -> end - q -> first , q -> capacity);
}

int safe_q_is_full(safe_q *q){
    return safe_q_count_full_slots(q) == (q->capacity - 1);
}

int mod(int numerator, int denominator){
    int n = numerator % denominator;
    if(n < 0){
        n += denominator;
    }
    return n;
}

int safe_q_is_empty(safe_q *q){
    return (q -> first == q -> end) && !safe_q_is_full(q);
    //return safe_q_count_full_slots(q) == 0;
}

int safe_q_push(safe_q *q, char *domainName){
    //pthread_mutex_lock(&q);
    if(safe_q_is_full(q)){
    //      pthread_mutex_unlock(&q);
          return 0;
    }
    q -> names[q -> end] = domainName;
    q -> end = ((q -> end + 1) % q -> capacity);
    //pthread_mutex_unlock(&q);
    return 1;
}

void *safe_q_pop(safe_q *q){
    if(safe_q_is_empty(q)){
       return NULL;
    }
    char *reset = q -> names[q -> first];
    q -> first = (q -> first + 1) % q -> capacity;
    return reset;
}

void safe_q_cleanup(safe_q *q){
    while(!safe_q_is_empty(q)){
        safe_q_pop(q);
    }
    free(q -> names);
}

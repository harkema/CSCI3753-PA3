<<<<<<< HEAD
#include "util.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "multi-lookup.h"
#include <sys/time.h>


#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

pthread_mutex_t shared_array_input_lock;
pthread_mutex_t shared_array_output_lock;
sharedArray shared_array;
bool finishedEnding = false;
FILE *perfFile;

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
        fprintf(stderr, "Please give more arguments %d\n", (argc - 5));
        fprintf(stderr, "USAGE: \n %s %s \n", argv[0], USAGE);
        return EXIT_FAILURE;
    }
    else if (argc > MAX_ARGUMENT){
        fprintf(stderr, "Please give less arguments %d \n", (argc - 5));
        fprintf(stderr, "USAGE: \n %s %s \n", argv[0], USAGE);
        return EXIT_FAILURE;
    }
    else if (num_requester_threads > MAX_REQUESTER_THREADS){
        fprintf(stderr, "Too many requester threads %d\n", num_requester_threads);
        return EXIT_FAILURE;
    }
    else if (num_resolver_threads > MAX_RESOLVER_THREADS){
        fprintf(stderr, "Too little resolver threads %d\n", num_resolver_threads);
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

    printf("Thread ID - Current Thread: %d\n", gettid());
    printf("Number for requester thread = %d\n", num_requester_threads);
    perfFile = fopen("performance.txt", "a");
    fprintf(perfFile, "Number for requester thread = %d\n", num_requester_threads);
    printf("Number for resolver threads = %d\n", num_resolver_threads);
    fprintf(perfFile, "Number for resolver threads = %d\n", num_resolver_threads);
    fclose(perfFile);

    // initialize the shared_array. Must be initialize before use
    shared_array = create_sharedArray(50);

    //Create requester thread pool for each input file
    int rc_req;
    pthread_t requester_threads[num_requester_threads];

    for (int t = 0; t < num_requester_threads; t++){
        printf("Creating Requester Thread(s) %d\n", t);
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
        printf("Creating Resolver Thread(s) %d\n", t);
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
    printf("Requester Threads Complete\n");

    //set the flag true after creating thread pools
    finishedEnding = true;

    /* Wait for resolver threads to finish */
    for (int i = 0; i < num_resolver_threads; i++){
        pthread_join(resolver_threads[i], NULL);
    }
    printf("Resolver Threads Complete\n");

    /* clean up the shared array*/
    sharedArray_cleanup(&shared_array);
    pthread_mutex_destroy(&shared_array_input_lock);
    pthread_mutex_destroy(&shared_array_output_lock);

    gettimeofday(&end, NULL);

    printf("Total run time: %ld\n", ((end.tv_sec - start.tv_sec)/1000000L + end.tv_usec) - start.tv_usec);
    perfFile = fopen("performance.txt", "a");
    fprintf(perfFile, "Total run time: %ld\n", ((end.tv_sec - start.tv_sec)/1000000L + end.tv_usec) - start.tv_usec);
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
        while(sharedArray_is_full(&shared_array)){
            fprintf(stderr, "Error: Array is full\n");
            /* while the queue is full, we put the thread to sleep for a random period of time between 0 and 100 microseconds.
            Unlock before they sleep so that another thread whether its resolver or requester can take the lock.
            Optimally it would be a resolver so that they could empty out the queue a bit and the requester thread could break out of this while loop.*/
            pthread_mutex_unlock(&shared_array_input_lock);
            usleep(rand() % 101);
            pthread_mutex_lock(&shared_array_input_lock); // lock shared array or try
        }
        sharedArray_push(&shared_array, push_in);
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

    while(!finishedEnding || !sharedArray_is_empty(&shared_array)){
        //Pull domains out of queue, look up and shit them in the result.txt file
        char *output_in = sharedArray_pop(&shared_array);
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
            printf("Resolving %s to be %s, %s\n", output_in, IPstr, IPPstr);

            pthread_mutex_unlock(&shared_array_output_lock);
        }
        free(output_in);
    }

    pthread_mutex_unlock(&shared_array_input_lock);
    fclose(outputfp);

    return NULL;
}

int sharedArray_count_full_slots(sharedArray *q){
    return mod(q -> end - q -> first , q -> capacity);
}

int sharedArray_is_full(sharedArray *q){
    return sharedArray_count_full_slots(q) == (q->capacity - 1);
}

int mod(int numerator, int denominator){
    int n = numerator % denominator;
    if(n < 0){
        n += denominator;
    }
    return n;
}

int sharedArray_is_empty(sharedArray *q){
    return (q -> first == q -> end) && !sharedArray_is_full(q);
    //return sharedArray_count_full_slots(q) == 0;
}

int sharedArray_push(sharedArray *q, char *domainName){
    //pthread_mutex_lock(&q);
    if(sharedArray_is_full(q)){
    //      pthread_mutex_unlock(&q);
          return 0;
    }
    q -> names[q -> end] = domainName;
    q -> end = ((q -> end + 1) % q -> capacity);
    //pthread_mutex_unlock(&q);
    return 1;
}

void *sharedArray_pop(sharedArray *q){
    if(sharedArray_is_empty(q)){
       return NULL;
    }
    char *reset = q -> names[q -> first];
    q -> first = (q -> first + 1) % q -> capacity;
    return reset;
}

void sharedArray_cleanup(sharedArray *q){
    while(!sharedArray_is_empty(q)){
        sharedArray_pop(q);
    }
    free(q -> names);
=======
#include <stdlib.h>
#include "multi-lookup.h"


pthread_mutex_t mutex;

//writing to shared array
pthread_mutex_t writeSharedArray;

//writing to serviced.txt (requester log)
pthread_mutex_t writeRequesterLog;

pthread_mutex_t readMutex;

//ouput log file
FILE* outputFileGlobal = NULL;

int sharedArrayInit(sharedArray* s_array, int size)
{

	if(size > 0){
		s_array->maxSize = size;
	}
	else
	{
		s_array->maxSize = 50;
	}
	int i;
	//allocate memory for the shared array
	s_array->array = malloc(sizeof(sharedArrayItem) * (s_array->maxSize));


	//set all items in array to null
	for(i=0; i<s_array->maxSize; ++i)
	{
		s_array->array[i].contains = NULL;
	}

	s_array->head = 0;
	s_array->tail = 0;


	return s_array->maxSize;


}

//function to deallocate memory, specificxally deallocating the items in the shared array

int releaseMemory(sharedArray* s_array)
{
	free(s_array);

	return 0;
}

int isArrayFull(sharedArray* s_array)
{
	//if the array is full, return true else return false
	if((s_array->head == s_array->tail) && (s_array->array[s_array->head].contains != NULL))
	{
		printf("Error: Array is full\n");
		return 1;
	}
	else
	{
		return 0;
	}
}

int addArrayItem(sharedArray* s_array, void* newItem)
{

	if(isArrayFull(s_array))
	{

			printf("Error: Cannot add item, array is full\n");
			return -1;
	}

	//tail of shared array now points to new item
	s_array->array[s_array->tail].contains = newItem;
	s_array->tail = ((s_array->tail+1) % s_array->maxSize);

	return 0;

}


void* createReqThread(void* threadArgument)
{

	struct reqThreadStruct * reqThreadStruct;
	char hostName[1025];

	//This is going to be "serviced.txt"
	FILE* inputFilePtr;

	sharedArray* s_array;

	//items in the array
	char* contains;

	long threadNumber;

	reqThreadStruct = (struct reqThreadStruct *) threadArgument;

	pid_t reqThreadId = syscall(__NR_gettid);

	//Begin writing to serviced.txt
	pthread_mutex_lock(&writeRequesterLog);

  outputFileGlobal = fopen("serviced.txt", "ab");

	fprintf(outputFileGlobal, "Requester thread #%d has serviced %d files\n", reqThreadId, reqThreadStruct->servicedFilesCount);
	printf("Requester thread #%d has serviced %d files\n", reqThreadId, reqThreadStruct->servicedFilesCount);

	fclose(outputFileGlobal);

	pthread_mutex_unlock(&writeRequesterLog);


	//start adding items to the shared array

	// printf("SIZE: %d", reqThreadStruct->dataFiles.size);

	for(int i=0; i<reqThreadStruct->dataFiles.size; i++)
	{
		//assign appropriate struct variables
		// input files are the data files being requested by the producer
		inputFilePtr = reqThreadStruct->dataFiles.array[i];
		s_array = reqThreadStruct->s_array;
		threadNumber = reqThreadStruct->threadNumber;


		while(fscanf(inputFilePtr, INPUTFS, hostName) > 0)
		{
					int completed = 0;
					int success = 0;
					while(completed!=1)
					{
						pthread_mutex_lock(&readMutex);
						pthread_mutex_lock(&writeSharedArray);

						if(!isArrayFull(s_array))
						{
							contains = malloc(1024);

							// printf("Contains Size:%d\n", sizeof(contains));
							// printf("HostName Size:%d\n", sizeof(hostName));

							//destination, source, size
							strncpy(contains, hostName, 1025);
							completed = 1;


						}

						pthread_mutex_unlock(&writeSharedArray);
						pthread_mutex_unlock(&readMutex);
						if (!completed)
						{
								sleep(5);

						}

					}
	 	}
		fclose(inputFilePtr);
	}
	FILE *perfFile = fopen("performance.txt", "ab");
	fprintf(perfFile, "Number for requester thread = %d\n", reqThreadId);
	fclose(perfFile);

	return NULL;
}



int main(int argc, char* argv[])
{

	int requesters = argv[1];
	int resolvers = argv[2];
	char *inputFile = argv[3];
	char *outputFile = argv[4];
	//data files will start at argv[5] and continue

	int filesToDo = (argc - 5);
	printf("Total Files: %d\n", filesToDo);



	FILE *requesterLog;
	FILE *resolverLog;

	//check that requesterLog exists (input file)
	if (!(requesterLog =  fopen(inputFile,"r")))
	{
		fprintf(stderr, "Input file does not exist\n");
		return EXIT_FAILURE;

	}


	//check that resolverLog exists (output file)
	 if (!(resolverLog =  fopen(outputFile,"w")))
        {
                fprintf(stderr, "Output file does not exist\n");
                return EXIT_FAILURE;

        }

	//delcare the shared array

	sharedArray s_array;


	sharedArrayInit(&s_array, 50);

	printf("Shared Array Created Successfully\n");

	//Create the requester (producer threads)
	struct reqThreadStruct reqThreadStruct[MAX_REQUESTER_THREADS];
	pthread_t reqThreads[MAX_REQUESTER_THREADS];
	int reqId;
	long threadCounter;

	//Get the number of requester threads given by user
	int numberOfReqThreads;

	sscanf(argv[1],"%d",&numberOfReqThreads);

	if(numberOfReqThreads > MAX_REQUESTER_THREADS)
	{
		numberOfReqThreads = MAX_REQUESTER_THREADS;
	}

	printf("NumberofReqThreads %d\n", numberOfReqThreads);

	//assume we're starting with one file/(max) thread
	int filesPerThread[numberOfReqThreads];

	//if there are more files than threads
	if(filesToDo > numberOfReqThreads)
	{
		//int remainingFiles = filesToDo;
		int remainingFiles = filesToDo;
		//int filesPerThreadCount = filesToDo/numberOfReqThreads;
		int filesPerThreadCount = remainingFiles/numberOfReqThreads;

		for(int i=0; i<numberOfReqThreads; i++)
		{
			filesPerThread[i] = filesPerThreadCount;
			remainingFiles = remainingFiles - filesPerThreadCount ;
		}

		filesPerThread[0] += remainingFiles;
	}
	//each thread gets at least one files
	else
	{
		for(int i=0; i<numberOfReqThreads; i++)
		{
			filesPerThread[i] = 1;
		}
	}

	int offset = 0;

	for(threadCounter=0; threadCounter < numberOfReqThreads && threadCounter < MAX_REQUESTER_THREADS; threadCounter++)
	{
	// 	reqThreadStruct[threadCounter].s_array = &s_array;
	// 	/* set the thread file zie to the file per thread, so we know how much to work with */
	// 	reqThreadStruct[threadCounter].dataFiles.size = filesPerThread[threadCounter];
	// 	/*and then the number of files each thread works on is equal to the files per thread per that thread */
	// 	int numberOfFilesToRead = filesPerThread[threadCounter];
	//
	// 	for(int i = 0; i < numberOfFilesToRead; i++)
	// 	{
	// 		/* open that the files were going to work on the offset per the argumetn, give it read only permissions */
	// 		reqThreadStruct[threadCounter].dataFiles.array[i] = fopen(argv[threadCounter+offset+5], "r");
	// 		/* print the argument */
	// 		printf("%s\n", argv[threadCounter+offset+5]);
	// 		/* add the offset value by 1 */
	// 		if(numberOfFilesToRead > 1 && i != numberOfFilesToRead - 1)
	// 		{
	// 			offset = offset + 1;
	// 		}
	//
	// 	}
	//
	// 	/* thread counter eq to that thrad num */
	// 	reqThreadStruct[threadCounter].threadNumber = threadCounter;
	// 	/* we have the files serviced equal to the number of files */
	// 	reqThreadStruct[threadCounter].servicedFilesCount = numberOfFilesToRead;
	//
	// }
		printf("ThreadCounter:%d\n", threadCounter);		//start accessing the shared array
		reqThreadStruct[threadCounter].s_array = &s_array;

		reqThreadStruct[threadCounter].dataFiles.size = filesPerThread[threadCounter];
		//numberofFilesTo read is incorrect
		int numberOfFilesToRead = filesPerThread[threadCounter];

		//begin reading files
		//Issue when requesters > files

		printf("NumberOfFiles:%d\n", numberOfFilesToRead);

		//Number of files to read/thread
		for(int i=0; i<numberOfFilesToRead; i++)
		{
			printf("Offset:%d\n", offset);

			if(offset >= filesToDo)
			{
				offset = offset - 1;
				reqThreadStruct[threadCounter].dataFiles.array[i]=fopen(argv[5+offset],"r");
				printf("Currently reading: %s\n", argv[5+offset]);
				break;
			}
			reqThreadStruct[threadCounter].dataFiles.array[i]=fopen(argv[5+offset],"r");
			printf("Currently reading: %s\n", argv[5+offset]);
			offset = offset+1;

		 }


		 //set struct variables
		 reqThreadStruct[threadCounter].threadNumber = threadCounter;
		 reqThreadStruct[threadCounter].servicedFilesCount = numberOfFilesToRead;


		 pthread_create(&reqThreads[threadCounter], NULL, createReqThread, &reqThreadStruct[threadCounter]);

		 pthread_join(reqThreads[threadCounter], NULL);

	}

		free(s_array.array);

>>>>>>> e0c095eaf3ff75abe7fbdffccfc2edff5f7852db
}

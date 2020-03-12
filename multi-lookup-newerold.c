#include <stdlib.h>
#include "multi-lookup.h"

//use condition variables
pthread_mutex_t mutex;

//writing to shared array
pthread_mutex_t writeSharedArray;

//pthread condition variable for writing to the shared array
pthread_cond_t writeCond;

//reader, writer solution (just use pthread)

//pthread condition variable for reading form shared array
pthread_cond_t readCond;

//writing to serviced.txt (requester log)
pthread_mutex_t writeRequesterLog;

pthread_mutex_t readSharedArray;

pthread_mutex_t readDataFiles;

pthread_mutex_t sharedArrayAccess;
//ouput log file
FILE* outputFileGlobal = NULL;
int reqThreadDone=0;
int resolveThreadDone;
int readCount;

/*
To Do:

Writers needs to read, grab the read mutex
If the array is not full, keep addings items
If the array is full, place writer in queue and wait for reader(resolver) to signal





*/
int sharedArrayInit(struct sharedArrayStruct* s_array)
{

	for(int i=0; i<BUFFERSIZE; i++)
	{
		s_array->hostNames[i] = malloc(50);
		s_array->hostNames[i] = NULL;
	}



	s_array->head = 0;

	s_array->tail = 0;

	return 0;

}


void clearArray(struct sharedArrayStruct* s_array){
	/* while it isnt empty remove the last item from the list */
	while(!isArrayEmpty(s_array)){
		removeArrayItem(s_array);
	}

	free(s_array->hostNames);
}

int isArrayFull(struct sharedArrayStruct* s_array)
{
	//if the array is full, return true else return false
	//fix how array is checked
	if((s_array->head == s_array->tail) && (s_array->hostNames[s_array->head] != NULL))
	{
		printf("Error: Array is full\n");
		return 1;
	}
	else
	{
		return 0;
	}
}

int isArrayEmpty(struct sharedArrayStruct* s_array)
{
	if((s_array->head == s_array->tail) && (s_array->hostNames[s_array->head] == NULL))
	{
		printf("Error: Array is empty\n");
		return 1;
	}
	else
	{
		return 0;
	}
}
//
int addArrayItem(struct sharedArrayStruct* s_array, char* newItem)
{

	if(isArrayFull(s_array))
	{

			printf("Error: Cannot add item, array is full\n");
			return -1;
	}

	//increment the tail by 1 and then set equal to new item
	s_array->hostNames[s_array->tail++]=newItem;


	return 0;

}

int removeArrayItem(struct sharedArrayStruct* s_array)
{
	char* returnElement;

	if(isArrayEmpty(s_array))
	{
		printf("Error: Cannot remove item, array is empty\n");
		return -1;
	}

	// returnElement = s_array->hostNames[s_array->head];
	// s_array->hostNames[s_array->head]=s_array->hostNames[s_array->head++];
	// return returnElement;

	char *reset = s_array -> hostNames[q -> head];
  s_array->head = (s_array -> head + 1) % 5;
 	return reset;

}

void* createReqThread(void* threadArgument)
{
	struct reqThreadStruct * reqThreadStruct;
	char hostNameStr[50];

	//This is going to be "serviced.txt"
	FILE* inputFilePtr;

//	struct sharedArrayStruct s_array;


	long threadNumber;

	reqThreadStruct = (struct reqThreadStruct *) threadArgument;

	pid_t reqThreadId = syscall(__NR_gettid);
//
	//start adding items to the shared array
//
	for(int i=0; i<reqThreadStruct->dataFilesSize; i++)
	{
// 		//assign appropriate struct variables
// 		// input files are the data files being requested by the producer
		inputFilePtr = reqThreadStruct->dataFiles[i];
		//s_array = reqThreadStruct->s_array;
		threadNumber = reqThreadStruct->threadNumber;
//
		if(!inputFilePtr){
			printf("Error Opening Input File\n");
			pthread_exit(NULL);
		}
//
		//int hostNameCounter = 0;

//make sure writing to the shared array is locked until all writers are done

//read off the buffer
		pthread_mutex_lock(&readDataFiles);
		while(!(feof(inputFilePtr)))
		{
			fgets(hostNameStr, 50, inputFilePtr);
			//printf("HostName:%s\n", hostNameStr);
			pthread_mutex_lock(&writeSharedArray);
			if(!isArrayFull(reqThreadStruct->s_array))
			{
				addArrayItem(reqThreadStruct->s_array, hostNameStr);
			}
			else
			{
				//Place self on queue, wait for readers (resolvers) to signal back before other threads can write
				pthread_cond_wait(&writeCond, &writeSharedArray);
			}
			pthread_mutex_unlock(&writeSharedArray);

		}
		fclose(inputFilePtr);
		pthread_mutex_unlock(&readDataFiles);
	}

	pthread_mutex_lock(&writeRequesterLog);

  outputFileGlobal = fopen("serviced.txt", "ab");

	fprintf(outputFileGlobal, "Requester thread #%d has serviced %d files\n", reqThreadId, reqThreadStruct->servicedFilesCount);
	printf("Requester thread #%d has serviced %d files\n", reqThreadId, reqThreadStruct->servicedFilesCount);

	fclose(outputFileGlobal);

	pthread_mutex_unlock(&writeRequesterLog);

	FILE *perfFile = fopen("performance.txt", "ab");
	fprintf(perfFile, "Number for requester thread = %d\n", reqThreadId);
	fclose(perfFile);

	return NULL;
}
//
void* createResolveThread(void *threadArg)
{

	struct resolveThreadStruct * resolveThreadStruct;
	FILE* resultsOutputFile;
	char ipStr[MAX_IP_LENGTH];
	char* hostName;
	//struct sharedArrayStruct* s_array;
	resolveThreadStruct = (struct resolveThreadStruct *) threadArg;


	resultsOutputFile= resolveThreadStruct->resultsFile;



	pid_t resolveThreadId = syscall(__NR_gettid);

	pthread_mutex_lock(&readSharedArray);

	pthread_mutex_lock(&mutex);

	readCount = readCount + 1;

	if(readCount == 1)
	{
		pthread_mutex_lock(&writeSharedArray);
	}

	pthread_mutex_unlock(&mutex);

	pthread_mutex_unlock(&readSharedArray);

	//actual reading

	if(!isArrayEmpty(&(resolveThreadStruct->s_array)))
	{
		//SEGFAULT

		hostName = removeArrayItem(&(resolveThreadStruct->s_array));

		if (dnslookup(hostName, ipStr, sizeof(ipStr) == UTIL_FAILURE))
		{
			printf("Error: Bogus hostname provided. Writing blank ip to file\n");
			strncpy(ipStr, "", sizeof(ipStr));
		}
		fprintf(resultsOutputFile, "%s, %s\n", hostName, ipStr);
		free(hostName);
	}

	pthread_mutex_lock(&mutex);
	readCount = readCount - 1;

	if (readCount == 0)
	{
		pthread_cond_signal(&writeCond);
		pthread_mutex_unlock(&writeSharedArray);
	}

	pthread_mutex_unlock(&mutex);


	FILE *perfFile = fopen("performance.txt", "ab");
	fprintf("Number for Resolver Thread=%d\n", resolveThreadId);
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
	struct sharedArrayStruct s_array;

	sharedArrayInit(&s_array);

	printf("Shared Array Created Successfully\n");

	// //**************Create the requesters (producer threads)**********************
	//
	struct reqThreadStruct reqThreadStruct[MAX_REQUESTER_THREADS];
	pthread_t reqThreads[MAX_REQUESTER_THREADS];
	int reqId;
	long threadCounter;
	//
	// //Get the number of requester threads given by user
	int numberOfReqThreads;

	sscanf(argv[1],"%d",&numberOfReqThreads);

	if(numberOfReqThreads > MAX_REQUESTER_THREADS)
	{
		numberOfReqThreads = MAX_REQUESTER_THREADS;
	}


	//assume we're starting with one file/(max) thread
	int filesPerThread[numberOfReqThreads];

	// //if there are more files than threads
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

	printf("FilesPerThread:%d\n", filesPerThread[0]);

	int offset = 0;

	for(threadCounter=0; threadCounter < numberOfReqThreads && threadCounter < MAX_REQUESTER_THREADS; threadCounter++)
	{
		printf("ThreadCounter:%d\n", threadCounter);		//start accessing the shared array
		reqThreadStruct[threadCounter].s_array = &s_array;
		reqThreadStruct[threadCounter].dataFilesSize = filesPerThread[threadCounter];

		int numberOfFilesToRead = filesPerThread[threadCounter];

		//begin reading files

		//printf("NumberOfFiles:%d\n", numberOfFilesToRead);
		//Number of files to read/thread
		for(int i=0; i<numberOfFilesToRead; i++)
		{

			//printf("Offset:%d\n", offset);
			reqThreadStruct[threadCounter].dataFiles[i]=fopen(argv[5+offset],"r");

			//offset = offset+1;

			if(numberOfFilesToRead > 1 && i != numberOfFilesToRead - 1)
			{
			 	offset = offset + 1;
			}


		 }
	//
	//
		 //set struct variables
		 reqThreadStruct[threadCounter].threadNumber = threadCounter;
		 reqThreadStruct[threadCounter].servicedFilesCount = numberOfFilesToRead;

		 pthread_create(&(reqThreads[threadCounter]), NULL, createReqThread, &(reqThreadStruct[threadCounter]));
	}

	for(int i=0; i<numberOfReqThreads; i++)
	{
		pthread_join(reqThreads[i], NULL);
	}


	//
	// 	//**************Create the resolvers (consumer threads)**********************

		struct resolveThreadStruct resolveThreadStruct;
		int numberOfResolveThreads;
		int finalResThreadId;
		long threadCounter2;
		resolveThreadStruct.s_array=&s_array;
		resolveThreadStruct.resultsFile = resolverLog;
		//start here

		// struct reqThreadStruct reqThreadStruct[MAX_REQUESTER_THREADS];
		// pthread_t reqThreads[MAX_REQUESTER_THREADS];

		sscanf(argv[2],"%d",&numberOfResolveThreads);
		pthread_t resolveThreads[numberOfResolveThreads];


		for(threadCounter2=0; threadCounter2<numberOfResolveThreads; threadCounter2++)
		{
			//thread_create(&reqThreads[threadCounter], NULL, createReqThread, &reqThreadStruct[threadCounter]);
			//printf("TEST%s\n", resolveThreadStruct[threadCounter2].s_array->hostNames[0]);
			finalResThreadId = pthread_create(&(resolveThreads[threadCounter2]), NULL, createResolveThread, &resolveThreadStruct);

		}

		for(threadCounter = 0; threadCounter < numberOfResolveThreads; threadCounter++)
		{
			pthread_join(resolveThreads[threadCounter],NULL);
		}



		fclose(resolverLog);
		clearArray(&s_array);
		return 0;
}

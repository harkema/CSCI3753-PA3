#include <stdlib.h>
#include "multi-lookup.h"


pthread_mutex_t mutex;

//writing to shared array
pthread_mutex_t writeSharedArray;

//writing to serviced.txt (requester log)
pthread_mutex_t writeRequesterLog;

pthread_mutex_t readMutex;

pthread_mutex_t sharedArrayAccess;
//ouput log file
FILE* outputFileGlobal = NULL;
int reqThreadDone=0;
int resolveThreadDone;
int readCount;


int sharedArrayInit(sharedArray* s_array, int size)
{
	if(size > 0)
	{
		s_array->maxSize = size;
	}
	else
	{
		s_array->maxSize = 50;
	}

	s_array->array = malloc(sizeof(sharedArrayItem) * (s_array->maxSize));

	int i;
	for(i = 0; i < s_array->maxSize; ++i){
		s_array->array[i].contains = NULL;
	}
	s_array->head = 0;
	s_array->tail = 0;
	return s_array->maxSize;

}


void clearArray(sharedArray* s_array){
	/* while it isnt empty remove the last item from the list */
	while(!isArrayEmpty(s_array)){
		removeArrayItem(s_array);
	}

	free(s_array->array);
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

int isArrayEmpty(sharedArray* s_array)
{
	if((s_array->head == s_array->tail) && (s_array->array[s_array->head].contains == NULL))
	{
		printf("Error: Array is empty\n");
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

int removeArrayItem(sharedArray* s_array)
{
	if(isArrayEmpty(s_array))
	{
		printf("Error: Cannot remove item, array is empty\n");
		return -1;
	}

	void* removedItem;

	removedItem = s_array->array[s_array->head].contains;
	s_array->array[s_array->head].contains = NULL;

	s_array->head = ((s_array->head + 1) % s_array->maxSize);

	return removedItem;

}

void* createReqThread(void* threadArgument)
{

	struct reqThreadStruct * reqThreadStruct;
	char hostName[100];

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

	for(int i=0; i<reqThreadStruct->dataFiles.size; i++)
	{
		//assign appropriate struct variables
		// input files are the data files being requested by the producer
		inputFilePtr = reqThreadStruct->dataFiles.array[i];
		s_array = reqThreadStruct->s_array;
		threadNumber = reqThreadStruct->threadNumber;

		if(!inputFilePtr){
			printf("Error Opening Input File\n");
			pthread_exit(NULL);
		}

		int hostNameCounter = 0;
		//read through every host name in the file
		//int reqCounter = fscanf(inputFilePtr, INPUTFS, hostName);
		//while(reqCounter > 0)
		pthread_mutex_lock(&readMutex);
		pthread_mutex_lock(&writeSharedArray);
		while(fgets(hostName, 100, inputFilePtr))
		{
					//int completed = 0;
					//While not completed
					// while(completed!=1)
					// {
						//printf("Array StatusA:%d\n", isArrayFull(s_array));
						if(!isArrayFull(s_array))
						{
							contains = malloc(100);

							printf("HostName:%s\n", hostName);
							//destination, source, size
							strncpy(contains, hostName, 100);

							addArrayItem(s_array, contains);
							//completed = 1;
						}

						else
						{
							printf("Size of Filled Array:%d\n", sizeof(s_array));
							break;

						}

					//}
				//reqCounter = reqCounter - 1;

	 	}
		pthread_mutex_unlock(&writeSharedArray);
		pthread_mutex_unlock(&readMutex);
		fclose(inputFilePtr);
	}
	FILE *perfFile = fopen("performance.txt", "ab");
	fprintf(perfFile, "Number for requester thread = %d\n", reqThreadId);
	fclose(perfFile);

	return NULL;
}

void* createResolveThread(void *threadArg)
{
	struct resolveThreadStruct * resolveThreadStruct;
	FILE* resultsOutputFile;
	char ipStr[MAX_IP_LENGTH];
	char * hostName;
	sharedArray* s_array;
	resolveThreadStruct = (struct resolveThreadStruct *) threadArg;

	resultsOutputFile= resolveThreadStruct->resultsFile;

	s_array = resolveThreadStruct->s_array;
	printf("Size of Resolver Array:%d\n", sizeof(s_array));

	pid_t resolveThreadId = syscall(__NR_gettid);

	//if empty, cannot resolve (consume)
	int isArrayEmptyBool = 0;

	while(!isArrayEmptyBool || !reqThreadDone)
	{
		int resolved = 0;

		pthread_mutex_lock(&read);
		pthread_mutex_lock(&mutex);
		//signifies there is a reader
		readCount = readCount + 1;

		if(readCount == 1)
		{
			pthread_mutex_lock(&writeSharedArray);
		}

		//but still want there to be multiple readers
		pthread_mutex_unlock(&mutex);
		pthread_mutex_unlock(&read);

		pthread_mutex_lock(&sharedArrayAccess);

		isArrayEmptyBool = isArrayEmpty(s_array);

		if(!isArrayEmptyBool)
		{
			hostName = removeArrayItem(&s_array);
			resolved = 1;
		}

		pthread_mutex_unlock(&sharedArrayAccess);

		pthread_mutex_lock(&mutex);

		readCount = readCount - 1;

		//signifies everything has been read, free to write
		if(readCount == 0)
		{
			pthread_mutex_unlock(&writeSharedArray);
		}

		if(resolved)
		{
			sharedArray ipArray;

			sharedArrayInit(&ipArray, 50);

			//perform DNS lookup
			if (dnslookup(hostName, ipStr, sizeof(ipStr) == UTIL_FAILURE))
			{
				printf("Error: Bogus hostname provided. Writing blank ip to file\n");
				strncpy(ipStr, "", sizeof(ipStr));
			}

			fprintf(resultsOutputFile, "%s, %s\n", hostName, ipStr);

			free(hostName);

			clearArray(&ipArray);


		}

	}

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
	sharedArray s_array;

	sharedArrayInit(&s_array,50);

	printf("Shared Array Created Successfully\n");

	//**************Create the requesters (producer threads)**********************

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
		printf("ThreadCounter:%d\n", threadCounter);		//start accessing the shared array
		reqThreadStruct[threadCounter].s_array = &s_array;

		reqThreadStruct[threadCounter].dataFiles.size = filesPerThread[threadCounter];

		int numberOfFilesToRead = filesPerThread[threadCounter];

		//begin reading files

		//printf("NumberOfFiles:%d\n", numberOfFilesToRead);
		//Number of files to read/thread
		for(int i=0; i<numberOfFilesToRead; i++)
		{

			//printf("Offset:%d\n", offset);
			reqThreadStruct[threadCounter].dataFiles.array[i]=fopen(argv[5+offset],"r");

			printf("Currently reading: %s\n", argv[5+offset]);
			//offset = offset+1;

			if(numberOfFilesToRead > 1 && i != numberOfFilesToRead - 1)
			{
			 	offset = offset + 1;
			}

			//
			// for(int i = 0; i < numberOfFilesToRead; i++)
			// {
			// /* open that the files were going to work on the offset per the argumetn, give it read only permissions */
			// reqThreadStruct[threadCounter].dataFiles.array[i] = fopen(argv[threadCounter+offset+3], "r");
			// /* print the argument */
			// printf("Currently reading: %s\n",argv[5+offset]);
			// /* add the offset value by 1 */
			// if(numberOfFilesToRead > 1 && i != numberOfFilesToRead - 1)
			// {
			// 	offset = offset + 1;
			// }


		 }


		 //set struct variables
		 reqThreadStruct[threadCounter].threadNumber = threadCounter;
		 reqThreadStruct[threadCounter].servicedFilesCount = numberOfFilesToRead;


		 pthread_create(&reqThreads[threadCounter], NULL, createReqThread, &reqThreadStruct[threadCounter]);

		 pthread_join(reqThreads[threadCounter], NULL);


	}

		//**************Create the resolvers (consumer threads)**********************
		struct resolveThreadStruct resolveThreadStruct;
		int numberOfResolveThreads;
		int finalResThreadId;
		long threadCounter2;

		sscanf (argv[2],"%d",&numberOfResolveThreads);

		pthread_t resolveThreads[numberOfResolveThreads];

		resolveThreadStruct.s_array = &s_array;
		resolveThreadStruct.resultsFile = resolverLog;


		printf("numberOfResolveThreads %d\n", numberOfResolveThreads);

		for(threadCounter2=0; threadCounter2<numberOfResolveThreads; threadCounter2++)
		{
			finalResThreadId = pthread_create(&(resolveThreads[threadCounter]), NULL, createResolveThread, &resolveThreadStruct);
			//pthread_join(resolveThreads[threadCounter],NULL);
		}

		// for(threadCounter = 0; (threadCounter < numberOfReqThreads) && (threadCounter < MAX_REQUESTER_THREADS); threadCounter++)
		// {
		// pthread_join(reqThreads[threadCounter],NULL);
		// /* join the rquester threads, wait for them to finish */
		// }

	  int reqThreadDone=1;

	/* join the resolver threads when they finished */
	for(threadCounter = 0; threadCounter < numberOfResolveThreads; threadCounter++)
	{
		pthread_join(resolveThreads[threadCounter],NULL);
	}

	fclose(resolverLog);
	clearArray(&s_array);

	return 0;

}

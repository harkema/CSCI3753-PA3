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
	if((s_array->head == s_array->tail) && (s_array->array[s_array->head].contains != NULL))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int addArrayItem(sharedArray* s_array, void* newItem)
{
	//tail of shared array now points to new item
	s_array->array[s_array->tail].contains = newItem;
	s_array->tail = ((s_array->tail+1) % s_array->maxSize);

	return 0;

}


void* createReqThread(void *threadArgument)
{
	struct reqThreadStruct * reqThreadStruct;
	char hostName[1025];

	//This is going to be "serviced.txt"
	FILE* inputFilePtr;

	sharedArray* s_array;

	//items in the array
	char contains[1025];

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

	printf("SIZE: %d", reqThreadStruct->dataFiles.size);

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
					while(!completed)
					{
						pthread_mutex_lock(&readMutex);
						pthread_mutex_lock(&writeSharedArray);

						if(!isArrayFull(s_array))
						{

							printf("Contains Size:%d\n", sizeof(contains));
							printf("HostName Size:%d\n", sizeof(hostName));

							//destination, source, size
							//bitch
							strncpy(contains, hostName, 1025);

							addArrayItem(s_array, contains);
							completed = 1;
						}
						pthread_mutex_unlock(&writeSharedArray);
						pthread_mutex_unlock(&readMutex);

						if (!completed)
						{
							usleep((rand()%100)*100000);
						}

					}
				fclose(inputFilePtr);
	 	}
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


	sharedArrayInit(&s_array, 20);

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

	//if there are more files than threads,
	if(filesToDo > numberOfReqThreads)
	{
		int remainingFiles = filesToDo;

		int filesPerThreadCount = filesToDo/numberOfReqThreads;

		for(int i=0; i<numberOfReqThreads; i++)
		{
			filesPerThread[i] = filesPerThreadCount;
			remainingFiles -= filesPerThreadCount;
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
		//start accessing the shared array


		reqThreadStruct[threadCounter].s_array = &s_array;

		reqThreadStruct[threadCounter].dataFiles.size = filesPerThread[threadCounter];

		//numberofFilesTo read is incorrect
		int numberOfFilesToRead = filesPerThread[threadCounter];


		//begin reading files

		printf("NumberOfFiles:%d\n", numberOfFilesToRead);

		for(int i=0; i<numberOfFilesToRead; i++)
		{
			reqThreadStruct[threadCounter].dataFiles.array[i]=fopen(argv[5+offset+threadCounter],"r");
			printf("Currently reading: %s\n", argv[5+offset+threadCounter]);

			if(numberOfFilesToRead > 1 && i != numberOfFilesToRead - 1){
				offset = offset + 1;
			}

		 }

		//set struct variables
		reqThreadStruct[threadCounter].threadNumber = threadCounter;
		reqThreadStruct[threadCounter].servicedFilesCount = numberOfFilesToRead;

		reqId = pthread_create(&(reqThreads[threadCounter]), NULL, createReqThread, &(reqThreadStruct[threadCounter]));


	}

		free(s_array.array);

}

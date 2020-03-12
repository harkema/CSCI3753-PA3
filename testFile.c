<<<<<<< HEAD
/* add to the shared array */
		while(fscanf(inputfp, INPUTFS, hostname) > 0){
			/*
				Notes on fscanf here:
				fscanf, scanf, sscanf - convert formatted input
				http://pubs.opengroup.org/onlinepubs/009695399/functions/fscanf.html
			*/
			/* completed would be a bool */
			int completed = 0;
			/* while it isnt completed reading loop */
			while(!completed){
				/* do the lock on the read and write so we can only read and write per thread */
				pthread_mutex_lock(&read_mutex);
				pthread_mutex_lock(&write1);
				/* first is to check if the array is full */
				if (!SHARED_ARRAY_TEST_FULL(s_array)){
					/* malloc size for the buffer */
					contains = malloc(BUFFSIZE);

					/*
						strncpy - copy part of a string
						http://pubs.opengroup.org/onlinepubs/7908799/xsh/strncpy.html
					*/
					strncpy(contains, hostname, BUFFSIZE);

					if(debug){
						printf("DEBUG> ADDING: %s TO THE SHARED ARRAY\n", contains);
					}
					/* add the item to the shared array */
					SHARED_ARRAY_ADD_ITEM(s_array, contains);

					/* were completed, set it to true, err, 1*/
					completed = 1;
				}

				/* unlock it so we can do it again */
				pthread_mutex_unlock(&write1);
				pthread_mutex_unlock(&read_mutex);

				/* sleep boi */
				if (!completed){
					usleep((rand()%100)*100000);
				}
			}
		}

		/* close the input file */
		fclose(inputfp);
=======
if (numFiles > num_req_ths){
		/* modular math */
		int remainingFiles = 5;
		/* so it says how many files each thread has */
		int fpt = 5 / num_req_ths;
		/* go through the remaining files, count how much they have to work on */
		for(int i = 0; i < num_req_ths; i++){
			files_per_thread[i] = fpt;
			remainingFiles -= fpt;
			/* hard coding preemptive files*/
		}

		/* and then we just have to append the files per thread */
		files_per_thread[0] += remainingFiles;

	}else{
		/* each thread gets (at least) one file */
		for(int i = 0; i < num_req_ths; i++){
			files_per_thread[i] = 1;
		}
>>>>>>> e0c095eaf3ff75abe7fbdffccfc2edff5f7852db
	}

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
	}

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
	}

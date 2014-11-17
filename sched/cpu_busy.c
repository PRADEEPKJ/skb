#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>


struct thread_info {    /* Used as argument to thread_start() */

    pthread_t thread_id;        /* ID returned by pthread_create() */
    int       thread_num;       /* Application-defined thread # */
    char     *argv_string;      /* From command-line argument */
    char     *start_addr;
};

int num_threads;

struct thread_info *tinfo; 

char *start = NULL;





void sig_handler(int signo) {

 int s;

 if (signo == SIGINT)
    printf("received SIGINT\n");

  int tnum;
  for (tnum = 0; tnum < num_threads; tnum++) 
	if(pthread_kill(tinfo[tnum].thread_id, SIGSEGV) == 0)
	 	printf("killed thread %d\n",tinfo[tnum].thread_num);
	else
		printf("no able to kill thread %d\n",tinfo[tnum].thread_num);

   numa_free(start,1024*num_threads);

  exit(signo);

}


void t_sig_handler(int signo) {

 int s;
 if (signo == SIGSEGV)
    printf("thread received SIGINT\n");

  int tnum;
  exit(signo);
}


 void thread_start(void *arg){

   struct thread_info *tinfo = arg;

   if (signal(SIGINT, t_sig_handler) == SIG_ERR)
  	printf("\ncan't catch SIGINT\n");


   printf("Thread :%d  argv_string=%s\n",
            tinfo->thread_num, tinfo->argv_string);
      while(1){
//		sleep(1);
	}

    printf("thread is exiting\n");
    pthread_exit(0);
}


int main (int argc, char *argv[]){

   num_threads = atoi(argv[1]);
   int tnum;
   int s;
   pthread_attr_t attr;
	
   start = (char*) numa_alloc_local(1024*num_threads);

   if (start == NULL)
	printf("failed to allocate memory on local node\n");

   if (signal(SIGINT, sig_handler) == SIG_ERR)
  	printf("\ncan't catch SIGINT\n");


  tinfo = calloc(num_threads, sizeof(struct thread_info));
  if (tinfo == NULL)
        printf("calloc failed\n");

  s = pthread_attr_init(&attr);

  numa_run_on_node(0);
  
  for (tnum = 0; tnum < num_threads; tnum++) {

        tinfo[tnum].thread_num = tnum + 1;
        tinfo[tnum].argv_string = argv[optind + tnum];

       s = pthread_create(&tinfo[tnum].thread_id, &attr,(void *) &thread_start, &tinfo[tnum]);

        if (s != 0)
            printf("pthread_create failed");
    }

    s = pthread_attr_destroy(&attr);
    if (s != 0)
	printf("failed to destroy the struct\n");


 //   sleep(5);

   //raise(SIGINT);



#if 1
  for (tnum = 0; tnum < num_threads; tnum++) {

        s = pthread_join(tinfo[tnum].thread_id, NULL);
        if (s != 0)
		printf("thread dint terminate\n");
  }
#endif
 
 return 0;
}

 

#include <numa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#define OneGB 1024*1024*1024
int OneMB;

pthread_attr_t attr;
struct thread_info
{				/* Used as argument to thread_start() */
  pthread_t thread_id;		/* ID returned by pthread_create() */
  int thread_num;		/* Application-defined thread # */
  char *argv_string;		/* From command-line argument */
  int chunk_size;
  int node_id;
};

int running_threads = 0;

pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;
int num_threads;
int memory;

struct thread_info *tinfo;
char *start = NULL;

void
thread_start (void *arg)
{

  struct thread_info *thinfo;
  thinfo = arg;

  size_t bytes = thinfo->chunk_size;
  thinfo->node_id = 0;
  numa_run_on_node (thinfo->node_id);
   void *start = (void*)numa_alloc_onnode(bytes,thinfo->node_id);

  if (start == NULL)
    printf ("failed to allocate the memory on node %d\n", thinfo->node_id);
  else
    fflush (stdout);

  memset(start,0,bytes);
  int *start_addr = (int *) start;
  int *end_addr = (int *) start + thinfo->chunk_size;
  printf ("start address is=> %p..end address is ==>%p\n", start_addr,
	  end_addr);
  fflush (stdout);

 int loop;
 for(loop = 0 ; loop < 4 ; loop++){

  start_addr = start;
  while (start_addr < end_addr)
    {
      *start_addr = 1;
      start_addr++;

    }
  printf ("write success ful for thread %d\n", tinfo->thread_num);
  fflush (stdout);

  start_addr = start;
  while (start_addr < end_addr)
    {
      if (*start_addr++ != 1)
	printf ("Pattern did not match\n");
    }

 }
  printf ("%d---thread finished read/write test\n", tinfo->thread_num);
  fflush (stdout);
  numa_free (start, OneMB);
  pthread_mutex_lock (&running_mutex);
  running_threads--;
  pthread_mutex_unlock (&running_mutex);

}


void thread_work(int node_id, int num_threads) {

  if (numa_run_on_node (node_id) < 0)
    printf ("Failed to run on the  node %d \n", node_id);
  printf (" run on the  node %d \n", node_id);


  int tnum;

  if (num_threads > 0)
    {
          int chunk_size = OneMB;

      printf ("the chunk size is ==>%d\n", chunk_size);
      int s = pthread_attr_init (&attr);
      if (s != 0)
	printf ("attr init failed \n");
      for (tnum = 0; tnum < num_threads; tnum++)
	{
	  tinfo = calloc (num_threads, sizeof (struct thread_info));

	      if (tinfo == NULL)
		printf ("calloc failed\n");

	  tinfo->thread_num = tnum + 1;
	  tinfo->chunk_size = chunk_size;
	  s =
	    pthread_create (&tinfo->thread_id, &attr,
			    (void *) &thread_start, &tinfo[tnum]);

	  pthread_mutex_lock (&running_mutex);
	  running_threads++;
	  pthread_mutex_unlock (&running_mutex);

	  if (s != 0)
	    printf ("pthread_create failed");
	  else
	    printf ("thread created successfully\n");
	}

      s = pthread_attr_destroy (&attr);
      if (s != 0)
	printf ("failed to destroy the struct\n");


      while (running_threads > 0)
	{
	  sleep (1);
	}
    }
}



int
main (int argc, char *argv[])
{


#if 0
   if( numa_available() < 0)
	printf("Numa available\n");
   else
	{
		printf("NUma not avaialale\n");
		exit(0);
      }

#endif
  OneMB = atoi(argv[3]);
  thread_work(atoi(argv[1]),atoi(argv[2]));

  return 0;
}

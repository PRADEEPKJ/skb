#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <numa.h>


pthread_attr_t attr;
struct thread_info
{				/* Used as argument to thread_start() */
  pthread_t thread_id;		/* ID returned by pthread_create() */
  int thread_num;		/* Application-defined thread # */
  char *argv_string;		/* From command-line argument */
  size_t chunk_size;
  int n_id;
};
int running_threads = 0;
int lcount = 0;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shared_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_spinlock_t spinlock ;
int num_threads;
struct thread_info *tinfo;

int same_node;

int node_id = 0;

int64_t shared_var = 0;

int numa_hint ;

int core_id = 0;

void
thread_start (void *arg)
{

   cpu_set_t cpuset;
  struct thread_info *thinfo;
  thinfo = arg;
  int loop;
  for (loop = 0; loop < lcount; loop++)
    {
      //spin_lock(&my_lock);
      if (thinfo->thread_num == 1){
      //pthread_mutex_lock (&shared_mutex);
       pthread_spin_lock(&spinlock);
	shared_var++;
       pthread_spin_unlock(&spinlock);
      //pthread_mutex_unlock (&shared_mutex);
     }
      else
     {
      //pthread_mutex_lock (&shared_mutex);
       pthread_spin_lock(&spinlock);
	shared_var--;
       pthread_spin_unlock(&spinlock);
      //pthread_mutex_unlock (&shared_mutex);
    }
      //spin_unlock(&my_lock);
    }
  pthread_mutex_lock (&running_mutex);
  running_threads--;
  pthread_mutex_unlock (&running_mutex);
  printf ("last count is ===>%ld\n", shared_var);
}

void
start_worker_threads (int num_thread)
{
  struct thread_info *tinfo;
  int tnum;
  if (num_threads > 0)
    {
      int s = pthread_attr_init (&attr);
      if (s != 0)
	printf ("attr init failed \n");
      for (tnum = 0; tnum < num_threads; tnum++)
	{
	  tinfo = calloc (1, sizeof (struct thread_info));
	  if (tinfo == NULL)
	    printf ("calloc failed\n");
	  tinfo->thread_num = tnum + 1;
	  if (same_node)
	    tinfo->n_id = node_id;
	  else
	    tinfo->n_id = node_id++;
	  s =
	    pthread_create (&tinfo->thread_id, &attr,
			    (void *) &thread_start, tinfo);

	  pthread_mutex_lock (&running_mutex);
	  running_threads++;
	  pthread_mutex_unlock (&running_mutex);
	  if (s != 0)
	    printf ("pthread_create failed");
	  else;			// printf ("thread created successfully\n");
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

  num_threads = atoi (argv[1]);

  lcount = atoi (argv[2]);

  pthread_spin_init(&spinlock, 0);
  start_worker_threads (num_threads);
 pthread_spin_destroy(&spinlock);

  return 0;
}

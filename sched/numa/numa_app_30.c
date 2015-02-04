#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#define OneGB 1024*1024*1024
int OneMB;
pthread_attr_t attr;
struct thread_info
{				/* Used as argument to thread_start() */
  pthread_t thread_id;		/* ID returned by pthread_create() */
  int thread_num;		/* Application-defined thread # */
  char *argv_string;		/* From command-line argument */
  size_t chunk_size;
};
int running_threads = 0;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;
int num_threads;
int memory;
struct thread_info *tinfo;
char *start = NULL;

size_t pages_per_thread;


void
thread_start (void *arg)
{
  struct thread_info *thinfo;
  thinfo = arg;
  size_t bytes = thinfo->chunk_size;
  printf("number of bytes ==>%u\n",bytes);
  fflush (stdout);
  void *src = (void *) malloc (bytes);
  void *dst = (void *) malloc (bytes);
  if (src == NULL || dst == NULL)
    printf ("failed to allocate the memory on node \n");
 int loop;
 char *src_addr =  src;
 char *dst_addr =  dst;
 for(loop = 0; loop < bytes ; loop++)
	 *src_addr++ = 'a';
  char *end_addr =  src + thinfo->chunk_size;
//  printf ("start address is=> %p..end address is ==>%p\n", start_addr,
//	  end_addr);

  for (loop = 0; loop < 1024; loop++)
    {


      src_addr = src;
      dst_addr = dst;
      while (src_addr < end_addr)
	{
	  *dst_addr = *src_addr++ ;
	}
      src_addr = src;
      dst_addr = dst;
      while (src_addr < end_addr)
	{
            *dst_addr++ = *src_addr++;
	}
	//sleep(2);
    }
  printf ("%d---thread finished read/write test\n", thinfo->thread_num);
  fflush (stdout);
  free (src);
  free (dst);
  pthread_mutex_lock (&running_mutex);
  running_threads--;
  pthread_mutex_unlock (&running_mutex);
}

void 
start_worker_threads (int num_thread, int mem)
{
  struct thread_info *tinfo;
  int tnum;
  size_t bytes=pages_per_thread*4096;


  clock_t start, end;
 double cpu_time_used;

 start = clock();
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
	  tinfo->chunk_size = bytes;
	  s =
	    pthread_create (&tinfo->thread_id, &attr,
			    (void *) &thread_start, tinfo);

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
 
      sleep(1);	
     
      while (running_threads > 0)
	{
	  sleep (1);
	}
    }

 end = clock();
 cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  
 printf("Time taken.....>%f\n",cpu_time_used/60);
}

int
main (int argc, char *argv[])
{
 size_t pages =  sysconf(_SC_AVPHYS_PAGES);
 size_t page_size = sysconf(_SC_PAGESIZE);

 num_threads = atoi (argv[1]);
 int s;

 pages_per_thread = atoi(argv[2]); 
  
 memory = (pages_per_thread*4096);
  
 printf("memory s ==%u\n", memory);

 start_worker_threads (num_threads, memory);
  
 return 0;
}

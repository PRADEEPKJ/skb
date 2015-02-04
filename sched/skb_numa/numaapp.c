#include "Skb.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <assert.h>

#define OneGB 1024*1024*1024
#define OneMB 1024*1024

struct thread_info
{				/* Used as argument to thread_start() */
  pthread_t thread_id;		/* ID returned by pthread_create() */
  int thread_num;		/* Application-defined thread # */
  char *argv_string;		/* From command-line argument */
  size_t chunk_size;
  int node_id;
};

size_t pages_per_thread = 0;
int running_threads = 0;

pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;
size_t num_threads;
size_t memory;

struct thread_info *tinfo;
char *start = NULL;

char L[200];
char query[200];
GMainLoop *loop;
Skb *proxy;

pthread_attr_t attr;

void
form_query (char *algo, char *fn_name, int input1, int input2)
{

  sprintf (query, "[%s], %s(%d,%d,L),write(L)", algo, fn_name, input1,
	   input2);
  printf ("query is =========>%s\n", query);

}


char **
str_split (char *a_str, const char a_delim)
{
  char **result = 0;
  size_t count = 0;
  char *tmp = a_str;
  char *last_comma = 0;
  char delim[2];
  delim[0] = a_delim;
  delim[1] = 0;

  /* Count how many elements will be extracted. */
  while (*tmp)
    {
      if (a_delim == *tmp)
	{
	  count++;
	  last_comma = tmp;
	}
      tmp++;
    }

  /* Add space for trailing token. */
  count += last_comma < (a_str + strlen (a_str) - 1);

  /* Add space for terminating null string so caller
     knows where the list of returned strings ends. */
  count++;

  result = malloc (sizeof (char *) * count);

  if (result)
    {
      size_t idx = 0;
      char *token = strtok (a_str, delim);

      while (token)
	{
	  assert (idx < count);
	  *(result + idx++) = strdup (token);
	  token = strtok (0, delim);
	}
      assert (idx == count - 1);
      *(result + idx) = 0;
    }

  return result;
}


int
parse_results (gchar * res)
{

  //char **result = str_split(res, );  

  printf ("the result is --->%s\n", res);
  while (*res != '\0')
    {
      if (*res == '[')
	{
	  res++;
	  continue;
	}
      else
	{
         printf("the rs is ==%c\n",*res);
	  return (*res - '0');
//       return  0 ;
	}
    }
  return -1;
}

void
sig_handler (int signo)
{

  int s;
  if (signo == SIGINT)
    printf ("received SIGINT\n");

  int tnum;
  for (tnum = 0; tnum < num_threads; tnum++)
    if (pthread_kill (tinfo[tnum].thread_id, SIGSEGV) == 0)
      printf ("killed thread %d\n", tinfo[tnum].thread_num);
    else
      printf ("no able to kill thread %d\n", tinfo[tnum].thread_num);

  //numa_free (start, 1024 * num_threads);

  exit (signo);

}


void
t_sig_handler (int signo)
{

  int s;
  if (signo == SIGSEGV)
    printf ("thread received SIGSEGV\n");

  int tnum;
  exit (signo);

}


void
thread_start (void *arg)
{
  struct thread_info *thinfo;
  thinfo = arg;
  size_t bytes = thinfo->chunk_size;
  thinfo->node_id = 0; 
  numa_run_on_node (thinfo->node_id);
  printf("number of bytes ==>%u\n",bytes);
bytes = 6;
  //void *start = (void *) numa_alloc_onnode (bytes, thinfo->node_id);
  void *start = (void*) numa_alloc_local(bytes);
  //void *start = (void *) malloc (bytes);
  if (start == NULL)
    printf ("failed to allocate the memory on node %d\n", thinfo->node_id);
  //memset (start, 0, bytes);
  char *start_addr =  start;
  char *end_addr =  start_addr + bytes;
  printf ("start address is=> %p..end address is ==>%p\n", start_addr,
	  end_addr);
  fflush (stdout);
  int loop;
  for (loop = 0; loop < 1; loop++)
    {
      start_addr = start;
      while (start_addr < end_addr)
	{
   	  printf("start address is ==>%p\n",start_addr);
	  *start_addr++ = 'a';
	}
      start_addr = start;
      while (start_addr < end_addr)
	{
	  if (*start_addr++ != 'a');
	}
    }
  printf ("%d---thread finished read/write test\n", thinfo->thread_num);
  fflush (stdout);
  numa_free (start, bytes);
  // free(start);
  pthread_mutex_lock (&running_mutex);
  running_threads--;
  pthread_mutex_unlock (&running_mutex);
}

#if 0

void
thread_start (void *arg)
{

  struct thread_info *thinfo = arg;
  if (signal (SIGSEGV, t_sig_handler) == SIG_ERR)
    printf ("\ncan't catch SIGINT\n");

  size_t bytes = thinfo->chunk_size;
  printf("the chunk size is ===>%u\n",bytes);
  thinfo->node_id = 0;
  numa_set_strict(0);
  numa_run_on_node (thinfo->node_id);
  void *start = (void *) numa_alloc_onnode (bytes, thinfo->node_id);
  // void *start = (void*)numa_alloc_local(bytes);
  if (start == NULL)
    printf ("failed to allocate the memory on node %d\n", thinfo->node_id);

  //memset(start,'a',bytes);
  char *start_addr = (char *) start;
  char *end_addr = (char *) start + bytes;
  printf ("start address is=> %p..end address is ==>%p\n", start_addr,
	  end_addr);
  fflush (stdout);

  while (start_addr < end_addr)
    {
      *start_addr = 'a';
      start_addr++;

    }
  printf ("write success ful for thread %d\n", thinfo->thread_num);
  fflush (stdout);

  start_addr = start;

  while (start_addr < end_addr)
    {
      if (*start_addr++ != 'a')
	printf ("Pattern does not match\n");
    }

  printf ("%d---thread finished read/write test\n", thinfo->thread_num);
  fflush (stdout);
  numa_free (start);
  pthread_mutex_lock (&running_mutex);
  running_threads--;
  pthread_mutex_unlock (&running_mutex);
  //pthread_exit (0);

}

#endif




void
callback_from_skb_query (GObject * source_object,
			 GAsyncResult * res, gpointer user_data)
{
  GError *error;
  error = NULL;
  gchar *qres;
  GAsyncResult *gasync;
  gboolean retval;
  gpointer result;
  struct thread_info *tinfo;
  retval = skb_call_query_finish (proxy, &qres, res, NULL);

  if (retval)
    {
      fprintf (stdout, "the function called back with the results==> %s\n",
	       qres);
      fflush (stdout);
    }
  else
    printf ("failed\n");

  int node_id = parse_results (qres);

  if (node_id == -1)
  {
	printf("there is no node to full fill the requirement\n");
        exit(0);
 }

 // if (numa_run_on_node (node_id) < 0)
   // printf ("Failed to run on the  node %d \n", node_id);
 // printf (" run on the  node %d \n", node_id);

  //if (signal (SIGINT, sig_handler) == SIG_ERR)
    //printf ("\ncan't catch SIGINT\n");

  int tnum;

  if (num_threads >= 1)
    {
      
      size_t bytes = pages_per_thread * 4096;
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
      while (running_threads > 0)
	{
	  sleep (1);
	}

#if 0
      for (tnum = 0; tnum < num_threads; tnum++)
	{
	  s = pthread_join (tinfo[tnum].thread_id, NULL);
	  if (s != 0)
	    printf ("thread dint terminate\n");
	  else
	    printf ("the thread finished is==> %d\n", tinfo[tnum].thread_num);
	}
#endif

    }
  else
    {

      size_t bytes = (memory * OneMB) + 10;
      char *big_chunk = (char *) numa_alloc_onnode (bytes, node_id);

      memset (big_chunk, 0, bytes);
      if (big_chunk == NULL)
	printf ("failed to allocate the memory on node %d\n", node_id);


      char *start_addr = big_chunk;
      char *end_addr = start_addr + ((memory * OneMB) + 10);

      while (start_addr < end_addr)
	{
	  *start_addr = 'a';
	  start_addr++;

	}

      start_addr = big_chunk;

      while (start_addr < end_addr)
	{
	  if (*start_addr++ != 'a')
	    printf ("Pattern does not match\n");
	}

      printf ("thread finished read/write test\n");

      numa_free (big_chunk);
    }


  g_main_loop_unref (loop);
  g_main_loop_quit (loop);

}

int
main (int argc, char *argv[])
{

  num_threads = atoi (argv[1]);
  int s;
  GError *error;
  error = NULL;
  gchar *res;
  GAsyncResult *gasync;
  loop = g_main_loop_new (NULL, FALSE);
  proxy = skb_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, "org.freedesktop.Skb",	/* bus name */
				      "/org/freedesktop/Skb",	/* object */
				      NULL,	/* GCancellable */
				      &error);


 size_t pages =  sysconf(_SC_AVPHYS_PAGES);
 size_t page_size = sysconf(_SC_PAGESIZE);
 pages_per_thread = pages / num_threads; 

 int cpu = atoi (argv[4]);

  pages_per_thread = atoi(argv[5]);
  
  memory = (pages_per_thread*4096*num_threads)/(1024*1024);
  
  printf("memory s ==%u\n", memory);

  form_query (argv[2], argv[3], num_threads * cpu, memory);

  skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);

  g_main_loop_run (loop);

  return 0;
}

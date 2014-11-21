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
  int chunk_size;
  int node_id;
};

int running_threads = 0;

pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;
int num_threads;
int memory;

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

  tinfo = arg;
  if (signal (SIGSEGV, t_sig_handler) == SIG_ERR)
    printf ("\ncan't catch SIGINT\n");

  size_t bytes = tinfo->chunk_size;
  tinfo->node_id = 0;
  numa_run_on_node (tinfo->node_id);
  //void *start = (void *) numa_alloc_onnode (bytes, tinfo->node_id);
   void *start = (void*)numa_alloc_local(bytes);
  if (start == NULL)
    printf ("failed to allocate the memory on node %d\n", tinfo->node_id);
  else
    //printf("allocated memory successfully\n");
    fflush (stdout);

  memset(start,0,bytes);
  int *start_addr = (int *) start;
  int *end_addr = (int *) start + tinfo->chunk_size;
  printf ("start address is=> %p..end address is ==>%p\n", start_addr,
	  end_addr);
  fflush (stdout);

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
	printf ("Pattern does not match\n");
    }

  printf ("%d---thread finished read/write test\n", tinfo->thread_num);
  fflush (stdout);
  numa_free (start);
  pthread_mutex_lock (&running_mutex);
  running_threads--;
  pthread_mutex_unlock (&running_mutex);
  //pthread_exit (0);

}





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
//    printf ("finish success\n");
      fprintf (stdout, "the function called back with the results==> %s\n",
	       qres);
      // printf("the ersiyt iis --->%s\n",L);
      fflush (stdout);
    }
  else
    printf ("failed\n");
  // result=g_async_result_get_user_data ();

  int node_id = parse_results (qres);
  if (numa_run_on_node (node_id) < 0)
    printf ("Failed to run on the  node %d \n", node_id);
  printf (" run on the  node %d \n", node_id);

  if (signal (SIGINT, sig_handler) == SIG_ERR)
    printf ("\ncan't catch SIGINT\n");


  int tnum;

  if (num_threads > 1)
    {
      tinfo = calloc (num_threads, sizeof (struct thread_info));

      if (tinfo == NULL)
	printf ("calloc failed\n");


      int chunk_size = (memory * OneMB) / num_threads;
      printf ("the chunk size is ==>%d\n", chunk_size);
      int s = pthread_attr_init (&attr);
      if (s != 0)
	printf ("attr init failed \n");
      for (tnum = 0; tnum < num_threads; tnum++)
	{
	  tinfo[tnum].thread_num = tnum + 1;
	  tinfo[tnum].chunk_size = chunk_size;
	  s =
	    pthread_create (&tinfo[tnum].thread_id, &attr,
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
      int *big_chunk = (int *) numa_alloc_onnode (bytes, node_id);

      memset (big_chunk, 0, bytes);
      if (big_chunk == NULL)
	printf ("failed to allocate the memory on node %d\n", node_id);


      int *start_addr = big_chunk;
      int *end_addr = start_addr + ((memory * OneMB) + 10);

      while (start_addr < end_addr)
	{
	  *start_addr = 1;
	  start_addr++;

	}

      start_addr = big_chunk;

      while (start_addr < end_addr)
	{
	  if (*start_addr++ != 1)
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


  int cpu = atoi (argv[4]);

  memory = atoi (argv[5]);

  form_query (argv[2], argv[3], num_threads * cpu, memory);

  skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);

  g_main_loop_run (loop);

  return 0;
}

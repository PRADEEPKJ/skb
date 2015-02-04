#include "Skb.h"
#include <numa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#define OneGB 1024*1024*1024
int OneMB;
pthread_attr_t attr;
struct thread_info
{				/* Used as argument to thread_start() */
  pthread_t thread_id;		/* ID returned by pthread_create() */
  int thread_num;		/* Application-defined thread # */
  char *argv_string;		/* From command-line argument */
  size_t chunk_size;
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
size_t pages_per_thread;

int
node_and_digits (const struct dirent *dptr)
{


  char *p = (char *) (dptr->d_name);
  if (*p++ != 'n')
    return 0;
  if (*p++ != 'o')
    return 0;
  if (*p++ != 'd')
    return 0;
  if (*p++ != 'e')
    return 0;
  do
    {
      if (!isdigit (*p++))
	return 0;
    }
  while (*p != '\0');
  return 1;
}

void
form_query (char *algo, char *fn_name, int input1, int input2)
{

  sprintf (query, "[%s], %s(%d,%d,L),write(L)", algo, fn_name, input1,
	   input2);
  printf ("query is =========>%s\n", query);

}

void
thread_start (void *arg)
{
  struct thread_info *thinfo;
  thinfo = arg;
  size_t bytes = thinfo->chunk_size;
//  thinfo->node_id = 0;
  void *start = (void *) numa_alloc_onnode (bytes, thinfo->node_id);
  if (start == NULL)
    printf ("failed to allocate the memory on node %d\n", thinfo->node_id);
  memset (start, 0, bytes);
  char *start_addr =  start;
  char *end_addr =  start + thinfo->chunk_size;
  //printf ("start address is=> %p..end address is ==>%p\n", start_addr,
//	  end_addr);
  fflush (stdout);

 
  int loop;
  printf("number of bytes ==>%u\n",bytes);
  numa_run_on_node (thinfo->node_id);
  for (loop = 0; loop < 1024; loop++)
    {
     start_addr = start;
      while (start_addr < end_addr)
	{
	  *start_addr++ = 'a';
	}
      start_addr = start;
      while (start_addr < end_addr)
	{
	  if (*start_addr++ != 'a');
	}
	sleep(2);
    }
  printf ("%d---thread finished read/write test\n", thinfo->thread_num);
  fflush (stdout);
  numa_free (start, bytes);
  pthread_mutex_lock (&running_mutex);
  running_threads--;
  pthread_mutex_unlock (&running_mutex);
}

int
parse_results (gchar * res)
{

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
	}
    }
  return -1;
}

int
get_num_nodes ()
{

  struct dirent **namelist;
  int num_nodes =
    scandir ("/sys/devices/system/node", &namelist, node_and_digits, NULL);

  printf ("Number of nodes===>%d\n", num_nodes);

  return num_nodes;

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
  int node_id = parse_results(qres); 
  if(node_id+1 > get_num_nodes())
    goto End ;
  if (numa_run_on_node (node_id) < 0)
    printf ("Failed to run on the node %d \n", node_id);
  printf (" run on the node %d \n", node_id);
 
  int tnum;
  size_t bytes=pages_per_thread*4096;
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
          tinfo->node_id = node_id;
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
      system("./addnumainfo"); 	
     
      while (running_threads > 0)
	{
	  sleep (1);
	}
    }
End:
  g_main_loop_unref (loop);
  g_main_loop_quit (loop);
}

int
main (int argc, char *argv[])
{
#if 1
  if (numa_available () < 0)
  {
      printf ("NUma not avaialale\n");
      exit (0);
  }
  else
    {
      printf ("Numa available\n");
    }
#endif
 size_t pages =  sysconf(_SC_AVPHYS_PAGES);
 size_t page_size = sysconf(_SC_PAGESIZE);

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

 //pages_per_thread = pages / num_threads; 
 pages_per_thread = atoi(argv[5]); 
  
 memory = (pages_per_thread*4096*num_threads)/(1024*1024);
  
 printf("memory s ==%u\n", memory);

 form_query (argv[2], argv[3], num_threads * cpu, memory);

 skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);

 g_main_loop_run (loop);

 
  //OneMB = atoi (argv[3]);
//  thread_work (pages_per_thread*page_size,atoi(argv[1]), atoi(argv[2]));
  return 0;
}

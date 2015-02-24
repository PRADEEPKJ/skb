#include "Skb.h"
#include <numa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

char L[200];
char query[200];
GMainLoop *loop;
Skb *proxy;
char binary_name[50];

int cpu, memory, num_threads, l_count;

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
	  if(*res == ']')
		return -1;
          else
	    return (*res - '0');
	}
    }
  return -1;
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
  
  char command[1024];
  sprintf(command,"numactl --cpunodebind %d --membind %d %s %d %d",node_id, node_id, binary_name, num_threads, l_count);

  printf("the command is == %s\n",command);
  system( command );  

  g_main_loop_unref (loop);
  g_main_loop_quit (loop);
}

void
form_query ( int memory, int cpu)
{

  sprintf (query, "[test_algo], get_free_numa_node(%d,%d,L),write(L)",memory, cpu);
  printf ("query is =========>%s\n", query);

}

void parse_inputs(int numargs, char**argv)
{

  int i;
  for(i = 2 ; i < numargs ; i++)
  {
    int pos = 0;
    char *rc = NULL;
    char delims[] = {"="};
    rc = strtok(argv[i], delims);
    if(!strcmp("memory", rc )){
    	rc = strtok(NULL, " \0");
	memory = atoi(rc);
    }
   else if(!strcmp("cpu", rc )){
    	rc = strtok(NULL, " \0");
	cpu  = atoi(rc);
    }
   else if(!strcmp("numthreads", rc )){
    	rc = strtok(NULL, " \0");
	num_threads = atoi(rc);
    }
   else if(!strcmp("loops", rc )){
    	rc = strtok(NULL, " \0");
	l_count = atoi(rc);
    }

    //printf("the %s is %d\n",rc[0],atoi(rc[1]));
  }	

    printf("the %d cpu %d memory %d threds \n",cpu, memory, num_threads);

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

  //num_threads = atoi (argv[1]);
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
 
 parse_inputs(argc, argv);
  
 strcpy(binary_name, argv[1]);

 printf(" Application asked for %u MB of memory and %u percentage of CPU\n", memory,cpu);

 form_query (cpu, memory);

 skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);

 g_main_loop_run (loop);
 
 return 0;
}

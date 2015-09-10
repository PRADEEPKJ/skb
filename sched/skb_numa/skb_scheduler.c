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
char binary[50];

int cpu, memory, num_threads, l_count;
char type[20];

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
  printf("the result is ==>%s\n", qres);
  exit(1);
  int node_id = parse_results(qres); 
  
  char command[1024];
  sprintf(command,"numactl --cpunodebind %d --membind %d %s %d %d",node_id, node_id, binary, num_threads, l_count);

  printf("the command is == %s\n",command);
  system( command );  

  g_main_loop_unref (loop);
  g_main_loop_quit (loop);

}

void
form_query (char *ty, int cpu, int memory)
{

  //sprintf (query, "[test_algo], get_free_numa_node(%d,%d,L),write(L)",memory, cpu);
  sprintf (query, "[test_algo], get_all_sysinfo(%s,%d,%d,L),write(L)",ty,cpu,memory);
  printf ("query is =========>%s\n", query);

}

void parse_inputs(int numargs, char**argv)
{

  int i;
  printf("num args===>%d\n", numargs);
  for(i = 1 ; i < numargs ; i++)
  {
    printf("the argvis ==>%s\n",argv[i]);
#if 1
    int pos = 0;
    char *rc = NULL;
    char delims[] = {"="};
    rc = strtok(argv[i], delims);
    if(!strcmp("memory", rc )){
    	rc = strtok(NULL, " \0");
	memory = atoi(rc);
	printf("mem is ==>%d\n",memory);
    }
   else if(!strcmp("cpu", rc )){
    	rc = strtok(NULL, " \0");
	cpu  = atoi(rc);
	printf("cpu is ==>%d\n",cpu);
    }
   else if(!strcmp("binary", rc )){
    	rc = strtok(NULL, " \0");
	sprintf (binary,"%s", rc);
	printf("binary is ==>%s\n",cpu);
    }
    else if(!strcmp("type", rc )){
    	rc = strtok(NULL, " \0");
	sprintf (type,"%s", rc);
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
#endif
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
  
 //strcpy(binary, argv[1]);

 printf(" Application asked for %u MB of memory and %u percentage of CPU\n", memory,cpu);

 form_query (type, cpu, memory);

 skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);

 g_main_loop_run (loop);
 
 return 0;
}

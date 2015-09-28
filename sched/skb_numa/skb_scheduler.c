#include "Skb.h"
#include <numa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

char L[200];
char query[200];
GMainLoop *loop;
Skb *proxy;
char binary[100];
char IP[50];
char **cmd;
char command[1024];

int cpu, memory, num_threads, l_count;
char type[20];
char watch_server[20], next_lvl_server[20];
int level;
bool numa_sys = true;

#if 1
char *
parse_node_results (gchar * res)
{

  //printf ("the result is --->%s\n", res);
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

#endif

char node_ip[20];
char *
parse_results (gchar * res)
{

  int last_pos = 0;
  memset(node_ip, 0, 20);
//  printf ("the result is --->%s\n", res);
  while (*res != '\0')
    {
      if (*res != '[' && *res != ']')
	{
	  node_ip[last_pos++] = *res++;
 //	  printf("char is ==>%c\n",*res);
	}
      else
	res++;
    }
 //	  printf("char is ==>%c\n", node_ip);
  return node_ip;
}

void write_to_nxt_level_etcd( char *node)
{

  
  //printf("the node is ======> %s\n",node);
  char* nxt_lvl_node = parse_results(node); 
  //printf("the node is ======> %s\n",nxt_lvl_node);

  create_etcd_session (nxt_lvl_node);
  do_set (NULL, NULL, NULL, NULL, "taskqueue"); 
  do_set ("taskqueue/task", command , NULL, NULL, NULL);	//write the task to nxt level node
  close_etcd_session ();

}

char *
watch_etcd_change (char *read_server, char *direct)
{

  int chflag;
  while (1)
    {
      create_etcd_session (read_server);
      chflag = do_watch (read_server, direct);
      if (chflag == 1)
	{
		return do_get(direct);
	}
      sleep (1);
    }
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
  //printf("ret value is ==>%s\n",qres);
  //fflush(stdout);

  while (1)
  {
	if(!strcmp(qres,"[]"))
  	{
  		g_main_loop_quit (loop);
  		//printf("q is ==>%s\n",query);
		skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);
 		g_main_loop_run (loop);
		sleep(5);
		continue;
	}
	else
	{
  		//g_main_loop_quit (loop);
		break;
	}
	
  }
	
  if(strcmp(qres,"[]"))
	  if(!level )
	  {
		sprintf(command,"binary=\"%s\" memory=%d cpu=%d IP=%s", binary, memory, cpu, parse_results(strtok(qres,",\0")));
		write_to_nxt_level_etcd(strtok(qres,",\0"));
	  }
	  else //@node level query for devices and schedule
	  {
		//printf("the cmd is ==>%s\n", qres);
		int node_id = parse_node_results(qres);
		if(node_id >= 0 )

		if(numa_sys)
			sprintf(command,"numactl --cpunodebind %d --membind %d %s",node_id, node_id, binary);
		else
			sprintf(command,"%s", binary);

		//printf("the cmd is ==>%s\n", command);
		//..fflush(stdout);
		system( command );  
	  }

  //printf("the command is == %s\n",*cmd);
  //printf("the command is == %s\n",nxt_lvl_node);

  g_main_loop_unref (loop);
  g_main_loop_quit (loop);

}

void
form_query (char *ty, int cpu, int memory)
{

  //sprintf (query, "[test_algo], get_free_numa_node(%d,%d,L),write(L)",memory, cpu);
  sprintf (query, "[test_algo], get_all_sysinfo(%s,%d,%d,L),write(L)",ty,memory,cpu);
  //printf ("query is =========>%s\n", query);

}

form_low_level_query (int cpu, int memory)
{

  sprintf (query, "[test_algo], get_free_numa_node(%d,%d,L),write(L)",memory, cpu);
  //sprintf (query, "[test_algo], get_free_numa_node(%d,%d,L),write(L)",cpu,memory);
  //printf ("query is =========>%s\n", query);

}

void parse_first_res (char *args)
{
	
  char *rc = strtok (args," =");

  while (rc != NULL)
  {
    if(!strcmp("memory", rc )){
    	rc = strtok(NULL, " ");
	memory = atoi(rc);
  //printf ("query is =========>%d\n", memory);
    }
   else if(!strcmp("cpu", rc )){
    	rc = strtok(NULL, " ");
	cpu  = atoi(rc);
  	//printf ("cpu is =========>%d\n", cpu);
    }
   else if(!strcmp("binary", rc )){
    	rc = strtok(NULL, "\"");
	sprintf (binary,"%s", rc);
	//printf ("binary is =========>%s\n", binary);
    }
  else if(!strcmp("IP", rc )){
    	rc = strtok(NULL, " ");
	sprintf (IP,"%s",(parse_results(rc)));
 	 //printf ("ip is =========>%s\n", IP);
    }

    rc = strtok (NULL, " =");
  }

}


void parse_inputs(int numargs, char**argv)
{

  int i;
  //printf("num args===>%d\n", numargs);
  for(i = 0 ; i < numargs ; i++)
  {
    //printf("the argvis ==>%s\n",argv[i]);
#if 1
    int pos = 0;
    char *rc = NULL;
    char *delims;
    rc = strtok(argv[i], "=");

    if(!strcmp("memory", rc )){
    	rc = strtok(NULL, " \0");
	memory = atoi(rc);
//	printf("mem is ==>%d\n",memory);
    }
   else if(!strcmp("cpu", rc )){
    	rc = strtok(NULL, " \0");
	cpu  = atoi(rc);
//	printf("cpu is ==>%d\n",cpu);
    }
   else if(!strcmp("binary", rc )){
    	rc = strtok(NULL, "\0");
	sprintf (binary,"%s", rc);
//	printf("binary is ==>%s\n",cpu);
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
   else if(!strcmp("watchserver", rc )){
    	rc = strtok(NULL, " \0");
	sprintf (watch_server,"%s", rc);
    }
   else if(!strcmp("nextserver", rc )){
    	rc = strtok(NULL, " \0");
	sprintf (next_lvl_server,"%s", rc);
    }
  else if(!strcmp("level", rc )){
    	rc = strtok(NULL, " \0");
	level = atoi(rc);
    }

    //printf("the %s is %d\n",rc[0],atoi(rc[1]));
#endif
  }	

    //printf("the %d cpu %d memory %d threds \n",cpu, memory, num_threads);

}

int
main (int argc, char *argv[])
{
#if 1
  if (numa_available () < 0)
  {
      printf ("NUma not avaialale\n");
      numa_sys = false; 
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
 if(proxy == NULL)
 	printf("Error in creating a proxy to SKB\n");


 		parse_inputs(argc, argv);
 if(level)
	 while (1) {
		char *cmd = watch_etcd_change(watch_server,"/taskqueue/task");
  		close_etcd_session ();
		//printf("the cmd is ==>%s\n",cmd);
        	parse_first_res(cmd);
		//printf("IP==>%s,watch_server=>%s\n",IP,watch_server);
		if(!strcmp(IP,watch_server))
		{
			printf(" Application asked for %u MB of memory and %u percentage of CPU\n", memory,cpu);
			form_low_level_query (cpu, memory);
			skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);
			//parse_inputs(argc, argv);
			g_main_loop_run (loop);
		}
		else
			continue;
	}
	form_query (type, cpu, memory);
 	skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);


// parse_inputs(argc, argv);
 g_main_loop_run (loop);
 
 return 0;

}

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



struct thread_info {    /* Used as argument to thread_start() */

    pthread_t thread_id;        /* ID returned by pthread_create() */
    int       thread_num;       /* Application-defined thread # */
    char     *argv_string;      /* From command-line argument */
    char     *start_addr;
};

int num_threads;

struct thread_info *tinfo;
char *start = NULL;

char L[200];
char query[200];
GMainLoop *loop;
Skb *proxy;

   pthread_attr_t attr;

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

	
  tinfo = arg;

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

void form_query (char *algo, char *fn_name, int input1, int input2){

  sprintf(query,"[%s], %s(%d,%d,L),write(L)",algo, fn_name, input1,input2);
  printf("query is =========>%s\n",query);

}


char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
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
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}


int parse_results(gchar *res){

   //char **result = str_split(res, );	

   printf("the result is --->%s\n",res);
   while(*res != '\0'){
	if(*res == '['){
	  res++; continue;
       }else{
	 return  (*res -'0') ;
//	 return  0 ;
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

  if (retval)
  {
//    printf ("finish success\n");
    fprintf (stdout, "the function called back with the results==> %s\n", qres);
   // printf("the ersiyt iis --->%s\n",L);
    fflush (stdout);
  }
  else
    printf ("failed\n");
  // result=g_async_result_get_user_data ();

  int node_id = parse_results(qres);
  if(numa_run_on_node(node_id) < 0)
 	printf("Failed to run on the  node %d \n", node_id);

  if (signal(SIGINT, sig_handler) == SIG_ERR)
  	printf("\ncan't catch SIGINT\n");

 tinfo = calloc(num_threads, sizeof(struct thread_info));

 if (tinfo == NULL)
        printf("calloc failed\n");

 int s = pthread_attr_init(&attr);
  

   int tnum;
 for (tnum = 0; tnum < num_threads; tnum++) {

       tinfo[tnum].thread_num = tnum + 1;
       tinfo[tnum].argv_string = "Hello";

       s = pthread_create(&tinfo[tnum].thread_id, &attr,(void *) &thread_start, &tinfo[tnum]);

       if (s != 0)
           printf("pthread_create failed");
  }
  s = pthread_attr_destroy(&attr);
  if (s != 0)
	printf("failed to destroy the struct\n");

#if 1
  for (tnum = 0; tnum < num_threads; tnum++) {

        s = pthread_join(tinfo[tnum].thread_id, NULL);
        if (s != 0)
		printf("thread dint terminate\n");
  }
#endif

 g_main_loop_unref (loop);
 g_main_loop_quit (loop);

}

int main (int argc, char *argv[]){

   num_threads = atoi(argv[1]);
   int s;
   start = (char*) numa_alloc_local(1024*num_threads);

   if (start == NULL)
	printf("failed to allocate memory on local node\n");

  GError *error;
  error = NULL;
  gchar *res;
  GAsyncResult *gasync;
  proxy = skb_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, 
				      G_DBUS_PROXY_FLAGS_NONE, 
				      "org.freedesktop.Skb",	/* bus name */
				      "/org/freedesktop/Skb",	/* object */
				      NULL,	                /* GCancellable */
				      &error);

  loop = g_main_loop_new (NULL, FALSE);

  form_query (argv[2],argv[3],atoi(argv[4]),atoi(argv[5]));

  skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);

  g_main_loop_run (loop);
  return 0;
}

 

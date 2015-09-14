#define _GNU_SOURCE
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <values.h>
#include "Skb.h"
#include <pthread.h>
#include <stdlib.h>




#define CONVERT_DIGITS_TO_NUM(p, n) \
    n = *p++ - '0'; \
    while (isdigit(*p)) { \
        n *= 10; \
        n += (*p++ - '0'); \
   }

int num_cpus = 0;
int num_nodes = 0;
char *sysIP;
char *sysName;



int timex = 0;

long sum_CPUs_total = 0;

GMainLoop *loop;
int call_count = 0;

pthread_attr_t attr;

void
callback_from_skb_query (GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data){

  fprintf(stdout,"the function called back with the results\n");
  fflush(stdout);
  if(call_count-- <= 1) {
	  g_main_loop_unref (loop);
	  g_main_loop_quit (loop);

 }

}

char query[100];

typedef struct watch_directory {
  char node_ip[50];
  char *dir[20];
}watch_dir ;


watch_dir *wdir;

void
form_query (char *algo, char *fn_name, char *node)
{

  sprintf (query, "[%s], %s(%s)", algo, fn_name, node);
  printf ("query is =========>%s\n", query);

}

int chflag  = 0;

void *watch_etcd_change (void *dir){
   
	watch_dir *watch = (watch_dir*) dir;
	printf("ip is ============>%s %s\n",watch->node_ip, watch->dir);
        create_etcd_session(watch->node_ip);
         while (1 && !chflag) {
		 chflag  =  do_watch (watch->node_ip,watch->dir);
		 sleep (1);
	 }
	close_etcd_session();
}

char nodes[][30]={"x86_64","x86_64","aarch64"};

void add_num_nodes_info_to_skb(watch_dir *wdir){

  int ix = 0;
  Skb *proxy;
  GError *error;
  error = NULL;
  create_etcd_session(wdir->node_ip);
  proxy = skb_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, 
					      G_DBUS_PROXY_FLAGS_NONE,
					      "org.freedesktop.Skb",	/* bus name */
					      "/org/freedesktop/Skb",	/* object */
					      NULL,	                /* GCancellable* */
					      &error);

		printf("the fact is ============>%s\n",wdir->dir);
  while (1) {

	if ( chflag ) {
		
		for (ix = 0; ix < 3; ix++){

		  form_query("test_algo","delete_sysinfo", nodes[ix]);
		  skb_call_query (proxy, query, NULL, NULL, NULL);

	 	 }

		char *fact = (char*)do_get(wdir->dir);
		char *p;
	 	p = strtok (fact, "\n");
		char *data = (char*)do_get(p);
		printf("data is ============>%s\n",data);
	        skb_call_add_fact (proxy, data, NULL, callback_from_skb_query, NULL);
        	while( p != NULL ) 
   		{
   			p = strtok(NULL, "\n");
			if(p != NULL)
			{
				data = (char*)do_get(p);
	        		skb_call_add_fact (proxy, data, NULL, callback_from_skb_query, NULL);
			}
			printf("fact is ============>%s\n",data);
   		}
 		chflag = 0;


	 		//sprintf(fact,"nodeinfo(%d, %d, %ld, %6ld, %ld, %ld)");
		//sprintf(fact,"nodeinfo(1,2,3,4,5,'1.2.3.4').");
	    }
    }
	close_etcd_session();
        g_main_loop_quit (loop);
 
#if 0
  for (ix = 0; ix < num_nodes; ix++)
    {

	sprintf(fact,"nodeinfo(%d, %d, %ld, %6ld, %ld, %ld)",ix, NUM_IDS_IN_LIST(node[ix].cpu_list_p),  node[ix].MBs_total, node[ix].MBs_free, node[ix].CPUs_total,
               node[ix].CPUs_free);
	skb_call_add_fact (proxy, fact, NULL, callback_from_skb_query, NULL);
	fprintf (stdout,
               " added to skb Node %d:  num cpus %d: MBs_total %ld, MBs_free %6ld, CPUs_total %ld, CPUs_free %4ld \n ",
               ix, NUM_IDS_IN_LIST(node[ix].cpu_list_p),  node[ix].MBs_total, node[ix].MBs_free, node[ix].CPUs_total,
               node[ix].CPUs_free);
 	call_count++;
    }
#endif
}



int
main (int argc, char* argv[])
{

        wdir = malloc (sizeof(watch_dir));
        strcpy(wdir->node_ip,argv[1]);
        strcpy(wdir->dir,argv[2]);
        pthread_t thread_id; 
        pthread_create(&thread_id, NULL,
                                  &watch_etcd_change, (void*)wdir);
        add_num_nodes_info_to_skb(wdir);
  	g_main_loop_run (loop);
	

   //show_nodes_info ();


}
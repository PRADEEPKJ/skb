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



#define KILOBYTE (1024)
#define MEGABYTE (1024 * 1024)

#define FNAME_SIZE 192
#define BUF_SIZE 1024
#define BIG_BUF_SIZE 4096

#define FNAME_SIZE 192

// The ONE_HUNDRED factor is used to scale time and CPU usage units.
// Several CPU quantities are measured in percents of a CPU; and
// several time values are counted in hundreths of a second.
#define ONE_HUNDRED 100


#define MIN_INTERVAL  5
#define MAX_INTERVAL 15
#define CPU_THRESHOLD     50
#define MEMORY_THRESHOLD 300
#define DEFAULT_HTT_PERCENT 20
#define DEFAULT_THP_SCAN_SLEEP_MS 1000
#define DEFAULT_UTILIZATION_PERCENT 85
#define DEFAULT_MEMLOCALITY_PERCENT 90


#define CONVERT_DIGITS_TO_NUM(p, n) \
    n = *p++ - '0'; \
    while (isdigit(*p)) { \
        n *= 10; \
        n += (*p++ - '0'); \
   }


#define NUM_IDS_IN_LIST(list_p)     CPU_COUNT_S(list_p->bytes, list_p->set_p)
#define ADD_ID_TO_LIST(k, list_p)  CPU_SET_S(k, list_p->bytes, list_p->set_p)

#define ID_IS_IN_LIST(k, list_p) CPU_ISSET_S(k, list_p->bytes, list_p->set_p)

#define INIT_ID_LIST(list_p, num_elements) \
    list_p = malloc(sizeof(id_list_t)); \
    if (list_p == NULL) { numad_log(LOG_CRIT, "INIT_ID_LIST malloc failed\n"); exit(EXIT_FAILURE); } \
    list_p->set_p = CPU_ALLOC(num_elements); \
    if (list_p->set_p == NULL) { numad_log(LOG_CRIT, "CPU_ALLOC failed\n"); exit(EXIT_FAILURE); } \
    list_p->bytes = CPU_ALLOC_SIZE(num_elements);


#define CLEAR_CPU_LIST(list_p) \
    if (list_p == NULL) { \
        INIT_ID_LIST(list_p, num_cpus); \
    } \
    CPU_ZERO_S(list_p->bytes, list_p->set_p)


#define CLEAR_NODE_LIST(list_p) \
    if (list_p == NULL) { \
        INIT_ID_LIST(list_p, num_nodes); \
    } \
    CPU_ZERO_S(list_p->bytes, list_p->set_p)


typedef struct id_list
{
  // Use CPU_SET(3) <sched.h> cpuset bitmasks,
  // but bundle size and pointer together
  // and genericize for both CPU and Node IDs
  cpu_set_t *set_p;
  size_t bytes;
} id_list_t, *id_list_p;

int num_cpus = 0;
int num_nodes = 0;


typedef struct node_data
{
  uint64_t node_id;
  uint64_t MBs_total;
  uint64_t MBs_free;
  uint64_t CPUs_total;		// scaled * ONE_HUNDRED
  uint64_t CPUs_free;		// scaled * ONE_HUNDRED
  id_list_p cpu_list_p;
} node_data_t;


node_data_t *node = NULL;
#define numad_log fprintf
#undef LOG_CRIT
#define LOG_CRIT stdout


typedef struct cpu_data
{
  uint64_t time_stamp;
  uint64_t *idle;
} cpu_data_t, *cpu_data_p;

cpu_data_t cpu_data_buf[2];	// Two sets, to calc deltas

int cur_cpu_data_buf = 0;
id_list_p all_cpus_list_p = NULL;
id_list_p all_nodes_list_p = NULL;

int timex = 0;

long sum_CPUs_total = 0;

GMainLoop *loop;
int call_count = 0;

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


int
add_ids_to_list_from_str (id_list_p list_p, char *s)
{
  if (list_p == NULL)
    {
      numad_log (LOG_CRIT, "Cannot add to NULL list\n");
      exit (EXIT_FAILURE);
    }
  if ((s == NULL) || (strlen (s) == 0))
    {
//      printf("buff is null............\n");
      goto return_list;
    }
  int in_range = 0;
  int next_id = 0;
  for (;;)
    {
      // skip over non-digits
      while (!isdigit (*s))
	{
	  if ((*s == '\n') || (*s == '\0'))
	    {
	      goto return_list;
	    }
	  if (*s++ == '-')
	    {
	      in_range = 1;
	    }
	}
      int id;
      CONVERT_DIGITS_TO_NUM (s, id);
      if (!in_range)
	{
	  next_id = id;
	}
      for (; (next_id <= id); next_id++)
	{
	  ADD_ID_TO_LIST (next_id, list_p);
//          printf("num of ids ---->%d\n",NUM_IDS_IN_LIST(list_p));

	}
      in_range = 0;
    }
return_list:
  return NUM_IDS_IN_LIST (list_p);
}



uint64_t
get_time_stamp ()
{
  // Return time stamp in hundredths of a second
  struct timespec ts;
  if (clock_gettime (CLOCK_MONOTONIC, &ts) < 0)
    {
      numad_log (LOG_CRIT, "Cannot get clock_gettime()\n");
      exit (EXIT_FAILURE);
    }
  return (ts.tv_sec * ONE_HUNDRED) +
    (ts.tv_nsec / (1000000000 / ONE_HUNDRED));
}


void
get_cpus_idle_data ()
{
  // Parse idle percents from CPU stats in /proc/stat cpu<N> lines

  int num_cpus = get_num_cpus ();
  static FILE *fs;
  if (fs != NULL)
    {
      rewind (fs);
    }
  else
    {
      fs = fopen ("/proc/stat", "r");
      if (!fs)
	{
	  numad_log (LOG_CRIT, "Cannot get /proc/stat contents\n");
	  exit (EXIT_FAILURE);
	}
         }
  // Use the other cpu_data buffer...
//  int new = 1 - cur_cpu_data_buf;
  //int new = 0;
  // First get the current time stamp
  cpu_data_buf[timex].time_stamp = get_time_stamp ();
  // Now pull the idle stat from each cpu<N> line 
  char buf[BUF_SIZE];
  while (fgets (buf, BUF_SIZE, fs))
    {
      /* 
       * Lines are of the form:
       *
       * cpu<N> user nice system idle iowait irq softirq steal guest guest_nice
       *
       * # cat /proc/stat
       * cpu  11105906 0 78639 3359578423 24607 151679 322319 0 0 0
       * cpu0 190540 0 1071 52232942 39 7538 234039 0 0 0
       * cpu1 124519 0 50 52545188 0 1443 6267 0 0 0
       * cpu2 143133 0 452 52531440 36 1573 834 0 0 0
       * . . . . 
       */
      if ((buf[0] == 'c') && (buf[1] == 'p') && (buf[2] == 'u')
	  && (isdigit (buf[3])))
	{
	  char *p = &buf[3];
	  int cpu_id = *p++ - '0';
	  while (isdigit (*p))
	    {
	      cpu_id *= 10;
	      cpu_id += (*p++ - '0');
	    }
	  while (!isdigit (*p))
	    {
	      p++;
	    }
	  while (isdigit (*p))
	    {
	      p++;
	    }			// skip user
	  while (!isdigit (*p))
	    {
	      p++;
	    }
	  while (isdigit (*p))
	    {
	      p++;
	    }			// skip nice
	  while (!isdigit (*p))
	    {
	      p++;
	    }
	  while (isdigit (*p))
	    {
	      p++;
	    }			// skip system
	  while (!isdigit (*p))
	    {
	      p++;
	    }
	  uint64_t idle;
	  CONVERT_DIGITS_TO_NUM (p, idle);
	  //printf("cpu %d idle==>%d\n",cpu_id, idle);
	  cpu_data_buf[timex].idle[cpu_id] = idle;
	}
    }
  //cur_cpu_data_buf = new;
}



int
count_set_bits_in_hex_list_file (char *fname)
{
  int sum = 0;
  int fd = open (fname, O_RDONLY, 0);
  if (fd >= 0)
    {
      char buf[BUF_SIZE];
      int bytes = read (fd, buf, BUF_SIZE);
      close (fd);
      int ix;
      for (ix = 0; (ix < bytes); ix++)
	{
	  char c = tolower (buf[ix]);
	  switch (c)
	    {
	    case '0':
	      sum += 0;
	      break;
	    case '1':
	      sum += 1;
	      break;
	    case '2':
	      sum += 1;
	      break;
	    case '3':
	      sum += 2;
	      break;
	    case '4':
	      sum += 1;
	      break;
	    case '5':
	      sum += 2;
	      break;
	    case '6':
	      sum += 2;
	      break;
	    case '7':
	      sum += 3;
	      break;
	    case '8':
	      sum += 1;
	      break;
	    case '9':
	      sum += 2;
	      break;
	    case 'a':
	      sum += 2;
	      break;
	    case 'b':
	      sum += 3;
	      break;
	    case 'c':
	      sum += 2;
	      break;
	    case 'd':
	      sum += 3;
	      break;
	    case 'e':
	      sum += 3;
	      break;
	    case 'f':
	      sum += 4;
	      break;
	    case ' ':
	      sum += 0;
	      break;
	    case ',':
	      sum += 0;
	      break;
	    case '\n':
	      sum += 0;
	      break;
	    default:
	      numad_log (LOG_CRIT, "Unexpected character in list\n");
	      exit (EXIT_FAILURE);
	    }
	}
    }
  return sum;
}

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
get_cpus_usage_per_node ()
{

  int num_nodes = get_num_nodes ();
  struct dirent **namelist;
  char fname[FNAME_SIZE];
  int threads_per_core = 0;

  char buf[BIG_BUF_SIZE];
  int node_ix;
  int n;

  CLEAR_CPU_LIST (all_cpus_list_p);
  CLEAR_NODE_LIST (all_nodes_list_p);


  int num_files =
    scandir ("/sys/devices/system/node", &namelist, node_and_digits, NULL);
  if (num_files < 1)
    {
      numad_log (LOG_CRIT, "Could not get NUMA node info\n");
      exit (EXIT_FAILURE);
    }

  for (node_ix = 0; (node_ix < num_nodes); node_ix++)
    {
      int node_id;
      char *p = &namelist[node_ix]->d_name[4];
      //printf("the num is--> %s\n",p);
      CONVERT_DIGITS_TO_NUM (p, node_id);
      free (namelist[node_ix]);
      node[node_ix].node_id = node_id;
      //printf ("node id is ==>%d\n", node_id);
      ADD_ID_TO_LIST (node_id, all_nodes_list_p);
     // printf ("node id is ==>%d\n", node_id);
      snprintf (fname, FNAME_SIZE, "/sys/devices/system/node/node%d/cpulist",
		node_id);
      int fd = open (fname, O_RDONLY, 0);
      
      if(fd < 0)
          printf("memtotal is---------------->%s \n",strerror(errno));

      if ((fd >= 0) && (read (fd, buf, BIG_BUF_SIZE) > 0))
	{
	  CLEAR_CPU_LIST (node[node_ix].cpu_list_p);
	  //printf("after clear=============\n");
	  n = add_ids_to_list_from_str (node[node_ix].cpu_list_p, buf);
	  n = NUM_IDS_IN_LIST(node[node_ix].cpu_list_p); 
	  //printf ("number of cpus ==>%d\n", n);
	}

#if 0
      threads_per_core =
	count_set_bits_in_hex_list_file
	("/sys/devices/system/cpu/cpu0/topology/thread_siblings");
      if (threads_per_core < 1)
	{
	  numad_log (LOG_CRIT, "Could not count threads per core\n");
	  exit (EXIT_FAILURE);
	}
      if (threads_per_core == 1)
	{
	  printf ("threads per core is one\n");
	  node[node_ix].CPUs_total = n * ONE_HUNDRED;
	}
      else
	{

	  printf ("threads per core is --->%d\n", threads_per_core);
	  n /= threads_per_core;
	  node[node_ix].CPUs_total = n * ONE_HUNDRED;
	  //printf("total cpus usage ==>%d\n",  node[node_ix].CPUs_total);
	  //  node[node_ix].CPUs_total += n * (threads_per_core - 1); 
	  //printf("total cpus usage ==>%d\n",  node[node_ix].CPUs_total);
	}
#endif
      node[node_ix].CPUs_total = n * ONE_HUNDRED;
      sum_CPUs_total += node[node_ix].CPUs_total;
      //printf ("total cpus usage ==>%d\n", sum_CPUs_total);
      close (fd);

      int old_cpu_data_buf = 1 - cur_cpu_data_buf;
      if (cpu_data_buf[old_cpu_data_buf].time_stamp > 0) 
	{
      	 // printf("the index is ==>%d\n",new);
	  uint64_t idle_ticks = 0;
	  int cpu = 0;
	  int num_lcpus = NUM_IDS_IN_LIST (node[node_ix].cpu_list_p);
	  printf("Number of lcpus are ==>%d\n",num_lcpus);
	  int num_cpus_to_process = num_lcpus;
	  while (num_cpus_to_process)
	    {
	      if (ID_IS_IN_LIST (cpu, node[node_ix].cpu_list_p))
		{
		  idle_ticks += cpu_data_buf[timex].idle[cpu]
		    - cpu_data_buf[timex-1].idle[cpu];
		  num_cpus_to_process -= 1;
		}
	      cpu += 1;
	    }
	  uint64_t time_diff = cpu_data_buf[timex].time_stamp
	    - cpu_data_buf[timex-1].time_stamp;
 
	   printf("num idle ticks ==>%d\n, time diff is ==>%d\n",idle_ticks,time_diff);  

	  node[node_ix].CPUs_free = (idle_ticks * ONE_HUNDRED) / time_diff;

	}
    }
}


int
get_num_cpus ()
{
  int n1 = sysconf (_SC_NPROCESSORS_CONF);
  int n2 = sysconf (_SC_NPROCESSORS_ONLN);
  if (n1 < n2)
    {
      n1 = n2;
    }
  if (n1 < 0)
    {
      numad_log (stderr, "Cannot count number of processors\n");
      exit (EXIT_FAILURE);
    }
  return n1;
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
show_nodes_info ()
{

  int num_nodes = get_num_nodes ();

  int ix = 0;
  for (ix = 0; ix < num_nodes; ix++)
    {
      //fprintf(stdout, "Node %d: MBs_total %ld, MBs_free %6ld\n", ix, node[ix].MBs_total, node[ix].MBs_free);

      fprintf (stdout,
	       "Node %d:  num cpus %d: MBs_total %ld, MBs_free %6ld, CPUs_total %ld, CPUs_free %4ld \n ",
	       ix, NUM_IDS_IN_LIST(node[ix].cpu_list_p),  node[ix].MBs_total, node[ix].MBs_free, node[ix].CPUs_total,
	       node[ix].CPUs_free);

    }
}


 char query[100];
void
form_query (char *algo, char *fn_name, int node_id)
{

  sprintf (query, "[%s], %s(%d,%d,L)", algo, fn_name,node_id,node_id);
  printf ("query is =========>%s\n", query);

}

void add_num_nodes_info_to_skb(){


// int num_nodes = get_num_nodes ();

  int ix = 0;
  Skb *proxy;
  GError *error;
  error = NULL;
  char fact[80];
  proxy = skb_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, 
				      G_DBUS_PROXY_FLAGS_NONE,
				      "org.freedesktop.Skb",	/* bus name */
				      "/org/freedesktop/Skb",	/* object */
				      NULL,	                /* GCancellable* */
				      &error);

  for (ix = 0; ix < num_nodes; ix++){

	  form_query("test_algo","delete_numa_node_info",ix);
	  skb_call_query (proxy, query, NULL, NULL, NULL);
  }

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
}


void
get_node_mem_info ()
{

  int num_nodes = get_num_nodes ();

  int node_ix;

  char fname[200];

  char buf[BUF_SIZE];

  errno = 0;


  for (node_ix = 0; (node_ix < num_nodes); node_ix++)
    {

      int node_id = node[node_ix].node_id;
      // Get available memory info from node<N>/meminfo file
      snprintf (fname, FNAME_SIZE, "/sys/devices/system/node/node%d/meminfo",
		node_id);
      int fd = open (fname, O_RDONLY, 0);
	printf("the file name is=====>%s\n",fname);
	if(fd < 0)
          printf("memtotal is---------------->%s \n",strerror(errno));
      if ((fd >= 0) && (read (fd, buf, BIG_BUF_SIZE) > 0))
	{
	  close (fd);
	  uint64_t KB;
	  char *p = strstr (buf, "MemTotal:");
	  if (p != NULL)
	    {
	      p += 9;
	    }
	  else
	    {
	      numad_log (stderr, "Could not get node MemTotal\n");
	      exit (EXIT_FAILURE);
	    }
	  while (!isdigit (*p))
	    {
	      p++;
	    }
	  CONVERT_DIGITS_TO_NUM (p, KB);
	  node[node_ix].MBs_total = (KB / KILOBYTE);
	  if (node[node_ix].MBs_total < 1)
	    {
	      // If a node has zero memory, remove it from the all_nodes_list...
	      //CLR_ID_IN_LIST(node_id, all_nodes_list_p);
	    }
	  p = strstr (p, "MemFree:");
	  if (p != NULL)
	    {
	      p += 8;
	    }
	  else
	    {
	      numad_log (stderr, "Could not get node MemFree\n");
	      exit (EXIT_FAILURE);
	    }
	  while (!isdigit (*p))
	    {
	      p++;
	    }
	  CONVERT_DIGITS_TO_NUM (p, KB);
	  node[node_ix].MBs_free = (KB / KILOBYTE);
	}
    }
}

void child_proc ()
{

 
  node = (node_data_t *) realloc (node, ( sizeof (node_data_t) * num_nodes));

   int ix ;
   for ( ix = 0;  (ix < num_nodes);  ix++) {
	// If new > old, nullify new node_data pointers
        node[ix].cpu_list_p = NULL;
   }

  cpu_data_buf[0].idle = malloc (num_cpus * sizeof (uint64_t));
  cpu_data_buf[1].idle = malloc (num_cpus * sizeof (uint64_t));
  if ((cpu_data_buf[0].idle == NULL) || (cpu_data_buf[1].idle == NULL))
  {
      numad_log (LOG_CRIT, "cpu_data_buf malloc failed\n");
      exit (EXIT_FAILURE);
  }
  get_cpus_idle_data ();
  sleep (2);
  timex++;
  get_cpus_idle_data ();
  get_cpus_usage_per_node ();
  get_node_mem_info ();

  loop = g_main_loop_new (NULL, FALSE);
  add_num_nodes_info_to_skb();

  free (node);

}



void
main ()
{

  num_cpus = get_num_cpus ();

  printf ("Number of cpus==>%d\n", num_cpus);

  num_nodes = get_num_nodes ();

  printf ("Number of nodes==>%d\n", num_nodes);

     child_proc();
  g_main_loop_run (loop);



   //show_nodes_info ();


}

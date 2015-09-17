#include "Skb.h"
#include "etcd-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <string.h>



/*This file implements the partial sharing of data
  The functionality does not exist by default with
  any DKV,so a watcher will read the data from a node
  and update to the other cluster based on the changes
*/

char *key;


void
read_and_update_master (char *write_server, char *direct)
{

  //char fact[1024];
  //memset(fact, 0 , 1024);

  char *fact = (char *) do_get (direct);	//read data
  //char *node = (char*)getenv("WATCHSERVERS");
  //printf ("argv[1]===>%s, argv[2]==>%s\n", write_server, direct);
  close_etcd_session ();	//close read session
  create_etcd_session (write_server);
  do_set (direct, fact, NULL, NULL, NULL);	//update the data
  close_etcd_session ();
}


void
watch_etcd_change (char *read_server, char *write_server, char *direct)
{

  int chflag;
  //printf ("argv[1]===>%s, argv[2]==>%s\n, argv[3]==>%s", read_server,
	  //write_server, direct);

  while (1)
    {
      create_etcd_session (read_server);
      chflag = do_watch (read_server, direct);
      if (chflag == 1)
	read_and_update_master (write_server, direct);

      sleep (1);
    }
  close_etcd_session ();
}


int
main (int argc, char *argv[])
{

  if (argc < 4)
    {
      printf
	("usage is ./watcher watch-etcd-node update-etcd-node key_to_be_watched \n");
      printf
	("example is ./addsysinfo 192.168.0.102:7001 192.168.0.193:7001 global/x86xeon \n");
      exit (0);

    }


  //printf ("argv[1]===>%s\n", argv[1]);
  //watch server (argv[1]), and dirctory/key (argv[2]), update the server (argv[3])
  watch_etcd_change (argv[1], argv[2], argv[3]);
  return 0;

}

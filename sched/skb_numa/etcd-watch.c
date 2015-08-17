#include "Skb.h"
#include "etcd-api.h"
#include <stdio.h>
#include <stdlib.h>


/*This file implements the partial sharing of data
  The functionality does not exist by default with
  any DKV,so a watcher will read the data from a node
  and update to the other cluster based on the changes
*/

char *key;

void read_and_update_master(char *server, char *direct) {

	//char fact[1024];
	//memset(fact, 0 , 1024);
	
	printf("argv[1]===>%s, argv[2]==>%s\n",server, direct);
	char *fact = (char*)do_get(direct);//read data
	char *node = (char*)getenv("WATCHSERVERS");
	printf("value changed is ==>%s\n",node);
	create_etcd_session(node);
	do_set(node,fact,NULL,NULL,NULL);//update the data
        close_etcd_session();
}


void watch_etcd_change (char *server, char *direct){
   
	 int chflag;
	 create_etcd_session(server);
	printf("argv[1]===>%s, argv[2]==>%s\n",server, direct);
         
         while (1) {
		 chflag  =  do_watch (server, direct);
		 if(chflag == 1)
			read_and_update_master(server, direct);
		  
		 sleep (1);
	 }
	close_etcd_session();
}


int main (int argc, char* argv[])
{

	printf("argv[1]===>%s\n",argv[1]);

	//watch server (argv[1]) and dirctory/key (argv[2])
	watch_etcd_change (argv[1], argv[2]);
	return 0;

}

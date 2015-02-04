#include "Skb.h"
#include <stdio.h>
#include <string.h>


char L[200];
char query[200];
GMainLoop *loop;
Skb *proxy;
int write_to_file = 0;
void
on_add_fact (GObject * connection, GAsyncResult * res, gpointer user_data)
{
//  DeliveryData *data = user_data;
  GError *error;
  //GVariant *result;

  //printf ("Fact added successfully\n");
  g_main_loop_unref (loop);
  g_main_loop_quit (loop);

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
  retval = skb_call_query_finish (proxy, &qres, res, NULL);
  FILE *fp; 
  if (retval)
  {
   fp = fopen("/tmp/skb_query_result.txt", "w+");
    fprintf (fp, "Passed Successful\n");
    if (write_to_file == 1)
    {
	fprintf(fp,"Query result is :%s\n",qres);
    }
  }
  else
    fprintf (fp, "Failed\n");
    fflush (fp);

  fclose(fp);
  g_main_loop_unref (loop);
  g_main_loop_quit (loop);
}

void form_query (char *algo, char *fn_name, int input1, int input2){

  sprintf(query,"[%s], %s(%d,%d,L),write(L)",algo, fn_name, input1,input2);
  printf("query is =========>%s\n",query);

}
 
void form_plain_query (char *algo, char *fn_name){

  sprintf(query,"[%s], %s(L),write(L)",algo, fn_name);
  printf("query is =========>%s\n",query);

}

void form_timeout_query (char *algo, char *fn_name, char *timeout){

  sprintf(query,"[%s], %s(%s)",algo, fn_name, timeout );
  printf("query is =========>%s\n",query);

}

void
main (int argc, char *argv[])
{

  GError *error;
  error = NULL;
  gchar *res;
  GAsyncResult *gasync;
  proxy = skb_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, "org.freedesktop.Skb",	/* bus name */
				      "/org/freedesktop/Skb",	/* object */
				      NULL,	/* GCancellable* */
				      &error);

  
 if (argc > 4 && !strcmp (argv[4], "-f")) 
 {
	write_to_file = 1;
 }
  
  loop = g_main_loop_new (NULL, FALSE);
  if (!strcmp (argv[1], "-a"))
    skb_call_add_fact (proxy, argv[2], NULL, on_add_fact, NULL);
  else if (!strcmp (argv[1], "-q"))
  {
      	form_plain_query (argv[2],argv[3]);
        skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);
  }
  else if (!strcmp (argv[1], "-t"))
  {
      	form_timeout_query (argv[2],argv[3],argv[4]);
        skb_call_query (proxy, query, NULL, callback_from_skb_query, NULL);
  }
  else
    printf ("Wrong option use -a to addfact and -q to query the fact\n");
  g_main_loop_run (loop);

}

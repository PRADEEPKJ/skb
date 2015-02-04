#include "Skb.h"
#include <stdio.h>
#include <string.h>


void
on_add_fact ()
{

  printf ("Fact added successfully\n");
}

void
main (int argc, char *argv[])
{

  Skb *proxy;

  GError *error;

  error = NULL;
  proxy = skb_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, "org.freedesktop.Skb",	/* bus name */
				      "/org/freedesktop/Skb",	/* object */
				      NULL,	/* GCancellable* */
				      &error);

  if (!(strcmp (argv[1], "-a")))
      skb_call_add_fact (proxy, argv[2], NULL, NULL, NULL);
  else if (!(strcmp (argv[1], "-q")))
      skb_call_query_fact (proxy, argv[2], NULL, NULL, NULL);
  else
	  printf ("Wrong option\n");
}

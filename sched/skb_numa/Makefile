CFLAGS  = -fPIC -O0 -Wall -lpthread
ARCH := $(shell uname -m 2>/dev/null)
ifeq ($(ARCH), x86_64)
	GTK_CFLAGS=-I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
else
	GTK_CFLAGS=-I/usr/include/glib-2.0 -I/usr/lib/aarch64-linux-gnu/glib-2.0/include
endif

GST_CFLAGS=-I/usr/include/gio-unix-2.0/ 
SKB_FLAGS =-I../../
GTK_LIBS=-lglib-2.0
GST_LIBS=-lgio-2.0 -lgobject-2.0
NUMA_LIBS=-lnuma
ETCD_FLAGS=-I/
ETCD_LIBS=-letcd
S_OBJS  = etcd-skb-int.o etcd-api.o
W_OBJS  : etcd-watch.o
	cc -c etcd-watch.o
SHLIB   = libetcd.so
WATCH_TARGET=../../Skb.c etcd-watch.c
$(SHLIB): $(S_OBJS)
	$(CC) $(ECD_FLAGS) -shared -nostartfiles $(S_OBJS) -lcurl -lyajl -o $@

add_query_fact_client: ../../Skb.c AddFact.c
	$(CC) -g $+ $(CFLAGS) $(GTK_CFLAGS) $(GTK_LIBS) $(SKB_FLAGS) $(GST_CFLAGS) $(GST_LIBS) -o $@
test_ipc_client_interface: ../../Skb.c IPC_Interface_Client_Lib.c
	$(CC) -g $+ $(CFLAGS) $(GTK_CFLAGS) $(GTK_LIBS) $(SKB_FLAGS) $(GST_CFLAGS) $(GST_LIBS) -o $@
skbscheduler: ../../Skb.c skb_scheduler.c	
	$(CC) -g $+ $(CFLAGS) $(GTK_CFLAGS) $(SKB_FLAGS) $(GTK_LIBS) $(GST_CFLAGS) $(GST_LIBS) $(NUMA_LIBS) -o $@
addsysinfo: ../../Skb.c getsysinfo.c	
	$(CC) -g $+ $(CFLAGS) $(GTK_CFLAGS) $(ETCD_FLAGS) $(SKB_FLAGS) $(GTK_LIBS) $(GST_CFLAGS) $(GST_LIBS) -L. $(ETCD_LIBS)  -o $@
loaddata: $(SHLIB) ../../Skb.c ld_etcd_skb.c 	
	$(CC) -g $+ $(CFLAGS) $(GTK_CFLAGS) $(ETCD_FLAGS) $(SKB_FLAGS) $(GTK_LIBS) $(GST_CFLAGS) $(GST_LIBS) -L. $(ETCD_LIBS)  -o $@
watcher: $(WATCH_TARGET) $(SHLIB) 
	$(CC) -g $(CFLAGS) $(WATCH_TARGET) $(GTK_CFLAGS) $(SKB_FLAGS) $(ETCD_FLAGS) $(GST_CFLAGS) $(GTK_LIBS) $(GST_LIBS) -L. -letcd -o $@
numaapp: ../../Skb.c numa_app.c	
	$(CC) -g $+ $(CFLAGS) $(GTK_CFLAGS) $(SKB_FLAGS) $(GTK_LIBS) $(GST_CFLAGS) $(GST_LIBS) $(NUMA_LIBS) -o $@
numaapp30: ../../Skb.c numa_app_30.c	
	$(CC) -g $+ $(CFLAGS) $(GTK_CFLAGS) $(SKB_FLAGS) $(GTK_LIBS) $(GST_CFLAGS) $(GST_LIBS) $(NUMA_LIBS) -o $@
clean:
	rm -rf *.o libetcd.so

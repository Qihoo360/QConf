CC      = g++
CFLAGS  = -c -D__STDC_FORMAT_MACROS -fPIC -g -pthread -Wall -I/usr/local/include/zookeeper -I../base/qlibc -I../base -DQCONF_CURL_ENABLE -DQCONF_AGENT_DIR=/home/liuzhongchao/code/qconf-bak/new/agent -DQCONF_INTERNAL
LIBS   	= /usr/local/lib/libzookeeper_mt.a /usr/local/lib/libgdbm.a /usr/local/lib/libcurl.a -lpthread  -lrt
OBJS 	= qconf_dump.o qconf_config.o qconf_daemon.o qconf_agent.o qconf_cmd.o qconf_zk.o qconf_feedback.o qconf_watcher.o qconf_script.o
COBJS 	= ../base/qlibc/qhasharr.o ../base/qlibc/md5.o ../base/qlibc/qhash.o
BASE    = ../base/qconf_format.o ../base/qconf_shm.o ../base/qconf_log.o ../base/qconf_msg.o

#.PHONY
.PHONY : all

all : $(OBJS) qconf_agent clean

qconf_agent : $(OBJS) $(BASE) $(COBJS)
	$(CC) -o $@ $^ $(LIBS)

clean : 
	rm -fr $(OBJS) $(BASE) $(COBJS)

#OBJS
$(OBJS) : %.o : %.cc
	$(CC) $(CFLAGS) -o $@ $<

$(BASE) : %.o : %.cc
	$(CC) $(CFLAGS) -o $@ $<

$(COBJS) : %.o : %.c
	$(CC) $(CFLAGS) -o $@ $<

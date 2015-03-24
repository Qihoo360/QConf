#gtest

#PARAM
QCONF_BASE 	= ../../base
DRIVER_BASE = ../../driver/c++/src
AGENT_BASE 	= ../../agent
CC			= g++
GTEST_DIR 	= /usr/local/gtest
CPPFLAGS 	+= -isystem $(GTEST_DIR)/include
CCFLAGS 	+= -g -Wall -DQCONF_CURL_ENABLE -pthread -I$(GTEST_DIR)/include -I/usr/local/include/zookeeper\
			   -I$(QCONF_BASE) -I$(QCONF_BASE)/qlibc -I$(DRIVER_BASE) -I$(DRIVER_BASE)/../include\
			   -I/usr/local/include\
			   -I$(AGENT_BASE) -D__STDC_FORMAT_MACROS
CCFLAGS_COV 	+= -g -Wall -pthread -I $(GTEST_DIR)/include --coverage\
				   -I/usr/local/gperftools/include -L/usr/local/gperftools/lib\
				   -lprofiler -ltcmalloc -fno-omit-frame-pointer \
				   -I/usr/local/libunwind/include -L/usr/local/libunwind/lib
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h 
LIBA 	= /usr/local/lib/libzookeeper_mt.a /usr/local/lib/libgdbm.a /usr/local/lib/libcurl.a
AR 			= ar
ARFLAGS 	= rcs
GTEST_LIBS	= gtest.a gtest_main.a
GTEST_OBJS  = gtest_all.o gtest_main.o
GTEST_FIRST	= no
#you only should change the OBJS here
GCC 		= gcc
COBJS		= $(QCONF_BASE)/qlibc/qhasharr.o $(QCONF_BASE)/qlibc/qhash.o\
			  $(QCONF_BASE)/qlibc/md5.o

CCOBJS 		= $(QCONF_BASE)/qconf_format.o $(QCONF_BASE)/qconf_log.o\
			  $(QCONF_BASE)/qconf_shm.o $(QCONF_BASE)/qconf_msg.o\
			  $(AGENT_BASE)/qconf_zk.o $(AGENT_BASE)/qconf_script.o\
			  $(AGENT_BASE)/qconf_cmd.o $(AGENT_BASE)/qconf_dump.o\
			  $(AGENT_BASE)/qconf_daemon.o $(AGENT_BASE)/qconf_config.o\
			  $(AGENT_BASE)/qconf_watcher.o $(AGENT_BASE)/qconf_feedback.o\
			  Test_qconf_format.o Test_qconf_msg.o\
 			  Test_qhasharr.o Test_qconf_shm.o\
              Test_qconf_dump.o Test_qconf_script.o Test_qconf_zk.o\
              Test_qconf_config.o Test_qconf_feedback.o


#.PHONY
ifeq ($(GTEST_FIRST), yes)
.PHONY : all $(GTEST_LIBS) build clean clean-all
all : $(GTEST_LIBS) build clean
else
.PHONY : all build clean clean-all
all : build clean
endif

build : $(COBJS) $(CCOBJS)
	$(CC) $(CPPFLAGS) -o $@ $^ $(LIBA) $(GTEST_LIBS) --coverage -lpthread -lrt

clean : 
	rm -fr $(COBJS) $(CCOBJS) $(GTEST_OBJS)

clean-all : 
	rm -fr $(COBJS) $(CCOBJS) $(GTEST_OBJS) *.gcda *.gcno $(QCONF_BASE)/*.gcda $(QCONF_BASE)/*.gcno\
		$(QCONF_BASE)/*.gcov\
		$(QCONF_BASE)/*.info $(QCONF_BASE)/_coverage\
		$(DRIVER_BASE)/*.gcov $(DRIVER_BASE)/*.gcda $(DRIVER_BASE)/*.gcno $(DRIVER_BASE)/*.info\
		$(AGENT_BASE)/*.gcov $(AGENT_BASE)/*gcda $(AGENT_BASE)/*.gcno $(AGENT_BASE)/*.info\
		key_value qconf.log.* *.prof

#OBJS
gtest_all.o : $(GTEST_SRCS_)
	$(CC) $(CPPFLAGS) $(CCFLAGS) -I $(GTEST_DIR) -c -o $@ $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CC) $(CPPFLAGS) $(CCFLAGS) -I $(GTEST_DIR) -c -o $@ $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest_all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest_all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

$(COBJS) : %.o : %.c
	$(CC) $(CCFLAGS)  -c -o $@ $<

$(CCOBJS) : %.o : %.cc
	$(CC) $(CCFLAGS) -c -o $@ $<

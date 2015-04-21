#gtest

#PARAM
QCONF_BASE 	= ../../
CC			= g++
GTEST_DIR 	= /usr/local/gtest
CPPFLAGS 	+= -isystem $(GTEST_DIR)/include
CCFLAGS 	+= -g -Wall -pthread -I $(GTEST_DIR)/include -I/usr/local/include/zookeeper -I$(QCONF_BASE)\
			   -I$(AGENT_BASE) -D__STDC_FORMAT_MACROS
CCFLAGS_COV 	+= -g -Wall -pthread -I $(GTEST_DIR)/include --coverage\
				   -I/usr/local/gperftools/include -L/usr/local/gperftools/lib\
				   -lprofiler -ltcmalloc -fno-omit-frame-pointer \
				   -I/usr/local/libunwind/include -L/usr/local/libunwind/lib
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h 
LIBA 	= /usr/local/lib/libzookeeper_mt.a
AR 			= ar
ARFLAGS 	= rcs
GTEST_LIBS	= gtest.a gtest_main.a
GTEST_OBJS  = gtest_all.o gtest_main.o
GTEST_FIRST	= yes
#you only should change the OBJS here
COBJS		= $(QCONF_BASE)/qhash.o $(QCONF_BASE)/md5.o
#CCOBJS  	= qconf_util_Test.o qconf_script_Test.o
CCOBJS  	= qconf_zk_Test.o $(QCONF_BASE)/qconf_zk.o \
			  qconf_config_Test.o $(QCONF_BASE)/qconf_config.o

#.PHONY
ifeq ($(GTEST_FIRST), yes)
.PHONY : all $(GTEST_LIBS) build clean clean-all
all : $(GTEST_LIBS) build clean
else
.PHONY : all build clean clean-all
all : build clean
endif

build : $(COBJS) $(CCOBJS)
	$(CC) $(CCFLAGS) -o $@ $^ $(LIBA) $(GTEST_LIBS) --coverage -lpthread 

clean : 
	rm -fr $(COBJS) $(CCOBJS) $(GTEST_OBJS)

clean-all : 
	rm -fr $(COBJS) $(CCOBJS) $(GTEST_OBJS) *.gcda *.gcno $(QCONF_BASE)/*.gcda $(QCONF_BASE)/*.gcno\
		$(QCONF_BASE)/*.gcov\
		$(QCONF_BASE)/*.info $(QCONF_BASE)/_coverage\
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

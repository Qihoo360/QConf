#qconf java lib

#PARAM
CC      = g++
JNI_DIR = /usr/java/jdk1.7.0_55/include/
JNI_MD_DIR = /usr/java/jdk1.7.0_55/include/linux/
QCONF_HEAD_DIR = /usr/local/include/qconf
LIBOBJS = /usr/local/qconf/lib/libqconf.a

CFLAGS  = -g -c -Wall -D__STDC_FORMAT_MACROS  -I$(JNI_DIR) -I$(JNI_MD_DIR) -I$(QCONF_HEAD_DIR) -Iinclude -pthread
FLAGS   = -g -Wall -D__STDC_FORMAT_MACROS -I$(JNI_DIR) -I$(JNI_MD_DIR) -I $(QCONF_HEAD_DIR) -Iinclude -pthread
COBJS 	= lib/java_qconf.o
SO_OBJS = libqconf_java.so
SOFLAGS = -shared -fPIC 
C_SRC   = lib/java_qconf.cc

#.PHONY
.PHONY : $(AR_OBJS) $(SO_OBJS) clean

all : $(AR_OBJS) $(SO_OBJS) clean

$(SO_OBJS) : $(C_SRC)
	$(CC) $(SOFLAGS) $(FLAGS) -o $@  $^ $(LIBOBJS)  -lpthread 

clean : 
	rm -fr $(COBJS)
	mv $(SO_OBJS) $(AR_OBJS) qconf_jar
	ant -buildfile qconf_jar/build.xml

$(COBJS) : %.o : %.cc
	$(CC) $(CFLAGS) -o $@ $<

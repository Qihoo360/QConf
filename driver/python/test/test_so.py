#!/usr/bin/python
import qconf_py as qconf

key = "/demo/conf/conf/conf/conf1"
#*********************Basic Usage*******************************
try:
    #get conf value
    value = qconf.get_conf(key)
    if value is "":
        print "empty string"
    print "value : " + value

    #get one service host
    host = qconf.get_host(key)
    print "host : " + host

    #get all service hosts
    hosts = qconf.get_allhost(key)
    #print hosts

    #get batch confs
    children = qconf.get_batch_conf(key)
    print "batch_conf :" 
    print children

    #get batch keys
    children_keys = qconf.get_batch_keys(key)
    print "batch_keys : " 
    print children_keys

except qconf.Error, Argument:
    print "error happend in Basic Usage! ", Argument
#*********************Basic Usage End****************************

key = "demo"
idc = "test"
#*********************Assign Idc Usage*******************************
try:
    #get conf value
    value = qconf.get_conf(key, idc)
    print "value : " + value

    #get one service host
    host = qconf.get_host(key, idc)
    print "host : " + host

    #get all service hosts
    hosts = qconf.get_allhost(key, idc)
    print hosts
    
    #get batch confs
    children = qconf.get_batch_conf(key)
    print "batch_conf :" 
    print children

    #get batch keys
    children_keys = qconf.get_batch_keys(key)
    print "batch_keys : " 
    print children_keys

except qconf.Error, Argument:
    print "error happend in Assign Idc Usage! ", Argument    
#*********************Assign Idc Usage End****************************

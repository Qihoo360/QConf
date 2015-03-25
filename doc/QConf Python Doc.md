QConf Python Doc
=====
## Build
- Build python extension
``` shell
python setup.py build
```
which will generate the qconf_py library in build/lib.linux-x86_64-X.X as qconf_py.so

- modify setup.py if you has specified the install prefix of Qconf.
``` shell
include_dirs : QCONF_INSTALL_PREFIX/include
extra_objects : QCONF_INSTALL_PREFIX/lib/libqconf.a
```

more information about python [Setup Script](https://docs.python.org/2/distutils/setupscript.html).

## API Doc


### QConf access functions 

----

### get_conf

`get_conf(path, idc)`

Description
>get configure value

Parameters
>path - key of configuration.
>
>idc - Optional，from which idc to get the value，get from local idc if  omit

Return Value
>value of the configuation, throw Exception qconf_py.Error if failed 
 
Example 
 >value = qconf.get_conf(key) 

### get_batch_keys

`get_batch_keys(path, idc);`

Description

>get all children nodes'key

Parameters
>path - key of configuration.
>
>idc - Optional，from which idc to get the keys，get from local idc if  omit

Return Value
>python list of the node's keys, throw Exception qconf_py.Error if failed 
 
Example 
 >children_keys = qconf.get_batch_keys(key)
 >
 > for item in children_keys:
 >
 > // iterate python list


### get_batch_conf

`get_batch_conf(path, idc);`

Description
>get all children nodes' key and value

Parameters
>path - key of configuration.
>
>idc - Optional， from which idc to get the children configurations，get from local idc if  omit

Return Value
>python dictionary of the children configuration, throw Exception qconf_py.Error if failed 
 
 Example 
 >children = qconf.get_batch_conf(key)
 >
 >for k, v in children.iteritems():
 >
 >//iterate dictionary

### get_allhost

`get_allhost(path, idc);`

Description
>get all available services under given path

Parameters
>path - key of configuration.
>idc - Optional， from which idc to get the services，get from local idc if  omit

Return Value
>python list of all available services, throw Exception qconf_py.Error if failed 
 
Example 
>hosts = qconf.get_allhost(key)
>
> for item in hosts:
>
> // iterate python list

### get_host

`get_host(path, idc);`

Description
>get one available service

Parameters
>path - key of configuration.
>
>idc - Optional，from which idc to get the host，get from local idc if  omit

Return Value
>available host, throw Exception qconf_py.Error if failed
 
Example 
>host = qconf.get_host(key)

---
## Example

``` python

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
      for h in hosts:
         print "one host: " + h

      #get batch confs
      children = qconf.get_batch_conf(key)
      for k, v in children.iteritems():
         print k + " => " + v

      #get batch keys
      children_keys = qconf.get_batch_keys(key)
      for h in children_keys:
         print "one key: " + h

  except qconf.Error, Argument:
      print "error happend in Basic Usage! ", Argument
```

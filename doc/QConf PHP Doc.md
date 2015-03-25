
 
QConf PHP Doc
=====
## Build
Build php extension

``` 

```



## API Doc


### QConf access functions 
----

### getConf

`getConf(key, idc, get_flag)`

Description
>get configure value

Parameters
>*path* - key of configuration.
>
>*idc* - Optional，from which idc to get the value，get from local idc if  omit
>
>*get_flag* - Optional，if set get_flag be 0, QConf will wait some time if the configuration is not in share memory yet, which may happen when key not exist or get key the first time. If 1, QConf return immediately return NULL, default get_flag is 0;

Return Value
>value of the configuation, NULL if failed 
 
Example 
 >$value = Qconf::getConf("/demo/conf");

### getBatchKeys

`getBatchKeys(path, idc, get_flag);`

Description
>get all children nodes'key

Parameters
>*path* - key of configuration.
>
>*idc* - Optional，from which idc to get the value，get from local idc if  omit
>
>*get_flag* - Optional，if set get_flag be 0, QConf will wait some time if the configuration is not in share memory yet, which may happen when key not exist or get key the first time. If 1, QConf return immediately return NULL, default get_flag is 0;

Return Value
>array of the children keys, NULL if failed 
 
Example 
 >$children_keys = Qconf::getBatchKeys("/demo/conf");


### getBatchConf

`getBatchConf(path, idc, get_flag);`

Description
>get all children nodes' key and value

Parameters
>path - key of configuration.
>
>idc - Optional， from which idc to get the children configurations，get from local idc if  omit
>
>*get_flag* - Optional，if set get_flag be 0, QConf will wait some time if the configuration is not in share memory yet, which may happen when key not exist or get key the first time. If 1, QConf return immediately return NULL, default get_flag is 0;

Return Value
>array of the children confs, NULL if failed 
 
Example 
 >$children_conf = Qconf::getBatchKeys("demo/conf");

### getAllHost

`getAllHost(path, idc, get_flag);`

Description
>get all available services under given path

Parameters
>path - key of configuration.
>
>idc - Optional， from which idc to get the services，get from local idc if  omit
>
>*get_flag* - Optional，if set get_flag be 0, QConf will wait some time if the configuration is not in share memory yet, which may happen when key not exist or get key the first time. If 1, QConf return immediately return NULL, default get_flag is 0;

Return Value
>array of the all available services, NULL if failed 
 
Example 
>$hosts = Qconf::getAllHost("demo/hosts");

### getHost

`getHost(path, idc, get_flag);`

Description
>get one available service

Parameters
>path - key of configuration.
>
>idc - Optional，from which idc to get the host，get from local idc if  omit
>
>*get_flag* - Optional，if set get_flag be 0, QConf will wait some time if the configuration is not in share memory yet, which may happen when key not exist or get key the first time. If 1, QConf return immediately return NULL, default get_flag is 0;

Return Value
>available host, NULL if failed
 
Example 
 >$host = Qconf::getHost("demo/hosts");

---
## Example

``` 

```

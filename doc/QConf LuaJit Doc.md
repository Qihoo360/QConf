QConf LuaJit Doc
=====
## Build
- Build luajit extension
``` shell
cp qconf.lua LuaJit_dir/share/lua/5.1/
```

## Usage
 - include the qconf.lua
```  luajit
 local qconf = require('qconf')
```

## API Doc


### QConf access functions 

----

### get_conf

`err, value = qconf.get_conf(path, idc)`

Description
>get configure value

Parameters
>path - key of configuration.
>
>idc - Optional，from which idc to get the value，get from local idc if  omit

Return Value
>err      - error code, 0 means success; others mean fail
>
>value  -  if err == 0, then it is configure of current path;  if err != 0, it is detailed information of current error code 
 
Example 
 >local err,  value = qconf.get_conf(key) 

### get_batch_keys

`err, value = qconf.get_batch_keys(path, idc);`

Description

>get all children nodes'key

Parameters
>path - key of configuration.
>
>idc - Optional，from which idc to get the keys，get from local idc if  omit

Return Value
>err      - error code, 0 means success; others mean fail
>
>value  -  if err == 0, then it is children nodes of current path;  if err != 0, it is detailed information of current error code 
 
Example 
 >err,  keys = qconf.get_batch_keys(key)
 >
 > for 1, #(keys) in keys  do
 > 
 >  -- do something
 >  
 > done


### get_batch_conf

`err, value = qconf.get_batch_conf(path, idc);`

Description
>get all children nodes' key and value

Parameters
>path - key of configuration.
>
>idc - Optional， from which idc to get the children configurations，get from local idc if  omit

Return Value
>err      - error code, 0 means success; others mean fail
>
>value  -  if err == 0, then it is children nodes' key and value of current path;  if err != 0, it is detailed information of current error code 
 
 Example 
 >err, children = qconf.get_batch_conf(key)
 >
 >for k, v in pairs(children) do
 >
 > -- do something
 > 
 >done

### get_allhost

`err, value = qconf.get_allhost(path, idc);`

Description
>get all available services under given path

Parameters
>path - key of configuration.
>idc - Optional， from which idc to get the services，get from local idc if  omit

Return Value
>err      - error code, 0 means success; others mean fail
>
>value  -  if err == 0, then it is available services of current path;  if err != 0, it is detailed information of current error code 
 
Example 
>err, hosts = qconf.get_allhost(key)
>
> for 1, #(hosts) in hosts  do
>
>  -- do something
>  
> done

### get_host

`err, value = qconf.get_host(path, idc);`

Description
>get one available service

Parameters
>path - key of configuration.
>
>idc - Optional，from which idc to get the host，get from local idc if  omit

Return Value
>err      - error code, 0 means success; others mean fail
>
>value  -  if err == 0, then it is one available service of current path;  if err != 0, it is detailed information of current error code 
 
Example 
>err, host = qconf.get_host(key)

---
## Example

``` luajit
local qconf = require('qconf')

local err, value = qconf.get_conf(key)
if err == 0 then
	print("value: " .. value)
end

local err, children = qconf.get_batch_conf(key)
if err == 0 then
	for k, v in pairs(children) do
		print(k)
		print(v)
	end
end

local err, keys = qconf.get_batch_keys(key)
if err == 0 then
	for v = 1, #(keys) do
		print(keys[v])
	end
end

local err, hosts = qconf.get_allhost(key)
if err == 0 then
	for v = 1, #(hosts) do
		print(hosts[v])
	end
end

local err, host = qconf.get_host(key)
if err == 0 then
	print(host)
end

```

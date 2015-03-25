QConf C\C++ Doc
=====

## C Interface
### **Environment initialisation and destroy functions.**

-------

### **qconf_init** 

`int qconf_init();`

Description
> Initial qconf environment

Parameters

Return Value**

> QCONF_OK if success,  others if failed. 

Example
> int ret = qconf_init();

### **qconf_destroy**
`int qconf_destroy();`

Description

>destroy qconf environment

Parameters

Return Value

Example
>qconf_destroy();

---

### **QConf access functions wait version, retry sometime if configure not exist**

----

### **qconf_get_conf**

`int qconf_get_conf(const char *path, char *buf, unsigned int buf_len, const char *idc);`

Description
>get configure value

Parameters
>path - key of configuration.
>
>buf - out parameter, buffer for value
>
>buf_len - lenghth of value buffer
>
>idc - from which idc to get the value，get from local idc if idc is NULL

Return Value
>QCONF_OK if success,  others if failed. QCONF_ERR_NOT_FOUND if configuration is not exists
 
Example 
>char value[QCONF_CONF_BUF_MAX_LEN];
>
>int ret = qconf_get_conf("demo/conf1", value, sizeof(value), NULL);
>
>assert(QCONF_OK == ret);   

### **qconf_get_batch_keys**

`int qconf_get_batch_keys(const char *path, string_vector_t *nodes, const char *idc);`

Description
>get all children nodes'key

Parameters
>path - key of configuration.
>
>nodes - out parameter, keep all children nodes'key
>
>idc - from which idc to get the keys，get from local idc if idc is NULL

Return Value
>QCONF_OK if success,  others if failed. QCONF_ERR_NOT_FOUND if not exists
 
Example
>string_vector_t bnodes_key;
>
>init_string_vector(&bnodes_key);
> 
>int ret = qconf_get_batch_keys(path, &bnodes_key, NULL);
>
>assert(QCONF_OK == ret);
> 
>for (i = 0; i < bnodes_key.count; i++)
>
>{ cout << bnodes_key.data[i] << endl;}
> 
>destroy_string_vector(&bnodes_key); 


### **qconf_get_batch_conf**

`int qconf_get_batch_conf(const char *path, string_vector_t *nodes, const char *idc);`

Description
>get all children nodes' key and value

Parameters
> path - key of configuration.
>
> nodes - out parameter, keep all children nodes' key and value
>
> idc - from which idc to get the children configurations，get from local idc if idc is NULL

Return Value
>QCONF_OK if success,  others if failed. QCONF_ERR_NOT_FOUND if not exists
 
Example
 >qconf_batch_nodes bnodes;
 >
 >init_qconf_batch_nodes(&bnodes);
 > 
 >int ret = qconf_get_batch_conf(path, &bnodes, NULL);
 >
 >assert(QCONF_OK == ret);
 > 
 >for (i = 0; i < bnodes.count; i++)
 >
 >{cout << bnodes.nodes[i].key << " : " << bnodes.nodes[i].value;}
 > 
 >destroy_qconf_batch_nodes(&bnodes); 

### **qconf_get_allhost**

`int qconf_get_allhost(const char *path, string_vector_t *nodes, const char *idc);`

Description
>get all available services under given path

Parameters
 > path - key of configuration.
 >
 > nodes - out parameter, keep all available services
 >
 > idc - from which idc to get the services，get from local idc if idc is NULL

Return Value
>QCONF_OK if success,  others if failed. QCONF_ERR_NOT_FOUND if not exists
 
Example 
>string_vector_t nodes;
>
>init_string_vector(&nodes);
> 
>int ret = qconf_get_batch_conf(path, &nodes, NULL);
>
>assert(QCONF_OK == ret);
> 
>for (i = 0; i < nodes.count; i++)
>
>{cout << nodes.data[i] << endl;}
> 
>destroy_string_vector(&nodes_key); 

### **qconf_get_host**

`int qconf_get_host(const char *path, char *buf, unsigned int buf_len, const char *idc);`

Description
>get one available service

Parameters
>path - key of configuration.
>
>buf - out parameter, keep the service
>
>buf_len - lenghth of buf
>
>idc - from which idc to get the value，get from local idc if idc is NULL

Return Value
>QCONF_OK if success,  others if failed. QCONF_ERR_NOT_FOUND if configuration is not exists
 
Example 
>char host[QCONF_HOST_BUF_MAX_LEN] = {0};
>
>int ret = qconf_get_host(path, host, sizeof(host), NULL);
>
>assert(QCONF_OK == ret);   

---
### **Data structure related functions**

----
``` c
typedef struct
  {
      int count;      // the number of services
      char **data;    // the array of services
  } string_vector_t;
```
### **init_string_vector**

`int init_string_vector(string_vector_t *nodes);`

Description
>initial array for keeping services
>
>**Tips:** the function should be called before calling qconf_get_batchkeys or qconf_get_allhosts

Parameters
>nodes - out parameter,  the array for keeping batch keys or services

Return Value
>QCONF_OK if success,  others if failed. QCONF_ERR_PARAM if nodes is null
 
Example 
>string_vector_t bnodes_key;
>
>init_string_vector(&bnodes_key);

### **destroy_string_vector**

`int destroy_string_vector(string_vector_t *nodes);`

Description
>destroy the array for keeping batch keys or services
>
>**Tips:** remember to call this function after the last use of string_vector_t

Parameters
>nodes - out parameter,  qconf_batch_nodes keeping batch keys or services

Return Value
>QCONF_OK if success,  others if failed. QCONF_ERR_PARAM if nodes is null
 
Example 
>qconf_batch_nodes nodes;
>
>destroy_string_vector(&nodes);


----

``` c
typedef struct qconf_node
  {
      char *key;
      char *value;
  } qconf_node;

  typedef struct qconf_batch_nodes
  {
      int count;
      qconf_node *nodes;
  } qconf_batch_nodes;
```

### **init_qconf_batch_nodes**

`int init_qconf_batch_nodes(qconf_batch_nodes *bnodes);`

Description
>initial nodes array for keeping batch conf
>
>**Tips:** the function should be called before calling qconf_get_batchconf

Parameters
>bnodes - out parameter,  qconf_batch_nodes to keeping batch nodes

Return Value
>QCONF_OK if success,  others if failed. QCONF_ERR_PARAM if nodes is null
 
Example 
>qconf_batch_nodes bnodes;
>
>init_qconf_batch_nodes(&bnodes);  

### **destroy_qconf_batch_nodes**

`int destroy_qconf_batch_nodes(qconf_batch_nodes *bnodes);`

Description
>destroy the nodes array for keeping batch conf
>
>**Tips:** remember to call this function after the last use of qconf_batch_nodes

Parameters
>bnodes - out parameter,  qconf_batch_nodes keeping batch nodes

Return Value
>QCONF_OK if success,  others if failed. QCONF_ERR_PARAM if nodes is null
 
Example
>qconf_batch_nodes bnodes;
>
>destroy_qconf_batch_nodes(&bnodes);




## Shell Command

### Usage:

``` c
qconf command key [idc]
command: can be one of below commands:
   get_conf        : get configure value
   get_host        : get one service
   get_allhost     : get all services available
   get_batch_keys  : get all children keys
   
key    : the path of your configure items
idc    : query from current idc if be omitted
```
### Example:

``` shell
       qconf get_conf "demo/conf"
       qconf get_conf "demo/conf" "test"
       
       qconf get_host "demo/hosts"
       qconf get_host "demo/hosts" "test"
       
       qconf get_allhost "demo/hosts"
       qconf get_allhost "demo/hosts" "test"

	   qconf get_conf "demo/batch"
       qconf get_conf "demo/batch" "test"
```
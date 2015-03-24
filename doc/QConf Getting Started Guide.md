QConf Getting Started Guide
=====

## Introduction
QConf is a Distrubuted Configuration Management System!
A better replacement of the traditional configuration file. As designed, configuration items which is constantly accessed and modified should be completely separated with application code, and QConf is where it should be.

 - Changes to any configuration will be synchronised across all client
   machines, in seconds.
 - High query efficiency.
 - Convenient deployment and simple interface.
 - High robustness and  fault-tolerance.


## Install
The QConf is built using CMake (version 2.6 or newer).

On most systems you can build the library using the following commands:
``` shell
mkdir build && cd build
cmake ..
make
make install
```
Alternatively you can also point the CMake GUI tool to the CMakeLists.txt file.

To install the QConf you can specify the install prefix by setting:
``` shell
cmake .. -DCMAKE_INSTALL_PREFIX=/install/prefix
```

## Usage

- **Set up Zookeeper servers**, create or modify znode with Zookeeper Client

	 More information about Zookeper: [ZooKeeper Getting Started Guide](http://zookeeper.apache.org/doc/r3.3.3/zookeeperStarted.html)
	 
- **Register** the Zookeeper server address with QConf

``` shell
vi QCONF_INSTALL_PREFIX/conf/idc.conf
```
``` php
  # all the zookeeper host configuration.
  #[zookeeper]
  zookeeper.test=127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183 #zookeeper of idc 'test'
```
 - **Assign** local idc
``` shell
echo test > QCONF_INSTALL_PREFIX/conf/localidc #assign local idc to test
```
 - **Run** QConf

``` shell
cd QCONF_INSTALL_PREFIX/bin && sh agent-cmd.sh start
```
 - **Code** to access QConf
## Component
* **ZooKeeper** as the server, restore all configurations, so the limit data size of single configuration item is 1MB, since the size limit of Znode.
* **qconf_agent** is a daemon run on client machine, maintain data in share memory.
* **share memory**,  QConf use share memory as the local cache of all configuration items needed by current client.
* **interface** in different programming languages, chose your language for accessing of QConf.  

![enter image description here](http://d.pcs.baidu.com/thumbnail/f1eeb4d1419f620c8e5cd689d73d4820?fid=237891803-250528-137309509566779&time=1427187600&sign=FDTAER-DCb740ccc5511e5e8fedcff06b081203-KVht4tlVOQXVuS7Ev9MugLEC7kE%3D&rt=sh&expires=2h&r=457518565&sharesign=unknown&size=c710_u500&quality=100 "QConf Componengt.png")


## Document
* [C Doc]() - the C reference to QConf APIs
* [PyDoc]() - Python 2.4,  2.5,  2.6 or 2.7 is required. Python 3.x is not yet supported.
* [PHP Doc]() -  the PHP reference to QConf APIs
* [Shell Command]() - the command qconf can be use in command line

## Example
* **C Exmaple**
``` c
	  // Init the qconf env
      ret = qconf_init();
      assert(QCONF_OK == ret);

      // Get Conf value
      char value[QCONF_CONF_BUF_MAX_LEN];
      ret = qconf_get_conf("/demo/node1", value, sizeof(value), NULL);
      assert(QCONF_OK == ret);

      // Get Batch keys
      string_vector_t bnodes_key;
      init_string_vector(&bnodes_key);

      ret = qconf_get_batch_keys("/demo/node2", &bnodes_key, NULL);
      assert(QCONF_OK == ret);

      int i = 0;
      for (i = 0; i < bnodes_key.count; i++)
      {
          // get keys from bnodes_key.data[i]
      }
      destroy_string_vector(&bnodes_key);

      // Destroy qconf env
      qconf_destroy();
```

* **Shell Exmaple**
``` shell
	   usage: qconf command key [idc]
       command: can be one of below commands:
                get_conf        : get configure value
                get_host        : get one service
                get_allhost     : get all services available
                get_batch_keys  : get all children keys
       key    : the path of your configure items
       idc    : query from current idc if be omitted
example:
       qconf get_conf "demo/conf"
       qconf get_conf "demo/conf" "test"
```
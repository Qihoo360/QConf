QConf
=====

[![Build Status](https://travis-ci.org/Qihoo360/QConf.svg?branch=master)](https://travis-ci.org/Qihoo360/QConf)

We have build another interesting proect [pika](https://github.com/Qihoo360/pika). Pika is a nosql compatible with redis protocol with huge storage space. You can have a try.

## Introduction [中文](https://github.com/Qihoo360/QConf/blob/master/README_ZH.md)

QConf is a Distributed Configuration Management System!
A better replacement of the traditional configuration file. As designed, configuration items which is constantly accessed and modified should be completely separated with application code, and QConf is where it should be.

## Features
* Changes to any configuration will be synchronised to all client machines in real-time.
* High query efficiency.
* Convenient deployment and simple interface.
* High robustness and  fault-tolerance.
* support c/c++, shell, php, python, lua, java, go, node and etc.

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
  
  #zookeeper of idc 'test'
  zookeeper.test=127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183
```
 - **Assign** local idc
``` 
echo test > QCONF_INSTALL_PREFIX/conf/localidc #assign local idc to 'test'
```
 - **Run** QConf

``` shell
cd QCONF_INSTALL_PREFIX/bin && sh agent-cmd.sh start
```
 - **Code** to access QConf


## Related
* [zkdash](https://github.com/ireaderlab/zkdash) - An excellent dashboard for QConf or ZooKeeper provided by IReader Team


## Performance
1.   statergy
 * **running times** :  ten million times altogether
 * **data size** :      the size of value of each key is 1k
 * **test method** :  multi-processes are running at the same time, and get the elapsed time that ten million times are running
 * **machine** : Intel(R) Xeon(R) CPU E5-2630 0 @ 2.30GHz, 24 cores; 64G memory
 * **language** : c++
2.   result
 * ![enter image description here](http://ww1.sinaimg.cn/bmiddle/69a9c739jw1eqgwvqxhhmj206207tdg5.jpg "Result")
3. conclusion: 
 * the lantency is 16μs
 * during multi-process, the QPS can reach one million
 
## Example

* **shell** 
``` shell
    qconf get_conf /demo/node1   # get the value of '/demo/node1'
```

* **c/c++**
``` c
	  // Init the qconf env
      ret = qconf_init();
      assert(QCONF_OK == ret);

      // Get Conf value
      char value[QCONF_CONF_BUF_MAX_LEN];
      ret = qconf_get_conf("/demo/node1", value, sizeof(value), NULL);
      assert(QCONF_OK == ret);
      
      // Destroy qconf env
      qconf_destroy();
```

## Document
* [Getting Started](https://github.com/Qihoo360/QConf/blob/master/doc/QConf%20Getting%20Started%20Guide.md) - a tutorial-style guide for developers to install, run, and program to QConf
* [Implement](http://catkang.github.io/2015/06/23/qconf.html)
* [wiki](https://github.com/Qihoo360/QConf/wiki)
* [qconf video guide](https://github.com/Qihoo360/QConf/wiki/QConf-%E7%AE%80%E6%98%93%E9%83%A8%E7%BD%B2%E5%92%8C%E4%BD%BF%E7%94%A8)

## Contact

* Email: g-qconf@list.qihoo.net
* QQ Group: 438042718 

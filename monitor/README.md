QConf monitor部署文档



QConf monitor提供服务监控功能，代码在QConf目录里，需要配合QConf使用

https://github.com/Qihoo360/QConf.git



QConf monitor需要在QConf安装部署完成的情况下使用，QConf的安装部署请见QConf文档: [QConf 简易部署和使用](https://github.com/Qihoo360/QConf/wiki/QConf-%E7%AE%80%E6%98%93%E9%83%A8%E7%BD%B2%E5%92%8C%E4%BD%BF%E7%94%A8)，[QConf Getting Started Guide](https://github.com/Qihoo360/QConf/blob/master/doc/QConf%20Getting%20Started%20Guide.md)

### 编译

我们提供CentOS 6.2 [rpm包](https://github.com/Qihoo360/QConf/releases/tag/1.2.1)，如果不支持可以按以下步骤编译安装，需要编译器支持C++11

```bash
git submodule init
git submodule update
yum install protobuf-devel.x86_64
cd QConf/monitor
mkdir build && cd build
cmake ..
make
make install
```



### 配置



配置文件默认在/usr/local/qconf/monitor/conf/monitor.conf

其中zookeeper.test: 127.0.0.1:2128为zookeeper的地址，"test"应该是部署monitor主机的域名一部分，例如部署monitor的主机是test01.foo.bar.net，则zookeeper地址可以配置为zookeeper.test01: 127.0.0.1:2181



### 使用



1. 通过QConf manager提供的接口（[PHP](https://github.com/Qihoo360/QConf/wiki/QConf-%E7%AE%A1%E7%90%86%E7%AB%AF%E6%8E%A5%E5%8F%A3%EF%BC%88PHP%EF%BC%89)，[C++](https://github.com/Qihoo360/QConf/wiki/QConf-%E7%AE%A1%E7%90%86%E7%AB%AF%E6%8E%A5%E5%8F%A3%EF%BC%88C---%EF%BC%89)）添加需要监控的主机为服务节点
2. 使用[zkdash](https://github.com/ireaderlab/zkdash)添加服务节点

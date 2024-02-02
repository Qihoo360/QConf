# qconf

Add Docker image support for https://github.com/Qihoo360/QConf

use env ZK_CLUSTER that set the zookeeper cluster

`ENV ZK_CLUSTER="127.0.0.1:2181,127.0.0.1:2182"`

* how to use

```
docker pull xidianwlc/qconf:1.2.1
docker run -it --privileged xidianwlc/qconf:1.2.1
```

在c++版的QConf客户端基础上包装的perl版客户端
2015-7-30 by fangyousong@qq.com 

安装说明
--------------
0. 按照QConf的说明对QConf进行编译安装，编译好的libqconf.so复制到/lib64/
1. 部署好zookeeper集群或单节点测试环境
2. 按QConf的文档说明配置好QConf对zookeeper的连接设置
3. 启动QConf agent
4. 执行命令：
	cd QConf
	perl Makefile.PL
	make
	make install
	make test
	
使用方式
-------------
api参考QConf/QConf.xs
示例参考QConf/test.pl
错误码请参考qconf_errno.h	
各个api函数的最后一个参数async(取值true/false)，表示是否异步调用，与c++api的同步和异步函数对应。同步和异步的概念请参考QConf的相关文档或如下：
	松仔 10:45:18
	同步和异步两种不同的api的区别是什么呢
	王康 10:47:44
	其实就是driver发现共享内存中没数据的时候的是尝试等待（最多0.5秒，分100次等），还是直接返回




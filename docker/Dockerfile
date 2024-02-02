FROM centos:centos7.2.1511

RUN rpm --rebuilddb && yum install -y gcc-c++ bzip2-devel mysql-devel snappy-devel libffi-devel python-devel

RUN yum install -y epel-release \
	&& yum install -y https://github.com/Qihoo360/QConf/releases/download/1.2.1/QConf-1.2.1-1.el6.x86_64.rpm \
	&& yum install -y https://github.com/Qihoo360/QConf/releases/download/1.2.1/QConf-monitor-1.2.1-1.el6.x86_64.rpm

ENV ZK_CLUSTER="127.0.0.1:2181"

COPY agent.conf /usr/local/qconf/conf/agent.conf

COPY entrypoint.sh /usr/local/qconf/bin/

ENTRYPOINT ["/bin/bash", "/usr/local/qconf/bin/entrypoint.sh"]

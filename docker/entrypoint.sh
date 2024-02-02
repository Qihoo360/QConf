#!/usr/bin/env bash

# entrypoint
# ~~~~~~~~~~~~

# :authors: smp
# :version: 1.0 of 2019-06-29
# :copyright: (c) 2016 smp

QCONF_CONF_PATH="/usr/local/qconf/conf"

sysctl -w kernel.shmmax=68719476736

echo "zookeeper.smp=$ZK_CLUSTER" > $QCONF_CONF_PATH/idc.conf
echo "smp" > $QCONF_CONF_PATH/localidc

/bin/bash /usr/local/qconf/bin/agent-cmd.sh start

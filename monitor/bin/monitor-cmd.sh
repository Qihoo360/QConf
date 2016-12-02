#!/bin/sh

monitor=qconf_monitor
cmdfile=tmp/cmd
pidfile=monitor_pid

chdpid=$(if [ -e $pidfile ]; then cat $pidfile; else echo 0; fi)
exist=$(ps -A | grep -w $monitor | grep -w $chdpid)

start () {
    if [ "$exist" = "" ]; then
        bin/$monitor > /dev/null 2>&1
        echo "$monitor start."
    else
        echo "$monitor is already running"
        exit 1
    fi
}

stop () {
    if [ "$exist" = "" ]; then
        echo "$monitor is not running"
        exit 1
    else
        killall -9 $monitor
        echo "$monitor stop."
    fi
}

list () {
    if [ "$exist" = "" ]; then
        echo "$monitor is not running"
        exit 1
    else
        echo "list:$1" > $cmdfile
        kill -USR1 $chdpid
        echo "Send list command to $monitor."
        sleep 2s
        tail -1000 tmp/list
    fi
}

reload () {
    if [ "$exist" = "" ]; then
        echo "$monitor is not running"
        exit 1
    else
        echo "reload:" > $cmdfile
        kill -USR1 $chdpid
        echo "Send reload command to $monitor."
    fi
}

version () {
    echo "$monitor version : 0.2.4"
}

case "$1" in
    start)
        start
    ;;

    stop)
        stop
    ;;

    version)
        version
    ;;

    list)
        list $2
    ;;

    reload)
        reload
    ;;

    *)
        echo "Usage: "
        echo "  $0 start              start qconf monitor. "
        echo "  $0 stop               stop qconf monitor."
        echo "  $0 reload             reload configuration."
        echo "  $0 list               list all service and their status."
        echo "  $0 list up            list service whose status is up."
        echo "  $0 list down          list service whose status is down."
        echo "  $0 list offline       list service whose status is offline."
        echo "  $0 list NODE          list service whose node is NODE. For example:$0 list /dba/p13309"
        echo "  $0 version            print monitor version."
        exit 1
    ;;

esac

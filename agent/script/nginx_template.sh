NODE_ADD=0
NODE_CHANGE=1
NODE_DEL=2

if [ "$qconf_event" != "$NODE_DEL" ]; then
    value=`./qconf get_conf $qconf_path`

# TODO: Nginx directory, You may change this to your own directory
    nginx=/usr/local/nginx
    nginx_conf_path=$nginx/conf/nginx.conf

    old_conf_value=`cat $nginx_conf_path`

    #if value is modified, then change the file
    if [ "$value" != "$old_conf_value" ]; then
        cp $nginx_conf_path ${nginx_conf_path}.bak
        echo "$value" > ${nginx_conf_path}.tmp
        mv ${nginx_conf_path}.tmp $nginx_conf_path

# TODO: Restart nginx, You may change this to your own command of nginx starting
        sudo /sbin/service nginx restart
    fi
fi

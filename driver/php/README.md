### The step to install php driver
- generate the qconf.so using the driver code sources, assume the path of php is '/usr/local/php'
change the directory to the driver source directory.
```
/path-to-php-install/bin/phpize
./configure --with-php-config=/path-to-php-install/bin/php-config \
            --with-libqconf-dir=/path-to-qconf-install/include \
            --enable-static LDFLAGS=/path-to-qconf-install/lib/libqconf.a
make
cp -v modules/qconf.so /path-to-php-extensions-dir/
```

- if php.ini doesn't include 'extension=qconf.so', then insert 'extension=qconf.so' into php.ini
- if current php has php-fpm service, then restart it `/path-to-php-install/sbin/php-fpm restart`

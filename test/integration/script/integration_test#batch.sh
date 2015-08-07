tmp_file="/tmp/qconf_test_script"

cat /dev/null > $tmp_file
echo "qconf_path = $qconf_path" >> $tmp_file 
echo "qconf_idc = $qconf_idc" >> $tmp_file 
time=`date +"%y-%m-%d %H:%M:%S"`
echo "qconf_time = $time" >> $tmp_file 
echo "qconf_type = $qconf_type" >> $tmp_file 

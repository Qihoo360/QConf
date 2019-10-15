<?php
// QConf批量获取节点500000个时，使用命令获取：qconf get_batch_keys
// 报错：     [ERROR]Failed to get batch keys! ret:8
// 因此编写脚本测试批量获取最大的节点数
// 最后跑完用了差不多14个小时，返回节点数：65536 。建议用二分法进行生成节点之后再读取。这样一边生成一遍读取太费时
$zoo_host = "192.168.0.1:2181,192.168.0.2:2181,192.168.0.3:2181";
$idc = "test";
$qzk = new QConfZK($zoo_host);

$zookeeper = new Zookeeper($zoo_host);
$aclArray = array(
  array(
    'perms'  => Zookeeper::PERM_ALL,
    'scheme' => 'world',
    'id'     => 'anyone'
  )
);
$arr_path="/mytest";
$p1=$zookeeper->exists($arr_path);
if(! $p1)
   $zookeeper->create($arr_path,null,$aclArray);

for($i=0;$i<=500000;$i++){
   $bkeys[$i]="bnode".($i+1);
   $zookeeper->create($arr_path."/conf".$i, $bkeys[$i],$aclArray);
$banchs=Qconf::getBatchKeys($arr_path);
if($banchs){
  var_dump($banchs);
}else{
  exit("返回节点数：".$i);
}
}

?>

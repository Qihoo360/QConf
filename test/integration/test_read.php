<?php
$zoo_host = "192.168.0.1:2181,192.168.0.2:2181,192.168.0.3:2181";
$hostname = "";
$idc = "test";
$waiting_time = 5;
$internal = false;
$prefix = ($internal) ? "/qconf" : "/";
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
 //  if($r){
     //echo 'SUCCESS';
   //  $s++;
  // }else{
     //echo 'ERR';
  //   $e++;
  // }
$banchs=Qconf::getBatchKeys($arr_path);
if($banchs){
  var_dump($banchs);
}else{
  exit("返回节点数：".$i);
}
}

?>

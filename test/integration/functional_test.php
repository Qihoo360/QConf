<?php
require_once (dirname(__FILE__)) . "/php_color.php";

// User Configuration
$zoo_host = "127.0.0.1:2181";
$hostname = "";
$idc = "test";
$waiting_time = 5;
$internal = false;

$qzk = new QConfZK($zoo_host);

$prefix = ($internal) ? "/qconf" : "/";

// Operation type
define ('OP_SERV_UP', 0);
define ('OP_SERV_DOWN', 1);
define ('OP_SERV_OFFLINE', 2);
define ('OP_SERV_DELETE', 3);
define ('OP_SERV_ADD', 4);
define ('OP_SERV_BEGIN', 5);
define ('OP_NODE_ADD', 6);
define ('OP_NODE_MODIFY', 7);
define ('OP_NODE_DELETE', 8);

// Statistical information
$success_num = 0;
$fail_num = 0;
$sc_succ_num = 0;
$sc_fail_num = 0;

// Service related
$service_path = "integration_test/services";
$skey = array("1.1.1.1:80", "1.1.1.2:80", "1.1.1.3:80", "1.1.1.4:80", "1.1.1.5:80", "1.1.1.6:80");
$history_ops = array();

// Node related
$config_path = "integration_test/config";

// Batch related
$batch_path = "integration_test/batch";
$history_nodes = array();
$bkeys = array("bnode1", "bnode2", "bnode3", "bnode4", "bnode5", "bnode6");

// Gray related
$gray_in_path = "integration_test/gray/gray_node"; // Node in gray process
$gray_in_val = "gray_in_val"; // Node in gray process
$gray_out_path = "integration_test/gray/nogray_node"; //Node not in gray process
$gray_out_val = "gray_out_val"; //Node not in gray process

// Script related
define ('SCRIPT_EXEC_RESULT_FILE', "/tmp/qconf_test_script");
define ('SCRIPT_PARAM_PATH', "qconf_path");
define ('SCRIPT_PARAM_IDC', "qconf_idc");
define ('SCRIPT_PARAM_TYPE', "qconf_type");
define ('SCRIPT_PARAM_TIME', "qconf_time");

// Feedback related
$check_feedback_url = "dev.qconf.qihoo.net/testfeature/checkFeedback";

/* {{{ Inner Function
 */

/*********************************
 * Script related fucntion
 *********************************/
function check_script_result($cur_path, $cur_idc, $cur_type, $cur_time_span)
{
    $result = parse_ini_file(SCRIPT_EXEC_RESULT_FILE);
    $result_path = $result[SCRIPT_PARAM_PATH];
    $result_type = $result[SCRIPT_PARAM_TYPE];
    $result_idc = $result[SCRIPT_PARAM_IDC];
    $result_time = $result[SCRIPT_PARAM_TIME];
    $time_span = ceil(abs(strtotime("now") - strtotime($result_time)));
    echo "$result_path vs $cur_path" . PHP_EOL;
    echo "$result_idc vs $cur_idc" . PHP_EOL;
    echo "$result_type vs $cur_type" . PHP_EOL;
    echo $time_span . PHP_EOL;
    if (trim($result_path, "/") === trim($cur_path, "/") && $result_idc == $cur_idc && $time_span <= $cur_time_span && $result_type == $cur_type)
    {
        return TRUE;
    }
    return FALSE;
}

function print_check_script($status, $err_line)
{
    global $sc_fail_num, $sc_succ_num;
    if ($status)
    {
        echo_color("check script result: SUCCESS", GREEN);
        $sc_succ_num++;
    }
    else
    {
        $time=date('Y-m-d H:i:s',time());
        echo_color("time: $time", RED);
        echo_color("check script result: FAILED, Failed line: ${err_line}", RED);
        $sc_fail_num++;
    }
}

/*********************************
 * Feedback related fucntion
 *********************************/
function curl_get($url, $args)
{
    $ch = curl_init();

    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $args);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

    $ret = curl_exec($ch);
    curl_close($ch);

    return $ret;
}

function feedback_compare($node_whole, $data_type)
{
    return TRUE;
    global $check_feedback_url, $idc, $hostname;
    $args = array("node_whole" => $node_whole, 
        "idc" => $idc,
        "data_type" => $data_type);
    $ret = curl_get($check_feedback_url, $args);
    if($ret === FALSE)
    {
        echo_color("failed to check feedback on $fd_url", RED);
        return FALSE;
    }

    $feedback_res = json_decode($ret, true);
    $errno = $feedback_res["errno"];
    if ($errno != 0)
    {
        echo_color("error happened when chech feedback, errno($errno), node_whole($node_whole), idc($idc))", RED);
        return FALSE;
    }
    echo_color("feedback success, node_whole($node_whole), idc($idc))", GREEN);
    return TRUE;
}

/*********************************
 * Service related fucntion
 *********************************/
function zoo_services_operation(&$ops, &$history_ops, &$right_hosts)
{
    global $qzk, $service_path, $prefix;

    $inner_service_path = "$prefix$service_path";
    $ret = -1;
    $right_host = array();
    foreach ($ops as $s => $current_s)
    {
        echo "current op : $s => $current_s" . PHP_EOL;
        $history_s = ($current_s === OP_SERV_ADD) ? OP_SERV_BEGIN :  $history_ops[$s];
       
        if ($history_s === $current_s) continue; //no need operation for current service

        switch($current_s)
        {
            case OP_SERV_UP:
                $ret = $qzk->serviceUP($inner_service_path, $s);
                $history_ops[$s] = $current_s;
                break;
            case OP_SERV_DOWN:
                $ret = $qzk->serviceDown($inner_service_path, $s);
                $history_ops[$s] = $current_s;
                break;
            case OP_SERV_OFFLINE:
                $ret = $qzk->serviceOffline($inner_service_path, $s);
                $history_ops[$s] = $current_s;
                break;
            case OP_SERV_ADD:
                $ret = $qzk->serviceAdd($inner_service_path, $s, OP_SERV_UP);
                $history_ops[$s] = OP_SERV_UP;
                break;
            case OP_SERV_DELETE:
                $ret = $qzk->serviceDelete($inner_service_path, $s);
                unset($history_ops[$s]);
                break;
            default:
                return FALSE;
        }
        if (0 != $ret) return FALSE;
    }

    foreach ($history_ops as $k => $v)
    {
        if (OP_SERV_UP === $v) $right_hosts[] = $k; //record which hosts should be get
    }
    $ops = array();

    return TRUE;
}

function print_info($status, $err_line)
{
    global $success_num, $fail_num;
    if ($status)
    {
        echo_color("result is: SUCCESS", GREEN);
        $success_num++;
    }
    else
    {
        $time=date('Y-m-d H:i:s',time());
        echo_color("time: $time", RED);
        echo_color("result is: FAILED, Failed line: ${err_line}", RED);
        $fail_num++;
    }
}

function service_operation(&$ops, &$history_ops, $line, $first_time=FALSE)
{
    global $service_path, $idc, $waiting_time;
    $right_hosts = array();
    echo_color("[==BEGIN===============================]", GREEN);
    assert(TRUE === zoo_services_operation($ops, $history_ops, $right_hosts));

    if ($first_time) 
        $host = QConf::getAllHost("$service_path");

    sleep($waiting_time);
    $host = QConf::getAllHost("$service_path");
    var_dump($host);
    var_dump($right_hosts);
    print_info(array_diff($right_hosts, $host) == array(), $line);
    print_check_script(TRUE === check_script_result($service_path, $idc, "3", $waiting_time + 2),
            $line); //check script execute
    feedback_compare($service_path, 3);//check feedback
    echo_color("[===============================END=====]", GREEN);
    echo PHP_EOL . PHP_EOL; 
}

/*********************************
 * Node config related fucntion
 *********************************/
function zoo_node_opeartion($op, $config_value)
{
    global $qzk, $config_path, $prefix;
    $inner_config_path = "$prefix$config_path";

    $ret = -1;
    switch ($op)
    {
    case OP_NODE_ADD:
    case OP_NODE_MODIFY:
        $ret = $qzk->nodeSet($inner_config_path, $config_value);  
        break;
    case OP_NODE_DELETE:
        $ret = $qzk->nodeDelete($inner_config_path);  
        break;
    default:
        return FALSE;
    }
    if (0 !== $ret)
        return FALSE;
    return TRUE;
}

function node_operation($op, $config_val, $line)
{
    global $config_path, $idc, $waiting_time;
    echo_color("[==BEGIN==============================]", GREEN);

    assert(TRUE === zoo_node_opeartion($op, $config_val));  

    if (OP_NODE_ADD === $op)
        $value = QConf::getConf("$config_path");
    
    sleep($waiting_time);
    $value = QConf::getConf("$config_path");
    
    var_dump($value);
    print_info($value == $config_val, $line);
    print_check_script(TRUE === check_script_result($config_path, $idc, "2", $waiting_time + 2),
        $line);//check script execute
    feedback_compare($config_path, 2);//check feedback
    echo_color("[===============================END==]", GREEN);
    echo PHP_EOL . PHP_EOL;
}

/*********************************
 * Batch related fucntion
 *********************************/
function zoo_batch_operation(&$nodes, &$ops, &$cur_batch_nodes)
{
    global $qzk, $batch_path, $prefix; //Record all the current children
    $inner_batch_path = "$prefix$batch_path";
  
    foreach ($nodes as $key => $val)
    {
        $ret = -1;
        switch($ops[$key])
        {
        case OP_NODE_ADD:
        case OP_NODE_MODIFY:
            $ret = $qzk->nodeSet("$inner_batch_path/$key", $val);
            $cur_batch_nodes[$key] = $val;
            break;
        case OP_NODE_DELETE:
            unset($cur_batch_nodes[$key]);
            $ret = $qzk->nodeDelete("$inner_batch_path/$key");
            break;
        default:
            return FALSE;
        }
        if (0 != $ret) return FALSE;
    }
    $nodes = $ops = array();
    return TRUE;
}

function batch_operation(&$nodes, &$ops, &$cur_batch_nodes, $line, $first_time=FALSE)
{
    global $batch_path, $idc, $waiting_time;
    echo_color("[==BEGIN==============================]", GREEN);

    assert(TRUE === zoo_batch_operation($nodes, $ops, $cur_batch_nodes));  

    if ($first_time)
        $children = QConf::getBatchConf($batch_path);
    
    sleep($waiting_time);
    $children = QConf::getBatchConf($batch_path);
    
    var_dump($children);
    print_info($children == $cur_batch_nodes, $line);
    
    $keys = QConf::getBatchKeys($batch_path);
    
    var_dump($keys);
    print_info($keys == array_keys($cur_batch_nodes), $line);
    //print_check_script(TRUE === check_script_result($batch_path, $idc, "4", $waiting_time + 2), $line);
    echo_color("[===============================END==]", GREEN);
    echo PHP_EOL . PHP_EOL;
}
/*}}}*/

/* {{{ One Service Operation
 */
/***************
 * clear all service
 ***************/
{
    echo_color("[-- clear all services: BEGIN --]", BROWN);
    assert(0 === $qzk->serviceAdd("$prefix$service_path", $skey[1], QCONF_STATUS_UP));
    sleep(10);
    assert(0 === $qzk->serviceClear("$prefix$service_path"));
    $histroy_ops = array();
    echo_color("[-- END --]", BROWN);
    echo PHP_EOL . PHP_EOL;
}
/**************
 **************/

/**************************************************************************************
 * one service: add one
 ***************************************************************************************/
{
    echo_color("one service, add one", CYAN);
    $ops[$skey[0]] = OP_SERV_ADD;
    service_operation($ops, $history_ops, __LINE__, TRUE);
}

/**************************************************************************************
 * one service: all down
 ***************************************************************************************/
{
    echo_color("one service, down one", CYAN);
    $ops[$skey[0]] = OP_SERV_DOWN;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * one service: all up
 ***************************************************************************************/
{
    echo_color("one service, up one", CYAN);
    $ops[$skey[0]] = OP_SERV_UP;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * one service: all offline
 ***************************************************************************************/
{
    echo_color("one service, offline one", CYAN);
    $ops[$skey[0]] = OP_SERV_OFFLINE;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * one service: all up
 ***************************************************************************************/
{
    echo_color("one service, up one", CYAN);
    $ops[$skey[0]] = OP_SERV_UP;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * one service: all delete
 ***************************************************************************************/
{
    echo_color("one service, delete one", CYAN);
    $ops[$skey[0]] = OP_SERV_DELETE;
    service_operation($ops, $history_ops, __LINE__);
}

/* }}}*/

/* {{{ Multi Services Operation
 */

/**************************************************************************************
 * set multi service 
 ***************************************************************************************/
{
    echo_color("multi service, add four", CYAN);
    for ($i = 0; $i < 4; ++$i)
    {
        $ops[$skey[$i]] = OP_SERV_ADD;
    }
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: one down
 ***************************************************************************************/
{
    echo_color("multi service, down all", CYAN);
    $ops[$skey[1]] = OP_SERV_DOWN;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: all up
 ***************************************************************************************/
{
    echo_color("multi service, up all", CYAN);
    foreach ($history_ops as $s => $op)
    {
        $ops[$s] = OP_SERV_UP;
    }
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: one offline
 ***************************************************************************************/
{
    echo_color("multi service, offline one", CYAN);
    $ops[$skey[1]] = OP_SERV_OFFLINE;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: all offline
 ***************************************************************************************/
{
    echo_color("multi service, offline all", CYAN);
    foreach ($history_ops as $s => $op)
    {
        $ops[$s] = OP_SERV_OFFLINE;
    }
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: all up
 ***************************************************************************************/
{
    echo_color("multi service, up all", CYAN);
    foreach ($history_ops as $s => $op)
    {
        $ops[$s] = OP_SERV_UP;
    }
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: one down
 ***************************************************************************************/
{
    echo_color("multi service, down one", CYAN);
    $ops[$skey[2]] = OP_SERV_DOWN;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: one down
 ***************************************************************************************/
{
    echo_color("multi service, down one", CYAN);
    $ops[$skey[3]] = OP_SERV_OFFLINE;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: all up
 ***************************************************************************************/
{
    echo_color("multi service, up all", CYAN);
    foreach ($history_ops as $s => $op)
    {
        $ops[$s] = OP_SERV_UP;
    }
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: two down
 ***************************************************************************************/
{
    echo_color("multi service, down two", CYAN);
    $ops[$skey[2]] = OP_SERV_DOWN;
    $ops[$skey[3]] = OP_SERV_DOWN;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: all up
 ***************************************************************************************/
{
    echo_color("multi service, up all", CYAN);
    foreach ($history_ops as $s => $op)
    {
        $ops[$s] = OP_SERV_UP;
    }
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: two offline
 ***************************************************************************************/
{
    echo_color("multi service, offline two", CYAN);
    $ops[$skey[1]] = OP_SERV_OFFLINE;
    $ops[$skey[3]] = OP_SERV_OFFLINE;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: two up
 ***************************************************************************************/
{
    echo_color("multi service, up two", CYAN);
    $ops[$skey[1]] = OP_SERV_UP;
    $ops[$skey[2]] = OP_SERV_UP;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: all down
 ***************************************************************************************/
{
    echo_color("multi service, down all", CYAN);
    foreach ($history_ops as $s => $op)
    {
        $ops[$s] = OP_SERV_DOWN;
    }
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: two up
 ***************************************************************************************/
{
    echo_color("multi service, up two", CYAN);
    $ops[$skey[1]] = OP_SERV_UP;
    $ops[$skey[2]] = OP_SERV_UP;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: all offline
 ***************************************************************************************/
{
    echo_color("multi service, offline all", CYAN);
    foreach ($history_ops as $s => $op)
    {
        $ops[$s] = OP_SERV_OFFLINE;
    }
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: one up
 ***************************************************************************************/
{
    echo_color("multi service, up one", CYAN);
    $ops[$skey[1]] = OP_SERV_UP;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: one delete
 ***************************************************************************************/
{
    echo_color("multi service, delete one", CYAN);
    $ops[$skey[1]] = OP_SERV_DELETE;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: two add up
 ***************************************************************************************/
{
    echo_color("multi service, add two", CYAN);
    $ops[$skey[1]] = OP_SERV_ADD;
    $ops[$skey[4]] = OP_SERV_ADD;
    service_operation($ops, $history_ops, __LINE__);
}


/**************************************************************************************
 * multi services: two delete
 ***************************************************************************************/
{
    echo_color("multi service, delete two", CYAN);
    $ops[$skey[1]] = OP_SERV_DELETE;
    $ops[$skey[3]] = OP_SERV_DELETE;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: one add up
 ***************************************************************************************/
{
    echo_color("multi service, add one", CYAN);
    $ops[$skey[3]] = OP_SERV_ADD;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: all delete
 ***************************************************************************************/
{
    echo_color("multi service, delete all", CYAN);
    foreach ($history_ops as $s => $op)
    {
        $ops[$s] = OP_SERV_DELETE;
    }
    service_operation($ops, $history_ops, __LINE__);
}


/**************************************************************************************
 * multi services: one add up
 ***************************************************************************************/
{
    echo_color("multi service, add one", CYAN);
    $ops[$skey[1]] = OP_SERV_ADD;
    service_operation($ops, $history_ops, __LINE__);
}

/**************************************************************************************
 * multi services: one offline one add
 ***************************************************************************************/
{
    echo_color("multi service, one offline one add", CYAN);
    $ops[$skey[1]] = OP_SERV_OFFLINE;
    $ops[$skey[2]] = OP_SERV_ADD;
}

/**************************************************************************************
 * multi services: two add up
 ***************************************************************************************/
{
    echo_color("multi service, add two", CYAN);
    $ops[$skey[3]] = OP_SERV_ADD;
    $ops[$skey[4]] = OP_SERV_ADD;
    service_operation($ops, $history_ops, __LINE__);
}

/* }}}*/

/*{{{ Gray Opertaion*/
/***************
 * register to gray node
 ***************/
{
    echo_color("[-- register to gray node: BEGIN --]", BROWN);
    assert(0 === $qzk->nodeSet("$prefix$gray_in_path", $gray_in_val));   
    assert(0 === $qzk->nodeSet("$prefix$gray_out_path", $gray_out_val));   
    QConf::getConf($gray_in_path);
    QConf::getConf($gray_out_path);
    echo_color("[-- register to gray node: END --]", BROWN);
}

/**************************************************************************************
 * current machine in gray process and commit
 ***************************************************************************************/
{
    echo_color("current machine in gray process and commit", CYAN);
    echo_color("[==BEGIN==============================]", GREEN);
    $machines = array($hostname);
    $nodes = array("$prefix$gray_in_path" => "value_in_new");
    $gray_id = $qzk->grayBegin($nodes, $machines);
    assert(NULL != $gray_id);
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == "value_in_new", __LINE__);
    print_check_script(TRUE === check_script_result($gray_in_path, $idc, "2", $waiting_time + 2),
            __line__); //check script execute
    
    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);

    //commit
    assert(0 === $qzk->grayCommit($gray_id));
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == "value_in_new", __LINE__);

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);
    echo_color("[==END==============================]", GREEN);
}

/***************
 * reset to gray node
 ***************/
{
    echo_color("[-- register to gray node: BEGIN --]", BROWN);
    assert(0 === $qzk->nodeSet("$prefix$gray_in_path", $gray_in_val));   
    assert(0 === $qzk->nodeSet("$prefix$gray_out_path", $gray_out_val));   
    echo_color("[-- register to gray node: END --]", BROWN);
}

/**************************************************************************************
 * current machine in gray process and rollback
 ***************************************************************************************/
{
    echo_color("current machine in gray process and rollback", CYAN);
    echo_color("[==BEGIN==============================]", GREEN);
    $machines = array($hostname);
    $nodes = array("$prefix$gray_in_path" => "value_in_new");
    $gray_id = $qzk->grayBegin($nodes, $machines);
    assert(NULL != $gray_id);
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == "value_in_new", __LINE__);
    print_check_script(TRUE === check_script_result($gray_in_path, $idc, "2", $waiting_time + 2),
            __line__); //check script execute

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);

    //rollback
    assert(0 === $qzk->grayRollback($gray_id));
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == $gray_in_val, __LINE__);
    print_check_script(TRUE === check_script_result($gray_in_path, $idc, "2", $waiting_time + 2),
            __line__); //check script execute

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);
    echo_color("[==END==============================]", GREEN);
}

/***************
 * reset to gray node
 ***************/
{
    echo_color("[-- register to gray node: BEGIN --]", BROWN);
    assert(0 === $qzk->nodeSet("$prefix$gray_in_path", $gray_in_val));   
    assert(0 === $qzk->nodeSet("$prefix$gray_out_path", $gray_out_val));   
    echo_color("[-- register to gray node: END --]", BROWN);
}

/**************************************************************************************
 * current machine not in gray process and commit
 ***************************************************************************************/
{
    echo_color("current machine not in gray process and commit", CYAN);
    echo_color("[==BEGIN==============================]", GREEN);
    $machines = array($hostname."1");
    $nodes = array("$prefix$gray_in_path" => "value_in_new");
    $gray_id = $qzk->grayBegin($nodes, $machines);
    assert(NULL != $gray_id);
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == $gray_in_val, __LINE__);

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);

    //commit
    assert(0 === $qzk->grayCommit($gray_id));
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == "value_in_new", __LINE__);
    print_check_script(TRUE === check_script_result($gray_in_path, $idc, "2", $waiting_time + 2),
            __line__); //check script execute

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);
    echo_color("[==END==============================]", GREEN);
}

/***************
 * reset to gray node
 ***************/
{
    echo_color("[-- register to gray node: BEGIN --]", BROWN);
    assert(0 === $qzk->nodeSet("$prefix$gray_in_path", $gray_in_val));   
    assert(0 === $qzk->nodeSet("$prefix$gray_out_path", $gray_out_val));   
    echo_color("[-- register to gray node: END --]", BROWN);
}

/**************************************************************************************
 * current machine not in gray process and rollback
 ***************************************************************************************/
{
    echo_color("current machine not in gray process and rollback", CYAN);
    echo_color("[==BEGIN==============================]", GREEN);
    $machines = array($hostname."1");
    $nodes = array("$prefix$gray_in_path" => "value_in_new");
    $gray_id = $qzk->grayBegin($nodes, $machines);
    assert(NULL != $gray_id);
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == $gray_in_val, __LINE__);

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);

    //rollback
    assert(0 === $qzk->grayRollback($gray_id));
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == $gray_in_val, __LINE__);

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);
    echo_color("[==END==============================]", GREEN);
}
/*}}}*/

/**************************************************************************************
 * current machine in gray process and commit immediately
 ***************************************************************************************/
{
    echo_color("current machine in gray process and commit immediately", CYAN);
    echo_color("[==BEGIN==============================]", GREEN);
    $machines = array($hostname);
    $nodes = array("$prefix$gray_in_path" => "value_in_new");
    $gray_id = $qzk->grayBegin($nodes, $machines);

    //commit
    assert(0 === $qzk->grayCommit($gray_id));
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == "value_in_new", __LINE__);

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);
    echo_color("[==END==============================]", GREEN);
}

/***************
 * reset to gray node
 ***************/
{
    echo_color("[-- register to gray node: BEGIN --]", BROWN);
    assert(0 === $qzk->nodeSet("$prefix$gray_in_path", $gray_in_val));   
    assert(0 === $qzk->nodeSet("$prefix$gray_out_path", $gray_out_val));   
    echo_color("[-- register to gray node: END --]", BROWN);
}

/**************************************************************************************
 * current machine in gray process and rollback immediately
 ***************************************************************************************/
{
    echo_color("current machine in gray process and rollback immediately", CYAN);
    echo_color("[==BEGIN==============================]", GREEN);
    $machines = array($hostname);
    $nodes = array("$prefix$gray_in_path" => "value_in_new");
    $gray_id = $qzk->grayBegin($nodes, $machines);
    assert(NULL != $gray_id);

    //rollback
    assert(0 === $qzk->grayRollback($gray_id));
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == $gray_in_val, __LINE__);
    print_check_script(TRUE === check_script_result($gray_in_path, $idc, "2", $waiting_time + 2),
            __line__); //check script execute

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);
    echo_color("[==END==============================]", GREEN);
}

/***************
 * reset to gray node
 ***************/
{
    echo_color("[-- register to gray node: BEGIN --]", BROWN);
    assert(0 === $qzk->nodeSet("$prefix$gray_in_path", $gray_in_val));   
    assert(0 === $qzk->nodeSet("$prefix$gray_out_path", $gray_out_val));   
    echo_color("[-- register to gray node: END --]", BROWN);
}

/**************************************************************************************
 * current machine not in gray process and commit immediately
 ***************************************************************************************/
{
    echo_color("current machine not in gray process and commit immediately", CYAN);
    echo_color("[==BEGIN==============================]", GREEN);
    $machines = array($hostname."1");
    $nodes = array("$prefix$gray_in_path" => "value_in_new");
    $gray_id = $qzk->grayBegin($nodes, $machines);
    assert(NULL != $gray_id);

    //commit
    assert(0 === $qzk->grayCommit($gray_id));
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == "value_in_new", __LINE__);
    print_check_script(TRUE === check_script_result($gray_in_path, $idc, "2", $waiting_time + 2),
            __line__); //check script execute

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);
    echo_color("[==END==============================]", GREEN);
}

/***************
 * reset to gray node
 ***************/
{
    echo_color("[-- register to gray node: BEGIN --]", BROWN);
    assert(0 === $qzk->nodeSet("$prefix$gray_in_path", $gray_in_val));   
    assert(0 === $qzk->nodeSet("$prefix$gray_out_path", $gray_out_val));   
    echo_color("[-- register to gray node: END --]", BROWN);
}

/**************************************************************************************
 * current machine not in gray process and rollback immediately
 ***************************************************************************************/
{
    echo_color("current machine not in gray process and rollback, immediately", CYAN);
    echo_color("[==BEGIN==============================]", GREEN);
    $machines = array($hostname."1");
    $nodes = array("$prefix$gray_in_path" => "value_in_new");
    $gray_id = $qzk->grayBegin($nodes, $machines);
    assert(NULL != $gray_id);

    //rollback
    assert(0 === $qzk->grayRollback($gray_id));
    sleep($waiting_time);

    //node in gray process
    $in_val = QConf::getConf($gray_in_path);
    var_dump($in_val);
    print_info($in_val == $gray_in_val, __LINE__);

    //node out of gray process
    $out_val = QConf::getConf($gray_out_path);
    var_dump($out_val);
    print_info($out_val == $gray_out_val, __LINE__);
    echo_color("[==END==============================]", GREEN);
}
/*}}}*/
/*{{{  Batch Config Operation
 */

/***************
 * clear the test batch configs 
 ***************/
{
    echo_color("[-- clear batch nodes childre: BEGIN --]", BROWN);
    assert(0 === $qzk->nodeSet("$prefix$batch_path", "batch node"));
    $children = $qzk->list("$prefix$batch_path");
    foreach ($children as $node)
    {
        assert(0 === $qzk->nodeDelete("$prefix$batch_path/$node"));
    }
    echo_color("[-- clear batch nodes childre: END --]", BROWN);
    echo PHP_EOL;
}
/**************
 **************/

/**************************************************************************************
 * 0 child node
 ***************************************************************************************/
{
    $nodes = $ops = array();
    echo_color("0 children node", CYAN);
    batch_operation($nodes, $ops, $history_nodes, __LINE__, TRUE);
}

/*************************************************************************************
 * 1 child node add
 ***************************************************************************************/
{
    echo_color("1 child add", CYAN);
    $nodes[$bkeys[0]] = "value1_new";
    $ops[$bkeys[0]] = OP_NODE_ADD;
    batch_operation($nodes, $ops, $history_nodes, __LINE__);
}

/**************************************************************************************
 * 1 child node change
 ***************************************************************************************/
{
    echo_color("1 child modify", CYAN);
    $nodes[$bkeys[0]] = "value1";
    $ops[$bkeys[0]] = OP_NODE_MODIFY;
    batch_operation($nodes, $ops, $history_nodes, __LINE__);
}

/**************************************************************************************
 * 4 children nodes
 ***************************************************************************************/
{
    echo_color("4 children", CYAN);
    $nodes[$bkeys[1]] = "value2";$ops[$bkeys[1]] = OP_NODE_ADD;
    $nodes[$bkeys[2]] = "value3";$ops[$bkeys[2]] = OP_NODE_ADD;
    $nodes[$bkeys[3]] = "value4";$ops[$bkeys[3]] = OP_NODE_ADD;
    batch_operation($nodes, $ops, $history_nodes, __LINE__);
}

/**************************************************************************************
 * 4 child node and 1 change
 ***************************************************************************************/
{
    echo_color("4 children node and 1 changed", CYAN);
    $nodes[$bkeys[2]] = "value3_new";$ops[$bkeys[2]] = OP_NODE_MODIFY;
    batch_operation($nodes, $ops, $history_nodes, __LINE__);
}

/**************************************************************************************
 * 4 child node and 2 change
 ***************************************************************************************/
{
    echo_color("4 children node and 2 changed", CYAN);
    $nodes[$bkeys[1]] = "value2_new";$ops[$bkeys[1]] = OP_NODE_MODIFY;
    $nodes[$bkeys[2]] = "value3_new1";$ops[$bkeys[2]] = OP_NODE_MODIFY;
    batch_operation($nodes, $ops, $history_nodes, __LINE__);
}

/**************************************************************************************
 * 4 child node and add 1 child
 ***************************************************************************************/
{
    echo_color("4 children node and add 1 child", CYAN);
    $nodes[$bkeys[4]] = "value5";$ops[$bkeys[4]] = OP_NODE_ADD;
    batch_operation($nodes, $ops, $history_nodes, __LINE__);
}

/**************************************************************************************
 * 5 child node and add 1 child
 ***************************************************************************************/
{
    echo_color("5 children node and add 1 child", CYAN);
    $nodes[$bkeys[5]] = "value6";$ops[$bkeys[5]] = OP_NODE_ADD;
    batch_operation($nodes, $ops, $history_nodes, __LINE__);
}

/**************************************************************************************
 * 6 child node and delete 1 child
 ***************************************************************************************/
{
    echo_color("6 children node and delete 1 child", CYAN);
    $nodes[$bkeys[4]] = "";$ops[$bkeys[4]] = OP_NODE_DELETE;
    batch_operation($nodes, $ops, $history_nodes, __LINE__);
}

/**************************************************************************************
 * 5 child node and delete 2 child
 ***************************************************************************************/
{
    echo_color("5 children node and delete 2 child", CYAN);
    $nodes[$bkeys[5]] = "";$ops[$bkeys[5]] = OP_NODE_DELETE;
    $nodes[$bkeys[0]] = "";$ops[$bkeys[0]] = OP_NODE_DELETE;
    batch_operation($nodes, $ops, $history_nodes, __LINE__);
}

/**************************************************************************************
 * 3 child node and delete 2 child
 ***************************************************************************************/
{
    echo_color("3 children node and delete 2 child", CYAN);
    $nodes[$bkeys[1]] = "";$ops[$bkeys[1]] = OP_NODE_DELETE;
    $nodes[$bkeys[2]] = "";$ops[$bkeys[2]] = OP_NODE_DELETE;
}
/*}}}*/

/*{{{ Node Config Operation
 */

/**************************************************************************************
 * node config: clear path
 ***************************************************************************************/
{
    echo_color("[-- clear the node: BEGIN --]", BROWN);
    assert(0 === $qzk->nodeSet("$prefix$config_path", "val"));
    echo_color("[-- END --]", BROWN);
    echo PHP_EOL . PHP_EOL;
}

/**************************************************************************************
 * node config: add node
 ***************************************************************************************/
{
    echo_color("node config: add node: ${config_path}", CYAN);
    node_operation(OP_NODE_ADD, "nodeval1", __LINE__);
}

/**************************************************************************************
 * node config: modify node
 ***************************************************************************************/
{
    echo_color("node config: modify node: ${config_path}", CYAN);
    node_operation(OP_NODE_MODIFY, "nodeval2", __LINE__);
}

/**************************************************************************************
 * node config: delete node
 ***************************************************************************************/
{
    echo_color("node config: delete node: ${config_path}", CYAN);
    node_operation(OP_NODE_DELETE, "", __LINE__);
}

/*}}}*/



/********************
 * summary
 *******************/
echo_color_summary($success_num + $fail_num, $success_num, $fail_num);
echo_color_summary($sc_succ_num + $sc_fail_num, $sc_succ_num, $sc_fail_num);



?>

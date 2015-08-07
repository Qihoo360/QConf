<?php

define ('RED', 31);
define ('GREEN', 32);
define ('BLUE', 34);
define ('CYAN', 36);
define ('PURPLE', 35);
define ('BROWN', 33);
define ('WHITE', 37);
define ('BOLD', 1);

function echo_color($string, $color)
{
    $cmd = "echo -ne \"\033[${color}m" . $string . "\033[0m\n\"";
    exec($cmd, $result, $ret);
    echo $result[0] . PHP_EOL;    
}

function echo_color_summary($total, $success_num, $fail_num)
{
    $cmd = "echo -ne \"\033[1;32mRESULT: ${total} TESTS, ${success_num} SUCCESS," .
        "\033[1;31m ${fail_num} FAILED" . 
        "\033[0m\n \"";
    exec($cmd, $result, $ret);
    echo $result[0] . PHP_EOL;
}
/*
$success = 10;
$fail = 5;
echo_color_summary($success + $fail, $success, $fail);

echo_color("my self", CYAN);
echo_color("my self", PURPLE);
echo_color("my self", BLUE);
echo_color("my self", GREEN);
 */
?>

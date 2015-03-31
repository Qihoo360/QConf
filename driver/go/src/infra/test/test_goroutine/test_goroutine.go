package main

import (
    "fmt"
    "time"
    "infra/go_qconf"
)

var c chan int

func doit(){
    key := "/demo/conf"
    idc := "corp"
    value, err_conf := go_qconf.GetConf(key, idc)
    if err_conf != nil{
        if err_conf == go_qconf.ErrQconfNotFound{
            fmt.Printf("Is ErrQconfNotFound\n")
        }
        fmt.Println(err_conf)
    } else {
        fmt.Printf("value of %v is %v\n", key, value)
    }

    host, err_host := go_qconf.GetHost(key, idc)
    if err_host != nil{
        fmt.Println(err_host)
    } else {
        fmt.Printf("one host of %v is %v\n", key, host)
    }

    hosts, err_hosts := go_qconf.GetAllHost(key, idc)
    if err_hosts != nil{
        fmt.Println(err_hosts)
    } else {
        for i := 0; i < len(hosts); i++ {
            cur := hosts[i]
            fmt.Println(cur)
        }
    }

    batch_conf, err_batch_conf := go_qconf.GetBatchConf(key, idc)
    if err_batch_conf != nil{
        fmt.Println(err_batch_conf)
    } else {
        fmt.Printf("%v\n", batch_conf)
    }

    batch_keys, err_batch_keys := go_qconf.GetBatchKeys(key, idc)
    if err_batch_keys != nil{
        fmt.Println(err_batch_keys)
    } else {
        fmt.Printf("%v\n", batch_keys)
    }

    
    version, err_version := go_qconf.Version()
    if err_version != nil{
        fmt.Println(err_version)
    } else {
        fmt.Printf("Version : %v\n", version)
    }
}

func doitwhile(times int){
    for k :=0;k <= times; k++{
        doit()
	time.Sleep(time.Millisecond *100 )
    }
    <-c
}

func main(){
    c = make(chan int)
    for i := 0; i < 10000; i++ {
	    go doitwhile(i * 10000)
    }
    for j := 0; j < 10000; j++ {
	    c <- 1
    }
    fmt.Printf("test goroutine finish\n")
}



package main

import (
    "fmt"
    "infra/go_qconf"
)

func main(){
    conf_key := "/demo/confs/conf"
    host_key := "/demo/hosts"
    batch_key := "/demo/confs"
    idc := "corp"

    value, err_conf := go_qconf.GetConf(conf_key, idc)
    if err_conf != nil{
        fmt.Println(err_conf)
    } else {
        fmt.Printf("value of %v is %v\n", conf_key, value)
    }

    host, err_host := go_qconf.GetHost(host_key, idc)
    if err_host != nil{
        fmt.Println(err_host)
    } else {
        fmt.Printf("one host of %v is %v\n", host_key, host)
    }

    hosts, err_hosts := go_qconf.GetAllHost(host_key, idc)
    if err_hosts != nil{
        fmt.Println(err_hosts)
    } else {
        for i := 0; i < len(hosts); i++ {
            cur := hosts[i]
            fmt.Println(cur)
        }
    }

    batch_conf, err_batch_conf := go_qconf.GetBatchConf(batch_key, idc)
    if err_batch_conf != nil{
        fmt.Println(err_batch_conf)
    } else {
        fmt.Printf("%v\n", batch_conf)
    }

    batch_keys, err_batch_keys := go_qconf.GetBatchKeys(batch_key, idc)
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



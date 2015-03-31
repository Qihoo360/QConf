package main

import (
    "time"
    "fmt"
    "infra/go_qconf"
)

const (
      	DEFAULT_VALUE = "1fyJnYW1lbGFuZCI6eyJ1cmwiOiJaodHRwOlwvXC9cLz8iLCJuYW1lIjoiMzYwXHU2ZTM4XHU2MjBmIiwibWQ1IjoiMDU0MzdjMDNmZmI2YTMyNTdhOGNmMGYxNDgzNDFmZTgiLCJwbGF0Zm9ybWlkIjoiMiJ9LCJ3ZWJnYW1lIjp7ImlkIjoiMTk5NyIsImxvZ28iOm51bGwsImljb24iOiIiLCJvaW1hZ2UiOm51bGwsInNpbWFnZSI6bnVsbCwiaW1nXzUwXzUwIjoiIiwiaW1nXzY0XzY0IjoiIiwic2ltcG5hbWUiOiJjdG9sIiwibmFtZSI6Ilx1OGQ2NFx1NTkyOSIsImNhdGUiOiIwIiwicGxhdGZvcm1pZCI6IjIiLCJpbnRybyI6IiJ9fQ==wibmFtZSI6Ilx1OGQ2NFx1NTkyOSIsImNhdGUiOiIwIiwicGxhdGZvcm1pZCI6IjIiLCJpbnRybyI6IiJ9fQ==1fyJnYW1lbGFuZCI6eyJ1cmwiOiJaodHRwOlwvXC9cLz8iLCJuYW1lIjoiMzYwXHU2ZTM4XHU2MjBmIiwibWQ1IjoiMDU0MzdjMDNmZmI2YTMyNTdhOGNmMGYxNDgzNDFmZTgiLCJwbGF0Zm9ybWlkIjoiMiJ9LCJ3ZWJnYW1lIjp7ImlkIjoiMTk5NyIsImxvZ28iOm51bGwsImljb24iOiIiLCJvaW1hZ2UiOm51bGwsInNpbWFnZSI6bnVsbCwiaW1nXzUwXzUwIjoiIiwiaW1nXzY0XzY0IjoiIiwic2ltcG5hbWUiOiJjdG9sIiwibmFtZSI6Ilx1OGQ2NFx1NTkyOSIsImNhdGUiOiIwIiwicGxhdGZvcm1pZCI6IjIiLCJpbnRybyI6IiJ9fQ==wibmFtZSI6Ilx1OGQ2NFx1NTkyOSIsImNhdGUiOiIwIiwicGxhdGZvcm1pZCI6IjIiLCJpbnRybyI6IiJ9fQ=="

)

func qconf_use(key string){
    fmt.Println(key)
    idc := "corp"
    value, err_conf := go_qconf.GetConf(key, "")
    if err_conf != nil{
        fmt.Println(err_conf)
    } else {
	if value != DEFAULT_VALUE{
	     fmt.Printf("Error value : %v", value)
	}
        //fmt.Printf("value of %v is %v\n", key, value)
    }

    host_key := "/demo/test_bcdppp"
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

func main(){
    key := "/test_million/million_"
    for index := 0; ; index++ {
	suffix := index % 100
	qconf_use(key + fmt.Sprint(suffix))
	time.Sleep(time.Millisecond *100 )
    }
}



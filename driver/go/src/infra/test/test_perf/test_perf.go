package main

import (
    "fmt"
    "time"
    "infra/go_qconf"
)

func main(){
    m := map[string]int{
        "/demo/test_my/test/test_1": 100000,
        "/demo/test_my/test/test_100": 50000,
        "/demo/test_my/test/test_1k": 5000,
        "/demo/test_my/test/test_10k": 100,
        "/demo/test_my/test/test_100k": 10,
    }
    start := time.Now()

    for key, times := range m{
        for i := 0; i < times; i++{
            //fmt.Printf("%v : %v\n", key, i)
            value, err_conf := go_qconf.GetConf(key, "")
            if err_conf != nil{
                fmt.Println(err_conf)
                value = "ERROR"
                fmt.Println(value)
            }
            //fmt.Println("")
        }
    }
    end := time.Now()
    fmt.Println(end.Sub(start))
}



package main

import (
	"fmt"
	"net/http"
	_ "net/http/pprof"
	"time"

	"github.com/shiaho/get-rss"
	"github.com/shiaho/go-v8"
)

func MainFunc(v8ctxs []*v8.V8Context) {
	// v8ctx.Eval("console.log(testa());")
	for _, ctx := range v8ctxs {
		ctx.CallFunc("testa")
		ctx.CallFunc("testa")
		ctx.CallFunc("testa")
		ctx.CallFunc("testa")
		ctx.CallFunc("testa")
	}

	// _ = res
	// fmt.Println(res)
}

func main() {
	go func() {
		http.ListenAndServe("localhost:6060", nil)
	}()
	fmt.Println(time.Now(), gs.StrRss())

	// setup console.log()
	// v8ctx.Eval(`
	// this.console = { "log": function(args) { _console_log.apply(null, arguments) }}`)
	// v8ctx.AddFunc("_console_log", func(args ...interface{}) (interface{}, error) {
	// 	fmt.Printf("Go console log: ")
	// 	for i := 0; i < len(args); i++ {
	// 		fmt.Printf("%v ", args[i])
	// 	}
	// 	fmt.Println()
	// 	return "", nil
	// })
	sss := `
	var a = 0;
	var testa = function(){
		a ++;
		return a;
	} `
	v8ctxs := make([]*v8.V8Context, 0)
	for i := 0; i < 20; i++ {
		v8ctx := v8.NewContext()
		v8ctx.Eval(sss)
		v8ctxs = append(v8ctxs, v8ctx)
	}
	fmt.Println(time.Now(), gs.StrRss())
	time.Sleep(100 * time.Millisecond)
	for i := 1; i <= 10000; i++ {
		MainFunc(v8ctxs)
		// v8ctx.Eval("console.log()")
		// if i%10 == 0 {
		fmt.Println(time.Now(), gs.StrRss())
		// }
		time.Sleep(1000 * time.Millisecond)
	}
}

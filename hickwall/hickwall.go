package main

func GetNewContext() *v8.V8Context {
	v8ctx := v8.NewContext()
	v8ctx.Eval(`this.console = { "log": function(args) { _console_log.apply(null, arguments) }}`)
	v8ctx.AddFunc("_console_log", func(args ...interface{}) (interface{}, error) {
		for i := 0; i < len(args); i++ {
			fmt.Printf("%v ", args[i])
		}
		fmt.Println()
		return "", nil
	})
	v8ctx.Eval(`
function Collecter(){
	this._resources = [];
}

Collecter.prototype.register_resource = function(type, option) {
    this._resources.push({
        "type": type,
        "option": option
    });
}


Collecter.prototype.get_resources = function(){
	return this._resources;
}

Collecter.prototype.register_collect = function(func){
	this._collect_func = func;
}


Collecter.prototype.run_collect = function(res){
	return this._collect_func(res);
}


function CollecterManager(){
	this.count = 0;
	this.collecters = {};
}

CollecterManager.prototype.add_collecter = function(id, script, config) {
	var collecter = this.collecters[id] = new Collecter();
	this.count++;
	var configFunc = new Function("collecter", "config", script);
	configFunc(collecter, config);
};

CollecterManager.prototype.get_resources = function(id) {
	return this.collecters[id].get_resources();
};

CollecterManager.prototype.run_collect = function(id, res) {
	return this.collecters[id].run_collect(res);
};

var collecterManager = new CollecterManager();

function CollecterAdd(id, script, config){ // api
	collecterManager.add_collecter(id, script, config);
}

function CollecterGetConfig(id){ // api
	return collecterManager.get_resources(id);
}

function CollectOnce(id, res){ // api
	return collecterManager.run_collect(id, res);
}
		`)
	// v8ctx.AddFunc("readFile", func(args ...interface{}) (interface{}, error) {
	// 	fmt.Println("readFile")
	// 	fileName := args[0].(string)
	// 	fmt.Printf("filename: %s.\n", fileName)
	// 	filedata, err := ioutil.ReadFile(fileName)
	// 	if err != nil {
	// 		fmt.Printf("error: %v", err)
	// 		return "", err
	// 	}
	// 	result := string(filedata)
	// 	return result, nil
	// })

	// v8ctx.AddFunc("exec", func(args ...interface{}) (interface{}, error) {
	// 	cmd := args[0].(string)
	// 	arr := strings.Split(cmd, " ")
	// 	name := arr[0]
	// 	argv := arr[1:]
	// 	c := exec.Command(name, argv...)
	// 	d, err := c.Output()
	// 	result := string(d)
	// 	return result, err
	// })

	// v8ctx.AddFunc("httpGet", func(args ...interface{}) (interface{}, error) {
	// 	url := args[0].(string)
	// 	fmt.Println("URL:>", url)
	// 	resp, err := http.Get(url)
	// 	if err != nil {
	// 		fmt.Println(err)
	// 	}
	// 	fmt.Println("response Status:", resp.Status)
	// 	fmt.Println("response Headers:", resp.Header)
	// 	body, err := ioutil.ReadAll(resp.Body)
	// 	return string(body), err
	// })
	// v8ctx.AddFunc("httpPost", func(args ...interface{}) (interface{}, error) {
	// 	url := args[0].(string)
	// 	data := args[1].(string)
	// 	fmt.Println("URL:>", url)
	// 	fmt.Println("data:>", data)
	// 	var query = []byte(data)
	// 	req, err := http.NewRequest("POST", url, bytes.NewBuffer(query))
	// 	// req.Header.Set("X-Custom-Header", "myvalue")
	// 	req.Header.Set("Content-Type", "application/json")

	// 	client := &http.Client{}
	// 	resp, err := client.Do(req)
	// 	if err != nil {
	// 		fmt.Println(err)
	// 	}
	// 	defer resp.Body.Close()

	// 	fmt.Println("response Status:", resp.Status)
	// 	fmt.Println("response Headers:", resp.Header)
	// 	body, _ := ioutil.ReadAll(resp.Body)
	// 	// fmt.Println("response Body:", string(body))
	// 	return string(body), err
	// })
	return v8ctx
}

func main() {

}

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

/**  TEST  **/
// CollecterAdd(1, 'console.log(collecter, config); collecter.register_resource("http",{"url": "http://www.baidu.com"}); collecter.register_collect(function(res){console.dir(res); return [config.HOST+111, res[1].a];});', {'HOST': 'SSDDS'});
// CollecterAdd(2, 'console.log(collecter, config); collecter.register_resource("http",{"url": "http://www.ssss.com"}); collecter.register_collect(function(res){console.dir(res); return [config.HOST+0000, res[1].a];});', {'HOST': 'SSDDS'});
// console.log(CollecterGetConfig(1));
// console.log(CollecterGetConfig(2));
// console.log(CollectOnce(1, ['sds', {a:12}]));
// console.log(CollectOnce(2, ['sds', {a:2}]));
// console.log(CollectOnce(1, ['sds', {a:3}]));
// console.log(CollectOnce(2, ['sds', {a:4}]));


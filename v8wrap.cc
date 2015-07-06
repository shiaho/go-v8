#include <v8.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include "v8wrap.h"

extern "C" {

char*
__strdup(const char* ptr) {
  int l = strlen(ptr);
  char* p = new char[l + 1];
  strcpy(p, ptr);
  return p;
}

static volatile v8wrap_callback ___go_v8_callback = NULL;

static std::string
to_json(v8::Handle<v8::Value> value) {
  v8::HandleScope scope(v8::Isolate::GetCurrent());
  v8::TryCatch try_catch;
  v8::Handle<v8::Object> json = v8::Handle<v8::Object>::Cast(
    v8::Context::GetCurrent()->Global()->Get(v8::String::New("JSON")));
  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(
    json->GetRealNamedProperty(v8::String::New("stringify")));
  v8::Handle<v8::Value> args[1];
  args[0] = value;
  v8::String::Utf8Value ret(
    func->Call(v8::Context::GetCurrent()->Global(), 1, args)->ToString());
  return (char*) *ret;
}

v8::Handle<v8::Value>
from_json(std::string str) {
  v8::HandleScope scope(v8::Isolate::GetCurrent());
  v8::TryCatch try_catch;

  v8::Handle<v8::Object> json = v8::Handle<v8::Object>::Cast(
    v8::Context::GetCurrent()->Global()->Get(v8::String::New("JSON")));
  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(
    json->GetRealNamedProperty(v8::String::New("parse")));
  v8::Handle<v8::Value> args[1];
  args[0] = v8::String::New(str.c_str());
  return func->Call(v8::Context::GetCurrent()->Global(), 1, args);
}

v8data
v8_get_array_item(v8data* array, int index) {
  return array[index];
}

void
_go_call(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Locker v8Locker;
  uint32_t id = args[0]->ToUint32()->Value();
  v8::String::Utf8Value name(args[1]);

  // Parse arguments
  v8::Array* realArgs = v8::Array::Cast(*args[2]);
  v8data* data = (v8data*) malloc(sizeof(v8data) * realArgs->Length());

  for (int i = 0; i < realArgs->Length(); i++) {
    v8::Local<v8::Value> arg = realArgs->Get(i);

    v8::String::Utf8Value argString(arg);
    if (arg->IsRegExp()) {
      data[i].obj_type = v8regexp;
      data[i].repr = __strdup(*argString);
    } else if (arg->IsFunction()) {
      data[i].obj_type = v8function;
      data[i].repr = __strdup(*argString);
    } else if (arg->IsNumber()) {
      data[i].obj_type = v8number;
      data[i].repr = __strdup(*argString);
    } else if (arg->IsBoolean()) {
      data[i].obj_type = v8boolean;
      data[i].repr = __strdup(*argString);
    } else if (arg->IsString()) {
      data[i].obj_type = v8string;
      data[i].repr = __strdup(*argString);
    } else if (arg->IsArray()) {
      data[i].obj_type = v8array;
      data[i].repr = __strdup(*argString);
    } else if (arg->IsObject()) {
      data[i].obj_type = v8object;
      data[i].repr = __strdup(*argString);
    } else {
      data[i].obj_type = v8string;
      data[i].repr = __strdup(to_json(arg).c_str());
    }
  }

  v8::TryCatch try_catch;
  char* retv;
  retv = ___go_v8_callback(id, *name, data, realArgs->Length());

  // Free args memory
  for (int i = 0; i < realArgs->Length(); i++) {
      free(data[i].repr);
  }

  free(data);

  if (retv != NULL) {
    v8::Handle<v8::Value> ret = from_json(retv);
    free(retv);
    args.GetReturnValue().Set(ret);
    return;
  }
  args.GetReturnValue().Set(v8::Undefined());
}

class V8Context {
public:
  V8Context() : err_("") {
    v8::Locker v8Locker;
    
    v8::ResourceConstraints rc;
    rc.set_max_young_space_size(2048); //KB
    rc.set_max_old_space_size(10); //MB
    rc.set_max_executable_size(10); //MB
    rc.set_stack_limit(reinterpret_cast<uint32_t*>((char*)&rc- 1024 * 400));
    v8::SetResourceConstraints(&rc);

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);
    v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
    global->Set(v8::String::New("_go_call"),
      v8::FunctionTemplate::New(_go_call));
    
    v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global);
    context_.Reset(isolate, context);
  
  };

  virtual ~V8Context() {
    printf("~V8Context\n");
    context_.Dispose();
  };
  v8::Handle<v8::Context> context() { return v8::Handle<v8::Context>::New(v8::Isolate::GetCurrent(), context_); };
  const char* err() const { return err_.c_str(); };
  void err(const char* e) { this->err_ = std::string(e); }

private:
  v8::Persistent<v8::Context> context_;
  std::string err_;
};

void
v8_init(void *p) {
  ___go_v8_callback = (v8wrap_callback) p;
}

void*
v8_create() {
  return (void*) new V8Context(); 
}

void
v8_release(void* ctx) {
  delete static_cast<V8Context *>(ctx);
}

char*
v8_error(void* ctx) {
  V8Context *context = static_cast<V8Context *>(ctx);
  return __strdup(context->err());
}

const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

static std::string
report_exception(v8::TryCatch& try_catch) {
  v8::HandleScope scope(v8::Isolate::GetCurrent());
  v8::String::Utf8Value exception(try_catch.Exception());
  const char* exception_string = ToCString(exception);
  v8::Handle<v8::Message> message = try_catch.Message();
  std::stringstream ss;
  if (message.IsEmpty()) {
    ss << *exception << std::endl;
  } else {
    v8::String::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = *filename;
    int linenum = message->GetLineNumber();
    // printf("filename:%s, linenum:%d, exception_string: %s\n", filename_string, linenum, exception_string);
    ss
      << filename_string
      << ":" << linenum
      << ": " << exception_string << std::endl;
    delete filename_string;
    v8::String::Utf8Value sourceline(message->GetSourceLine());
    // const char* sourceline_string = ToCString(sourceline);
    // printf("sourceline_string: %s\n", sourceline_string);
    // delete sourceline_string;
    ss << *sourceline << std::endl;
    int start = message->GetStartColumn();
    for (int n = 0; n < start; n++) {
      ss << " ";
    }
    // printf("start: %d\n", start);
    int end = message->GetEndColumn();
    // printf("end: %d\n", end);
    
    for (int n = start; n < end; n++) {
      ss << "^";
    }
    ss << std::endl;
    v8::String::Utf8Value stack_trace(try_catch.StackTrace());
    // printf("stack_trace.length(): %d\n", stack_trace.length());
    if (stack_trace.length() > 0) {
      const char* stack_trace_string = *stack_trace;
      ss << stack_trace_string << std::endl;
      delete stack_trace_string;
      // printf("%s\n", stack_trace_string);
    }
  }
  delete exception_string;
  return ss.str();
}

char*
v8_execute(void *ctx, char* source) {
  v8::Locker v8Locker;
  V8Context *context = static_cast<V8Context *>(ctx);
  v8::HandleScope scope(v8::Isolate::GetCurrent());
  v8::TryCatch try_catch;

  v8::Context::Scope context_scope(context->context());

  context->err("");
  v8::Handle<v8::Script> script
    = v8::Script::Compile(v8::String::New(source), v8::Undefined());
  if (script.IsEmpty()) {
    v8::ThrowException(try_catch.Exception());
    context->err(report_exception(try_catch).c_str());
    return NULL;
  } else {
    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty()) {
      v8::ThrowException(try_catch.Exception());
      context->err(report_exception(try_catch).c_str());
      return NULL;
    } else if (result->IsUndefined()) {
      return __strdup("");
    } else if (result->IsFunction()) {
      v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(result);
      v8::String::Utf8Value ret(func->ToString());
      return __strdup(*ret);
    } else if (result->IsRegExp()) {
      v8::Handle<v8::RegExp> re = v8::Handle<v8::RegExp>::Cast(result);
      v8::String::Utf8Value ret(re->ToString());
      return __strdup(*ret);
    } else {
      return __strdup(to_json(result).c_str());
    }
  }
}

char*
v8_callfunc(void *ctx, char* func_name, int cid, char* res1, char* res2, char* res3){
  v8::Locker v8Locker;
  V8Context *context = static_cast<V8Context *>(ctx);
  v8::HandleScope scope(v8::Isolate::GetCurrent());
  v8::TryCatch try_catch;

  v8::Context::Scope context_scope(context->context());
  context->err("");
  v8::Handle<v8::Object>globalObj = context->context()->Global();
  //获取Javascrip全局变量
  v8::Handle<v8::Value>value = globalObj->Get(v8::String::New(func_name));
  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(value);
  // v8::Handle<v8::Value> resv[] = {v8::String::New(res1), v8::String::New(res2), v8::String::New(res3)}; 
  v8::Handle<v8::Array> res = v8::Array::New(3);
  res->Set(0, v8::String::New(res1));
  res->Set(1, v8::String::New(res2));
  res->Set(2, v8::String::New(res3));
  v8::Handle<v8::Value> args[2] = {v8::Integer::New(cid), res};
  v8::Handle<v8::Value> result = func->Call(globalObj, 2, args);
  // return __strdup(""); 
  if (result.IsEmpty()) {
      v8::ThrowException(try_catch.Exception());
      context->err(report_exception(try_catch).c_str());
      return NULL;
    } else if (result->IsUndefined()) {
      return __strdup("");
    } else if (result->IsFunction()) {
      v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(result);
      v8::String::Utf8Value ret(func->ToString());
      return __strdup(*ret);
    } else if (result->IsRegExp()) {
      v8::Handle<v8::RegExp> re = v8::Handle<v8::RegExp>::Cast(result);
      v8::String::Utf8Value ret(re->ToString());
      return __strdup(*ret);
    } else {
      return __strdup(to_json(result).c_str());
    }
}

  
}

// vim:set et sw=2 ts=2 ai:

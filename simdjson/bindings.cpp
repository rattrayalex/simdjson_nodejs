#ifdef __AVX2__
#include "bindings.h"

Napi::Boolean simdjsonnode::HasAVX2Wrapped(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, true);
}

bool simdjsonnode::isValid(std::string_view p) {
  ParsedJson pj = build_parsed_json(p);
  return pj.isValid();
}

Napi::Boolean simdjsonnode::IsValidWrapped(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string jstr = info[0].As<Napi::String>();
  Napi::Boolean returnValue = Napi::Boolean::New(env, simdjsonnode::isValid(jstr));
  return returnValue;
}

Napi::Object simdjsonnode::parse(Napi::Env env, std::string_view p) {
  ParsedJson pj = build_parsed_json(p);
  if (!pj.isValid()) {
    Napi::Error::New(env, "Invalid JSON Exception").ThrowAsJavaScriptException();
  }
  ParsedJson::iterator pjh(pj);
  return simdjsonnode::makeJSONObject(env, pjh).As<Napi::Object>();
}

Napi::Value simdjsonnode::makeJSONObject(Napi::Env env, ParsedJson::iterator & pjh) {
  Napi::Value v;
  if (pjh.is_object()) {
    Napi::Object obj = Napi::Object::New(env); // {
    if (pjh.down()) {
      // must be a string
      Napi::String key = Napi::String::New(env, pjh.get_string());
      // :
      pjh.next();
      Napi::Value value = simdjsonnode::makeJSONObject(env, pjh); // let us recurse
      obj.Set(key, value);
      while (pjh.next()) { // ,
        key = Napi::String::New(env, pjh.get_string());
        pjh.next();
        // :
        value = simdjsonnode::makeJSONObject(env, pjh); // let us recurse
        obj.Set(key, value);
      }
      pjh.up();
    }
    v = obj; // }
  } else if (pjh.is_array()) {
    std::vector<Napi::Value> arr;
    if (pjh.down()) {
      // [
      Napi::Value value = simdjsonnode::makeJSONObject(env, pjh); // let us recurse
      arr.push_back(value);
      while (pjh.next()) { // ,
        value = simdjsonnode::makeJSONObject(env, pjh); // let us recurse
        arr.push_back(value);
      }
      pjh.up();
    }
    // ]
    Napi::Array array = Napi::Array::New(env, arr.size());
    for (std::size_t i{ 0 }; i < arr.size(); i++) array.Set(i, arr[i]);
    v = array;
  } else if (pjh.is_string()) {
    v = Napi::String::New(env, pjh.get_string());
  } else if (pjh.is_double()) {
    v = Napi::Number::New(env, pjh.get_double());
  } else if (pjh.is_integer()) {
    v = Napi::Number::New(env, pjh.get_integer());
  } else {
    switch (pjh.get_type()) {
      case 't':  {
        v = Napi::Boolean::New(env, true);
        break;
      }
      case 'f': {
        v = Napi::Boolean::New(env, false);
        break;
      }
      case 'n': {
        v = env.Null();
        break;
      }
      default : break;
    }
  }
  
  return v;
}

Napi::Value simdjsonnode::ValueForKeyPathWrapped(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string path = info[0].As<Napi::String>();
  return Napi::String::New(env, path);;
}

Napi::Object simdjsonnode::ParseWrapped(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string jstr = info[0].As<Napi::String>();
  Napi::Object json = simdjsonnode::parse(env, jstr);
  json.Set("valueForKeyPath", Napi::Function::New(env, simdjsonnode::ValueForKeyPathWrapped));
  return json;
}

Napi::Object simdjsonnode::Init(Napi::Env env, Napi::Object exports) {
  exports.Set("hasAVX2", Napi::Function::New(env, simdjsonnode::HasAVX2Wrapped));
  exports.Set("isValid", Napi::Function::New(env, simdjsonnode::IsValidWrapped));
  exports.Set("parse", Napi::Function::New(env, simdjsonnode::ParseWrapped));
  return exports;
}

#endif
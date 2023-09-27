#include "extism.hpp"

namespace extism {
static void functionCallback(ExtismCurrentPlugin *plugin,
                             const ExtismVal *inputs, ExtismSize n_inputs,
                             ExtismVal *outputs, ExtismSize n_outputs,
                             void *user_data) {
  UserData *data = (UserData *)user_data;
  data->func(CurrentPlugin(plugin, inputs, n_inputs, outputs, n_outputs),
             data->userData);
}

static void freeUserData(void *user_data) {
  UserData *data = (UserData *)user_data;
  if (data->userData != nullptr && data->freeUserData != nullptr) {
    data->freeUserData(data->userData);
  }
}

Function::Function(std::string name, const std::vector<ValType> inputs,
                   const std::vector<ValType> outputs, FunctionType f,
                   void *userData, std::function<void(void *)> free)
    : name(name) {
  this->userData.func = f;
  this->userData.userData = userData;
  this->userData.freeUserData = free;
  auto ptr = extism_function_new(
      this->name.c_str(), inputs.data(), inputs.size(), outputs.data(),
      outputs.size(), functionCallback, &this->userData, freeUserData);
  this->func = std::shared_ptr<ExtismFunction>(ptr, extism_function_free);
}

void Function::setNamespace(std::string s) {
  extism_function_set_namespace(this->func.get(), s.c_str());
}

Function::Function(const Function &f) { this->func = f.func; }

ExtismFunction *Function::get() { return this->func.get(); }

}; // namespace extism

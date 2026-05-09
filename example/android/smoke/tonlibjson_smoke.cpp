#include <dlfcn.h>
#include <stdio.h>

#include <string>

using tonlib_client_json_create_t = void *(*)();
using tonlib_client_set_verbosity_level_t = void (*)(int verbosity_level);
using tonlib_client_json_destroy_t = void (*)(void *client);

template <class T>
T load_symbol(void *handle, const char *symbol_name) {
  dlerror();
  auto *symbol = reinterpret_cast<T>(dlsym(handle, symbol_name));
  const char *err = dlerror();
  if (err != nullptr || symbol == nullptr) {
    fprintf(stderr, "Failed to resolve symbol '%s': %s\n", symbol_name, err != nullptr ? err : "unknown dlsym error");
    return nullptr;
  }
  return symbol;
}

int main(int argc, char **argv) {
  const char *lib_dir = argc > 1 ? argv[1] : ".";

  std::string tonlibjson_path = std::string(lib_dir) + "/libtonlibjson.so";
  std::string native_lib_path = std::string(lib_dir) + "/libnative-lib.so";

  dlerror();
  void *tonlibjson = dlopen(tonlibjson_path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (tonlibjson == nullptr) {
    fprintf(stderr, "Failed to load %s: %s\n", tonlibjson_path.c_str(), dlerror());
    return 1;
  }

  auto tonlib_client_json_create = load_symbol<tonlib_client_json_create_t>(tonlibjson, "tonlib_client_json_create");
  auto tonlib_client_set_verbosity_level =
      load_symbol<tonlib_client_set_verbosity_level_t>(tonlibjson, "tonlib_client_set_verbosity_level");
  auto tonlib_client_json_destroy = load_symbol<tonlib_client_json_destroy_t>(tonlibjson, "tonlib_client_json_destroy");

  if (tonlib_client_json_create == nullptr || tonlib_client_set_verbosity_level == nullptr ||
      tonlib_client_json_destroy == nullptr) {
    dlclose(tonlibjson);
    return 2;
  }

  tonlib_client_set_verbosity_level(0);
  void *client = tonlib_client_json_create();
  if (client == nullptr) {
    fprintf(stderr, "tonlib_client_json_create returned nullptr\n");
    dlclose(tonlibjson);
    return 3;
  }
  tonlib_client_json_destroy(client);

  dlerror();
  void *native_lib = dlopen(native_lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (native_lib == nullptr) {
    fprintf(stderr, "Failed to load %s: %s\n", native_lib_path.c_str(), dlerror());
    dlclose(tonlibjson);
    return 4;
  }

  dlclose(native_lib);
  dlclose(tonlibjson);

  fprintf(stdout, "OK: loaded libtonlibjson.so + libnative-lib.so and executed tonlib_client_json_create / "
                  "tonlib_client_set_verbosity_level\n");
  return 0;
}

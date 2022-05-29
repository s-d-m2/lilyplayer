#ifndef LOAD_FILE_FROM_WASM_HH
#define LOAD_FILE_FROM_WASM_HH

#include <string_view>
#include <functional>

void load_from_wasm(const char* const accept, std::function<void(const std::string_view /* content */)> fileDataReady);

#endif

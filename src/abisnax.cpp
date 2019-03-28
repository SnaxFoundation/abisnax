// copyright defined in abisnax/LICENSE.txt

#include "abisnax.h"
#include "abisnax.hpp"

#include <memory>

inline const bool catch_all = true;

using namespace abisnax;

struct abisnax_context_s {
    const char* last_error = "";
    std::string last_error_buffer{};
    std::string result_str{};
    std::vector<char> result_bin{};

    std::map<name, contract> contracts{};
};

void fix_null_str(const char*& s) {
    if (!s)
        s = "";
}

bool set_error(abisnax_context* context, std::string error) noexcept {
    context->last_error_buffer = std::move(error);
    context->last_error = context->last_error_buffer.c_str();
    return false;
}

template <typename T, typename F>
auto handle_exceptions(abisnax_context* context, T errval, F f) noexcept -> decltype(f()) {
    if (!context)
        return errval;
    try {
        return f();
    } catch (std::exception& e) {
        if (!catch_all)
            throw;
        set_error(context, e.what());
        return errval;
    } catch (...) {
        if (!catch_all)
            throw;
        set_error(context, "unknown exception");
        return errval;
    }
}

extern "C" abisnax_context* abisnax_create() {
    try {
        return new abisnax_context{};
    } catch (...) {
        if (!catch_all)
            throw;
        return nullptr;
    }
}

extern "C" void abisnax_destroy(abisnax_context* context) { delete context; }

extern "C" const char* abisnax_get_error(abisnax_context* context) {
    if (!context)
        return "context is null";
    return context->last_error;
}

extern "C" int abisnax_get_bin_size(abisnax_context* context) {
    if (!context)
        return 0;
    return context->result_bin.size();
}

extern "C" const char* abisnax_get_bin_data(abisnax_context* context) {
    if (!context)
        return nullptr;
    return context->result_bin.data();
}

extern "C" const char* abisnax_get_bin_hex(abisnax_context* context) {
    return handle_exceptions(context, nullptr, [&] {
        context->result_str.clear();
        hex(context->result_bin.begin(), context->result_bin.end(), std::back_inserter(context->result_str));
        return context->result_str.c_str();
    });
}

extern "C" uint64_t abisnax_string_to_name(abisnax_context* context, const char* str) {
    fix_null_str(str);
    return string_to_name(str);
}

extern "C" const char* abisnax_name_to_string(abisnax_context* context, uint64_t name) {
    return handle_exceptions(context, nullptr, [&] {
        context->result_str = name_to_string(name);
        return context->result_str.c_str();
    });
}

extern "C" abisnax_bool abisnax_set_abi(abisnax_context* context, uint64_t contract, const char* abi) {
    fix_null_str(abi);
    return handle_exceptions(context, false, [&]() {
        context->last_error = "abi parse error";
        abi_def def{};
        std::string error;
        if (!json_to_native(def, error, abi)) {
            if (!error.empty())
                set_error(context, std::move(error));
            return false;
        }
        if (!check_abi_version(def.version, error))
            return set_error(context, std::move(error));
        abisnax::contract c;
        if (!fill_contract(c, error, def)) {
            if (!error.empty())
                set_error(context, std::move(error));
            return false;
        }
        context->contracts.insert({name{contract}, std::move(c)});
        return true;
    });
}

extern "C" abisnax_bool abisnax_set_abi_bin(abisnax_context* context, uint64_t contract, const char* data, size_t size) {
    return handle_exceptions(context, false, [&] {
        context->last_error = "abi parse error";
        if (!data || !size)
            return set_error(context, "no data");
        std::string error;
        if (!check_abi_version(input_buffer{data, data + size}, error))
            return set_error(context, std::move(error));
        abi_def def{};
        input_buffer buf{data, data + size};
        if (!bin_to_native(def, error, buf)) {
            if (!error.empty())
                set_error(context, std::move(error));
            return false;
        }
        abisnax::contract c;
        if (!fill_contract(c, error, def)) {
            if (!error.empty())
                set_error(context, std::move(error));
            return false;
        }
        context->contracts.insert({name{contract}, std::move(c)});
        return true;
    });
}

extern "C" abisnax_bool abisnax_set_abi_hex(abisnax_context* context, uint64_t contract, const char* hex) {
    fix_null_str(hex);
    return handle_exceptions(context, false, [&]() -> abisnax_bool {
        std::vector<char> data;
        std::string error;
        if (!unhex(error, hex, hex + strlen(hex), std::back_inserter(data))) {
            if (!error.empty())
                set_error(context, std::move(error));
            return false;
        }
        return abisnax_set_abi_bin(context, contract, data.data(), data.size());
    });
}

extern "C" const char* abisnax_get_type_for_action(abisnax_context* context, uint64_t contract, uint64_t action) {
    return handle_exceptions(context, nullptr, [&] {
        auto contract_it = context->contracts.find(::abisnax::name{contract});
        if (contract_it == context->contracts.end())
            throw std::runtime_error("contract \"" + name_to_string(contract) + "\" is not loaded");
        auto& c = contract_it->second;

        auto action_it = c.action_types.find(name{action});
        if (action_it == c.action_types.end())
            throw std::runtime_error("contract \"" + name_to_string(contract) + "\" does not have action \"" +
                                     name_to_string(action) + "\"");
        return action_it->second.c_str();
    });
}

extern "C" const char* abisnax_get_type_for_table(abisnax_context* context, uint64_t contract, uint64_t table) {
    return handle_exceptions(context, nullptr, [&] {
        auto contract_it = context->contracts.find(::abisnax::name{contract});
        if (contract_it == context->contracts.end())
            throw std::runtime_error("contract \"" + name_to_string(contract) + "\" is not loaded");
        auto& c = contract_it->second;

        auto table_it = c.table_types.find(name{table});
        if (table_it == c.table_types.end())
            throw std::runtime_error("contract \"" + name_to_string(contract) + "\" does not have table \"" +
                                     name_to_string(table) + "\"");
        return table_it->second.c_str();
    });
}

extern "C" abisnax_bool abisnax_json_to_bin(abisnax_context* context, uint64_t contract, const char* type,
                                          const char* json) {
    fix_null_str(type);
    fix_null_str(json);
    return handle_exceptions(context, false, [&] {
        context->last_error = "json parse error";
        auto contract_it = context->contracts.find(::abisnax::name{contract});
        if (contract_it == context->contracts.end())
            return set_error(context, "contract \"" + name_to_string(contract) + "\" is not loaded");
        abi_type* t;
        std::string error;
        if (!get_type(t, error, contract_it->second.abi_types, type, 0))
            return set_error(context, error);
        context->result_bin.clear();
        if (!json_to_bin(context->result_bin, error, t, json)) {
            if (!error.empty())
                set_error(context, std::move(error));
            return false;
        }
        return true;
    });
}

extern "C" abisnax_bool abisnax_json_to_bin_reorderable(abisnax_context* context, uint64_t contract, const char* type,
                                                      const char* json) {
    fix_null_str(type);
    fix_null_str(json);
    return handle_exceptions(context, false, [&] {
        context->last_error = "json parse error";
        auto contract_it = context->contracts.find(::abisnax::name{contract});
        if (contract_it == context->contracts.end())
            return set_error(context, "contract \"" + name_to_string(contract) + "\" is not loaded");
        abi_type* t;
        std::string error;
        if (!get_type(t, error, contract_it->second.abi_types, type, 0))
            return set_error(context, error);
        context->result_bin.clear();
        ::abisnax::jvalue value;
        if (!json_to_jvalue(value, error, json)) {
            if (!error.empty())
                set_error(context, std::move(error));
            return false;
        }
        if (!json_to_bin(context->result_bin, error, t, value)) {
            if (!error.empty())
                set_error(context, std::move(error));
            return false;
        }
        return true;
    });
}

extern "C" const char* abisnax_bin_to_json(abisnax_context* context, uint64_t contract, const char* type,
                                          const char* data, size_t size) {
    fix_null_str(type);
    return handle_exceptions(context, nullptr, [&]() -> const char* {
        if (!data)
            size = 0;
        context->last_error = "binary decode error";
        auto contract_it = context->contracts.find(::abisnax::name{contract});
        std::string error;
        if (contract_it == context->contracts.end()) {
            (void)set_error(error, "contract \"" + name_to_string(contract) + "\" is not loaded");
            return nullptr;
        }
        abi_type* t;
        if (!get_type(t, error, contract_it->second.abi_types, type, 0)) {
            (void)set_error(context, error);
            return nullptr;
        }
        input_buffer bin{data, data + size};
        if (!bin_to_json(bin, error, t, context->result_str)) {
            if (!error.empty())
                set_error(context, std::move(error));
            return nullptr;
        }
        if (bin.pos != bin.end)
            throw std::runtime_error("Extra data");
        return context->result_str.c_str();
    });
}

extern "C" const char* abisnax_hex_to_json(abisnax_context* context, uint64_t contract, const char* type,
                                          const char* hex) {
    fix_null_str(hex);
    return handle_exceptions(context, nullptr, [&]() -> const char* {
        std::vector<char> data;
        std::string error;
        if (!unhex(error, hex, hex + strlen(hex), std::back_inserter(data))) {
            if (!error.empty())
                set_error(context, std::move(error));
            return nullptr;
        }
        return abisnax_bin_to_json(context, contract, type, data.data(), data.size());
    });
}

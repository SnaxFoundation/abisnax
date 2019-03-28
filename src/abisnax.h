// copyright defined in abisnax/LICENSE.txt

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct abisnax_context_s abisnax_context;
typedef int abisnax_bool;

// Create a context. The context holds all memory allocated by functions in this header. Returns null on failure.
abisnax_context* abisnax_create();

// Destroy a context.
void abisnax_destroy(abisnax_context* context);

// Get last error. Never returns null. The context owns the returned string.
const char* abisnax_get_error(abisnax_context* context);

// Get generated binary. The context owns the returned memory. Functions return null on error; use abisnax_get_error to
// retrieve error.
int abisnax_get_bin_size(abisnax_context* context);
const char* abisnax_get_bin_data(abisnax_context* context);

// Convert generated binary to hex. The context owns the returned string. Returns null on error; use abisnax_get_error to
// retrieve error.
const char* abisnax_get_bin_hex(abisnax_context* context);

// Name conversion. The context owns the returned memory. Functions return null on error; use abisnax_get_error to
// retrieve error.
uint64_t abisnax_string_to_name(abisnax_context* context, const char* str);
const char* abisnax_name_to_string(abisnax_context* context, uint64_t name);

// Set abi (JSON format). Returns false on error.
abisnax_bool abisnax_set_abi(abisnax_context* context, uint64_t contract, const char* abi);

// Set abi (binary format). Returns false on error.
abisnax_bool abisnax_set_abi_bin(abisnax_context* context, uint64_t contract, const char* data, size_t size);

// Set abi (hex format). Returns false on error.
abisnax_bool abisnax_set_abi_hex(abisnax_context* context, uint64_t contract, const char* hex);

// Get the type name for an action. The contract owns the returned memory. Returns null on error; use abisnax_get_error
// to retrieve error.
const char* abisnax_get_type_for_action(abisnax_context* context, uint64_t contract, uint64_t action);

// Get the type name for a table. The contract owns the returned memory. Returns null on error; use abisnax_get_error
// to retrieve error.
const char* abisnax_get_type_for_table(abisnax_context* context, uint64_t contract, uint64_t table);

// Convert json to binary. Use abisnax_get_bin_* to retrieve result. Returns false on error.
abisnax_bool abisnax_json_to_bin(abisnax_context* context, uint64_t contract, const char* type, const char* json);

// Convert json to binary. Allow json field reordering. Use abisnax_get_bin_* to retrieve result. Returns false on error.
abisnax_bool abisnax_json_to_bin_reorderable(abisnax_context* context, uint64_t contract, const char* type,
                                           const char* json);

// Convert binary to json. The context owns the returned string. Returns null on error; use abisnax_get_error to retrieve
// error.
const char* abisnax_bin_to_json(abisnax_context* context, uint64_t contract, const char* type, const char* data,
                               size_t size);

// Convert hex to json. The context owns the returned memory. Returns null on error; use abisnax_get_error to retrieve
// error.
const char* abisnax_hex_to_json(abisnax_context* context, uint64_t contract, const char* type, const char* hex);

#ifdef __cplusplus
}
#endif

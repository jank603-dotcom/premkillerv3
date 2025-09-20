#pragma once
#include <cstdint>

// Auto-initialize offsets:: values from the generated dump at runtime.
// Reads: premkiller-user/include/core/client_dll.cs
// Safe to call multiple times; it will only initialize once.
void load_offsets_from_client_dll();

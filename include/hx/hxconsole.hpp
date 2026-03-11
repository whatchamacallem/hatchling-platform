#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxconsole.hpp Implements a simple console for debugging, remote use
/// or for parsing configuration files. Output is directed to the system log
/// with `hxloglevel_console`.
///
/// A remote console requires forwarding commands to the target and reporting
/// the system log back. Configuration files only need file I/O. C-style calls
/// that return `bool` with any number of arguments using `double`, `int32_t`,
/// `uint32_t`, `uint64_t`, or `const char*` parameter types are required for
/// the bindings to work. See the following commands for examples.
///
/// | Parameter Type | Purpose |
/// | --- | --- |
/// | `double` | Parsed with strtod. |
/// | `int32_t` | Parsed with strtol (base 0, accepts 0x prefix). |
/// | `uint32_t` | Parsed with strtoul (base 0). |
/// | `uint64_t` | Parsed with strtoull (base 16, hexadecimal). |
/// | `const char*` | Captures remainder of line (must be last parameter). |

#include "hatchling.h"

class hxfile;

/// `hxconsole_command` - Registers a function using a global constructor. Use in
/// a global scope. The command uses the same name and arguments as the function.
/// - `x` : Valid C identifier that evaluates to a function pointer.
///   e.g., `hxconsole_command(srand);`
#define hxconsole_command(x_) static hxconsole_constructor_ \
	g_hxconsole_symbol_##x_(hxconsole_command_factory_(&(x_)), #x_)

/// `hxconsole_command_named` - Registers a named function using a global
/// constructor. Use in a global scope. The provided name must be a valid C
/// identifier.
/// - `x` : Any expression that evaluates to a function pointer.
/// - `name` : Valid C identifier that identifies the command.
///   e.g., `hxconsole_command_named(srand, seed_rand);`
#define hxconsole_command_named(x_, name_) static hxconsole_constructor_ \
	g_hxconsole_symbol_##name_(hxconsole_command_factory_(&(x_)), #name_)

/// `hxconsole_variable` - Registers a variable. Use in a global scope. The
/// command has the same name as the variable.
/// - `x` : Valid C identifier that evaluates to a variable.
///   e.g.,
/// ```cpp
///   static bool is_my_hack_enabled=false;
///   hxconsole_variable(is_my_hack_enabled);
/// ```
#define hxconsole_variable(x_) static hxconsole_constructor_ \
	g_hxconsole_symbol_##x_(hxconsole_variable_factory_(&(x_)), #x_)

/// `hxconsole_variable_named` - Registers a named variable. Use in a global
/// scope. The provided name must be a valid C identifier.
/// - `x` : Any expression that evaluates to a variable.
/// - `name` : Valid C identifier that identifies the variable.
///   e.g.,
/// ```cpp
///   static bool is_my_hack_enabled=false;
///   hxconsole_variable_named(is_my_hack_enabled, f_hack); // add "f_hack" to the console.
/// ```
#define hxconsole_variable_named(x_, name_) static hxconsole_constructor_ \
	g_hxconsole_symbol_##name_(hxconsole_variable_factory_(&(x_)), #name_)

/// `hxconsole_deregister` - Explicitly deregisters a console symbol.
/// - `id` : Non-null identifier string for the variable or command being
///   removed.
void hxconsole_deregister(const char* id_) hxattr_nonnull(1);

/// `hxconsole_exec_line` - Evaluates a console command to either call a function
/// or set a variable. e.g., `srand 77` or `a_variable 5`.
/// - `command` : Non-null UTF-8 command string executed by the console.
bool hxconsole_exec_line(const char* command_) hxattr_nonnull(1);

/// `hxconsole_exec_file` - Executes a configuration file that is opened for
/// reading. Ignores blank lines and comments that start with `#`.
/// - `file` : A file containing commands.
bool hxconsole_exec_file(hxfile& file_);

/// `hxconsole_exec_filename` - Opens a configuration file by name and executes it.
/// - `filename` : Non-null UTF-8 path to a file containing commands.
bool hxconsole_exec_filename(const char* filename_) hxattr_nonnull(1);

/// `hxconsole_help` - Logs every console symbol to the console log when
/// `HX_RELEASE < 2`. Otherwise returns true without producing output.
bool hxconsole_help(void);

// Include internals
#include "detail/hxconsole_detail.hpp"

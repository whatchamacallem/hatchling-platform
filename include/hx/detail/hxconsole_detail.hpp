#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
// hxconsole inline header and internals. See hxconsole.hpp.

#include "../hxutility.h"

// NOLINTBEGIN

namespace hxdetail_ {

// ----------------------------------------------------------------------------
// Argument parsing. Explicit specializations parse each supported type.

template<typename arg_t_>
arg_t_ hxconsole_parse_arg_(const char* str_, char** next_) = delete;

template<> inline float hxconsole_parse_arg_<float>(const char* str_, char** next_) {
	return (float)::strtod(str_, next_);
}

template<> inline double hxconsole_parse_arg_<double>(const char* str_, char** next_) {
	return ::strtod(str_, next_);
}

template<> inline int32_t hxconsole_parse_arg_<int32_t>(const char* str_, char** next_) {
	return (int32_t)::strtol(str_, next_, 0);
}

template<> inline uint32_t hxconsole_parse_arg_<uint32_t>(const char* str_, char** next_) {
	return (uint32_t)::strtoul(str_, next_, 0);
}

template<> inline uint64_t hxconsole_parse_arg_<uint64_t>(const char* str_, char** next_) {
	return ::strtoull(str_, next_, 16);
}

// const char* captures remainder of line including comments starting with #'s.
// Leading whitespace is discarded and string may be empty.
template<> inline const char* hxconsole_parse_arg_<const char*>(const char* str_, char** next_) {
	while(*str_ != '\0' && !hxisgraph(*str_)) {
		++str_;
	}
	const char* result_ = str_;
	while(*str_ != '\0') { ++str_; }
	*next_ = const_cast<char*>(str_);
	return result_;
}

// ----------------------------------------------------------------------------
// Argument labels for usage strings.

template<typename arg_t_> const char* hxconsole_arg_label_() = delete;
template<> inline const char* hxconsole_arg_label_<float>() { return "f32"; }
template<> inline const char* hxconsole_arg_label_<double>() { return "f64"; }
template<> inline const char* hxconsole_arg_label_<int32_t>() { return "i32"; }
template<> inline const char* hxconsole_arg_label_<uint32_t>() { return "u32"; }
template<> inline const char* hxconsole_arg_label_<uint64_t>() { return "hex"; }
template<> inline const char* hxconsole_arg_label_<const char*>() { return "char*"; }

// ----------------------------------------------------------------------------
// C++20 concept for parseable types.

#if HX_CPLUSPLUS >= 202002L
template<typename t_>
concept hxconsole_parseable_ = requires(const char* s_, char** n_) {
	requires hxis_same<decltype(hxconsole_parse_arg_<t_>(s_, n_)), t_>::value;
};
#else
#define hxconsole_parseable_ typename
#endif

// ----------------------------------------------------------------------------
// Checks for printing characters.

inline bool hxconsole_is_end_of_line_(const char* str_) {
	while(*str_ != '\0' && !hxisgraph(*str_)) {
		++str_;
	}
	return *str_ == '\0' || *str_ == '#'; // Skip comments
}

// ----------------------------------------------------------------------------
// Cast double to variable type. Integer types clamp. Floating-point types
// assign directly.

template<typename var_t_>
inline var_t_ hxconsole_cast_number_(double number_) {
	// Integer path: clamp to representable range.
	const bool is_signed_ = static_cast<var_t_>(-1) < var_t_(0u);
	const var_t_ min_value_ = is_signed_ ? var_t_(var_t_(1u) << (sizeof(var_t_) * 8 - 1)) : var_t_(0u);
	const var_t_ max_value_ = (var_t_)~min_value_;

	double clamped_ = hxclamp(number_, (double)min_value_, (double)max_value_);
	hxassertmsg(number_ == clamped_, "parameter_overflow %lf -> %lf", number_, clamped_);
	return (var_t_)clamped_;
}

template<> inline float hxconsole_cast_number_<float>(double number_) {
	return static_cast<float>(number_);
}

template<> inline double hxconsole_cast_number_<double>(double number_) {
	return number_;
}

template<> inline bool hxconsole_cast_number_<bool>(double number_) {
	return number_ != 0.0;
}

// ----------------------------------------------------------------------------
// hxconsole_command_ base class.

class hxconsole_command_ {
public:
	virtual bool execute_(const char* str_) = 0; // Return false for parse errors.
	virtual void usage_(const char* id_=hxnull) = 0; // Expects command name.

	// Returns 0 if no parameter. Returns 1 if a single number was found. Returns
	// 2 to indicate a parse error. This avoids template bloat by being in a
	// base class.
	static int execute_number_(const char* str_, double* number_) {
		if(hxconsole_is_end_of_line_(str_)) {
			return 0; // success, do not modify
		}

		char* ptr_ = const_cast<char*>(str_);
		*number_ = ::strtod(str_, &ptr_);
		if(str_ < ptr_ && hxconsole_is_end_of_line_(ptr_)) {
			return 1; // success, do modify
		}

		hxlogconsole("parse error: %s", str_);
		return 2; // failure, do not modify
	}
};

// ----------------------------------------------------------------------------
// Single variadic command template. Replaces hxconsole_command0_ through
// hxconsole_command4_.

template<hxconsole_parseable_... args_t_>
class hxconsole_command_impl_ : public hxconsole_command_ {
public:
	hxconsole_command_impl_(bool(*fn_)(args_t_...)) : m_fn_(fn_) { }

	bool execute_(const char* str_) override {
		if constexpr (sizeof...(args_t_) == 0) {
			if(hxconsole_is_end_of_line_(str_)) {
				return m_fn_();
			}
			usage_();
			return false;
		} else {
			char* next_ = const_cast<char*>(str_);
			bool ok_ = call_<args_t_...>(m_fn_, str_, next_);
			if(!ok_) { usage_(); }
			return ok_;
		}
	}

	void usage_(const char* id_=hxnull) override {
		hxlogconsole("%s", id_ ? id_ : "usage:");
		if constexpr (sizeof...(args_t_) == 0) {
			hxlogconsole("\n");
		} else {
			(hxlogconsole(" %s", hxconsole_arg_label_<args_t_>()), ...);
			hxlogconsole("\n");
		}
	}

private:
	// Terminal: all args parsed, check end of line and call.
	template<typename fn_t_, typename... parsed_t_>
	static bool call_(fn_t_ fn_, const char* pos_, char*, parsed_t_... parsed_) {
		if(hxconsole_is_end_of_line_(pos_)) {
			return fn_(parsed_...);
		}
		return false;
	}

	// Recursive: parse one arg, recurse with remaining types.
	template<typename first_t_, typename... rest_t_, typename fn_t_, typename... parsed_t_>
	static bool call_(fn_t_ fn_, const char* pos_, char* next_, parsed_t_... parsed_) {
		first_t_ val_ = hxconsole_parse_arg_<first_t_>(pos_, &next_);
		if(pos_ < next_) {
			return call_<rest_t_...>(fn_, next_, next_, parsed_..., val_);
		}
		return false;
	}

	bool(*m_fn_)(args_t_...);
};

// ----------------------------------------------------------------------------
// Variable template. Uses execute_number_ from the base class to avoid
// template bloat. Assignment casts through double with clamping.

template<typename var_t_>
class hxconsole_variable_ : public hxconsole_command_ {
public:
	hxconsole_variable_(volatile var_t_* var_) : m_var_(var_) { }

	bool execute_(const char* str_) override {
		double number_ = 0.0;
		int code_ = execute_number_(str_, &number_);
		if(code_ == 0) {
			// 0 parameters is a query
			hxloghandler(hxloglevel_console, "%.15g\n", (double)*m_var_);
			return true;
		}
		if(code_ == 1) {
			// 1 parameter is assignment.
			*m_var_ = hxconsole_cast_number_<var_t_>(number_);
			return true;
		}
		return false; // 2 is unexpected args.
	}

	void usage_(const char* id_=hxnull) override {
		(void)id_;
		hxlogconsole("%s <optional-value>\n", id_ ? id_ : "usage:");
	}
private:
	volatile var_t_* m_var_;
};

// ----------------------------------------------------------------------------
// Single factory function. The compiler deduces args_t_... from the function
// pointer.

template<typename... args_t_>
inline hxconsole_command_impl_<args_t_...> hxconsole_command_factory_(bool(*fn_)(args_t_...)) {
	return hxconsole_command_impl_<args_t_...>(fn_);
}

template<typename var_t_>
inline hxconsole_variable_<var_t_> hxconsole_variable_factory_(volatile var_t_* var_) {
	return hxconsole_variable_<var_t_>(var_);
}

// ERROR: Pointers cannot be console variables.
template<typename var_t_>
inline void hxconsole_variable_factory_(volatile var_t_** var_) = delete;
template<typename var_t_>
inline void hxconsole_variable_factory_(const volatile var_t_** var_) = delete;

// ----------------------------------------------------------------------------
// Hash table infrastructure (unchanged).

// Wrap the string literal type because it is not used normally.
class hxconsole_hash_table_key_ {
public:
	explicit hxconsole_hash_table_key_(const char* s_) : str_(s_) { }
	const char* str_;
};

// Uses FNV-1a string hashing. Stops at whitespace.
inline hxhash_t hxkey_hash(hxconsole_hash_table_key_ k_) {
	hxhash_t x_ = (hxhash_t)0x811c9dc5;
	while(hxisgraph(*k_.str_)) {
		x_ ^= (hxhash_t)*k_.str_++;
		x_ *= (hxhash_t)0x01000193;
	}
	return x_;
}

// A version of ::strcmp that stops at the first non-graphical characters.
inline bool hxkey_equal(hxconsole_hash_table_key_ a_, hxconsole_hash_table_key_ b_) {
	while(hxisgraph(*a_.str_) && *a_.str_ == *b_.str_) { ++a_.str_; ++b_.str_; }
	return !hxisgraph(*a_.str_) && !hxisgraph(*b_.str_);
};

// this is how to write a hash node without including hash table code.
class hxconsole_hash_table_node_ {
public:
	using key_t = hxconsole_hash_table_key_;

	hxconsole_hash_table_node_(hxconsole_hash_table_key_ key_)
			: m_hash_next_(hxnull), m_key_(key_), m_hash_(hxkey_hash(key_)), m_command_(hxnull) {
		if((HX_RELEASE) < 1) {
			const char* k_ = key_.str_;
			while(hxisgraph(*k_)) {
				++k_;
			}
			hxassertmsg(*k_ == '\0', "bad_console_symbol \"%s\"", key_.str_);
		}
	}

	// Boilerplate required by hxhash_table.
	void* hash_next(void) const { return m_hash_next_; }
	void*& hash_next(void) { return m_hash_next_; }

	hxconsole_hash_table_key_ key(void) const { return m_key_; }
	hxhash_t hash(void) const { return m_hash_; }
	hxconsole_command_* command_(void) const { return m_command_; }
	void set_command_(hxconsole_command_* x_) { m_command_ = x_; }

private:
	void* m_hash_next_;
	hxconsole_hash_table_key_ m_key_;
	hxhash_t m_hash_;
	hxconsole_command_* m_command_;
};

void hxconsole_register_(hxconsole_hash_table_node_* node);

// registers a console command using a global variable without memory allocations.
// There is no reason to deregister or destruct anything.
class hxconsole_constructor_ {
public:
	template<typename command_t_>
	hxconsole_constructor_(command_t_ fn_, const char* id_)
			: m_node_(hxconsole_hash_table_key_(id_)) {
		static_assert(sizeof(command_t_) <= sizeof(m_storage_), "command_storage_overflow");
		::new(m_storage_ + 0) command_t_(fn_);
		m_node_.set_command_((command_t_*)(m_storage_ + 0));
		hxconsole_register_(&m_node_);
	}

private:
	// Provide static storage instead of using allocator before main.
	// Size: vtable pointer + function pointer.
	hxconsole_hash_table_node_ m_node_;
	char m_storage_[sizeof(void*) + sizeof(void*)];
};

} // hxdetail_
using namespace hxdetail_;

// NOLINTEND

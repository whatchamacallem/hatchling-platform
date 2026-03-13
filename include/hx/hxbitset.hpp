#pragma once
// SPDX-FileCopyrightText: © 2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"

/// A fixed-size bitset stored as an array of `size_t` words with no heap
/// allocation.
/// - `bits` : The number of bits in the hxbitset. Must be greater than zero.
template<size_t bits_>
class hxbitset {
public:
	/// Constructs a zero-initialized hxbitset.
	hxbitset(void) { this->reset(); }

	/// Constructs a hxbitset from a single `size_t` value. Only valid when
	/// `bits` equals the number of bits in `size_t`.
	/// - `val` : The value to initialize the bitset with.
	explicit hxbitset(size_t val_) {
		static_assert(bits_ == s_bits_per_word_,
			"hxbitset(size_t) requires bits_ == sizeof(size_t) * 8.");
		m_data_[0] = val_;
	}

	/// Constructs a hxbitset as a copy of `x`.
	/// - `x` : The hxbitset to copy from.
	hxbitset(const hxbitset& x_) {
		const size_t* hxrestrict src_ = x_.m_data_;
		size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ = *src_++; }
	}

	/// Assigns the bits of `x` to this hxbitset. Asserts that `&x` is not `this`.
	/// - `x` : The hxbitset to copy from.
	void operator=(const hxbitset& x_) {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Assignment to self.");
		const size_t* hxrestrict src_ = x_.m_data_;
		size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ = *src_++; }
	}

	/// Returns the number of bits.
	static constexpr size_t size(void) hxattr_nodiscard { return bits_; }

	/// Returns the size of the underlying storage in bytes.
	static constexpr size_t bytes(void) hxattr_nodiscard { return s_words_ * sizeof(size_t); }

	/// Returns a pointer to the underlying word storage.
	size_t* data(void) hxattr_nodiscard { return m_data_; }

	/// Returns a const pointer to the underlying word storage.
	const size_t* data(void) const hxattr_nodiscard { return m_data_; }

	/// Copies `len` bytes from `src` into the hxbitset storage. Asserts that
	/// `len` does not exceed `bytes()`. Trailing bits beyond `bits` are masked
	/// to zero after the copy.
	/// - `src` : Pointer to the source data.
	/// - `len` : Number of bytes to copy. Must not exceed `bytes()`.
	void load(const char* src_, size_t len_) {
		hxassertmsg(len_ <= bytes(), "overflow_load %zu", len_);
		::memcpy(m_data_, src_, len_); // NOLINT
		m_data_[s_words_ - 1u] &= s_trailing_mask_;
	}

	/// Returns the value of the bit at position `pos`. Asserts that `pos` is in
	/// range.
	/// - `pos` : Bit index. Must be less than `bits`.
	bool operator[](size_t pos_) const hxattr_nodiscard {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		return (m_data_[pos_ / s_bits_per_word_]
			& (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_))) != 0u;
	}

	/// Returns the value of the bit at position `pos`. Asserts that `pos` is in
	/// range.
	/// - `pos` : Bit index must be less than `bits`.
	bool test(size_t pos_) const hxattr_nodiscard {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		return (m_data_[pos_ / s_bits_per_word_]
			& (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_))) != 0u;
	}

	/// Sets all bits to 1.
	hxbitset& set(void) {
		size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ = ~static_cast<size_t>(0u); }
		m_data_[s_words_ - 1u] &= s_trailing_mask_;
		return *this;
	}

	/// Sets or clears the bit at position `pos`. Asserts that `pos` is in
	/// range.
	/// - `pos` : Bit index that must be less than `bits`.
	/// - `value` : The value to assign, defaults to `true`.
	hxbitset& set(size_t pos_, bool value_=true) {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		const size_t mask_ = static_cast<size_t>(1u) << (pos_ % s_bits_per_word_);
		if(value_) {
			m_data_[pos_ / s_bits_per_word_] |= mask_;
		} else {
			m_data_[pos_ / s_bits_per_word_] &= ~mask_;
		}
		return *this;
	}

	/// Clears all bits to 0.
	hxbitset& reset(void) {
		size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ = 0u; }
		return *this;
	}

	/// Clears the bit at position `pos`. Asserts that `pos` is in range.
	/// - `pos` : Bit index that must be less than `bits`.
	hxbitset& reset(size_t pos_) {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		m_data_[pos_ / s_bits_per_word_] &= ~(static_cast<size_t>(1u) << (pos_ % s_bits_per_word_));
		return *this;
	}

	/// Flips all bits.
	hxbitset& flip(void) {
		size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ ^= ~static_cast<size_t>(0u); }
		m_data_[s_words_ - 1u] &= s_trailing_mask_;
		return *this;
	}

	/// Flips the bit at position `pos`. Asserts that `pos` is in range.
	/// - `pos` : Bit index that must be less than `bits`.
	hxbitset& flip(size_t pos_) {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		m_data_[pos_ / s_bits_per_word_] ^= (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_));
		return *this;
	}

	/// Returns `true` if all bits are set.
	bool all(void) const hxattr_nodiscard {
		assert_no_trailing_bits_();
		const size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + (s_words_ - 1u);
		while(dst_ != end_) {
			if(*dst_++ != ~static_cast<size_t>(0u)) { return false; }
		}
		return m_data_[s_words_ - 1u] == s_trailing_mask_;
	}

	/// Returns `true` if at least one bit is set.
	bool any(void) const hxattr_nodiscard {
		assert_no_trailing_bits_();
		const size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) {
			if(*dst_++ != 0u) { return true; }
		}
		return false;
	}

	/// Returns `true` if no bits are set.
	bool none(void) const hxattr_nodiscard { return !this->any(); }

	/// Applies bitwise AND with `x` in place. Asserts that `&x` is not `this`.
	/// - `x` : The hxbitset to AND with.
	hxbitset& operator&=(const hxbitset& x_) {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Operation with self.");
		size_t* hxrestrict dst_ = m_data_;
		const size_t* hxrestrict src_ = x_.m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ &= *src_++; }
		return *this;
	}

	/// Applies bitwise OR with `x` in place. Asserts that `&x` is not `this`.
	/// - `x` : The hxbitset to OR with.
	hxbitset& operator|=(const hxbitset& x_) {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Operation with self.");
		size_t* hxrestrict dst_ = m_data_;
		const size_t* hxrestrict src_ = x_.m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ |= *src_++; }
		assert_no_trailing_bits_();
		return *this;
	}

	/// Applies bitwise XOR with `x` in place. Asserts that `&x` is not `this`.
	/// - `x` : The hxbitset to XOR with.
	hxbitset& operator^=(const hxbitset& x_) {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Operation with self.");
		size_t* hxrestrict dst_ = m_data_;
		const size_t* hxrestrict src_ = x_.m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ ^= *src_++; }
		assert_no_trailing_bits_();
		return *this;
	}

	/// Shifts all bits left by `count` positions, filling vacated bits with 0.
	/// - `count` : Number of positions to shift left.
	hxbitset& operator<<=(size_t count_) {
		if(count_ == 0u) { return *this; }
		const size_t word_shift_ = count_ / s_bits_per_word_;
		const size_t bit_shift_ = count_ % s_bits_per_word_;
		if(bit_shift_ == 0u) {
			for(size_t i_ = s_words_; i_-- != 0u; ) {
				m_data_[i_] = (i_ >= word_shift_) ? m_data_[i_ - word_shift_] : 0u;
			}
		} else {
			for(size_t i_ = s_words_; i_-- != 0u; ) {
				const size_t lo_ = (i_ >= word_shift_)
					? (m_data_[i_ - word_shift_] << bit_shift_) : 0u;
				const size_t hi_ = (i_ > word_shift_)
					? (m_data_[i_ - word_shift_ - 1u] >> (s_bits_per_word_ - bit_shift_)) : 0u;
				m_data_[i_] = lo_ | hi_;
			}
		}
		m_data_[s_words_ - 1u] &= s_trailing_mask_;
		assert_no_trailing_bits_();
		return *this;
	}

	/// Shifts all bits right by `count` positions, filling vacated bits with 0.
	/// - `count` : Number of positions to shift right.
	hxbitset& operator>>=(size_t count_) {
		if(count_ == 0u) { return *this; }
		const size_t word_shift_ = count_ / s_bits_per_word_;
		const size_t bit_shift_ = count_ % s_bits_per_word_;
		if(bit_shift_ == 0u) {
			for(size_t i_ = 0u; i_ < s_words_; ++i_) {
				m_data_[i_] = ((i_ + word_shift_) < s_words_) ? m_data_[i_ + word_shift_] : 0u;
			}
		} else {
			for(size_t i_ = 0u; i_ < s_words_; ++i_) {
				const size_t lo_ = ((i_ + word_shift_) < s_words_)
					? (m_data_[i_ + word_shift_] >> bit_shift_) : 0u;
				const size_t hi_ = ((i_ + word_shift_ + 1u) < s_words_)
					? (m_data_[i_ + word_shift_ + 1u] << (s_bits_per_word_ - bit_shift_)) : 0u;
				m_data_[i_] = lo_ | hi_;
			}
		}
		assert_no_trailing_bits_();
		return *this;
	}

	/// Returns `true` if all bits compare equal to those of `x`. Asserts that
	/// `&x` is not `this`.
	/// - `x` : The hxbitset to compare with.
	bool operator==(const hxbitset& x_) const hxattr_nodiscard {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Operation with self.");
		const size_t* hxrestrict dst_ = m_data_;
		const size_t* hxrestrict src_ = x_.m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { if(*dst_++ != *src_++) { return false; } }
		return true;
	}

#if HX_CPLUSPLUS < 202002L
	/// Returns `true` if any bits differ from those of `x`. Only defined when
	/// `HX_CPLUSPLUS < 202002L`.
	/// - `x` : The hxbitset to compare with.
	bool operator!=(const hxbitset& x_) const hxattr_nodiscard {
		return !(*this == x_);
	}
#endif

private:
	static_assert(bits_ > 0u, "hxbitset requires bits_ > 0.");
	static constexpr size_t s_bits_per_word_ = sizeof(size_t) * 8u;
	static constexpr size_t s_words_ = (bits_ + s_bits_per_word_ - 1u) / s_bits_per_word_;
	static constexpr size_t s_trailing_bits_ = bits_ % s_bits_per_word_;
	static constexpr size_t s_trailing_mask_ = s_trailing_bits_ != 0u
		? (static_cast<size_t>(1u) << s_trailing_bits_) - 1u
		: ~static_cast<size_t>(0u);

	void assert_no_trailing_bits_(void) const {
		hxassertmsg((m_data_[s_words_ - 1u] & ~s_trailing_mask_) == 0u, "trailing_bits_set");
	}

	size_t m_data_[s_words_];
};

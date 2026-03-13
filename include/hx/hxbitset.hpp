#pragma once
// SPDX-FileCopyrightText: © 2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"

template<size_t bits_>
class hxbitset {
public:
	hxbitset(void) { this->reset(); }

	hxbitset(const hxbitset& x_) {
		const size_t* hxrestrict src_ = x_.m_data_;
		size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ = *src_++; }
	}

	void operator=(const hxbitset& x_) {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Assignment to self.");
		const size_t* hxrestrict src_ = x_.m_data_;
		size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ = *src_++; }
	}

	static constexpr size_t size(void) { return bits_; }
	static constexpr size_t bytes(void) { return s_words_ * sizeof(size_t); }
	size_t* data(void) { return m_data_; }
	const size_t* data(void) const { return m_data_; }

	void load(const char* src_, size_t len_) {
		hxassertmsg(len_ <= bytes(), "overflow_load %zu", len_);
		::memcpy(m_data_, src_, len_); // NOLINT
		assert_no_trailing_bits_();
	}

	bool operator[](size_t pos_) const {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		return (m_data_[pos_ / s_bits_per_word_] & (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_))) != 0u;
	}

	bool test(size_t pos_) const hxattr_nodiscard {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		return (m_data_[pos_ / s_bits_per_word_] & (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_))) != 0u;
	}

	hxbitset& set(void) {
		size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ = ~static_cast<size_t>(0u); }
		m_data_[s_words_ - 1u] &= s_trailing_mask_;
		return *this;
	}

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

	hxbitset& reset(void) {
		size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ = 0u; }
		return *this;
	}

	hxbitset& reset(size_t pos_) {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		m_data_[pos_ / s_bits_per_word_] &= ~(static_cast<size_t>(1u) << (pos_ % s_bits_per_word_));
		return *this;
	}

	hxbitset& flip(void) {
		size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ ^= ~static_cast<size_t>(0u); }
		m_data_[s_words_ - 1u] &= s_trailing_mask_;
		return *this;
	}

	hxbitset& flip(size_t pos_) {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		m_data_[pos_ / s_bits_per_word_] ^= (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_));
		return *this;
	}

	bool all(void) const hxattr_nodiscard {
		assert_no_trailing_bits_();
		const size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + (s_words_ - 1u);
		while(dst_ != end_) { if(*dst_++ != ~static_cast<size_t>(0u)) { return false; } }
		return m_data_[s_words_ - 1u] == s_trailing_mask_;
	}

	bool any(void) const hxattr_nodiscard {
		assert_no_trailing_bits_();
		const size_t* hxrestrict dst_ = m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { if(*dst_++ != 0u) { return true; } }
		return false;
	}

	bool none(void) const hxattr_nodiscard { return !this->any(); }

	hxbitset& operator&=(const hxbitset& x_) {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Operation with self.");
		size_t* hxrestrict dst_ = m_data_;
		const size_t* hxrestrict src_ = x_.m_data_;
		const size_t* const end_ = dst_ + s_words_;
		while(dst_ != end_) { *dst_++ &= *src_++; }
		return *this;
	}

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
				const size_t lo_ = (i_ >= word_shift_) ? (m_data_[i_ - word_shift_] << bit_shift_) : 0u;
				const size_t hi_ = (i_ > word_shift_) ? (m_data_[i_ - word_shift_ - 1u] >> (s_bits_per_word_ - bit_shift_)) : 0u;
				m_data_[i_] = lo_ | hi_;
			}
		}
		m_data_[s_words_ - 1u] &= s_trailing_mask_;
		assert_no_trailing_bits_();
		return *this;
	}

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
				const size_t lo_ = ((i_ + word_shift_) < s_words_) ? (m_data_[i_ + word_shift_] >> bit_shift_) : 0u;
				const size_t hi_ = ((i_ + word_shift_ + 1u) < s_words_) ? (m_data_[i_ + word_shift_ + 1u] << (s_bits_per_word_ - bit_shift_)) : 0u;
				m_data_[i_] = lo_ | hi_;
			}
		}
		assert_no_trailing_bits_();
		return *this;
	}

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

template<size_t bits_>
hxbitset<bits_> operator&(const hxbitset<bits_>& x_, const hxbitset<bits_>& y_) {
	hxassertmsg(static_cast<const void*>(&x_) != static_cast<const void*>(&y_),
		"invalid_reference Operation with self.");
	hxbitset<bits_> result_(x_);
	result_ &= y_;
	return result_;
}

template<size_t bits_>
hxbitset<bits_> operator|(const hxbitset<bits_>& x_, const hxbitset<bits_>& y_) {
	hxassertmsg(static_cast<const void*>(&x_) != static_cast<const void*>(&y_),
		"invalid_reference Operation with self.");
	hxbitset<bits_> result_(x_);
	result_ |= y_;
	return result_;
}

template<size_t bits_>
hxbitset<bits_> operator^(const hxbitset<bits_>& x_, const hxbitset<bits_>& y_) {
	hxassertmsg(static_cast<const void*>(&x_) != static_cast<const void*>(&y_),
		"invalid_reference Operation with self.");
	hxbitset<bits_> result_(x_);
	result_ ^= y_;
	return result_;
}

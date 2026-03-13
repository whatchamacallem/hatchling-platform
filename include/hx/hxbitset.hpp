#pragma once
// SPDX-FileCopyrightText: © 2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hxallocator.hpp"

template<size_t bits_>
class hxbitset : public hxallocator<size_t, (bits_ + sizeof(size_t) * 8u - 1u) / (sizeof(size_t) * 8u)> {
public:
	static_assert(bits_ > 0u, "hxbitset requires bits_ > 0.");
	static constexpr size_t s_bits_per_word_ = sizeof(size_t) * 8u;
	static constexpr size_t s_words_ = (bits_ + s_bits_per_word_ - 1u) / s_bits_per_word_;

	hxbitset(void) {
		size_t* d_ = this->data();
		for(size_t i_ = s_words_; i_-- != 0u; ) { d_[i_] = 0u; }
	}

	hxbitset(const hxbitset& x_) {
		const size_t* s_ = x_.data();
		size_t* d_ = this->data();
		for(size_t i_ = s_words_; i_-- != 0u; ) { d_[i_] = s_[i_]; }
	}

	void operator=(const hxbitset& x_) {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Assignment to self.");
		const size_t* s_ = x_.data();
		size_t* d_ = this->data();
		for(size_t i_ = s_words_; i_-- != 0u; ) { d_[i_] = s_[i_]; }
	}

	static constexpr size_t size(void) { return bits_; }

	bool operator[](size_t pos_) const {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		return (this->data()[pos_ / s_bits_per_word_] & (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_))) != 0u;
	}

	bool test(size_t pos_) const hxattr_nodiscard {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		return (this->data()[pos_ / s_bits_per_word_] & (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_))) != 0u;
	}

	hxbitset& set(void) {
		size_t* d_ = this->data();
		for(size_t i_ = s_words_; i_-- != 0u; ) { d_[i_] = ~static_cast<size_t>(0u); }
		return *this;
	}

	hxbitset& set(size_t pos_, bool value_=true) {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		const size_t mask_ = static_cast<size_t>(1u) << (pos_ % s_bits_per_word_);
		if(value_) {
			this->data()[pos_ / s_bits_per_word_] |= mask_;
		} else {
			this->data()[pos_ / s_bits_per_word_] &= ~mask_;
		}
		return *this;
	}

	hxbitset& reset(void) {
		size_t* d_ = this->data();
		for(size_t i_ = s_words_; i_-- != 0u; ) { d_[i_] = 0u; }
		return *this;
	}

	hxbitset& reset(size_t pos_) {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		this->data()[pos_ / s_bits_per_word_] &= ~(static_cast<size_t>(1u) << (pos_ % s_bits_per_word_));
		return *this;
	}

	hxbitset& flip(void) {
		size_t* d_ = this->data();
		for(size_t i_ = s_words_; i_-- != 0u; ) { d_[i_] ^= ~static_cast<size_t>(0u); }
		return *this;
	}

	hxbitset& flip(size_t pos_) {
		hxassertmsg(pos_ < bits_, "invalid_index %zu", pos_);
		this->data()[pos_ / s_bits_per_word_] ^= (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_));
		return *this;
	}

	bool all(void) const hxattr_nodiscard {
		const size_t* d_ = this->data();
		for(size_t i_ = s_words_; i_-- != 0u; ) {
			if(d_[i_] != ~static_cast<size_t>(0u)) { return false; }
		}
		return true;
	}

	bool any(void) const hxattr_nodiscard {
		const size_t* d_ = this->data();
		for(size_t i_ = s_words_; i_-- != 0u; ) {
			if(d_[i_] != 0u) { return true; }
		}
		return false;
	}

	bool none(void) const hxattr_nodiscard { return !this->any(); }

	hxbitset& operator&=(const hxbitset& x_) {
		size_t* d_ = this->data();
		const size_t* s_ = x_.data();
		for(size_t i_ = s_words_; i_-- != 0u; ) { d_[i_] &= s_[i_]; }
		return *this;
	}

	hxbitset& operator|=(const hxbitset& x_) {
		size_t* d_ = this->data();
		const size_t* s_ = x_.data();
		for(size_t i_ = s_words_; i_-- != 0u; ) { d_[i_] |= s_[i_]; }
		return *this;
	}

	hxbitset& operator^=(const hxbitset& x_) {
		size_t* d_ = this->data();
		const size_t* s_ = x_.data();
		for(size_t i_ = s_words_; i_-- != 0u; ) { d_[i_] ^= s_[i_]; }
		return *this;
	}

	hxbitset& operator<<=(size_t count_) {
		if(count_ == 0u) { return *this; }
		const size_t word_shift_ = count_ / s_bits_per_word_;
		const size_t bit_shift_ = count_ % s_bits_per_word_;
		size_t* d_ = this->data();
		if(bit_shift_ == 0u) {
			for(size_t i_ = s_words_; i_-- != 0u; ) {
				d_[i_] = (i_ >= word_shift_) ? d_[i_ - word_shift_] : 0u;
			}
		} else {
			for(size_t i_ = s_words_; i_-- != 0u; ) {
				const size_t lo_ = (i_ >= word_shift_) ? (d_[i_ - word_shift_] << bit_shift_) : 0u;
				const size_t hi_ = (i_ > word_shift_) ? (d_[i_ - word_shift_ - 1u] >> (s_bits_per_word_ - bit_shift_)) : 0u;
				d_[i_] = lo_ | hi_;
			}
		}
		return *this;
	}

	hxbitset& operator>>=(size_t count_) {
		if(count_ == 0u) { return *this; }
		const size_t word_shift_ = count_ / s_bits_per_word_;
		const size_t bit_shift_ = count_ % s_bits_per_word_;
		size_t* d_ = this->data();
		if(bit_shift_ == 0u) {
			for(size_t i_ = 0u; i_ < s_words_; ++i_) {
				d_[i_] = ((i_ + word_shift_) < s_words_) ? d_[i_ + word_shift_] : 0u;
			}
		} else {
			for(size_t i_ = 0u; i_ < s_words_; ++i_) {
				const size_t lo_ = ((i_ + word_shift_) < s_words_) ? (d_[i_ + word_shift_] >> bit_shift_) : 0u;
				const size_t hi_ = ((i_ + word_shift_ + 1u) < s_words_) ? (d_[i_ + word_shift_ + 1u] << (s_bits_per_word_ - bit_shift_)) : 0u;
				d_[i_] = lo_ | hi_;
			}
		}
		return *this;
	}

	bool operator==(const hxbitset& x_) const hxattr_nodiscard {
		const size_t* d0_ = this->data();
		const size_t* d1_ = x_.data();
		for(size_t i_ = s_words_; i_-- != 0u; ) {
			if(d0_[i_] != d1_[i_]) { return false; }
		}
		return true;
	}

#if HX_CPLUSPLUS < 202002L
	bool operator!=(const hxbitset& x_) const hxattr_nodiscard {
		return !(*this == x_);
	}
#endif
};

template<size_t bits_>
hxbitset<bits_> operator&(const hxbitset<bits_>& x_, const hxbitset<bits_>& y_) {
	hxbitset<bits_> result_(x_);
	result_ &= y_;
	return result_;
}

template<size_t bits_>
hxbitset<bits_> operator|(const hxbitset<bits_>& x_, const hxbitset<bits_>& y_) {
	hxbitset<bits_> result_(x_);
	result_ |= y_;
	return result_;
}

template<size_t bits_>
hxbitset<bits_> operator^(const hxbitset<bits_>& x_, const hxbitset<bits_>& y_) {
	hxbitset<bits_> result_(x_);
	result_ ^= y_;
	return result_;
}

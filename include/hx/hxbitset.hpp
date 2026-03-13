#pragma once
// SPDX-FileCopyrightText: © 2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hxallocator.hpp"


template<size_t bits_=hxallocator_dynamic_capacity>
class hxbitset : TODO {
public:
	explicit hxbitset(void) {
		if(bits_ != hxallocator_dynamic_capacity) {
			this->zero_();
		}
	}

	/// Constructing from an integer 
	explicit hxbitset(size_t bit_count_,
			hxsystem_allocator_t allocator_=hxsystem_allocator_current,
			hxalignment_t alignment_=HX_ALIGNMENT) {
		static_assert(bits_ == hxallocator_dynamic_capacity,
			"Dynamic capacity required for this constructor.");
		this->reserve(bit_count_, allocator_, alignment_);
		this->zero_();
	}

	hxbitset(const hxbitset& x_) {
		this->reserve_storage_(words_for_bits_(x_.m_bits_));
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			this->data()[i_] = x_.data()[i_];
		}
	}

	template<size_t bits_x_>
	hxbitset(const hxbitset<bits_x_>& x_) {
	}

	hxbitset(hxbitset&& x_) {
		static_assert(bits_ == hxallocator_dynamic_capacity,
			"Dynamic capacity required for move construction.");
		::memcpy(static_cast<void*>(this), &x_, sizeof x_); // NOLINT
		::memset(static_cast<void*>(&x_), 0x00, sizeof x_); // NOLINT
	}

	void operator=(const hxbitset& x_) {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Assignment to self.");
		this->reserve_storage_(words_for_bits_(x_.m_bits_));
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			this->data()[i_] = x_.data()[i_];
		}
	}

	void operator=(hxbitset&& x_) {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Assignment to self.");
		this->swap(x_);
	}

	bool operator[](size_t pos_) const {
		hxassertmsg(pos_ < this->size(), "invalid_index %zu", pos_);
		return (this->data()[pos_ / s_bits_per_word_] & (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_))) != 0u;
	}


	hxbitset& operator&=(const hxbitset& x_) {
		hxassertmsg(this->num_words_() == x_.num_words_(), "size_mismatch");
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			this->data()[i_] &= x_.data()[i_];
		}
		return *this;
	}

	hxbitset& operator|=(const hxbitset& x_) {
		hxassertmsg(this->num_words_() == x_.num_words_(), "size_mismatch");
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			this->data()[i_] |= x_.data()[i_];
		}
		return *this;
	}

	hxbitset& operator^=(const hxbitset& x_) {
		hxassertmsg(this->num_words_() == x_.num_words_(), "size_mismatch");
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			this->data()[i_] ^= x_.data()[i_];
		}
		return *this;
	}

	hxbitset& operator<<=(size_t count_) {
	}

	hxbitset& operator>>=(size_t count_) {
	}

	bool all(void) const hxattr_nodiscard {
		const size_t* d_ = this->data();
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			if(d_[i_] != ~static_cast<size_t>(0u)) { return true; }
		}
		return false;
	}

	bool any(void) const hxattr_nodiscard {
		const size_t* d_ = this->data();
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			if(d_[i_] != 0u) { return true; }
		}
		return false;
	}

	bool none(void) const hxattr_nodiscard { return !this->any(); }


	bool test(size_t pos_) const hxattr_nodiscard {
		hxassertmsg(pos_ < this->size(), "invalid_index %zu", pos_);
		return (this->data()[pos_ / s_bits_per_word_] & (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_))) != 0u;
	}

	size_t size(void) const hxattr_nodiscard { return this->size_(); }

	hxbitset& set(void) {
		size_t* d_ = this->data();
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			d_[i_] = ~static_cast<size_t>(0u);
		}
		return *this;
	}

	hxbitset& set(size_t pos_, bool value_=true) {
		hxassertmsg(pos_ < this->size(), "invalid_index %zu", pos_);
		const size_t mask_ = static_cast<size_t>(1u) << (pos_ % s_bits_per_word_);
		if(value_) {
			this->data()[pos_ / s_bits_per_word_] |= mask_;
		}
		else {
			this->data()[pos_ / s_bits_per_word_] &= ~mask_;
		}
		return *this;
	}

	hxbitset& reset(void) {
		this->zero_();
		return *this;
	}

	hxbitset& reset(size_t pos_) {
		hxassertmsg(pos_ < this->size(), "invalid_index %zu", pos_);
		this->data()[pos_ / s_bits_per_word_] &= ~(static_cast<size_t>(1u) << (pos_ % s_bits_per_word_));
		return *this;
	}

	hxbitset& flip(void) {
		size_t* d_ = this->data();
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			d_[i_] ^= ~static_cast<size_t>(0u);
		}
		return *this;
	}

	hxbitset& flip(size_t pos_) {
		hxassertmsg(pos_ < this->size(), "invalid_index %zu", pos_);
		this->data()[pos_ / s_bits_per_word_] ^= (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_));
		return *this;
	}

	bool operator==(const hxbitset& x_) const hxattr_nodiscard {
		hxassertmsg(this->num_words_() == x_.num_words_(), "size_mismatch");
		const size_t* d0_ = this->data();
		const size_t* d1_ = x_.data();
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			if(d0_[i_] != d1_[i_]) { return false; }
		}
		return true;
	}

	// In C++20 the compiler automatically generates operator!= from operator==.
#if HX_CPLUSPLUS < 202002L
	bool operator!=(const hxbitset& x_) const hxattr_nodiscard {
		return !(*this == x_);
	}
#endif

	void reserve(size_t bit_count_,
			hxsystem_allocator_t allocator_=hxsystem_allocator_current,
			hxalignment_t alignment_=HX_ALIGNMENT) {
		static_assert(bits_ == hxallocator_dynamic_capacity,
			"Dynamic capacity required for reserve.");
		this->reserve_storage_(words_for_bits_(bit_count_), allocator_, alignment_);
		this->m_bits_ = bit_count_;
	}

	void swap(hxbitset& x_) {
		static_assert(bits_ == hxallocator_dynamic_capacity,
			"Dynamic capacity required for hxbitset::swap.");
		hxswap_memcpy(*this, x_);
	}

private:
	static constexpr size_t s_bits_per_word_ = sizeof(size_t) * 8u;

	static constexpr size_t words_for_bits_(size_t n_) {
		return (n_ + s_bits_per_word_ - 1u) / s_bits_per_word_;
	}

	size_t num_words_(void) const {
		return words_for_bits_(this->size());
	}

	void zero_(void) {
		size_t* d_ = this->data();
		for(size_t i_ = this->num_words_(); i_-- != 0u; ) {
			d_[i_] = 0u;
		}
	}
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

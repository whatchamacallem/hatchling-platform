#pragma once
// SPDX-FileCopyrightText: © 2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hxallocator.hpp"

template<size_t bits_>
struct hxbitset_adapter_
	: public hxallocator<size_t, (bits_ + (sizeof(size_t) * 8u - 1u)) / (sizeof(size_t) * 8u)> {
	static constexpr size_t size_() { return bits_; }
};

template<>
struct hxbitset_adapter_<hxallocator_dynamic_capacity>
	: public hxallocator<size_t, hxallocator_dynamic_capacity> {
	size_t m_bits_ = 0u;
	size_t size_() const { return m_bits_; }
};

template<size_t bits_=hxallocator_dynamic_capacity>
class hxbitset : public hxbitset_adapter_<bits_> {
public:
	explicit hxbitset(void) {
		if(bits_ != hxallocator_dynamic_capacity) {
			this->zero_();
		}
	}

	explicit hxbitset(size_t bit_count_,
			hxsystem_allocator_t allocator_=hxsystem_allocator_current,
			hxalignment_t alignment_=HX_ALIGNMENT) {
		static_assert(bits_ == hxallocator_dynamic_capacity,
			"Dynamic capacity required for this constructor.");
		this->reserve(bit_count_, allocator_, alignment_);
		this->zero_();
	}

	explicit hxbitset(size_t value_) {
		static_assert(bits_ != hxallocator_dynamic_capacity,
			"Fixed capacity required for value constructor.");
		this->zero_();
		const size_t n_ = this->num_words_();
		if(n_ > 0u) {
			this->data()[0] = value_;
		}
		this->mask_last_word_();
	}

	hxbitset(const hxbitset& x_) {
		if(bits_ == hxallocator_dynamic_capacity && x_.size_() > 0u) {
			this->m_bits_ = x_.m_bits_;
			this->reserve_storage(words_for_bits_(x_.m_bits_));
		}
		const size_t words_ = this->num_words_();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			this->data()[i_] = x_.data()[i_];
		}
	}

	template<size_t bits_x_>
	hxbitset(const hxbitset<bits_x_>& x_) { // NOLINT
		if(bits_ == hxallocator_dynamic_capacity && x_.size() > 0u) {
			this->m_bits_ = x_.size();
			this->reserve_storage(words_for_bits_(x_.size()));
		}
		const size_t dst_words_ = this->num_words_();
		const size_t src_words_ = words_for_bits_(x_.size());
		const size_t copy_words_ = dst_words_ < src_words_ ? dst_words_ : src_words_;
		for(size_t i_ = 0u; i_ < copy_words_; ++i_) {
			this->data()[i_] = x_.data()[i_];
		}
		for(size_t i_ = copy_words_; i_ < dst_words_; ++i_) {
			this->data()[i_] = 0u;
		}
		this->mask_last_word_();
	}

	hxbitset(hxbitset&& x_) hxattr_noexcept {
		static_assert(bits_ == hxallocator_dynamic_capacity,
			"Dynamic capacity required for move construction.");
		::memcpy(static_cast<void*>(this), &x_, sizeof x_); // NOLINT
		::memset(static_cast<void*>(&x_), 0x00, sizeof x_); // NOLINT
	}

	void operator=(const hxbitset& x_) {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Assignment to self.");
		if(bits_ == hxallocator_dynamic_capacity) {
			this->m_bits_ = x_.m_bits_;
			if(x_.m_bits_ > 0u) {
				this->reserve_storage(words_for_bits_(x_.m_bits_));
			}
		}
		const size_t words_ = this->num_words_();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			this->data()[i_] = x_.data()[i_];
		}
	}

	void operator=(hxbitset&& x_) hxattr_noexcept {
		hxassertmsg(static_cast<const void*>(this) != static_cast<const void*>(&x_),
			"invalid_reference Assignment to self.");
		this->swap(x_);
	}

	bool operator[](size_t pos_) const {
		hxassertmsg(pos_ < this->size(), "invalid_index %zu", pos_);
		return (this->data()[pos_ / s_bits_per_word_] & (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_))) != 0u;
	}


	hxbitset& operator&=(const hxbitset& x_) {
		hxassertmsg(this->size() == x_.size(), "size_mismatch");
		const size_t words_ = this->num_words_();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			this->data()[i_] &= x_.data()[i_];
		}
		return *this;
	}

	hxbitset& operator|=(const hxbitset& x_) {
		hxassertmsg(this->size() == x_.size(), "size_mismatch");
		const size_t words_ = this->num_words_();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			this->data()[i_] |= x_.data()[i_];
		}
		return *this;
	}

	hxbitset& operator^=(const hxbitset& x_) {
		hxassertmsg(this->size() == x_.size(), "size_mismatch");
		const size_t words_ = this->num_words_();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			this->data()[i_] ^= x_.data()[i_];
		}
		return *this;
	}

	hxbitset operator~(void) const {
		hxbitset result_(*this);
		result_.flip();
		return result_;
	}

	hxbitset operator<<(size_t count_) const {
		hxbitset result_(*this);
		result_ <<= count_;
		return result_;
	}

	hxbitset& operator<<=(size_t count_) {
		const size_t n_ = this->size();
		if(count_ >= n_) {
			this->zero_();
			return *this;
		}
		const size_t word_shift_ = count_ / s_bits_per_word_;
		const size_t bit_shift_ = count_ % s_bits_per_word_;
		const size_t words_ = this->num_words_();
		size_t* d_ = this->data();
		if(bit_shift_ == 0u) {
			for(size_t i_ = words_; i_-- > word_shift_;) {
				d_[i_] = d_[i_ - word_shift_];
			}
		}
		else {
			const size_t rshift_ = s_bits_per_word_ - bit_shift_;
			for(size_t i_ = words_; i_-- > word_shift_;) {
				d_[i_] = d_[i_ - word_shift_] << bit_shift_;
				if(i_ - word_shift_ > 0u) {
					d_[i_] |= d_[i_ - word_shift_ - 1u] >> rshift_;
				}
			}
		}
		for(size_t i_ = 0u; i_ < word_shift_; ++i_) {
			d_[i_] = 0u;
		}
		this->mask_last_word_();
		return *this;
	}

	hxbitset operator>>(size_t count_) const {
		hxbitset result_(*this);
		result_ >>= count_;
		return result_;
	}

	hxbitset& operator>>=(size_t count_) {
		const size_t n_ = this->size();
		if(count_ >= n_) {
			this->zero_();
			return *this;
		}
		const size_t word_shift_ = count_ / s_bits_per_word_;
		const size_t bit_shift_ = count_ % s_bits_per_word_;
		const size_t words_ = this->num_words_();
		size_t* d_ = this->data();
		if(bit_shift_ == 0u) {
			for(size_t i_ = 0u; i_ + word_shift_ < words_; ++i_) {
				d_[i_] = d_[i_ + word_shift_];
			}
		}
		else {
			const size_t lshift_ = s_bits_per_word_ - bit_shift_;
			for(size_t i_ = 0u; i_ + word_shift_ < words_; ++i_) {
				d_[i_] = d_[i_ + word_shift_] >> bit_shift_;
				if(i_ + word_shift_ + 1u < words_) {
					d_[i_] |= d_[i_ + word_shift_ + 1u] << lshift_;
				}
			}
		}
		const size_t end_ = words_ - word_shift_;
		for(size_t i_ = end_; i_ < words_; ++i_) {
			d_[i_] = 0u;
		}
		return *this;
	}

	bool all(void) const hxattr_nodiscard {
		const size_t n_ = this->size();
		if(n_ == 0u) { return true; }
		const size_t full_words_ = n_ / s_bits_per_word_;
		const size_t* d_ = this->data();
		for(size_t i_ = 0u; i_ < full_words_; ++i_) {
			if(d_[i_] != ~static_cast<size_t>(0u)) { return false; }
		}
		const size_t rem_ = n_ % s_bits_per_word_;
		if(rem_ != 0u) {
			const size_t mask_ = (static_cast<size_t>(1u) << rem_) - 1u;
			return (d_[full_words_] & mask_) == mask_;
		}
		return true;
	}

	bool any(void) const hxattr_nodiscard {
		const size_t words_ = this->num_words_();
		const size_t* d_ = this->data();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			if(d_[i_] != 0u) { return true; }
		}
		return false;
	}

	bool none(void) const hxattr_nodiscard { return !this->any(); }

	size_t count(void) const hxattr_nodiscard {
		size_t count_ = 0u;
		const size_t words_ = this->num_words_();
		const size_t* d_ = this->data();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			size_t w_ = d_[i_];
			while(w_ != 0u) {
				count_ += static_cast<size_t>(w_ & 1u);
				w_ >>= 1u;
			}
		}
		return count_;
	}

	bool test(size_t pos_) const hxattr_nodiscard {
		hxassertmsg(pos_ < this->size(), "invalid_index %zu", pos_);
		return (this->data()[pos_ / s_bits_per_word_] & (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_))) != 0u;
	}

	size_t size(void) const hxattr_nodiscard { return this->size_(); }

	hxbitset& set(void) {
		const size_t words_ = this->num_words_();
		size_t* d_ = this->data();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			d_[i_] = ~static_cast<size_t>(0u);
		}
		this->mask_last_word_();
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
		const size_t words_ = this->num_words_();
		size_t* d_ = this->data();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			d_[i_] ^= ~static_cast<size_t>(0u);
		}
		this->mask_last_word_();
		return *this;
	}

	hxbitset& flip(size_t pos_) {
		hxassertmsg(pos_ < this->size(), "invalid_index %zu", pos_);
		this->data()[pos_ / s_bits_per_word_] ^= (static_cast<size_t>(1u) << (pos_ % s_bits_per_word_));
		return *this;
	}

	bool operator==(const hxbitset& x_) const hxattr_nodiscard {
		hxassertmsg(this->size() == x_.size(), "size_mismatch");
		const size_t words_ = this->num_words_();
		const size_t* d0_ = this->data();
		const size_t* d1_ = x_.data();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			if(d0_[i_] != d1_[i_]) { return false; }
		}
		return true;
	}

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
		this->reserve_storage(words_for_bits_(bit_count_), allocator_, alignment_);
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
		const size_t words_ = this->num_words_();
		size_t* d_ = this->data();
		for(size_t i_ = 0u; i_ < words_; ++i_) {
			d_[i_] = 0u;
		}
	}

	void mask_last_word_(void) {
		const size_t n_ = this->size();
		const size_t rem_ = n_ % s_bits_per_word_;
		if(rem_ != 0u) {
			const size_t words_ = this->num_words_();
			if(words_ > 0u) {
				this->data()[words_ - 1u] &= (static_cast<size_t>(1u) << rem_) - 1u;
			}
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

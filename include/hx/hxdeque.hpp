#pragma once
// SPDX-FileCopyrightText: © 2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"
#include "hxallocator.hpp"
#include "hxutility.h"

/// A fixed-capacity deque backed by a power-of-two ring buffer. All operations
/// are O(1). The capacity must be a power of two and greater than zero.
/// - `T` : The element type stored in the deque.
/// - `capacity` : Maximum element count or `hxallocator_dynamic_capacity` for dynamic storage.
template<typename T_, size_t capacity_=hxallocator_dynamic_capacity>
class hxdeque : public hxallocator<T_, capacity_> {
public:
	/// Constructs an empty hxdeque. When using static storage the capacity is
	/// fixed at compile time. Asserts that the capacity is a power of two
	/// and greater than zero.
	/// - `dynamic_capacity` : Element capacity for dynamic storage.
	explicit hxdeque(size_t dynamic_capacity_=0u)
		: m_mask_(0u)
		, m_head_(0u)
		, m_tail_(0u)
		, m_count_(0u)
	{
		hxassertrelease(dynamic_capacity_ == 0u || (dynamic_capacity_ & (dynamic_capacity_ - 1u)) == 0u,
			"invalid_capacity capacity must be a power of 2");

		if(dynamic_capacity_ != 0u) {
			this->reserve_storage_(dynamic_capacity_);
		}

		m_mask_ = this->capacity() - 1u;
	}

	/// Destroys all elements in the deque.
	~hxdeque(void) { clear(); }

	hxdeque(const hxdeque&) = delete;
	void operator=(const hxdeque&) = delete;

	/// Destroys all elements and resets the deque to empty without deallocating.
	void clear(void) {
		T_* const data_ = this->data();
		for(size_t i_ = 0u; i_ < m_count_; ++i_) {
			data_[(m_head_ + i_) & m_mask_].~T_();
		}
		m_head_ = 0u;
		m_tail_ = 0u;
		m_count_ = 0u;
	}

	/// Constructs an element in place at the back using forwarded arguments.
	/// Returns `false` if the deque is full or unallocated.
	/// - `args` : Arguments forwarded to `T`'s constructor.
	template<typename... args_t_>
	bool emplace_back(args_t_&&... args_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ >= this->capacity()) { return false; }
		T_* slot_ = this->data() + m_tail_;
		m_tail_ = (m_tail_ + 1u) & m_mask_; ++m_count_;
		::new(slot_) T_(hxforward<args_t_>(args_)...);
		return true;
	}

	/// Constructs an element in place at the front using forwarded arguments.
	/// Returns `false` if the deque is full or unallocated.
	/// - `args` : Arguments forwarded to `T`'s constructor.
	template<typename... args_t_>
	bool emplace_front(args_t_&&... args_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ >= this->capacity()) { return false; }
		m_head_ = (m_head_ + m_mask_) & m_mask_;
		T_* slot_ = this->data() + m_head_;
		++m_count_;
		::new(slot_) T_(hxforward<args_t_>(args_)...);
		return true;
	}

	/// Inserts `v` at the back. Returns `false` if the deque is full or
	/// unallocated.
	/// - `v` : Value to insert.
	bool push_back(const T_& v_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ >= this->capacity()) { return false; }
		::new(this->data() + m_tail_) T_(v_);
		m_tail_ = (m_tail_ + 1u) & m_mask_; ++m_count_;
		return true;
	}

	/// Inserts `v` at the front. Returns `false` if the deque is full or
	/// unallocated.
	/// - `v` : Value to insert.
	bool push_front(const T_& v_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ >= this->capacity()) { return false; }
		m_head_ = (m_head_ + m_mask_) & m_mask_;
		::new(this->data() + m_head_) T_(v_);
		++m_count_;
		return true;
	}

	/// Removes and destroys the front element, returning it in `out`. Returns
	/// `false` if empty. Asserts if unallocated.
	/// - `out` : Receives the removed element.
	bool pop_front(T_& out_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ == 0u) { return false; }
		T_* slot_ = this->data() + m_head_;
		m_head_ = (m_head_ + 1u) & m_mask_;
		out_ = hxmove(*slot_);
		slot_->~T_();
		--m_count_;
		return true;
	}

	/// Removes and destroys the back element, returning it in `out`. Returns
	/// `false` if empty. Asserts if unallocated.
	/// - `out` : Receives the removed element.
	bool pop_back(T_& out_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ == 0u) { return false; }
		m_tail_ = (m_tail_ + m_mask_) & m_mask_;
		T_* slot_ = this->data() + m_tail_;
		out_ = hxmove(*slot_);
		slot_->~T_();
		--m_count_;
		return true;
	}

	/// Returns a reference to the front element.
	T_& front(void) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		hxassertmsg(m_count_ > 0u, "empty_deque");
		return this->data()[m_head_];
	}

	/// Returns a const reference to the front element.
	const T_& front(void) const hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		hxassertmsg(m_count_ > 0u, "empty_deque");
		return this->data()[m_head_];
	}

	/// Returns a reference to the back element.
	T_& back(void) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		hxassertmsg(m_count_ > 0u, "empty_deque");
		return this->data()[(m_tail_ + m_mask_) & m_mask_];
	}

	/// Returns a const reference to the back element.
	const T_& back(void) const hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		hxassertmsg(m_count_ > 0u, "empty_deque");
		return this->data()[(m_tail_ + m_mask_) & m_mask_];
	}

	/// Returns a reference to the element at logical index `index`. Asserts if
	/// `index` is out of range or the deque is unallocated.
	/// - `index` : Zero-based index from the front.
	T_& operator[](size_t index_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		hxassertmsg(index_ < m_count_, "invalid_index %zu", index_);
		return this->data()[(m_head_ + index_) & m_mask_];
	}

	/// Returns a const reference to the element at logical index `index`.
	/// Asserts if `index` is out of range or the deque is unallocated.
	/// - `index` : Zero-based index from the front.
	const T_& operator[](size_t index_) const hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		hxassertmsg(index_ < m_count_, "invalid_index %zu", index_);
		return this->data()[(m_head_ + index_) & m_mask_];
	}

	/// Returns a reference to the element at logical index `index`. Asserts if
	/// `index` is out of range or the deque is unallocated.
	/// - `index` : Zero-based index from the front.
	T_& at(size_t index_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		hxassertmsg(index_ < m_count_, "invalid_index %zu", index_);
		return this->data()[(m_head_ + index_) & m_mask_];
	}

	/// Returns a const reference to the element at logical index `index`.
	/// Asserts if `index` is out of range or the deque is unallocated.
	/// - `index` : Zero-based index from the front.
	const T_& at(size_t index_) const hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		hxassertmsg(index_ < m_count_, "invalid_index %zu", index_);
		return this->data()[(m_head_ + index_) & m_mask_];
	}

	/// Returns the number of elements currently in the deque.
	size_t size(void) const hxattr_nodiscard { return m_count_; }

	/// Returns `true` if the deque contains no elements.
	bool empty(void) const hxattr_nodiscard { return m_count_ == 0u; }

	/// Returns `true` if the deque is at capacity.
	bool full(void) const hxattr_nodiscard { return m_count_ == this->capacity(); }

private:
	// This is raw underlying data and would not be what was expected.
	using hxallocator<T_, capacity_>::data;
	size_t m_mask_;
	size_t m_head_;
	size_t m_tail_;
	size_t m_count_;
};

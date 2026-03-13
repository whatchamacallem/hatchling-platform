#pragma once
// SPDX-FileCopyrightText: © 2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hxallocator.hpp"

/// A fixed-capacity deque backed by a power-of-two ring buffer. All operations
/// are O(1). The capacity must be a power of two and greater than zero.
/// - `T` : The element type stored in the deque.
/// - `capacity` : Maximum element count or `hxallocator_dynamic_capacity` for dynamic storage.
template<typename T_, size_t capacity_=hxallocator_dynamic_capacity>
class hxdeque : public hxallocator<T_, capacity_> {
public:
	/// Constructs an empty hxdeque. When using static storage the capacity is
	/// fixed at compile time. When using dynamic storage `reserve` is called
	/// with `dynamic_capacity`. Asserts that the capacity is a power of two
	/// and greater than zero.
	/// - `dynamic_capacity` : Element capacity for dynamic storage. Ignored for static storage.
	explicit hxdeque(size_t dynamic_capacity_=0u)
		: m_mask_(0u)
		, m_head_(0u)
		, m_tail_(0u)
		, m_count_(0u)
	{
		reserve(dynamic_capacity_);
	}

	hxdeque(const hxdeque&) = delete;
	void operator=(const hxdeque&) = delete;

	/// Allocates dynamic storage for `cap` elements. Asserts if a reallocation
	/// is requested. Asserts that `cap` is a power of two and greater than zero.
	/// - `cap` : Element capacity. Must be a power of two.
	void reserve(size_t dynamic_capacity_) {
		size_t current_ = this->capacity();
		if(current_ == 0u) {
			this->reserve_storage_(dynamic_capacity_);
			current_ = this->capacity();
		}
		hxassertmsg(current_ > 0u && (current_ & (current_ - 1u)) == 0u,
			"invalid_capacity capacity must be a power of two");
		m_mask_ = current_ - 1u;
	}

	/// Inserts `v` at the back. Asserts if the deque is full or unallocated.
	/// - `v` : Value to insert.
	bool push_back(const T_& v_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ >= this->capacity()) { return false; }
		this->data()[m_tail_ & m_mask_] = v_;
		++m_tail_; ++m_count_;
		return true;
	}

	/// Inserts `v` at the front. Asserts if the deque is full or unallocated.
	/// - `v` : Value to insert.
	bool push_front(const T_& v_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ >= this->capacity()) { return false; }
		this->data()[--m_head_ & m_mask_] = v_;
		++m_count_;
		return true;
	}

	/// Removes and returns the front element in `out`. Returns `false` if empty.
	/// - `out` : Receives the removed element.
	bool pop_front(T_& out_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ == 0u) { return false; }
		out_ = this->data()[m_head_++ & m_mask_];
		--m_count_;
		return true;
	}

	/// Removes and returns the back element in `out`. Returns `false` if empty.
	/// - `out` : Receives the removed element.
	bool pop_back(T_& out_) hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ == 0u) { return false; }
		out_ = this->data()[--m_tail_ & m_mask_];
		--m_count_;
		return true;
	}

	/// Copies the front element into `out` without removing it. Returns `false`
	/// if empty.
	/// - `out` : Receives the front element.
	bool peek_front(T_& out_) const hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ == 0u) { return false; }
		out_ = this->data()[m_head_ & m_mask_];
		return true;
	}

	/// Copies the back element into `out` without removing it. Returns `false`
	/// if empty.
	/// - `out` : Receives the back element.
	bool peek_back(T_& out_) const hxattr_nodiscard {
		hxassertmsg(this->capacity() > 0u, "unallocated_deque");
		if(m_count_ == 0u) { return false; }
		out_ = this->data()[(m_tail_ - 1u) & m_mask_];
		return true;
	}

	/// Returns the number of elements currently in the deque.
	size_t size(void) const hxattr_nodiscard { return m_count_; }

	/// Returns `true` if the deque contains no elements.
	bool empty(void) const hxattr_nodiscard { return m_count_ == 0u; }

	/// Returns `true` if the deque is at capacity.
	bool full(void) const hxattr_nodiscard { return m_count_ == this->capacity(); }

private:
	size_t m_mask_;
	size_t m_head_;
	size_t m_tail_;
	size_t m_count_;
};

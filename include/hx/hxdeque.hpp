#pragma once
// SPDX-FileCopyrightText: © 2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"

/// A fixed-capacity deque backed by a power-of-two ring buffer with no heap
/// allocation after construction. All operations are O(1). The capacity must
/// be a power of two and greater than zero.
/// - `value_t_` : The element type stored in the deque.
template<typename value_t_>
class hxdeque {
public:
	/// Constructs an empty hxdeque with the given `capacity`. Asserts that
	/// `capacity` is greater than zero and a power of two.
	/// - `capacity` : Maximum number of elements. Must be a power of two.
	explicit hxdeque(size_t capacity_)
		: m_data_(new value_t_[capacity_])
		, m_capacity_(capacity_)
		, m_mask_(capacity_ - 1u)
		, m_head_(0u)
		, m_tail_(0u)
		, m_count_(0u)
	{
		hxassertmsg(capacity_ > 0u && (capacity_ & m_mask_) == 0u,
			"invalid_capacity capacity must be a power of two");
	}

	/// Destroys the hxdeque and releases the backing buffer.
	~hxdeque(void) { delete[] m_data_; }

	hxdeque(const hxdeque&) = delete;
	void operator=(const hxdeque&) = delete;

	/// Inserts `v` at the back. Returns `false` if the deque is full.
	/// - `v` : Value to insert.
	bool push_back(const value_t_& v_) {
		if(m_count_ >= m_capacity_) { return false; }
		m_data_[m_tail_ & m_mask_] = v_;
		++m_tail_; ++m_count_;
		return true;
	}

	/// Inserts `v` at the front. Returns `false` if the deque is full.
	/// - `v` : Value to insert.
	bool push_front(const value_t_& v_) {
		if(m_count_ >= m_capacity_) { return false; }
		m_data_[--m_head_ & m_mask_] = v_;
		++m_count_;
		return true;
	}

	/// Removes and returns the front element in `out`. Returns `false` if empty.
	/// - `out` : Receives the removed element.
	bool pop_front(value_t_& out_) {
		if(m_count_ == 0u) { return false; }
		out_ = m_data_[m_head_++ & m_mask_];
		--m_count_;
		return true;
	}

	/// Removes and returns the back element in `out`. Returns `false` if empty.
	/// - `out` : Receives the removed element.
	bool pop_back(value_t_& out_) {
		if(m_count_ == 0u) { return false; }
		out_ = m_data_[--m_tail_ & m_mask_];
		--m_count_;
		return true;
	}

	/// Copies the front element into `out` without removing it. Returns `false`
	/// if empty.
	/// - `out` : Receives the front element.
	bool peek_front(value_t_& out_) const {
		if(m_count_ == 0u) { return false; }
		out_ = m_data_[m_head_ & m_mask_];
		return true;
	}

	/// Copies the back element into `out` without removing it. Returns `false`
	/// if empty.
	/// - `out` : Receives the back element.
	bool peek_back(value_t_& out_) const {
		if(m_count_ == 0u) { return false; }
		out_ = m_data_[(m_tail_ - 1u) & m_mask_];
		return true;
	}

	/// Returns the number of elements currently in the deque.
	size_t size(void) const hxattr_nodiscard { return m_count_; }

	/// Returns the maximum number of elements the deque can hold.
	size_t capacity(void) const hxattr_nodiscard { return m_capacity_; }

	/// Returns `true` if the deque contains no elements.
	bool empty(void) const hxattr_nodiscard { return m_count_ == 0u; }

	/// Returns `true` if the deque is at capacity.
	bool full(void) const hxattr_nodiscard { return m_count_ == m_capacity_; }

private:
	value_t_* m_data_;
	size_t m_capacity_;
	size_t m_mask_;
	size_t m_head_;
	size_t m_tail_;
	size_t m_count_;
};

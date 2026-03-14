#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxlist.hpp An intrusive doubly linked list. Nodes embed their
/// linkage by inheriting from `hxlist_node<derived_t>` and are owned by the
/// list, which calls the deleter on destruction.
///
/// For example:
/// ```cpp
///   struct example_t : public hxlist_node<example_t> {
///       example_t(int x) : value(x) { }
///       int value;
///   };
///
///   hxlist<example_t> list;
///   list.push_back(hxnew<example_t>());
///
///   for(example_t& n : list) {
///       ::printf("%d\n", n.value);
///   }
/// ```

#include "hxkey.hpp"
#include "hxutility.h"

/// \cond HIDDEN
#if HX_CPLUSPLUS >= 202002L
template<typename node_t_>
concept hxlist_concept_ =
	requires(node_t_& node_, const node_t_& const_node_) {
		sizeof(node_t_);
		{ node_.list_prev() = static_cast<node_t_*>(hxnull) } -> hxconvertible_to<node_t_*&>;
		{ const_node_.list_prev() } -> hxconvertible_to<node_t_*>;
		{ node_.list_next() = static_cast<node_t_*>(hxnull) } -> hxconvertible_to<node_t_*&>;
		{ const_node_.list_next() } -> hxconvertible_to<node_t_*>;
	};
#else
#define hxlist_concept_ typename
#endif
/// \endcond

/// Intrusive doubly linked list node base using the Curiously Recurring
/// Template Pattern. Derive from `hxlist_node<derived_t>` to make a type
/// linkable into an `hxlist`. Nodes default to unlinked on construction.
template<typename derived_t_>
class hxlist_node {
public:
	/// Constructs an unlinked node with both pointers set to null.
	hxlist_node(void) : m_list_prev_(hxnull), m_list_next_(hxnull) { }

	/// Returns the previous node pointer.
	derived_t_* list_prev(void) const { return m_list_prev_; }

	/// Returns a reference to the previous node pointer, allowing the list to
	/// update linkage.
	derived_t_*& list_prev(void) { return m_list_prev_; }

	/// Returns the next node pointer.
	derived_t_* list_next(void) const { return m_list_next_; }

	/// Returns a reference to the next node pointer, allowing the list to
	/// update linkage.
	derived_t_*& list_next(void) { return m_list_next_; }

private:
	hxlist_node(const hxlist_node&) = delete;
	void operator=(const hxlist_node&) = delete;

	derived_t_* m_list_prev_;
	derived_t_* m_list_next_;
};

/// An intrusive doubly linked list that takes ownership of nodes via a
/// `deleter_t` functor, defaulting to `hxdefault_delete`. Nodes must derive
/// from `hxlist_node` or otherwise expose `list_prev()` and `list_next()`
/// per `hxlist_concept_`. The destructor calls `clear()` which invokes the
/// deleter on all remaining nodes.
/// - `node_t` : The node type. Must satisfy `hxlist_concept_`.
/// - `deleter_t` : A callable that frees a node pointer. Defaults to `hxdefault_delete`.
template<hxlist_concept_ node_t_,
	typename deleter_t_=hxdefault_delete>
class hxlist {
public:
	using node_t = node_t_;

	/// Bidirectional iterator over const nodes. Incrementing past `end()` or
	/// decrementing past `begin()` is undefined.
	class const_iterator {
	public:
		const_iterator(node_t_* current_, node_t_* sentinel_)
			: m_current_node_(current_), m_sentinel_(sentinel_) { }

		/// Constructs a singular iterator that must not be incremented or dereferenced.
		const_iterator(void) : m_current_node_(hxnull), m_sentinel_(hxnull) { }

		/// Advances to the next node and returns this iterator. Asserts the
		/// iterator is not at `end()`.
		const_iterator& operator++(void);
		/// Post-increment: advances to the next node and returns the prior position.
		const_iterator operator++(int);
		/// Retreats to the previous node and returns this iterator.
		const_iterator& operator--(void);
		/// Post-decrement: retreats to the previous node and returns the prior position.
		const_iterator operator--(int);

		/// Returns `true` if both iterators point to the same node.
		bool operator==(const const_iterator& x_) const;
		/// Returns `true` if the iterators point to different nodes.
		bool operator!=(const const_iterator& x_) const;

		/// Returns a const reference to the current node.
		const node_t_& operator*(void) const { return *m_current_node_; }
		/// Returns a const pointer to the current node.
		const node_t_* operator->(void) const { return m_current_node_; }

	protected:
		/// \cond HIDDEN
		node_t_* m_current_node_;
		node_t_* m_sentinel_;
		/// \endcond
	};

	/// Bidirectional iterator over mutable nodes. Incrementing past `end()` or
	/// decrementing past `begin()` is undefined.
	class iterator : public const_iterator {
	public:
		iterator(node_t_* current_, node_t_* sentinel_)
			: const_iterator(current_, sentinel_) { }

		/// Constructs a singular iterator that must not be incremented or dereferenced.
		iterator(void) { }

		/// Advances to the next node and returns this iterator. Asserts the
		/// iterator is not at `end()`.
		iterator& operator++(void) { const_iterator::operator++(); return *this; }
		/// Post-increment: advances to the next node and returns the prior position.
		iterator operator++(int);
		/// Retreats to the previous node and returns this iterator.
		iterator& operator--(void) { const_iterator::operator--(); return *this; }
		/// Post-decrement: retreats to the previous node and returns the prior position.
		iterator operator--(int);

		/// Returns a mutable reference to the current node.
		node_t_& operator*(void) const { return *this->m_current_node_; }
		/// Returns a mutable pointer to the current node.
		node_t_* operator->(void) const { return this->m_current_node_; }
	};

	/// Constructs an empty list with the sentinel node linked to itself.
	explicit hxlist(void);

	/// Destroys the list by calling `clear()`, which invokes the deleter on
	/// every remaining node.
	~hxlist(void) { this->clear(); }

	/// Returns an iterator to the first node, or `end()` if the list is empty.
	iterator begin(void) { return iterator(m_head_.list_next(), sentinel_()); }
	/// Returns a const iterator to the first node, or `end()` if the list is empty.
	const_iterator begin(void) const;

	/// Returns an iterator to the sentinel, representing one past the last node.
	iterator end(void) { return iterator(sentinel_(), sentinel_()); }
	/// Returns a const iterator to the sentinel, representing one past the last node.
	const_iterator end(void) const;

	/// Returns a const iterator to the first node, or `cend()` if the list is empty.
	const_iterator cbegin(void) const { return begin(); }

	/// Returns a const iterator to the sentinel, representing one past the last node.
	const_iterator cend(void) const { return end(); }

	/// Returns the number of nodes currently in the list.
	size_t size(void) const { return m_size_; }

	/// Returns `true` if the list contains no nodes.
	bool empty(void) const { return m_head_.list_next() == sentinel_(); }

	/// Returns a pointer to the first node. The list must not be empty.
	node_t_* front(void);
	/// Returns a const pointer to the first node. The list must not be empty.
	const node_t_* front(void) const;

	/// Returns a pointer to the last node. The list must not be empty.
	node_t_* back(void);
	/// Returns a const pointer to the last node. The list must not be empty.
	const node_t_* back(void) const;

	/// Inserts `ptr` at the front of the list. `ptr` must not be null.
	void push_front(node_t_* ptr_);

	/// Inserts `ptr` at the back of the list. `ptr` must not be null.
	void push_back(node_t_* ptr_);

	/// Removes and returns the first node without invoking the deleter, or null
	/// if the list is empty.
	node_t_* pop_front(void);

	/// Removes and returns the last node without invoking the deleter, or null
	/// if the list is empty.
	node_t_* pop_back(void);

	/// Inserts `ptr` immediately before `pos`. Both `pos` and `ptr` must not be null.
	void insert_before(node_t_* pos_, node_t_* ptr_);

	/// Inserts `ptr` immediately after `pos`. Both `pos` and `ptr` must not be null.
	void insert_after(node_t_* pos_, node_t_* ptr_);

	/// Unlinks `ptr` from the list and returns it without invoking the deleter.
	/// `ptr` must not be null.
	node_t_* extract(node_t_* ptr_);

	/// Unlinks `ptr` from the list and invokes `deleter` on it. If `deleter`
	/// evaluates to false it is not called.
	/// - `deleter` : Override deleter callable. Called only if it evaluates to true.
	template<typename deleter_override_t_>
	void erase(node_t_* ptr_, const deleter_override_t_& deleter_);

	/// Unlinks and deletes `ptr` using the list's default `deleter_t`.
	void erase(node_t_* ptr_) { this->erase(ptr_, deleter_t_()); }

	/// Removes all nodes, invoking `deleter` on each. If `deleter` evaluates to
	/// false nodes are unlinked but not freed.
	/// - `deleter` : Override deleter callable. Called only if it evaluates to true.
	template<typename deleter_override_t_>
	void clear(const deleter_override_t_& deleter_);

	/// Removes all nodes using the list's default `deleter_t`.
	void clear(void) { this->clear(deleter_t_()); }

	/// Resets the list to empty without invoking the deleter on any node.
	/// Ownership of all nodes is abandoned; use only when nodes are managed
	/// elsewhere or have already been freed.
	void release_all(void);

private:
	hxlist(const hxlist&) = delete;

	node_t_* sentinel_(void) { return reinterpret_cast<node_t_*>(&m_head_); }
	const node_t_* sentinel_(void) const;
	void extract_(node_t_* ptr_);

	size_t m_size_;
	hxlist_node<node_t_> m_head_;
};

// const_iterator

template<hxlist_concept_ node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::const_iterator::operator++(void)
	-> const_iterator& {
	hxassertmsg(m_current_node_ != m_sentinel_, "invalid_iterator");
	m_current_node_ = m_current_node_->list_next();
	return *this;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::const_iterator::operator++(int)
	-> const_iterator {
	const_iterator t_(*this);
	operator++();
	return t_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::const_iterator::operator--(void)
	-> const_iterator& {
	m_current_node_ = m_current_node_->list_prev();
	return *this;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::const_iterator::operator--(int)
	-> const_iterator {
	const_iterator t_(*this);
	operator--();
	return t_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline bool hxlist<node_t_, deleter_t_>::const_iterator::operator==(
	const const_iterator& x_) const {
	return m_current_node_ == x_.m_current_node_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline bool hxlist<node_t_, deleter_t_>::const_iterator::operator!=(
	const const_iterator& x_) const {
	return m_current_node_ != x_.m_current_node_;
}

// iterator

template<hxlist_concept_ node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::iterator::operator++(int) -> iterator {
	iterator t_(*this);
	const_iterator::operator++();
	return t_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::iterator::operator--(int) -> iterator {
	iterator t_(*this);
	const_iterator::operator--();
	return t_;
}

// hxlist

template<hxlist_concept_ node_t_, typename deleter_t_>
inline hxlist<node_t_, deleter_t_>::hxlist(void) {
	m_size_ = 0u;
	m_head_.list_prev() = sentinel_();
	m_head_.list_next() = sentinel_();
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::begin(void) const -> const_iterator {
	return const_iterator(m_head_.list_next(), sentinel_());
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::end(void) const -> const_iterator {
	return const_iterator(sentinel_(), sentinel_());
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline node_t_* hxlist<node_t_, deleter_t_>::front(void) {
	hxassertmsg(!empty(), "empty_list");
	return m_head_.list_next();
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline const node_t_* hxlist<node_t_, deleter_t_>::front(void) const {
	hxassertmsg(!empty(), "empty_list");
	return m_head_.list_next();
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline node_t_* hxlist<node_t_, deleter_t_>::back(void) {
	hxassertmsg(!empty(), "empty_list");
	return m_head_.list_prev();
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline const node_t_* hxlist<node_t_, deleter_t_>::back(void) const {
	hxassertmsg(!empty(), "empty_list");
	return m_head_.list_prev();
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::push_front(node_t_* ptr_) {
	hxassertmsg(ptr_ != hxnull, "null_node");
	node_t_* old_first_ = m_head_.list_next();
	ptr_->list_next() = old_first_;
	ptr_->list_prev() = sentinel_();
	old_first_->list_prev() = ptr_;
	m_head_.list_next() = ptr_;
	++m_size_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::push_back(node_t_* ptr_) {
	hxassertmsg(ptr_ != hxnull, "null_node");
	node_t_* old_last_ = m_head_.list_prev();
	ptr_->list_prev() = old_last_;
	ptr_->list_next() = sentinel_();
	old_last_->list_next() = ptr_;
	m_head_.list_prev() = ptr_;
	++m_size_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline node_t_* hxlist<node_t_, deleter_t_>::pop_front(void) {
	if(empty()) {
		return hxnull;
	}
	node_t_* ptr_ = m_head_.list_next();
	extract_(ptr_);
	return ptr_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline node_t_* hxlist<node_t_, deleter_t_>::pop_back(void) {
	if(empty()) {
		return hxnull;
	}
	node_t_* ptr_ = m_head_.list_prev();
	extract_(ptr_);
	return ptr_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::insert_before(
	node_t_* pos_, node_t_* ptr_) {
	hxassertmsg(pos_ != hxnull && ptr_ != hxnull, "null_node");
	node_t_* prev_ = pos_->list_prev();
	ptr_->list_prev() = prev_;
	ptr_->list_next() = pos_;
	prev_->list_next() = ptr_;
	pos_->list_prev() = ptr_;
	++m_size_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::insert_after(
	node_t_* pos_, node_t_* ptr_) {
	hxassertmsg(pos_ != hxnull && ptr_ != hxnull, "null_node");
	node_t_* next_ = pos_->list_next();
	ptr_->list_next() = next_;
	ptr_->list_prev() = pos_;
	next_->list_prev() = ptr_;
	pos_->list_next() = ptr_;
	++m_size_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline node_t_* hxlist<node_t_, deleter_t_>::extract(node_t_* ptr_) {
	hxassertmsg(ptr_ != hxnull, "null_node");
	extract_(ptr_);
	return ptr_;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
template<typename deleter_override_t_>
inline void hxlist<node_t_, deleter_t_>::erase(
	node_t_* ptr_, const deleter_override_t_& deleter_) {
	extract_(ptr_);
	if(deleter_) {
		deleter_(ptr_);
	}
}

template<hxlist_concept_ node_t_, typename deleter_t_>
template<typename deleter_override_t_>
inline void hxlist<node_t_, deleter_t_>::clear(
	const deleter_override_t_& deleter_) {
	if(m_size_ != 0u) {
		if(deleter_) {
			node_t_* n_ = m_head_.list_next();
			while(n_ != sentinel_()) {
				node_t_* next_ = n_->list_next();
				deleter_(n_);
				n_ = next_;
			}
			release_all();
		}
		else {
			release_all();
		}
	}
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::release_all(void) {
	m_head_.list_prev() = sentinel_();
	m_head_.list_next() = sentinel_();
	m_size_ = 0u;
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::sentinel_(void) const -> const node_t_* {
	return reinterpret_cast<const node_t_*>(&m_head_);
}

template<hxlist_concept_ node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::extract_(node_t_* ptr_) {
	ptr_->list_prev()->list_next() = ptr_->list_next();
	ptr_->list_next()->list_prev() = ptr_->list_prev();
	--m_size_;
}

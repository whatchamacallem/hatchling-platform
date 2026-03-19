#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxlist.hpp An intrusive doubly linked list. Nodes embed their
/// linkage by inheriting from `hxlist_node` and are owned by the list, which
/// calls the deleter on destruction. The list is parameterized on `node_t`,
/// which must derive from `hxlist_node`. All public API returns are downcast to
/// `node_t*`, so inserting further subclasses of `node_t` heterogeneously is
/// supported: recover the concrete type with an additional `static_cast`.
///
/// For example:
/// ```cpp
///   struct example_t : public hxlist_node {
///       example_t(int x) : value(x) { }
///       int value;
///   };
///
///   hxlist<example_t> list;
///   list.push_back(hxnew<example_t>(7));
///
///   for(example_t& n : list) {
///       ::printf("%d\n", n.value);
///   }
/// ```

#include "hxkey.hpp"
#include "hxutility.h"

/// Intrusive doubly linked list node base. Derive from `hxlist_node` to make a
/// type linkable into an `hxlist`. Nodes default to unlinked on construction.
/// Copy and move construction and assignment produce or leave a fresh unlinked
/// node so that subclasses may implement the standard operators naturally. All
/// four assert that the source node is not currently linked.
class hxlist_node {
public:
	/// Constructs an unlinked node with both pointers set to null.
	hxlist_node(void) : m_list_prev_(hxnull), m_list_next_(hxnull) { }

	/// Constructs an unlinked node. Asserts the source is not linked.
	hxlist_node(const hxlist_node& src_) : hxlist_node() { }

	/// Constructs an unlinked node. Asserts the source is not linked.
	hxlist_node(hxlist_node&& src_) : hxlist_node() {
		hxassertmsg(src_.m_list_prev_ == hxnull, "move of linked node");
	}

	/// Assigns nothing. List linkage of either node is not affected. Asserts
	/// the source is not linked.
	hxlist_node& operator=(const hxlist_node& src_) { }

	/// Assigns nothing. List linkage of either node is not affected. Asserts
	/// the source is not linked.
	hxlist_node& operator=(hxlist_node&& src_) {
		hxassertmsg(src_.m_list_prev_ == hxnull, "move of linked node");
		return *this;
	}

private:
	template<typename, typename> friend class hxlist;

	hxlist_node* list_prev_(void) const { return m_list_prev_; }
	hxlist_node*& list_prev_(void) { return m_list_prev_; }
	hxlist_node* list_next_(void) const { return m_list_next_; }
	hxlist_node*& list_next_(void) { return m_list_next_; }

	hxlist_node* m_list_prev_;
	hxlist_node* m_list_next_;
};

/// An intrusive doubly linked list that takes ownership of nodes via a
/// `deleter_t` functor, defaulting to `hxdefault_delete`. `node_t` must derive
/// from `hxlist_node`. The destructor calls `clear()` which invokes the deleter
/// on all remaining nodes. Subclasses of `node_t` may be inserted
/// heterogeneously. Recover the concrete type with `static_cast`.
/// - `node_t` : The node type. Must derive from `hxlist_node`.
/// - `deleter_t` : A callable that frees a node pointer. Defaults to `hxdefault_delete`.
template<typename node_t_, typename deleter_t_=hxdefault_delete>
class hxlist {
public:
	using node_t = node_t_;

	/// Bidirectional iterator over const nodes. Incrementing past `end()` or
	/// decrementing past `begin()` is undefined.
	class const_iterator {
	public:
		const_iterator(hxlist_node* current_, hxlist_node* sentinel_)
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
		const node_t_& operator*(void) const { return *static_cast<const node_t_*>(m_current_node_); }
		/// Returns a const pointer to the current node.
		const node_t_* operator->(void) const { return static_cast<const node_t_*>(m_current_node_); }

	protected:
		/// \cond HIDDEN
		hxlist_node* m_current_node_;
		hxlist_node* m_sentinel_;
		/// \endcond
	};

	/// Bidirectional iterator over mutable nodes. Incrementing past `end()` or
	/// decrementing past `begin()` is undefined.
	class iterator : public const_iterator {
	public:
		iterator(hxlist_node* current_, hxlist_node* sentinel_)
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
		node_t_& operator*(void) const { return *static_cast<node_t_*>(this->m_current_node_); }
		/// Returns a mutable pointer to the current node.
		node_t_* operator->(void) const { return static_cast<node_t_*>(this->m_current_node_); }
	};

	/// Constructs an empty list with the sentinel node linked to itself.
	explicit hxlist(void);

	/// Destroys the list by calling `clear()`, which invokes the deleter on
	/// every remaining node.
	~hxlist(void) { this->clear(); }

	/// Returns a pointer to the last node. The list must not be empty.
	hxattr_nodiscard node_t_* back(void);
	/// Returns a const pointer to the last node. The list must not be empty.
	hxattr_nodiscard const node_t_* back(void) const;

	/// Returns an iterator to the first node, or `end()` if the list is empty.
	iterator begin(void) { return iterator(m_head_.list_next_(), sentinel_()); }
	/// Returns a const iterator to the first node, or `end()` if the list is empty.
	const_iterator begin(void) const;

	/// Returns a const iterator to the first node, or `cend()` if the list is empty.
	const_iterator cbegin(void) const { return begin(); }

	/// Returns a const iterator to the sentinel, representing one past the last node.
	const_iterator cend(void) const { return end(); }

	/// Removes all nodes, invoking `deleter` on each. If `deleter` evaluates to
	/// false nodes are unlinked but not freed.
	/// - `deleter` : Override deleter callable. Called only if it evaluates to true.
	template<typename deleter_override_t_>
	void clear(const deleter_override_t_& deleter_);

	/// Removes all nodes using the list's default `deleter_t`.
	void clear(void) { this->clear(deleter_t_()); }

	/// Returns `true` if the list contains no nodes.
	hxattr_nodiscard bool empty(void) const { return m_head_.list_next_() == sentinel_(); }

	/// Returns an iterator to the sentinel, representing one past the last node.
	iterator end(void) { return iterator(sentinel_(), sentinel_()); }
	/// Returns a const iterator to the sentinel, representing one past the last node.
	const_iterator end(void) const;

	/// Unlinks `ptr` from the list and invokes `deleter` on it. If `deleter`
	/// evaluates to false it is not called.
	/// - `deleter` : Override deleter callable. Called only if it evaluates to true.
	template<typename deleter_override_t_>
	void erase(node_t_* ptr_, const deleter_override_t_& deleter_);

	/// Unlinks and deletes `ptr` using the list's default `deleter_t`.
	void erase(node_t_* ptr_) { this->erase(ptr_, deleter_t_()); }

	/// Unlinks `ptr` from the list and returns it without invoking the deleter.
	/// `ptr` must not be null.
	node_t_* extract(node_t_* ptr_);

	/// Returns a pointer to the first node. The list must not be empty.
	hxattr_nodiscard node_t_* front(void);
	/// Returns a const pointer to the first node. The list must not be empty.
	hxattr_nodiscard const node_t_* front(void) const;

	/// Inserts `ptr` immediately after `pos`. Both `pos` and `ptr` must not be null.
	void insert_after(node_t_* pos_, node_t_* ptr_);

	/// Inserts `ptr` immediately before `pos`. Both `pos` and `ptr` must not be null.
	void insert_before(node_t_* pos_, node_t_* ptr_);

	/// Removes and returns the last node without invoking the deleter, or null
	/// if the list is empty.
	node_t_* pop_back(void);

	/// Removes and returns the first node without invoking the deleter, or null
	/// if the list is empty.
	node_t_* pop_front(void);

	/// Inserts `ptr` at the back of the list. `ptr` must not be null.
	void push_back(node_t_* ptr_);

	/// Inserts `ptr` at the front of the list. `ptr` must not be null.
	void push_front(node_t_* ptr_);

	/// Resets the list to empty without invoking the deleter on any node.
	/// Ownership of all nodes is abandoned. Use only when nodes are managed
	/// elsewhere or have already been freed.
	void release_all(void);

	/// Returns the number of nodes currently in the list.
	hxattr_nodiscard size_t size(void) const { return m_size_; }

private:
	hxlist(const hxlist&) = delete;

	hxlist_node* sentinel_(void) { return &m_head_; }
	const hxlist_node* sentinel_(void) const { return &m_head_; }
	void extract_(hxlist_node* ptr_);

	size_t m_size_;
	hxlist_node m_head_;
};

// const_iterator

template<typename node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::const_iterator::operator++(void)
	-> const_iterator& {
	hxassertmsg(m_current_node_ != m_sentinel_, "invalid_iterator");
	m_current_node_ = m_current_node_->list_next_();
	return *this;
}

template<typename node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::const_iterator::operator++(int)
	-> const_iterator {
	const_iterator t_(*this);
	operator++();
	return t_;
}

template<typename node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::const_iterator::operator--(void)
	-> const_iterator& {
	m_current_node_ = m_current_node_->list_prev_();
	return *this;
}

template<typename node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::const_iterator::operator--(int)
	-> const_iterator {
	const_iterator t_(*this);
	operator--();
	return t_;
}

template<typename node_t_, typename deleter_t_>
inline bool hxlist<node_t_, deleter_t_>::const_iterator::operator==(
	const const_iterator& x_) const {
	return m_current_node_ == x_.m_current_node_;
}

template<typename node_t_, typename deleter_t_>
inline bool hxlist<node_t_, deleter_t_>::const_iterator::operator!=(
	const const_iterator& x_) const {
	return m_current_node_ != x_.m_current_node_;
}

// iterator

template<typename node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::iterator::operator++(int) -> iterator {
	iterator t_(*this);
	const_iterator::operator++();
	return t_;
}

template<typename node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::iterator::operator--(int) -> iterator {
	iterator t_(*this);
	const_iterator::operator--();
	return t_;
}

// hxlist

template<typename node_t_, typename deleter_t_>
inline hxlist<node_t_, deleter_t_>::hxlist(void) {
	m_size_ = 0u;
	m_head_.list_prev_() = sentinel_();
	m_head_.list_next_() = sentinel_();
}

template<typename node_t_, typename deleter_t_>
inline node_t_* hxlist<node_t_, deleter_t_>::back(void) {
	hxassertmsg(!empty(), "empty_list");
	return static_cast<node_t_*>(m_head_.list_prev_());
}

template<typename node_t_, typename deleter_t_>
inline const node_t_* hxlist<node_t_, deleter_t_>::back(void) const {
	hxassertmsg(!empty(), "empty_list");
	return static_cast<const node_t_*>(m_head_.list_prev_());
}

template<typename node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::begin(void) const -> const_iterator {
	return const_iterator(m_head_.list_next_(), sentinel_());
}

template<typename node_t_, typename deleter_t_>
inline auto hxlist<node_t_, deleter_t_>::end(void) const -> const_iterator {
	return const_iterator(sentinel_(), sentinel_());
}

template<typename node_t_, typename deleter_t_>
template<typename deleter_override_t_>
inline void hxlist<node_t_, deleter_t_>::clear(
	const deleter_override_t_& deleter_) {
	if(m_size_ != 0u) {
		if(deleter_) {
			hxlist_node* n_ = m_head_.list_next_();
			while(n_ != sentinel_()) {
				hxlist_node* next_ = n_->list_next_();
				deleter_(static_cast<node_t_*>(n_));
				n_ = next_;
			}
			release_all();
		}
		else {
			release_all();
		}
	}
}

template<typename node_t_, typename deleter_t_>
template<typename deleter_override_t_>
inline void hxlist<node_t_, deleter_t_>::erase(
	node_t_* ptr_, const deleter_override_t_& deleter_) {
	extract_(ptr_);
	if(deleter_) {
		deleter_(ptr_);
	}
}

template<typename node_t_, typename deleter_t_>
inline node_t_* hxlist<node_t_, deleter_t_>::extract(node_t_* ptr_) {
	hxassertmsg(ptr_ != hxnull, "null_node");
	extract_(ptr_);
	return ptr_;
}

template<typename node_t_, typename deleter_t_>
inline node_t_* hxlist<node_t_, deleter_t_>::front(void) {
	hxassertmsg(!empty(), "empty_list");
	return static_cast<node_t_*>(m_head_.list_next_());
}

template<typename node_t_, typename deleter_t_>
inline const node_t_* hxlist<node_t_, deleter_t_>::front(void) const {
	hxassertmsg(!empty(), "empty_list");
	return static_cast<const node_t_*>(m_head_.list_next_());
}

template<typename node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::insert_after(
	node_t_* pos_, node_t_* ptr_) {
	hxassertmsg(pos_ != hxnull && ptr_ != hxnull, "insert_after null node");
	hxassertmsg(ptr_->m_list_prev_ == hxnull, "insert_after already linked");
	hxlist_node* next_ = pos_->list_next_();
	ptr_->list_next_() = next_;
	ptr_->list_prev_() = pos_;
	next_->list_prev_() = ptr_;
	pos_->list_next_() = ptr_;
	++m_size_;
}

template<typename node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::insert_before(
	node_t_* pos_, node_t_* ptr_) {
	hxassertmsg(pos_ != hxnull && ptr_ != hxnull, "insert_before null node");
	hxassertmsg(ptr_->m_list_prev_ == hxnull, "insert_before already linked");
	hxlist_node* prev_ = pos_->list_prev_();
	ptr_->list_prev_() = prev_;
	ptr_->list_next_() = pos_;
	prev_->list_next_() = ptr_;
	pos_->list_prev_() = ptr_;
	++m_size_;
}

template<typename node_t_, typename deleter_t_>
inline node_t_* hxlist<node_t_, deleter_t_>::pop_back(void) {
	if(empty()) {
		return hxnull;
	}
	node_t_* ptr_ = static_cast<node_t_*>(m_head_.list_prev_());
	extract_(ptr_);
	return ptr_;
}

template<typename node_t_, typename deleter_t_>
inline node_t_* hxlist<node_t_, deleter_t_>::pop_front(void) {
	if(empty()) {
		return hxnull;
	}
	node_t_* ptr_ = static_cast<node_t_*>(m_head_.list_next_());
	extract_(ptr_);
	return ptr_;
}

template<typename node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::push_back(node_t_* ptr_) {
	hxassertmsg(ptr_ != hxnull, "push_back null node");
	hxassertmsg(ptr_->m_list_prev_ == hxnull, "push_back already linked");
	hxlist_node* old_last_ = m_head_.list_prev_();
	ptr_->list_prev_() = old_last_;
	ptr_->list_next_() = sentinel_();
	old_last_->list_next_() = ptr_;
	m_head_.list_prev_() = ptr_;
	++m_size_;
}

template<typename node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::push_front(node_t_* ptr_) {
	hxassertmsg(ptr_ != hxnull, "push_front null node");
	hxassertmsg(ptr_->m_list_prev_ == hxnull, "push_front already linked");
	hxlist_node* old_first_ = m_head_.list_next_();
	ptr_->list_next_() = old_first_;
	ptr_->list_prev_() = sentinel_();
	old_first_->list_prev_() = ptr_;
	m_head_.list_next_() = ptr_;
	++m_size_;
}

template<typename node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::release_all(void) {
	m_head_.list_prev_() = sentinel_();
	m_head_.list_next_() = sentinel_();
	m_size_ = 0u;
}

template<typename node_t_, typename deleter_t_>
inline void hxlist<node_t_, deleter_t_>::extract_(hxlist_node* ptr_) {
	ptr_->list_prev_()->list_next_() = ptr_->list_next_();
	ptr_->list_next_()->list_prev_() = ptr_->list_prev_();
	--m_size_;
}

#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxrbtree.hpp An intrusive red-black tree based on the Linux kernel
/// rb-tree algorithms. Nodes embed their linkage by inheriting from
/// `hxrbtree_node` and are owned by the tree, which calls the deleter on
/// destruction. The tree is parameterized on `node_t`, which must derive from
/// `hxrbtree_node`. All public API returns are downcast to `node_t*`, so
/// inserting further subclasses of `node_t` heterogeneously is supported:
/// recover the concrete type with an additional `static_cast`.
///
/// The node color is stored in the low bit of the parent pointer, matching the
/// Linux kernel `rb_node` layout. `RB_RED` is `0` and `RB_BLACK` is `1`.
///
/// `compare_t` is a strict less-than predicate called as `compare_t()(a, b)`
/// returning `true` when `a < b`. The default `compare_t` is
/// `hxkey_less`. Each traversal step calls the predicate twice: once as
/// `compare_t()(node, key)` to test whether to go right, and once as
/// `compare_t()(key, node)` to test whether to go left. Equality is the
/// remaining case. Custom functors are recommended for complex key types.
///
/// The generated assembly should be ideal for common use cases. Explicit support
/// for the spaceship operator `<=>` is easy enough to add.
///
/// When `multi_t` is `false` (set or map semantics), `insert` returns the
/// existing node on a key collision without inserting the new node. The caller
/// detects a collision by comparing the return value to the argument. When
/// `multi_t` is `true` (multiset or multimap semantics), duplicate keys are
/// always inserted after existing equal nodes.
///
/// Whether the tree functions as a set, multiset, map, or multimap is
/// determined entirely by what data `node_t` carries and the value of
/// `multi_t`. The tree itself has no knowledge of mapped values.
///
/// For example:
/// ```cpp
///   struct example_t : public hxrbtree_node {
///       example_t(int k) : key(k) { }
///       int key;
///   };
///
///   struct example_compare_t {
///       bool operator()(const example_t& a, const example_t& b) const {
///           return a.key < b.key;
///       }
///   };
///
///   hxrbtree<example_t, example_t, example_compare_t> tree;
///   tree.insert(hxnew<example_t>(7));
///
///   for(example_t& n : tree) {
///       ::printf("%d\n", n.key);
///   }
/// ```

#include "hxkey.hpp"
#include "hxmemory_manager.h"
#include "hxutility.h"

/// Intrusive red-black tree node base. Derive from `hxrbtree_node` to make a
/// type linkable into an `hxrbtree`. Nodes default to unlinked on construction.
/// The node color is stored in the low bit of `m_rb_parent_color_`, requiring
/// at least two-byte alignment, which is verified by a `static_assert`.
class hxrbtree_node {
public:
	/// Constructs an unlinked node with all pointers and color set to zero.
	hxrbtree_node(void) : m_rb_parent_color_(0u), m_rb_right_(hxnull), m_rb_left_(hxnull) { }

private:
	template<typename, typename, typename, bool, typename> friend class hxrbtree;

	hxrbtree_node(const hxrbtree_node&) = delete;
	void operator=(const hxrbtree_node&) = delete;

	hxrbtree_node* rb_parent_(void) const;
	void rb_set_parent_(hxrbtree_node* parent_);
	int rb_color_(void) const;
	void rb_set_color_(int color_);
	void rb_set_parent_color_(hxrbtree_node* parent_, int color_);
	bool rb_is_red_(void) const;
	bool rb_is_black_(void) const;

	uintptr_t m_rb_parent_color_;
	hxrbtree_node* m_rb_right_;
	hxrbtree_node* m_rb_left_;
};

static_assert(alignof(hxrbtree_node) >= 2, "hxrbtree_node alignment insufficient for color bit");

/// An intrusive red-black tree that takes ownership of nodes via a `deleter_t`
/// functor, defaulting to `hxdefault_delete`. `node_t` must derive from
/// `hxrbtree_node`. The destructor calls `clear()` which invokes the deleter
/// on all remaining nodes. Subclasses of `node_t` may be inserted
/// heterogeneously. Recover the concrete type with `static_cast`.
///
/// Iteration is in ascending key order. `front()` returns the minimum node and
/// `back()` returns the maximum node.
///
/// - `node_t` : The node type. Must derive from `hxrbtree_node`.
/// - `key_t` : The key type used by `find`, `lower_bound`, and `upper_bound`.
///   Defaults to `node_t`.
/// - `compare_t` : A strict less-than predicate with signature
///   `bool(const a&, const b&)`. Defaults to `hxrbtree_less`.
/// - `multi_t` : When `false`, duplicate keys are rejected by `insert` and the
///   existing node is returned. When `true`, duplicates are allowed. Defaults
///   to `false`.
/// - `deleter_t` : A callable that frees a node pointer. Defaults to
///   `hxdefault_delete`.
template<
	typename node_t_,
	typename key_t_       = node_t_,
	typename compare_t_   = hxkey_less<node_t_, key_t_>,
	bool     multi_t_     = false,
	typename deleter_t_   = hxdefault_delete>
class hxrbtree {
public:
	using node_t    = node_t_;
	using key_t     = key_t_;
	using compare_t = compare_t_;
	using multi_t   = multi_t_;
	using deleter_t = deleter_t_;

	/// Bidirectional iterator over const nodes in ascending key order.
	/// Incrementing past `end()` or decrementing past `begin()` is undefined.
	class const_iterator {
	public:
		const_iterator(hxrbtree_node* current_, const hxrbtree* tree_)
			: m_current_node_(current_), m_tree_(tree_) { }

		/// Constructs a singular iterator that must not be incremented or dereferenced.
		const_iterator(void) : m_current_node_(hxnull), m_tree_(hxnull) { }

		/// Advances to the in-order successor and returns this iterator. Asserts
		/// the iterator is not at `end()`.
		const_iterator& operator++(void);
		/// Post-increment: advances to the in-order successor and returns the prior position.
		const_iterator  operator++(int);
		/// Retreats to the in-order predecessor and returns this iterator.
		/// Decrementing `end()` yields the maximum node.
		const_iterator& operator--(void);
		/// Post-decrement: retreats to the in-order predecessor and returns the prior position.
		const_iterator  operator--(int);

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
		hxrbtree_node*   m_current_node_;
		const hxrbtree*  m_tree_;
		/// \endcond
	};

	/// Bidirectional iterator over mutable nodes in ascending key order.
	/// Incrementing past `end()` or decrementing past `begin()` is undefined.
	class iterator : public const_iterator {
	public:
		iterator(hxrbtree_node* current_, const hxrbtree* tree_)
			: const_iterator(current_, tree_) { }

		/// Constructs a singular iterator that must not be incremented or dereferenced.
		iterator(void) { }

		/// Advances to the in-order successor and returns this iterator.
		iterator& operator++(void) { const_iterator::operator++(); return *this; }
		/// Post-increment: advances to the in-order successor and returns the prior position.
		iterator  operator++(int);
		/// Retreats to the in-order predecessor and returns this iterator.
		iterator& operator--(void) { const_iterator::operator--(); return *this; }
		/// Post-decrement: retreats to the in-order predecessor and returns the prior position.
		iterator  operator--(int);

		/// Returns a mutable reference to the current node.
		node_t_& operator*(void) const { return *static_cast<node_t_*>(this->m_current_node_); }
		/// Returns a mutable pointer to the current node.
		node_t_* operator->(void) const { return static_cast<node_t_*>(this->m_current_node_); }
	};

	/// Constructs an empty tree.
	explicit hxrbtree(void);

	/// Destroys the tree by calling `clear()`, which invokes the deleter on
	/// every remaining node.
	~hxrbtree(void) { this->clear(); }

	/// Returns a pointer to the maximum (rightmost) node. The tree must not be empty.
	hxattr_nodiscard node_t_* back(void);
	/// Returns a const pointer to the maximum (rightmost) node. The tree must not be empty.
	hxattr_nodiscard const node_t_* back(void) const;

	/// Returns an iterator to the minimum node, or `end()` if the tree is empty.
	iterator begin(void) { return iterator(rb_first_(m_root_), this); }
	/// Returns a const iterator to the minimum node, or `end()` if the tree is empty.
	const_iterator begin(void) const;

	/// Returns a const iterator to the minimum node, or `cend()` if the tree is empty.
	const_iterator cbegin(void) const { return begin(); }
	/// Returns a const iterator past the last node.
	const_iterator cend(void) const { return end(); }

	/// Removes all nodes, invoking `deleter` on each. If `deleter` evaluates to
	/// false nodes are unlinked but not freed.
	/// - `deleter` : Override deleter callable. Called only if it evaluates to true.
	template<typename deleter_override_t_>
	void clear(const deleter_override_t_& deleter_);

	/// Removes all nodes using the tree's default `deleter_t`.
	void clear(void) { this->clear(deleter_t_()); }

	/// Returns `true` if the tree contains no nodes.
	hxattr_nodiscard bool empty(void) const { return m_root_ == hxnull; }

	/// Returns an iterator past the last node, representing the end of in-order traversal.
	iterator end(void) { return iterator(hxnull, this); }
	/// Returns a const iterator past the last node.
	const_iterator end(void) const;

	/// Unlinks `ptr` from the tree and invokes `deleter` on it. If `deleter`
	/// evaluates to false it is not called.
	/// - `deleter` : Override deleter callable. Called only if it evaluates to true.
	template<typename deleter_override_t_>
	void erase(node_t_* ptr_, const deleter_override_t_& deleter_);

	/// Unlinks and deletes `ptr` using the tree's default `deleter_t`.
	void erase(node_t_* ptr_) { this->erase(ptr_, deleter_t_()); }

	/// Unlinks `ptr` from the tree and returns it without invoking the deleter.
	/// `ptr` must not be null.
	node_t_* extract(node_t_* ptr_);

	/// Returns a pointer to the first node whose key compares equal to `key`,
	/// or null if not found. When `multi_t` is `true` the first equal node in
	/// ascending order is returned.
	hxattr_nodiscard node_t_* find(const key_t_& key_);
	/// Returns a const pointer to the first node whose key compares equal to `key`.
	hxattr_nodiscard const node_t_* find(const key_t_& key_) const;

	/// Returns a pointer to the minimum (leftmost) node. The tree must not be empty.
	hxattr_nodiscard node_t_* front(void);
	/// Returns a const pointer to the minimum (leftmost) node. The tree must not be empty.
	hxattr_nodiscard const node_t_* front(void) const;

	/// Inserts `ptr` into the tree and returns it. When `multi_t` is `false` and
	/// a node with an equal key already exists, the existing node is returned and
	/// `ptr` is not inserted. The caller detects a collision by comparing the
	/// return value to the argument. `ptr` must not be null.
	node_t_* insert(node_t_* ptr_);

	/// Returns an iterator to the first node with key greater than or equal to
	/// `key`, or `end()` if no such node exists.
	iterator lower_bound(const key_t_& key_);
	/// Returns a const iterator to the first node with key greater than or equal to `key`.
	const_iterator lower_bound(const key_t_& key_) const;

	/// Returns an iterator to the first node with key strictly greater than
	/// `key`, or `end()` if no such node exists.
	iterator upper_bound(const key_t_& key_);
	/// Returns a const iterator to the first node with key strictly greater than `key`.
	const_iterator upper_bound(const key_t_& key_) const;

	/// Removes and returns the maximum node without invoking the deleter, or
	/// null if the tree is empty.
	node_t_* pop_back(void);

	/// Removes and returns the minimum node without invoking the deleter, or
	/// null if the tree is empty.
	node_t_* pop_front(void);

	/// Resets the tree to empty without invoking the deleter on any node.
	/// Ownership of all nodes is abandoned. Use only when nodes are managed
	/// elsewhere or have already been freed.
	void release_all(void);

	/// Returns the number of nodes currently in the tree.
	hxattr_nodiscard size_t size(void) const { return m_size_; }

private:
	hxrbtree(const hxrbtree&) = delete;
	void operator=(const hxrbtree&) = delete;

	static void rb_rotate_left_(hxrbtree_node* node_, hxrbtree_node*& root_);
	static void rb_rotate_right_(hxrbtree_node* node_, hxrbtree_node*& root_);
	static void rb_insert_color_(hxrbtree_node* node_, hxrbtree_node*& root_);
	static void rb_erase_(hxrbtree_node* node_, hxrbtree_node*& root_);
	static hxrbtree_node* rb_first_(hxrbtree_node* root_);
	static hxrbtree_node* rb_last_(hxrbtree_node* root_);
	static hxrbtree_node* rb_next_(hxrbtree_node* node_);
	static hxrbtree_node* rb_prev_(hxrbtree_node* node_);

	hxrbtree_node* m_root_;
	size_t         m_size_;
};

// hxrbtree_node

inline hxrbtree_node* hxrbtree_node::rb_parent_(void) const {
	return reinterpret_cast<hxrbtree_node*>(m_rb_parent_color_ & ~uintptr_t(1u));
}

inline void hxrbtree_node::rb_set_parent_(hxrbtree_node* parent_) {
	m_rb_parent_color_ = (m_rb_parent_color_ & uintptr_t(1u)) | reinterpret_cast<uintptr_t>(parent_);
}

inline int hxrbtree_node::rb_color_(void) const {
	return static_cast<int>(m_rb_parent_color_ & uintptr_t(1u));
}

inline void hxrbtree_node::rb_set_color_(int color_) {
	m_rb_parent_color_ = (m_rb_parent_color_ & ~uintptr_t(1u)) | static_cast<uintptr_t>(color_);
}

inline void hxrbtree_node::rb_set_parent_color_(hxrbtree_node* parent_, int color_) {
	m_rb_parent_color_ = reinterpret_cast<uintptr_t>(parent_) | static_cast<uintptr_t>(color_);
}

inline bool hxrbtree_node::rb_is_red_(void) const {
	return (m_rb_parent_color_ & uintptr_t(1u)) == 0u;
}

inline bool hxrbtree_node::rb_is_black_(void) const {
	return (m_rb_parent_color_ & uintptr_t(1u)) != 0u;
}

// const_iterator

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::const_iterator::operator++(void)
	-> const_iterator& {
	hxassertmsg(m_current_node_ != hxnull, "invalid_iterator");
	m_current_node_ = hxrbtree::rb_next_(m_current_node_);
	return *this;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::const_iterator::operator++(int)
	-> const_iterator {
	const_iterator t_(*this);
	operator++();
	return t_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::const_iterator::operator--(void)
	-> const_iterator& {
	if(m_current_node_ != hxnull) {
		m_current_node_ = hxrbtree::rb_prev_(m_current_node_);
	}
	else {
		m_current_node_ = rb_last_(m_tree_->m_root_);
	}
	return *this;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::const_iterator::operator--(int)
	-> const_iterator {
	const_iterator t_(*this);
	operator--();
	return t_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline bool hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::const_iterator::operator==(
	const const_iterator& x_) const {
	return m_current_node_ == x_.m_current_node_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline bool hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::const_iterator::operator!=(
	const const_iterator& x_) const {
	return m_current_node_ != x_.m_current_node_;
}

// iterator

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::iterator::operator++(int)
	-> iterator {
	iterator t_(*this);
	const_iterator::operator++();
	return t_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::iterator::operator--(int)
	-> iterator {
	iterator t_(*this);
	const_iterator::operator--();
	return t_;
}

// hxrbtree

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::hxrbtree(void)
	: m_root_(hxnull), m_size_(0u) { }

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::back(void) {
	hxassertmsg(m_root_ != hxnull, "empty_tree");
	return static_cast<node_t_*>(rb_last_(m_root_));
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline const node_t_* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::back(void) const {
	hxassertmsg(m_root_ != hxnull, "empty_tree");
	return static_cast<const node_t_*>(rb_last_(m_root_));
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::begin(void) const
	-> const_iterator {
	return const_iterator(rb_first_(m_root_), this);
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::end(void) const
	-> const_iterator {
	return const_iterator(hxnull, this);
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
template<typename deleter_override_t_>
inline void hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::clear(
	const deleter_override_t_& deleter_) {
	hxrbtree_node* node_ = rb_first_(m_root_);
	while(node_ != hxnull) {
		hxrbtree_node* next_ = rb_next_(node_);
		if(deleter_) {
			deleter_(static_cast<node_t_*>(node_));
		}
		node_ = next_;
	}
	release_all();
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
template<typename deleter_override_t_>
inline void hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::erase(
	node_t_* ptr_, const deleter_override_t_& deleter_) {
	rb_erase_(ptr_, m_root_);
	--m_size_;
	if(deleter_) {
		deleter_(ptr_);
	}
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::extract(node_t_* ptr_) {
	hxassertmsg(ptr_ != hxnull, "null_node");
	rb_erase_(ptr_, m_root_);
	--m_size_;
	return ptr_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::find(const key_t_& key_) {
	hxrbtree_node* node_ = m_root_;
	while(node_ != hxnull) {
		if(compare_t_()(static_cast<node_t_&>(*node_), key_)) {
			node_ = node_->m_rb_right_;
		}
		else if(compare_t_()(key_, static_cast<node_t_&>(*node_))) {
			node_ = node_->m_rb_left_;
		}
		else {
			return static_cast<node_t_*>(node_);
		}
	}
	return hxnull;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline const node_t_* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::find(
	const key_t_& key_) const {
	return const_cast<hxrbtree*>(this)->find(key_);
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::front(void) {
	hxassertmsg(m_root_ != hxnull, "empty_tree");
	return static_cast<node_t_*>(rb_first_(m_root_));
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline const node_t_* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::front(void) const {
	hxassertmsg(m_root_ != hxnull, "empty_tree");
	return static_cast<const node_t_*>(rb_first_(m_root_));
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::insert(node_t_* ptr_) {
	hxassertmsg(ptr_ != hxnull, "null_node");
	hxrbtree_node** link_ = &m_root_;
	hxrbtree_node* parent_ = hxnull;
	while(*link_ != hxnull) {
		parent_ = *link_;
		if(compare_t_()(static_cast<node_t_&>(*parent_), static_cast<key_t_&>(*ptr_))) {
			link_ = &parent_->m_rb_right_;
		}
		else if(compare_t_()(static_cast<key_t_&>(*ptr_), static_cast<node_t_&>(*parent_))) {
			link_ = &parent_->m_rb_left_;
		}
		else if(!multi_t_) {
			return static_cast<node_t_*>(parent_);
		}
		else {
			link_ = &parent_->m_rb_right_;
		}
	}
	ptr_->m_rb_left_  = hxnull;
	ptr_->m_rb_right_ = hxnull;
	ptr_->rb_set_parent_color_(parent_, 0);
	*link_ = ptr_;
	rb_insert_color_(ptr_, m_root_);
	++m_size_;
	return ptr_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::lower_bound(const key_t_& key_)
	-> iterator {
	hxrbtree_node* node_ = m_root_;
	hxrbtree_node* result_ = hxnull;
	while(node_ != hxnull) {
		if(compare_t_()(static_cast<node_t_&>(*node_), key_)) {
			node_ = node_->m_rb_right_;
		}
		else {
			result_ = node_;
			node_ = node_->m_rb_left_;
		}
	}
	return iterator(result_, this);
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::lower_bound(const key_t_& key_)
	const -> const_iterator {
	return const_cast<hxrbtree*>(this)->lower_bound(key_);
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::upper_bound(const key_t_& key_)
	-> iterator {
	hxrbtree_node* node_ = m_root_;
	hxrbtree_node* result_ = hxnull;
	while(node_ != hxnull) {
		if(compare_t_()(key_, static_cast<node_t_&>(*node_))) {
			result_ = node_;
			node_ = node_->m_rb_left_;
		}
		else {
			node_ = node_->m_rb_right_;
		}
	}
	return iterator(result_, this);
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::upper_bound(const key_t_& key_)
	const -> const_iterator {
	return const_cast<hxrbtree*>(this)->upper_bound(key_);
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::pop_back(void) {
	if(m_root_ == hxnull) {
		return hxnull;
	}
	node_t_* ptr_ = static_cast<node_t_*>(rb_last_(m_root_));
	rb_erase_(ptr_, m_root_);
	--m_size_;
	return ptr_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::pop_front(void) {
	if(m_root_ == hxnull) {
		return hxnull;
	}
	node_t_* ptr_ = static_cast<node_t_*>(rb_first_(m_root_));
	rb_erase_(ptr_, m_root_);
	--m_size_;
	return ptr_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline void hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::release_all(void) {
	m_root_  = hxnull;
	m_size_  = 0u;
}

// rb_first_ / rb_last_ / rb_next_ / rb_prev_

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline hxrbtree_node* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::rb_first_(
	hxrbtree_node* root_) {
	if(root_ == hxnull) {
		return hxnull;
	}
	while(root_->m_rb_left_ != hxnull) {
		root_ = root_->m_rb_left_;
	}
	return root_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline hxrbtree_node* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::rb_last_(
	hxrbtree_node* root_) {
	if(root_ == hxnull) {
		return hxnull;
	}
	while(root_->m_rb_right_ != hxnull) {
		root_ = root_->m_rb_right_;
	}
	return root_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline hxrbtree_node* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::rb_next_(
	hxrbtree_node* node_) {
	if(node_->m_rb_right_ != hxnull) {
		return rb_first_(node_->m_rb_right_);
	}
	hxrbtree_node* parent_ = node_->rb_parent_();
	while(parent_ != hxnull && node_ == parent_->m_rb_right_) {
		node_   = parent_;
		parent_ = parent_->rb_parent_();
	}
	return parent_;
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline hxrbtree_node* hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::rb_prev_(
	hxrbtree_node* node_) {
	if(node_->m_rb_left_ != hxnull) {
		return rb_last_(node_->m_rb_left_);
	}
	hxrbtree_node* parent_ = node_->rb_parent_();
	while(parent_ != hxnull && node_ == parent_->m_rb_left_) {
		node_   = parent_;
		parent_ = parent_->rb_parent_();
	}
	return parent_;
}

// rb_rotate_left_ / rb_rotate_right_

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline void hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::rb_rotate_left_(
	hxrbtree_node* node_, hxrbtree_node*& root_) {
	hxrbtree_node* right_ = node_->m_rb_right_;
	hxrbtree_node* parent_ = node_->rb_parent_();
	node_->m_rb_right_ = right_->m_rb_left_;
	if(right_->m_rb_left_ != hxnull) {
		right_->m_rb_left_->rb_set_parent_(node_);
	}
	right_->m_rb_left_ = node_;
	right_->rb_set_parent_(parent_);
	if(parent_ != hxnull) {
		if(node_ == parent_->m_rb_left_) {
			parent_->m_rb_left_ = right_;
		}
		else {
			parent_->m_rb_right_ = right_;
		}
	}
	else {
		root_ = right_;
	}
	node_->rb_set_parent_(right_);
}

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline void hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::rb_rotate_right_(
	hxrbtree_node* node_, hxrbtree_node*& root_) {
	hxrbtree_node* left_ = node_->m_rb_left_;
	hxrbtree_node* parent_ = node_->rb_parent_();
	node_->m_rb_left_ = left_->m_rb_right_;
	if(left_->m_rb_right_ != hxnull) {
		left_->m_rb_right_->rb_set_parent_(node_);
	}
	left_->m_rb_right_ = node_;
	left_->rb_set_parent_(parent_);
	if(parent_ != hxnull) {
		if(node_ == parent_->m_rb_right_) {
			parent_->m_rb_right_ = left_;
		}
		else {
			parent_->m_rb_left_ = left_;
		}
	}
	else {
		root_ = left_;
	}
	node_->rb_set_parent_(left_);
}

// rb_insert_color_

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline void hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::rb_insert_color_(
	hxrbtree_node* node_, hxrbtree_node*& root_) {
	hxrbtree_node* parent_ = node_->rb_parent_();
	while(parent_ != hxnull && parent_->rb_is_red_()) {
		hxrbtree_node* gparent_ = parent_->rb_parent_();
		if(parent_ == gparent_->m_rb_left_) {
			hxrbtree_node* uncle_ = gparent_->m_rb_right_;
			if(uncle_ != hxnull && uncle_->rb_is_red_()) {
				uncle_->rb_set_color_(1);
				parent_->rb_set_color_(1);
				gparent_->rb_set_color_(0);
				node_   = gparent_;
				parent_ = node_->rb_parent_();
			}
			else {
				if(node_ == parent_->m_rb_right_) {
					rb_rotate_left_(parent_, root_);
					hxrbtree_node* tmp_ = parent_;
					parent_ = node_;
					node_   = tmp_;
				}
				parent_->rb_set_color_(1);
				gparent_->rb_set_color_(0);
				rb_rotate_right_(gparent_, root_);
			}
		}
		else {
			hxrbtree_node* uncle_ = gparent_->m_rb_left_;
			if(uncle_ != hxnull && uncle_->rb_is_red_()) {
				uncle_->rb_set_color_(1);
				parent_->rb_set_color_(1);
				gparent_->rb_set_color_(0);
				node_   = gparent_;
				parent_ = node_->rb_parent_();
			}
			else {
				if(node_ == parent_->m_rb_left_) {
					rb_rotate_right_(parent_, root_);
					hxrbtree_node* tmp_ = parent_;
					parent_ = node_;
					node_   = tmp_;
				}
				parent_->rb_set_color_(1);
				gparent_->rb_set_color_(0);
				rb_rotate_left_(gparent_, root_);
			}
		}
	}
	root_->rb_set_color_(1);
}

// rb_erase_

template<typename node_t_, typename key_t_, typename compare_t_, bool multi_t_, typename deleter_t_>
inline void hxrbtree<node_t_, key_t_, compare_t_, multi_t_, deleter_t_>::rb_erase_(
	hxrbtree_node* node_, hxrbtree_node*& root_) {
	hxrbtree_node* child_  = hxnull;
	hxrbtree_node* parent_ = hxnull;
	int color_ = 0;

	if(node_->m_rb_left_ == hxnull) {
		child_ = node_->m_rb_right_;
	}
	else if(node_->m_rb_right_ == hxnull) {
		child_ = node_->m_rb_left_;
	}
	else {
		hxrbtree_node* successor_ = rb_first_(node_->m_rb_right_);
		child_  = successor_->m_rb_right_;
		parent_ = successor_->rb_parent_();
		color_  = successor_->rb_color_();
		if(child_ != hxnull) {
			child_->rb_set_parent_(parent_);
		}
		if(parent_ == node_) {
			if(child_ != hxnull) {
				child_->rb_set_parent_(successor_);
			}
			parent_ = successor_;
		}
		else {
			parent_->m_rb_left_ = child_;
		}
		successor_->m_rb_right_ = node_->m_rb_right_;
		successor_->m_rb_left_  = node_->m_rb_left_;
		successor_->rb_set_parent_color_(node_->rb_parent_(), node_->rb_color_());
		if(node_->rb_parent_() != hxnull) {
			if(node_->rb_parent_()->m_rb_left_ == node_) {
				node_->rb_parent_()->m_rb_left_ = successor_;
			}
			else {
				node_->rb_parent_()->m_rb_right_ = successor_;
			}
		}
		else {
			root_ = successor_;
		}
		node_->m_rb_right_->rb_set_parent_(successor_);
		node_->m_rb_left_->rb_set_parent_(successor_);
		goto rebalance_;
	}

	parent_ = node_->rb_parent_();
	color_  = node_->rb_color_();
	if(child_ != hxnull) {
		child_->rb_set_parent_(parent_);
	}
	if(parent_ != hxnull) {
		if(parent_->m_rb_left_ == node_) {
			parent_->m_rb_left_ = child_;
		}
		else {
			parent_->m_rb_right_ = child_;
		}
	}
	else {
		root_ = child_;
	}

rebalance_:
	if(color_ == 0) {
		return;
	}
	while((child_ == hxnull || child_->rb_is_black_()) && child_ != root_) {
		if(parent_->m_rb_left_ == child_) {
			hxrbtree_node* sibling_ = parent_->m_rb_right_;
			if(sibling_->rb_is_red_()) {
				sibling_->rb_set_color_(1);
				parent_->rb_set_color_(0);
				rb_rotate_left_(parent_, root_);
				sibling_ = parent_->m_rb_right_;
			}
			if((sibling_->m_rb_left_ == hxnull || sibling_->m_rb_left_->rb_is_black_()) &&
				(sibling_->m_rb_right_ == hxnull || sibling_->m_rb_right_->rb_is_black_())) {
				sibling_->rb_set_color_(0);
				child_  = parent_;
				parent_ = child_->rb_parent_();
			}
			else {
				if(sibling_->m_rb_right_ == hxnull || sibling_->m_rb_right_->rb_is_black_()) {
					sibling_->m_rb_left_->rb_set_color_(1);
					sibling_->rb_set_color_(0);
					rb_rotate_right_(sibling_, root_);
					sibling_ = parent_->m_rb_right_;
				}
				sibling_->rb_set_color_(parent_->rb_color_());
				parent_->rb_set_color_(1);
				sibling_->m_rb_right_->rb_set_color_(1);
				rb_rotate_left_(parent_, root_);
				child_ = root_;
				break;
			}
		}
		else {
			hxrbtree_node* sibling_ = parent_->m_rb_left_;
			if(sibling_->rb_is_red_()) {
				sibling_->rb_set_color_(1);
				parent_->rb_set_color_(0);
				rb_rotate_right_(parent_, root_);
				sibling_ = parent_->m_rb_left_;
			}
			if((sibling_->m_rb_right_ == hxnull || sibling_->m_rb_right_->rb_is_black_()) &&
				(sibling_->m_rb_left_ == hxnull || sibling_->m_rb_left_->rb_is_black_())) {
				sibling_->rb_set_color_(0);
				child_  = parent_;
				parent_ = child_->rb_parent_();
			}
			else {
				if(sibling_->m_rb_left_ == hxnull || sibling_->m_rb_left_->rb_is_black_()) {
					sibling_->m_rb_right_->rb_set_color_(1);
					sibling_->rb_set_color_(0);
					rb_rotate_left_(sibling_, root_);
					sibling_ = parent_->m_rb_left_;
				}
				sibling_->rb_set_color_(parent_->rb_color_());
				parent_->rb_set_color_(1);
				sibling_->m_rb_left_->rb_set_color_(1);
				rb_rotate_right_(parent_, root_);
				child_ = root_;
				break;
			}
		}
	}
	if(child_ != hxnull) {
		child_->rb_set_color_(1);
	}
}

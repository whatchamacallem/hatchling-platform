#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxrbtree.hpp A red-black tree is a balanced binary search tree.
///
/// Based on Cormen, T. H., Leiserson, C. E., Rivest, R. L., and Stein, C.
/// Introduction to Algorithms, 4th ed. (MIT Press, 2022), Chapter 13: Red-Black
/// Trees.
///
/// Nodes embed their linkage by inheriting from `hxrbtree_node` and are owned
/// by the tree, which calls the deleter on destruction. The tree is
/// parameterized on `node_t`, which must derive from `hxrbtree_node`. All
/// public API returns are downcast to `node_t*`, so inserting further
/// subclasses of `node_t` heterogeneously is supported: recover the concrete
/// type with an additional `static_cast`.
///
/// Any node `T` using key `K` will work as long as it has the following.
/// ```
/// class T : public hxrbtree_node {
///   using key_t = K;                                        // Tell the tree what key type to use.
///   const key_t& rbtree_key() const;                        // Returns the key.
///   static bool rbtree_less(const K& a, const K& b);        // Returns a < b.
/// };
/// ```
/// `hxrbtree_set_node` and `hxrbtree_map_node` are provided and recommended as
/// replacements for `std::set`, `std::map`. `std::multiset` and
/// `std::multimap`.
///
/// Each traversal step calls `node_t::rbtree_less` twice: once as
/// `node_t::rbtree_less(node.rbtree_key(), key)` to test whether to go right, and once as
/// `node_t::rbtree_less(key, node.rbtree_key())` to test whether to go left. Equality is the
/// remaining case.
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
///   using example_node_t = hxrbtree_set_node<int>;
///   hxrbtree<example_node_t> tree;
///   tree.insert(hxnew<example_node_t>(7));
///
///   for(example_node_t& n : tree) {
///       ::printf("%d\n", n.rbtree_key());
///   }
/// ```

#include "hxkey.hpp"
#include "hxmemory_manager.h"
#include "hxutility.h"
#include "detail/hxrbtree_detail.hpp"

#if HX_CPLUSPLUS >= 202002L
/// Concept capturing the interface requirements for `hxrbtree` nodes.
template<typename node_t_>
concept hxrbtree_concept_ =
	requires(const node_t_& const_node_, const typename node_t_::key_t& key_) {
		sizeof(typename node_t_::key_t);
		{ const_node_.rbtree_key() } -> hxconvertible_to<const typename node_t_::key_t&>;
		{ node_t_::rbtree_less(key_, key_) } -> hxconvertible_to<bool>;
	};
#else
#define hxrbtree_concept_ typename
#endif

/// Intrusive red-black tree node base. Derive from `hxrbtree_node` to make a
/// type linkable into an `hxrbtree`. Nodes default to unlinked on construction.
/// The node color is stored in the low bit of `m_parent_color_`, requiring at
/// least two-byte alignment, which is verified by a `static_assert`.
class hxrbtree_node {
public:
	/// Constructs an unlinked node with all pointers and color set to zero.
	hxrbtree_node(void) : m_parent_color_(0u), m_right_(hxnull), m_left_(hxnull) { }

private:
	template<hxrbtree_concept_, bool, typename> friend class hxrbtree;

	friend hxrbtree_node* hxrbtree_red_parent_(const hxrbtree_node* node_);
	friend void hxrbtree_rotate_set_parents_(
		hxrbtree_node* old_, hxrbtree_node* new_, hxrbtree_node*& root_, int color_);
	friend void hxrbtree_insert_color_(hxrbtree_node* node_, hxrbtree_node*& root_);
	friend void hxrbtree_erase_(hxrbtree_node* node_, hxrbtree_node*& root_);
	friend hxrbtree_node* hxrbtree_first_(hxrbtree_node* root_);
	friend hxrbtree_node* hxrbtree_last_(hxrbtree_node* root_);
	friend hxrbtree_node* hxrbtree_next_(hxrbtree_node* node_);
	friend hxrbtree_node* hxrbtree_prev_(hxrbtree_node* node_);

	hxrbtree_node(const hxrbtree_node&) = delete;
	void operator=(const hxrbtree_node&) = delete;

	hxrbtree_node* parent_(void) const;
	void set_parent_(hxrbtree_node* parent_);
	int color_(void) const;
	void set_color_(int color_);
	void set_parent_color_(hxrbtree_node* parent_, int color_);
	bool is_red_(void) const;
	bool is_black_(void) const;

	uintptr_t m_parent_color_;
	hxrbtree_node* m_right_;
	hxrbtree_node* m_left_;
};

/// `hxrbtree_set_node` - Optional base class for ordered set entries. Caches
/// the key by value. Copying and modification are disallowed to protect the
/// integrity of the tree. See `hxrbtree_map_node` if you need a mutable node.
template<typename key_t_>
class hxrbtree_set_node : public hxrbtree_node {
public:
	using key_t = key_t_;

	/// Constructs a node from the key.
	/// - `key` : Key used to order the node.
	template<typename ref_t_>
	hxrbtree_set_node(ref_t_&& key_)
		: m_key_(hxforward<ref_t_>(key_)) { }

	/// The key identifies the node and should not change once inserted.
	const key_t_& rbtree_key(void) const { return m_key_; }

	/// Compares two keys for `a < b`.
	static bool rbtree_less(const key_t_& a_, const key_t_& b_) { return hxkey_less(a_, b_); }

private:
	hxrbtree_set_node(void) = delete;
	hxrbtree_set_node(const hxrbtree_set_node&) = delete;
	hxrbtree_set_node(hxrbtree_set_node&&) = delete;
	void operator=(const hxrbtree_set_node&) = delete;

	const key_t_ m_key_;
};

/// `hxrbtree_map_node` - Optional base class for ordered map entries.
template<typename key_t_, typename value_t_>
class hxrbtree_map_node : public hxrbtree_set_node<key_t_> {
public:
	using key_t = key_t_;
	using value_t = value_t_;

	/// Default-initializes a node. `value_t` must default-construct when
	/// accessed without a value argument.
	/// - `key` : Key used to order the node.
	hxrbtree_map_node(const key_t_& key_)
		: hxrbtree_set_node<key_t_>(key_) { }

	/// Constructs a node whose value is copy- or move-initialized.
	/// - `key` : Key used to order the node.
	/// - `value` : Value forwarded into storage.
	template<typename ref_t_>
	hxrbtree_map_node(const key_t_& key_, ref_t_&& value_)
		: hxrbtree_set_node<key_t_>(key_), m_value_(hxforward<ref_t_>(value_)) { }

	/// Returns the stored value.
	const value_t_& value(void) const { return m_value_; }
	/// Returns the stored value, allowing mutation.
	value_t_& value(void) { return m_value_; }

private:
	value_t_ m_value_;
};

static_assert(alignof(hxrbtree_node) >= 2,
	"hxrbtree_node alignment insufficient for color bit");

/// An intrusive red-black tree that takes ownership of nodes via a `deleter_t`
/// functor, defaulting to `hxdefault_delete`. `node_t` must derive from
/// `hxrbtree_node`. The destructor calls `clear()` which invokes the deleter on
/// all remaining nodes. Subclasses of `node_t` may be inserted heterogeneously.
/// Recover the concrete type with `static_cast`.
///
/// Iteration is in ascending key order. `front()` returns the minimum node and
/// `back()` returns the maximum node.
///
/// - `node_t` : The node type. Must derive from `hxrbtree_node` and provide
///   `using key_t = K` and `const K& rbtree_key() const`.
/// - `multi_t` : When `false`, duplicate keys are rejected by `insert` and the
///   existing node is returned. When `true`, duplicates are allowed. Defaults
///   to `false`.
/// - `deleter_t` : A callable that frees a node pointer. Defaults to
///   `hxdefault_delete`.
template<hxrbtree_concept_ node_t_,
	bool multi_t_ = false,
	typename deleter_t_ = hxdefault_delete>
class hxrbtree {
public:
	using node_t    = node_t_;
	using key_t     = typename node_t_::key_t;
	using deleter_t = deleter_t_;

	/// Bidirectional iterator over const nodes in ascending key order.
	/// Incrementing past `end()` or decrementing past `begin()` is undefined.
	class const_iterator {
	public:
		const_iterator(hxrbtree_node* current_, const hxrbtree* tree_)
			: m_current_node_(current_), m_tree_(tree_) { }

		/// Constructs a singular iterator that must not be incremented or
		/// dereferenced.
		const_iterator(void) : m_current_node_(hxnull), m_tree_(hxnull) { }

		/// Advances to the in-order successor and returns this iterator.
		/// Asserts the iterator is not at `end()`.
		const_iterator& operator++(void);
		/// Post-increment: advances to the in-order successor and returns the
		/// prior position.
		const_iterator  operator++(int);
		/// Retreats to the in-order predecessor and returns this iterator.
		/// Decrementing `end()` yields the maximum node.
		const_iterator& operator--(void);
		/// Post-decrement: retreats to the in-order predecessor and returns the
		/// prior position.
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

		/// Constructs a singular iterator that must not be incremented or
		/// dereferenced.
		iterator(void) { }

		/// Advances to the in-order successor and returns this iterator.
		iterator& operator++(void) { const_iterator::operator++(); return *this; }
		/// Post-increment: advances to the in-order successor and returns the
		/// prior position.
		iterator  operator++(int);
		/// Retreats to the in-order predecessor and returns this iterator.
		iterator& operator--(void) { const_iterator::operator--(); return *this; }
		/// Post-decrement: retreats to the in-order predecessor and returns the
		/// prior position.
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

	/// Returns a pointer to the maximum (rightmost) node. The tree must not be
	/// empty.
	hxattr_nodiscard node_t_* back(void);
	/// Returns a const pointer to the maximum (rightmost) node. The tree must
	/// not be empty.
	hxattr_nodiscard const node_t_* back(void) const;

	/// Returns an iterator to the minimum node, or `end()` if the tree is
	/// empty.
	iterator begin(void) { return iterator(hxrbtree_first_(m_root_), this); }
	/// Returns a const iterator to the minimum node, or `end()` if the tree is
	/// empty.
	const_iterator begin(void) const;

	/// Returns a const iterator to the minimum node, or `cend()` if the tree is
	/// empty.
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

	/// Returns an iterator past the last node, representing the end of in-order
	/// traversal.
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
	hxattr_nodiscard node_t_* find(const typename node_t_::key_t& key_);
	/// Returns a const pointer to the first node whose key compares equal to
	/// `key`.
	hxattr_nodiscard const node_t_* find(const typename node_t_::key_t& key_) const;

	/// Returns a pointer to the minimum (leftmost) node. The tree must not be
	/// empty.
	hxattr_nodiscard node_t_* front(void);
	/// Returns a const pointer to the minimum (leftmost) node. The tree must
	/// not be empty.
	hxattr_nodiscard const node_t_* front(void) const;

	/// Inserts `ptr` into the tree and returns it. When `multi_t` is `false`
	/// and a node with an equal key already exists, the existing node is
	/// returned and `ptr` is not inserted. The caller detects a collision by
	/// comparing the return value to the argument. `ptr` must not be null.
	node_t_* insert(node_t_* ptr_);

	/// Returns an iterator to the first node with key greater than or equal to
	/// `key`, or `end()` if no such node exists.
	iterator lower_bound(const typename node_t_::key_t& key_);
	/// Returns a const iterator to the first node with key greater than or
	/// equal to `key`.
	const_iterator lower_bound(const typename node_t_::key_t& key_) const;

	/// Returns an iterator to the first node with key strictly greater than
	/// `key`, or `end()` if no such node exists.
	iterator upper_bound(const typename node_t_::key_t& key_);
	/// Returns a const iterator to the first node with key strictly greater
	/// than `key`.
	const_iterator upper_bound(const typename node_t_::key_t& key_) const;

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

	hxrbtree_node* m_root_;
	size_t         m_size_;
};

// hxrbtree_node

inline hxrbtree_node* hxrbtree_node::parent_(void) const {
	return reinterpret_cast<hxrbtree_node*>(m_parent_color_ & ~static_cast<uintptr_t>(1u));
}

inline void hxrbtree_node::set_parent_(hxrbtree_node* parent_) {
	m_parent_color_ = (m_parent_color_ & static_cast<uintptr_t>(1u)) | reinterpret_cast<uintptr_t>(parent_);
}

inline int hxrbtree_node::color_(void) const {
	return static_cast<int>(m_parent_color_ & static_cast<uintptr_t>(1u));
}

inline void hxrbtree_node::set_color_(int color_) {
	m_parent_color_ = (m_parent_color_ & ~static_cast<uintptr_t>(1u)) | static_cast<uintptr_t>(color_);
}

inline void hxrbtree_node::set_parent_color_(hxrbtree_node* parent_, int color_) {
	m_parent_color_ = reinterpret_cast<uintptr_t>(parent_) | static_cast<uintptr_t>(color_);
}

inline bool hxrbtree_node::is_red_(void) const {
	return (m_parent_color_ & static_cast<uintptr_t>(1u)) == 0u;
}

inline bool hxrbtree_node::is_black_(void) const {
	return (m_parent_color_ & static_cast<uintptr_t>(1u)) != 0u;
}

// const_iterator

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::const_iterator::operator++(void)
		-> const_iterator& {
	hxassertmsg(m_current_node_ != hxnull, "invalid_iterator");
	m_current_node_ = hxrbtree_next_(m_current_node_);
	return *this;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::const_iterator::operator++(int)
		-> const_iterator {
	const_iterator t_(*this);
	operator++();
	return t_;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::const_iterator::operator--(void)
		-> const_iterator& {
	if(m_current_node_ != hxnull) {
		m_current_node_ = hxrbtree_prev_(m_current_node_);
	}
	else {
		m_current_node_ = hxrbtree_last_(m_tree_->m_root_);
	}
	return *this;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::const_iterator::operator--(int)
		-> const_iterator {
	const_iterator t_(*this);
	operator--();
	return t_;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline bool hxrbtree<node_t_, multi_t_, deleter_t_>::const_iterator::operator==(
		const const_iterator& x_) const {
	return m_current_node_ == x_.m_current_node_;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline bool hxrbtree<node_t_, multi_t_, deleter_t_>::const_iterator::operator!=(
		const const_iterator& x_) const {
	return m_current_node_ != x_.m_current_node_;
}

// iterator

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::iterator::operator++(int)
		-> iterator {
	iterator t_(*this);
	const_iterator::operator++();
	return t_;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::iterator::operator--(int)
		-> iterator {
	iterator t_(*this);
	const_iterator::operator--();
	return t_;
}

// hxrbtree

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline hxrbtree<node_t_, multi_t_, deleter_t_>::hxrbtree(void)
	: m_root_(hxnull), m_size_(0u) { }

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, multi_t_, deleter_t_>::back(void) {
	hxassertmsg(m_root_ != hxnull, "empty_tree");
	return static_cast<node_t_*>(hxrbtree_last_(m_root_));
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline const node_t_* hxrbtree<node_t_, multi_t_, deleter_t_>::back(void) const {
	hxassertmsg(m_root_ != hxnull, "empty_tree");
	return static_cast<const node_t_*>(hxrbtree_last_(m_root_));
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::begin(void) const
		-> const_iterator {
	return const_iterator(hxrbtree_first_(m_root_), this);
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::end(void) const
		-> const_iterator {
	return const_iterator(hxnull, this);
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
template<typename deleter_override_t_>
inline void hxrbtree<node_t_, multi_t_, deleter_t_>::clear(
	const deleter_override_t_& deleter_) {
	// Rotate left children up to form a right-spine, then delete along the spine.
	// This avoids reading parent pointers of deleted nodes.
	hxrbtree_node* node_ = m_root_;
	while(node_ != hxnull) {
		if(node_->m_left_ != hxnull) {
			hxrbtree_node* left_ = node_->m_left_;
			node_->m_left_       = left_->m_right_;
			left_->m_right_      = node_;
			node_                = left_;
		}
		else {
			hxrbtree_node* right_ = node_->m_right_;
			if(deleter_) {
				deleter_(static_cast<node_t_*>(node_));
			}
			node_ = right_;
		}
	}
	release_all();
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
template<typename deleter_override_t_>
inline void hxrbtree<node_t_, multi_t_, deleter_t_>::erase(
	node_t_* ptr_, const deleter_override_t_& deleter_) {
	hxrbtree_erase_(ptr_, m_root_);
	--m_size_;
	if(deleter_) {
		deleter_(ptr_);
	}
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, multi_t_, deleter_t_>::extract(node_t_* ptr_) {
	hxassertmsg(ptr_ != hxnull, "null_node");
	hxrbtree_erase_(ptr_, m_root_);
	--m_size_;
	return ptr_;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, multi_t_, deleter_t_>::find(const typename node_t_::key_t& key_) {
	hxrbtree_node* node_ = m_root_;
	while(node_ != hxnull) {
		if(node_t_::rbtree_less(static_cast<node_t_&>(*node_).rbtree_key(), key_)) {
			node_ = node_->m_right_;
		}
		else if(node_t_::rbtree_less(key_, static_cast<node_t_&>(*node_).rbtree_key())) {
			node_ = node_->m_left_;
		}
		else {
			return static_cast<node_t_*>(node_);
		}
	}
	return hxnull;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline const node_t_* hxrbtree<node_t_, multi_t_, deleter_t_>::find(
	const typename node_t_::key_t& key_) const {
	return const_cast<hxrbtree*>(this)->find(key_);
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, multi_t_, deleter_t_>::front(void) {
	hxassertmsg(m_root_ != hxnull, "empty_tree");
	return static_cast<node_t_*>(hxrbtree_first_(m_root_));
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline const node_t_* hxrbtree<node_t_, multi_t_, deleter_t_>::front(void) const {
	hxassertmsg(m_root_ != hxnull, "empty_tree");
	return static_cast<const node_t_*>(hxrbtree_first_(m_root_));
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, multi_t_, deleter_t_>::insert(node_t_* ptr_) {
	hxassertmsg(ptr_ != hxnull, "null_node");
	hxrbtree_node** link_ = &m_root_;
	hxrbtree_node* parent_ = hxnull;
	while(*link_ != hxnull) {
		parent_ = *link_;
		if(node_t_::rbtree_less(ptr_->rbtree_key(), static_cast<node_t_&>(*parent_).rbtree_key())) {
			link_ = &parent_->m_left_;
		}
		else if(!node_t_::rbtree_less(static_cast<node_t_&>(*parent_).rbtree_key(), ptr_->rbtree_key()) && !multi_t_) {
			return static_cast<node_t_*>(parent_);
		}
		else {
			link_ = &parent_->m_right_;
		}
	}
	ptr_->m_left_  = hxnull;
	ptr_->m_right_ = hxnull;
	ptr_->set_parent_color_(parent_, 0);
	*link_ = ptr_;
	hxrbtree_insert_color_(ptr_, m_root_);
	++m_size_;
	return ptr_;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::lower_bound(const typename node_t_::key_t& key_)
	-> iterator {
	hxrbtree_node* node_ = m_root_;
	hxrbtree_node* result_ = hxnull;
	while(node_ != hxnull) {
		if(node_t_::rbtree_less(static_cast<node_t_&>(*node_).rbtree_key(), key_)) {
			node_ = node_->m_right_;
		}
		else {
			result_ = node_;
			node_ = node_->m_left_;
		}
	}
	return iterator(result_, this);
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::lower_bound(const typename node_t_::key_t& key_)
	const -> const_iterator {
	return const_cast<hxrbtree*>(this)->lower_bound(key_);
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::upper_bound(const typename node_t_::key_t& key_)
	-> iterator {
	hxrbtree_node* node_ = m_root_;
	hxrbtree_node* result_ = hxnull;
	while(node_ != hxnull) {
		if(node_t_::rbtree_less(key_, static_cast<node_t_&>(*node_).rbtree_key())) {
			result_ = node_;
			node_ = node_->m_left_;
		}
		else {
			node_ = node_->m_right_;
		}
	}
	return iterator(result_, this);
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline auto hxrbtree<node_t_, multi_t_, deleter_t_>::upper_bound(const typename node_t_::key_t& key_)
	const -> const_iterator {
	return const_cast<hxrbtree*>(this)->upper_bound(key_);
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, multi_t_, deleter_t_>::pop_back(void) {
	if(m_root_ == hxnull) {
		return hxnull;
	}
	node_t_* ptr_ = static_cast<node_t_*>(hxrbtree_last_(m_root_));
	hxrbtree_erase_(ptr_, m_root_);
	--m_size_;
	return ptr_;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline node_t_* hxrbtree<node_t_, multi_t_, deleter_t_>::pop_front(void) {
	if(m_root_ == hxnull) {
		return hxnull;
	}
	node_t_* ptr_ = static_cast<node_t_*>(hxrbtree_first_(m_root_));
	hxrbtree_erase_(ptr_, m_root_);
	--m_size_;
	return ptr_;
}

template<hxrbtree_concept_ node_t_, bool multi_t_, typename deleter_t_>
inline void hxrbtree<node_t_, multi_t_, deleter_t_>::release_all(void) {
	m_root_  = hxnull;
	m_size_  = 0u;
}

// hxrbtree_first_ / hxrbtree_last_ / hxrbtree_next_ / hxrbtree_prev_

hxattr_hot inline hxrbtree_node* hxrbtree_first_(hxrbtree_node* root_) {
	if(root_ == hxnull) {
		return hxnull;
	}
	while(root_->m_left_ != hxnull) {
		root_ = root_->m_left_;
	}
	return root_;
}

hxattr_hot inline hxrbtree_node* hxrbtree_last_(hxrbtree_node* root_) {
	if(root_ == hxnull) {
		return hxnull;
	}
	while(root_->m_right_ != hxnull) {
		root_ = root_->m_right_;
	}
	return root_;
}

hxattr_hot inline hxrbtree_node* hxrbtree_next_(hxrbtree_node* node_) {
	if(node_->m_right_ != hxnull) {
		node_ = node_->m_right_;
		while(node_->m_left_ != hxnull) {
			node_ = node_->m_left_;
		}
		return node_;
	}
	hxrbtree_node* parent_ = node_->parent_();
	while(parent_ != hxnull && node_ == parent_->m_right_) {
		node_   = parent_;
		parent_ = parent_->parent_();
	}
	return parent_;
}

hxattr_hot inline hxrbtree_node* hxrbtree_prev_(hxrbtree_node* node_) {
	if(node_->m_left_ != hxnull) {
		node_ = node_->m_left_;
		while(node_->m_right_ != hxnull) {
			node_ = node_->m_right_;
		}
		return node_;
	}
	hxrbtree_node* parent_ = node_->parent_();
	while(parent_ != hxnull && node_ == parent_->m_left_) {
		node_   = parent_;
		parent_ = parent_->parent_();
	}
	return parent_;
}



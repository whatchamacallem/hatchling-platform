// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxrbtree.hpp>
#include <hx/hxtest.hpp>

namespace {

// Counts destructor calls to verify deleter behavior.
static int s_hxtest_destructor_count = 0;

// Custom deleter that counts invocations.
static int s_hxtest_custom_deleter_count = 0;

// Basic set node wrapping int with destructor tracking.
struct hxtest_rbtree_node_t : hxrbtree_set_node<int> {
	explicit hxtest_rbtree_node_t(int k) : hxrbtree_set_node<int>(k) { }
	~hxtest_rbtree_node_t(void) { ++s_hxtest_destructor_count; }
};

// Aligned storage for N hxtest_rbtree_node_t objects constructed in-place from
// an array of int keys. C++11-compatible: avoids copy or move construction.
template<size_t n_t_>
struct hxtest_rbtree_buf_t {
	hxtest_rbtree_buf_t(const int (&keys_)[n_t_]) {
		for(size_t i_ = 0u; i_ < n_t_; ++i_) {
			::new(static_cast<void*>(&m_buf_[i_])) hxtest_rbtree_node_t(keys_[i_]);
		}
	}
	~hxtest_rbtree_buf_t(void) {
		for(size_t i_ = n_t_; i_-- > 0u;) {
			reinterpret_cast<hxtest_rbtree_node_t*>(&m_buf_[i_])->~hxtest_rbtree_node_t();
		}
	}
	hxtest_rbtree_node_t& operator[](size_t i_) {
		return *reinterpret_cast<hxtest_rbtree_node_t*>(&m_buf_[i_]);
	}
	const hxtest_rbtree_node_t& operator[](size_t i_) const {
		return *reinterpret_cast<const hxtest_rbtree_node_t*>(&m_buf_[i_]);
	}
	hxtest_rbtree_buf_t(const hxtest_rbtree_buf_t&) = delete;
	void operator=(const hxtest_rbtree_buf_t&) = delete;
	alignas(hxtest_rbtree_node_t) unsigned char m_buf_[n_t_][sizeof(hxtest_rbtree_node_t)];
};

struct hxtest_rbtree_custom_deleter_t {
	void operator()(hxtest_rbtree_node_t* ptr_) const {
		++s_hxtest_custom_deleter_count;
		hxdelete(ptr_);
	}
	operator bool(void) const { return true; }
};

// Validates that iteration visits keys in strictly ascending order and
// the count matches the expected size.
template<bool multi_t_, typename deleter_t_>
static void hxtest_check_order(
	const hxrbtree<hxtest_rbtree_node_t, multi_t_, deleter_t_>& tree_,
	size_t expected_size_) {
	size_t count_ = 0u;
	int prev_ = -2147483647 - 1;
	for(const hxtest_rbtree_node_t& n_ : tree_) {
		EXPECT_LT(prev_, n_.key());
		prev_ = n_.key();
		++count_;
	}
	EXPECT_EQ(count_, expected_size_);
	EXPECT_EQ(tree_.size(), expected_size_);
}


} // namespace

// Construction and empty state

TEST(hxrbtree_test, empty_on_construction) {
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	EXPECT_TRUE(tree.empty());
	EXPECT_EQ(tree.size(), (size_t)0);
	EXPECT_EQ(tree.begin(), tree.end());
	EXPECT_EQ(tree.cbegin(), tree.cend());
}

// insert - set semantics (multi_t = false)

// Insert a single node: size becomes 1, not empty, begin != end.
TEST(hxrbtree_test, insert_single_node) {
	hxtest_rbtree_node_t n(10);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	hxtest_rbtree_node_t* ret = tree.insert(&n);
	EXPECT_EQ(ret, &n);
	EXPECT_FALSE(tree.empty());
	EXPECT_EQ(tree.size(), (size_t)1);
	EXPECT_EQ(tree.front(), &n);
	EXPECT_EQ(tree.back(), &n);
	tree.release_all();
}

// Insert ascending keys: iteration must visit them in order 1,2,3.
TEST(hxrbtree_test, insert_ascending_order) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	hxtest_check_order(tree, 3u);
	tree.release_all();
}

// Insert descending keys: tree rebalances; iteration must still be in order.
TEST(hxrbtree_test, insert_descending_order) {
	hxtest_rbtree_node_t a(3), b(2), c(1);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	hxtest_check_order(tree, 3u);
	tree.release_all();
}

// Insert in an order that exercises all rotation paths (zig-zig and zig-zag).
TEST(hxrbtree_test, insert_mixed_order) {
	const int keys_[7] = { 4, 2, 6, 1, 3, 5, 7 };
	hxtest_rbtree_buf_t<7> nodes(keys_);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	for(size_t i = 0u; i < 7u; ++i) {
		tree.insert(&nodes[i]);
	}
	hxtest_check_order(tree, 7u);
	tree.release_all();
}

// Duplicate key with multi_t=false: insert returns the existing node, size unchanged.
TEST(hxrbtree_test, insert_duplicate_set_returns_existing) {
	hxtest_rbtree_node_t a(5), b(5);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	hxtest_rbtree_node_t* first  = tree.insert(&a);
	hxtest_rbtree_node_t* second = tree.insert(&b);
	EXPECT_EQ(first, &a);
	EXPECT_EQ(second, &a);
	EXPECT_NE(second, &b);
	EXPECT_EQ(tree.size(), (size_t)1);
	tree.release_all();
}

// insert - multiset semantics (multi_t = true)

// Duplicate key with multi_t=true: both nodes are inserted, size becomes 2.
TEST(hxrbtree_test, insert_duplicate_multi_both_inserted) {
	hxtest_rbtree_node_t a(5), b(5);
	hxrbtree<hxtest_rbtree_node_t, true, hxdo_not_delete> tree;
	hxtest_rbtree_node_t* ra = tree.insert(&a);
	hxtest_rbtree_node_t* rb = tree.insert(&b);
	EXPECT_EQ(ra, &a);
	EXPECT_EQ(rb, &b);
	EXPECT_EQ(tree.size(), (size_t)2);
	tree.release_all();
}

// Three equal keys in multi tree: size is 3 and all are visited in iteration.
TEST(hxrbtree_test, insert_multi_three_equal_keys) {
	hxtest_rbtree_node_t a(3), b(3), c(3);
	hxrbtree<hxtest_rbtree_node_t, true, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	EXPECT_EQ(tree.size(), (size_t)3);
	size_t count_ = 0u;
	for(const hxtest_rbtree_node_t& n_ : tree) {
		EXPECT_EQ(n_.key(), 3);
		++count_;
	}
	EXPECT_EQ(count_, (size_t)3);
	tree.release_all();
}

// find

// find on empty tree returns null.
TEST(hxrbtree_test, find_empty_tree_returns_null) {
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	EXPECT_EQ(tree.find(1), (hxtest_rbtree_node_t*)hxnull);
}

// find returns the correct node when the key exists.
TEST(hxrbtree_test, find_existing_key) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	EXPECT_EQ(tree.find(1), &a);
	EXPECT_EQ(tree.find(2), &b);
	EXPECT_EQ(tree.find(3), &c);
	tree.release_all();
}

// find returns null for a key one below the minimum.
TEST(hxrbtree_test, find_key_below_min_returns_null) {
	hxtest_rbtree_node_t a(5), b(10);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	EXPECT_EQ(tree.find(4), (hxtest_rbtree_node_t*)hxnull);
	tree.release_all();
}

// find returns null for a key one above the maximum.
TEST(hxrbtree_test, find_key_above_max_returns_null) {
	hxtest_rbtree_node_t a(5), b(10);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	EXPECT_EQ(tree.find(11), (hxtest_rbtree_node_t*)hxnull);
	tree.release_all();
}

// find returns null for a key between two existing keys (gap test).
TEST(hxrbtree_test, find_gap_between_keys_returns_null) {
	hxtest_rbtree_node_t a(1), b(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	EXPECT_EQ(tree.find(2), (hxtest_rbtree_node_t*)hxnull);
	tree.release_all();
}

// const find on a const tree returns the correct node.
TEST(hxrbtree_test, find_const_tree) {
	hxtest_rbtree_node_t a(7);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>& ct_ = tree;
	EXPECT_EQ(ct_.find(7), &a);
	EXPECT_EQ(ct_.find(8), (const hxtest_rbtree_node_t*)hxnull);
	tree.release_all();
}

// front / back

// front and back on a single-node tree return that node.
TEST(hxrbtree_test, front_back_single_node) {
	hxtest_rbtree_node_t a(42);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	EXPECT_EQ(tree.front(), &a);
	EXPECT_EQ(tree.back(), &a);
	tree.release_all();
}

// front returns minimum, back returns maximum in a multi-node tree.
TEST(hxrbtree_test, front_back_multi_node) {
	hxtest_rbtree_node_t a(1), b(5), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	EXPECT_EQ(tree.front()->key(), 1);
	EXPECT_EQ(tree.back()->key(), 5);
	tree.release_all();
}

// const front and back return the correct nodes.
TEST(hxrbtree_test, front_back_const) {
	hxtest_rbtree_node_t a(2), b(8);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>& ct_ = tree;
	EXPECT_EQ(ct_.front()->key(), 2);
	EXPECT_EQ(ct_.back()->key(), 8);
	tree.release_all();
}

// pop_front / pop_back

// pop_front on empty tree returns null.
TEST(hxrbtree_test, pop_front_empty_returns_null) {
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	EXPECT_EQ(tree.pop_front(), (hxtest_rbtree_node_t*)hxnull);
}

// pop_back on empty tree returns null.
TEST(hxrbtree_test, pop_back_empty_returns_null) {
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	EXPECT_EQ(tree.pop_back(), (hxtest_rbtree_node_t*)hxnull);
}

// pop_front removes and returns the minimum; tree shrinks.
TEST(hxrbtree_test, pop_front_removes_minimum) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	hxtest_rbtree_node_t* popped_ = tree.pop_front();
	EXPECT_EQ(popped_, &a);
	EXPECT_EQ(tree.size(), (size_t)2);
	EXPECT_EQ(tree.front()->key(), 2);
	tree.release_all();
}

// pop_back removes and returns the maximum; tree shrinks.
TEST(hxrbtree_test, pop_back_removes_maximum) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	hxtest_rbtree_node_t* popped_ = tree.pop_back();
	EXPECT_EQ(popped_, &c);
	EXPECT_EQ(tree.size(), (size_t)2);
	EXPECT_EQ(tree.back()->key(), 2);
	tree.release_all();
}

// pop_front the last node leaves tree empty and returns null on the next call.
TEST(hxrbtree_test, pop_front_to_empty) {
	hxtest_rbtree_node_t a(1);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	hxtest_rbtree_node_t* popped_ = tree.pop_front();
	EXPECT_EQ(popped_, &a);
	EXPECT_TRUE(tree.empty());
	EXPECT_EQ(tree.pop_front(), (hxtest_rbtree_node_t*)hxnull);
}

// extract

// extract a node that has no children (leaf), in the middle key ordering.
TEST(hxrbtree_test, extract_leaf_node) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&b);
	tree.insert(&a);
	tree.insert(&c);
	tree.extract(&a);
	EXPECT_EQ(tree.size(), (size_t)2);
	hxtest_check_order(tree, 2u);
	tree.release_all();
}

// extract the root of a three-node tree (has two children).
TEST(hxrbtree_test, extract_root_two_children) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&b);
	tree.insert(&a);
	tree.insert(&c);
	tree.extract(&b);
	EXPECT_EQ(tree.size(), (size_t)2);
	hxtest_check_order(tree, 2u);
	tree.release_all();
}

// extract does not call the deleter.
TEST(hxrbtree_test, extract_does_not_delete) {
	const hxsystem_allocator_scope scope_(hxsystem_allocator_temporary_stack);
	s_hxtest_destructor_count = 0;
	hxtest_rbtree_node_t* n_ = hxnew<hxtest_rbtree_node_t>(1);
	hxrbtree<hxtest_rbtree_node_t> tree;
	tree.insert(n_);
	hxtest_rbtree_node_t* ex_ = tree.extract(n_);
	EXPECT_EQ(ex_, n_);
	EXPECT_EQ(s_hxtest_destructor_count, 0);
	EXPECT_TRUE(tree.empty());
	hxdelete(n_);
}

// erase

// erase with default deleter calls destructor exactly once.
TEST(hxrbtree_test, erase_calls_destructor) {
	const hxsystem_allocator_scope scope_(hxsystem_allocator_temporary_stack);
	s_hxtest_destructor_count = 0;
	hxrbtree<hxtest_rbtree_node_t> tree;
	tree.insert(hxnew<hxtest_rbtree_node_t>(1));
	tree.insert(hxnew<hxtest_rbtree_node_t>(2));
	tree.insert(hxnew<hxtest_rbtree_node_t>(3));
	hxtest_rbtree_node_t* front_ = tree.front();
	tree.erase(front_);
	EXPECT_EQ(s_hxtest_destructor_count, 1);
	EXPECT_EQ(tree.size(), (size_t)2);
	EXPECT_EQ(tree.front()->key(), 2);
}

// erase with hxdo_not_delete override does not call destructor.
TEST(hxrbtree_test, erase_do_not_delete_override) {
	s_hxtest_destructor_count = 0;
	hxtest_rbtree_node_t a(5);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.erase(&a, hxdo_not_delete());
	EXPECT_EQ(s_hxtest_destructor_count, 0);
	EXPECT_TRUE(tree.empty());
}

// erase with custom deleter override is called exactly once.
TEST(hxrbtree_test, erase_custom_deleter_override) {
	const hxsystem_allocator_scope scope_(hxsystem_allocator_temporary_stack);
	s_hxtest_custom_deleter_count = 0;
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	hxtest_rbtree_node_t* n_ = hxnew<hxtest_rbtree_node_t>(7);
	tree.insert(n_);
	tree.erase(n_, hxtest_rbtree_custom_deleter_t());
	EXPECT_EQ(s_hxtest_custom_deleter_count, 1);
	EXPECT_TRUE(tree.empty());
}

// Erase a node with two children: successor splice and rebalance.
TEST(hxrbtree_test, erase_node_with_two_children) {
	const int keys_[6] = { 4, 2, 6, 1, 3, 5 };
	hxtest_rbtree_buf_t<6> nodes(keys_);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	for(size_t i = 0u; i < 6u; ++i) {
		tree.insert(&nodes[i]);
	}
	// nodes[1] has key 2, which has two children (1 and 3).
	tree.erase(&nodes[1], hxdo_not_delete());
	hxtest_check_order(tree, 5u);
	tree.release_all();
}

// clear

// clear on an empty tree is a no-op.
TEST(hxrbtree_test, clear_empty_tree) {
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.clear();
	EXPECT_TRUE(tree.empty());
	EXPECT_EQ(tree.size(), (size_t)0);
}

// clear with default deleter calls destructor on every node.
TEST(hxrbtree_test, clear_default_deleter_calls_all_destructors) {
	const hxsystem_allocator_scope scope_(hxsystem_allocator_temporary_stack);
	s_hxtest_destructor_count = 0;
	{
		hxrbtree<hxtest_rbtree_node_t> tree;
		tree.insert(hxnew<hxtest_rbtree_node_t>(1));
		tree.insert(hxnew<hxtest_rbtree_node_t>(2));
		tree.insert(hxnew<hxtest_rbtree_node_t>(3));
		tree.clear();
		EXPECT_EQ(s_hxtest_destructor_count, 3);
		EXPECT_TRUE(tree.empty());
	}
}

// clear with hxdo_not_delete override unlinks all nodes without calling destructor.
TEST(hxrbtree_test, clear_do_not_delete_override) {
	s_hxtest_destructor_count = 0;
	hxtest_rbtree_node_t a(1), b(2);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.clear(hxdo_not_delete());
	EXPECT_EQ(s_hxtest_destructor_count, 0);
	EXPECT_TRUE(tree.empty());
	EXPECT_EQ(tree.size(), (size_t)0);
}

// Destructor calls clear(), invoking the deleter on remaining nodes.
TEST(hxrbtree_test, destructor_calls_clear) {
	const hxsystem_allocator_scope scope_(hxsystem_allocator_temporary_stack);
	s_hxtest_destructor_count = 0;
	{
		hxrbtree<hxtest_rbtree_node_t> tree;
		tree.insert(hxnew<hxtest_rbtree_node_t>(10));
		tree.insert(hxnew<hxtest_rbtree_node_t>(20));
	}
	EXPECT_EQ(s_hxtest_destructor_count, 2);
}

// release_all

// release_all resets to empty without calling destructor.
TEST(hxrbtree_test, release_all_no_destructor) {
	s_hxtest_destructor_count = 0;
	hxtest_rbtree_node_t a(1), b(2);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.release_all();
	EXPECT_TRUE(tree.empty());
	EXPECT_EQ(tree.size(), (size_t)0);
	EXPECT_EQ(s_hxtest_destructor_count, 0);
}

// Tree can be reused after release_all.
TEST(hxrbtree_test, reuse_after_release_all) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.release_all();
	tree.insert(&c);
	EXPECT_EQ(tree.size(), (size_t)1);
	EXPECT_EQ(tree.front()->key(), 3);
	tree.release_all();
}

// lower_bound

// lower_bound on empty tree returns end().
TEST(hxrbtree_test, lower_bound_empty_tree) {
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	EXPECT_EQ(tree.lower_bound(5), tree.end());
}

// lower_bound for a key equal to an existing node returns that node.
TEST(hxrbtree_test, lower_bound_equal_key) {
	hxtest_rbtree_node_t a(2), b(4), c(6);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it_ =
		tree.lower_bound(4);
	EXPECT_EQ(&(*it_), &b);
	tree.release_all();
}

// lower_bound for a key one below an existing node returns that node.
TEST(hxrbtree_test, lower_bound_one_below_existing) {
	hxtest_rbtree_node_t a(2), b(4), c(6);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it_ =
		tree.lower_bound(3);
	EXPECT_EQ(it_->key(), 4);
	tree.release_all();
}

// lower_bound for a key one above the maximum returns end().
TEST(hxrbtree_test, lower_bound_above_maximum_returns_end) {
	hxtest_rbtree_node_t a(2), b(4);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	EXPECT_EQ(tree.lower_bound(5), tree.end());
	tree.release_all();
}

// lower_bound for a key one below the minimum returns the minimum.
TEST(hxrbtree_test, lower_bound_below_minimum_returns_front) {
	hxtest_rbtree_node_t a(2), b(4);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	EXPECT_EQ(tree.lower_bound(1)->key(), 2);
	tree.release_all();
}

// const lower_bound returns the same result.
TEST(hxrbtree_test, lower_bound_const) {
	hxtest_rbtree_node_t a(3), b(7);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>& ct_ = tree;
	EXPECT_EQ(ct_.lower_bound(4)->key(), 7);
	EXPECT_EQ(ct_.lower_bound(8), ct_.end());
	tree.release_all();
}

// upper_bound

// upper_bound on empty tree returns end().
TEST(hxrbtree_test, upper_bound_empty_tree) {
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	EXPECT_EQ(tree.upper_bound(5), tree.end());
}

// upper_bound for a key equal to an existing node returns the next node.
TEST(hxrbtree_test, upper_bound_equal_key) {
	hxtest_rbtree_node_t a(2), b(4), c(6);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it_ =
		tree.upper_bound(4);
	EXPECT_EQ(it_->key(), 6);
	tree.release_all();
}

// upper_bound for the maximum key returns end().
TEST(hxrbtree_test, upper_bound_maximum_key_returns_end) {
	hxtest_rbtree_node_t a(2), b(4), c(6);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	EXPECT_EQ(tree.upper_bound(6), tree.end());
	tree.release_all();
}

// upper_bound for a key one below the minimum returns the minimum.
TEST(hxrbtree_test, upper_bound_below_minimum_returns_front) {
	hxtest_rbtree_node_t a(2), b(4);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	EXPECT_EQ(tree.upper_bound(1)->key(), 2);
	tree.release_all();
}

// upper_bound for a key one above the maximum returns end().
TEST(hxrbtree_test, upper_bound_one_above_maximum_returns_end) {
	hxtest_rbtree_node_t a(2), b(4);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	EXPECT_EQ(tree.upper_bound(5), tree.end());
	tree.release_all();
}

// const upper_bound returns the same result.
TEST(hxrbtree_test, upper_bound_const) {
	hxtest_rbtree_node_t a(3), b(7);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>& ct_ = tree;
	EXPECT_EQ(ct_.upper_bound(3)->key(), 7);
	EXPECT_EQ(ct_.upper_bound(7), ct_.end());
	tree.release_all();
}

// Iterator traversal

// Pre-increment steps through all nodes in ascending order and reaches end().
TEST(hxrbtree_test, iterator_pre_increment) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it_ =
		tree.begin();
	EXPECT_EQ(it_->key(), 1);
	++it_;
	EXPECT_EQ(it_->key(), 2);
	++it_;
	EXPECT_EQ(it_->key(), 3);
	++it_;
	EXPECT_EQ(it_, tree.end());
	tree.release_all();
}

// Post-increment returns the prior position and advances the iterator.
TEST(hxrbtree_test, iterator_post_increment) {
	hxtest_rbtree_node_t a(1), b(2);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it_ =
		tree.begin();
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator old_ = it_++;
	EXPECT_EQ(old_->key(), 1);
	EXPECT_EQ(it_->key(), 2);
	tree.release_all();
}

// Pre-decrement from end() yields the maximum, and stepping back reaches begin().
TEST(hxrbtree_test, iterator_pre_decrement_from_end) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it_ =
		tree.end();
	--it_;
	EXPECT_EQ(it_->key(), 3);
	--it_;
	EXPECT_EQ(it_->key(), 2);
	--it_;
	EXPECT_EQ(it_->key(), 1);
	EXPECT_EQ(it_, tree.begin());
	tree.release_all();
}

// Post-decrement returns the prior position and retreats the iterator.
TEST(hxrbtree_test, iterator_post_decrement) {
	hxtest_rbtree_node_t a(1), b(2);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it_ =
		tree.end();
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator old_ = it_--;
	EXPECT_EQ(old_, tree.end());
	EXPECT_EQ(it_->key(), 2);
	tree.release_all();
}

// Iterator equality and inequality operators.
TEST(hxrbtree_test, iterator_equality) {
	hxtest_rbtree_node_t a(1), b(2);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it1_ =
		tree.begin();
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it2_ =
		tree.begin();
	EXPECT_TRUE(it1_ == it2_);
	EXPECT_FALSE(it1_ != it2_);
	++it2_;
	EXPECT_FALSE(it1_ == it2_);
	EXPECT_TRUE(it1_ != it2_);
	tree.release_all();
}

// Arrow operator on mutable iterator returns a mutable pointer.
TEST(hxrbtree_test, iterator_arrow_operator) {
	hxtest_rbtree_node_t a(1);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it_ =
		tree.begin();
	EXPECT_EQ(it_->key(), 1);
	tree.release_all();
}

// Post-increment and post-decrement on const_iterator return the prior position.
TEST(hxrbtree_test, const_iterator_post_increment_and_decrement) {
	hxtest_rbtree_node_t a(1), b(2);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::const_iterator it_ =
		tree.cbegin();
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::const_iterator old_ = it_++;
	EXPECT_EQ(old_->key(), 1);
	EXPECT_EQ(it_->key(), 2);
	old_ = it_--;
	EXPECT_EQ(old_->key(), 2);
	EXPECT_EQ(it_->key(), 1);
	tree.release_all();
}

// Dereference operator on const_iterator returns a const reference.
TEST(hxrbtree_test, const_iterator_dereference) {
	hxtest_rbtree_node_t a(5);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>& ct_ = tree;
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::const_iterator it_ =
		ct_.begin();
	EXPECT_EQ((*it_).key(), 5);
	tree.release_all();
}

// hxrbtree_map_node

// map_node stores and retrieves key and value independently.
TEST(hxrbtree_test, map_node_key_and_value) {
	using map_node_t = hxrbtree_map_node<int, int>;
	map_node_t n(3, 99);
	EXPECT_EQ(n.key(), 3);
	EXPECT_EQ(n.value(), 99);
}

// Map tree preserves insertion order and mutable value access.
TEST(hxrbtree_test, map_node_mutable_value) {
	using map_node_t = hxrbtree_map_node<int, int>;
	map_node_t a(1, 10), b(2, 20), c(3, 30);
	hxrbtree<map_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	int expected_key = 1;
	int expected_val = 10;
	for(map_node_t& n_ : tree) {
		EXPECT_EQ(n_.key(), expected_key++);
		n_.value() = expected_val + 1;
		expected_val += 10;
	}
	EXPECT_EQ(a.value(), 11);
	EXPECT_EQ(b.value(), 21);
	EXPECT_EQ(c.value(), 31);
	tree.release_all();
}

// Larger sequence: exercises uncle-recolor and double-rotation paths

// Insert 15 nodes in ascending order and verify all are in order.
// Ascending insertion exercises the right-leaning red uncle recolor path
// and the left-right zig-zag rotation repeatedly.
TEST(hxrbtree_test, insert_15_ascending_check_order) {
	const int keys_[15] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	hxtest_rbtree_buf_t<15> nodes(keys_);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	for(size_t i = 0u; i < 15u; ++i) {
		tree.insert(&nodes[i]);
	}
	hxtest_check_order(tree, 15u);
	EXPECT_EQ(tree.front()->key(), 1);
	EXPECT_EQ(tree.back()->key(), 15);
	tree.release_all();
}

// Insert 15 nodes in descending order and verify all are in order.
// Descending insertion exercises the left-leaning red uncle recolor path
// and the right-left zig-zag rotation.
TEST(hxrbtree_test, insert_15_descending_check_order) {
	const int keys_[15] = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
	hxtest_rbtree_buf_t<15> nodes(keys_);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	for(size_t i = 0u; i < 15u; ++i) {
		tree.insert(&nodes[i]);
	}
	hxtest_check_order(tree, 15u);
	EXPECT_EQ(tree.front()->key(), 1);
	EXPECT_EQ(tree.back()->key(), 15);
	tree.release_all();
}

// Erase all nodes from a 7-node tree one at a time verifying order after each.
// Covers erase rebalance loops with black sibling, red sibling, and
// two-black-nephews cases on both left and right sides.
TEST(hxrbtree_test, erase_all_nodes_one_by_one) {
	const int keys_[7] = { 4, 2, 6, 1, 3, 5, 7 };
	hxtest_rbtree_buf_t<7> nodes(keys_);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	for(size_t i = 0u; i < 7u; ++i) {
		tree.insert(&nodes[i]);
	}
	// Erase in key order to exercise the left-child rebalance cases.
	const int erase_order[] = { 1, 2, 3, 4, 5, 6, 7 };
	for(size_t i = 0u; i < 7u; ++i) {
		hxtest_rbtree_node_t* n_ = tree.find(erase_order[i]);
		EXPECT_NE(n_, static_cast<hxtest_rbtree_node_t*>(hxnull));
		tree.erase(n_, hxdo_not_delete());
		hxtest_check_order(tree, static_cast<size_t>(6 - static_cast<int>(i)));
	}
	EXPECT_TRUE(tree.empty());
}

// Erase in reverse key order to exercise the right-child rebalance cases.
TEST(hxrbtree_test, erase_all_nodes_reverse_order) {
	const int keys_[7] = { 4, 2, 6, 1, 3, 5, 7 };
	hxtest_rbtree_buf_t<7> nodes(keys_);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	for(size_t i = 0u; i < 7u; ++i) {
		tree.insert(&nodes[i]);
	}
	const int erase_order[] = { 7, 6, 5, 4, 3, 2, 1 };
	for(size_t i = 0u; i < 7u; ++i) {
		hxtest_rbtree_node_t* n_ = tree.find(erase_order[i]);
		EXPECT_NE(n_, static_cast<hxtest_rbtree_node_t*>(hxnull));
		tree.erase(n_, hxdo_not_delete());
		hxtest_check_order(tree, static_cast<size_t>(6 - static_cast<int>(i)));
	}
	EXPECT_TRUE(tree.empty());
}

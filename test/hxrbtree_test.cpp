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

// Plain node exposing copy/move for hxrbtree_node operator tests.
struct hxtest_rbtree_base_node_t : hxrbtree_node {
	using key_t = int;
	explicit hxtest_rbtree_base_node_t(int k) : m_key(k) { }
	const int& rbtree_key(void) const { return m_key; }
	static bool rbtree_less(const int& a_, const int& b_) { return a_ < b_; }
	int m_key;
};

// Aligned storage for N hxtest_rbtree_node_t objects constructed in-place from
// an array of int keys. C++11-compatible: avoids copy or move construction.
template<size_t n_t>
struct hxtest_rbtree_buf_t {
	hxtest_rbtree_buf_t(const int (&keys)[n_t]) {
		for(size_t i_ = 0u; i_ < n_t; ++i_) {
			::new(static_cast<void*>(&m_buf[i_])) hxtest_rbtree_node_t(keys[i_]);
		}
	}
	~hxtest_rbtree_buf_t(void) {
		for(size_t i_ = n_t; i_-- > 0u;) {
			reinterpret_cast<hxtest_rbtree_node_t*>(&m_buf[i_])->~hxtest_rbtree_node_t();
		}
	}
	hxtest_rbtree_node_t& operator[](size_t i_) {
		return *reinterpret_cast<hxtest_rbtree_node_t*>(&m_buf[i_]);
	}
	const hxtest_rbtree_node_t& operator[](size_t i_) const {
		return *reinterpret_cast<const hxtest_rbtree_node_t*>(&m_buf[i_]);
	}
	hxtest_rbtree_buf_t(const hxtest_rbtree_buf_t&) = delete;
	void operator=(const hxtest_rbtree_buf_t&) = delete;
	alignas(hxtest_rbtree_node_t) unsigned char m_buf[n_t][sizeof(hxtest_rbtree_node_t)];
};

struct hxtest_rbtree_custom_deleter_t {
	void operator()(hxtest_rbtree_node_t* ptr) const {
		++s_hxtest_custom_deleter_count;
		hxdelete(ptr);
	}
	operator bool(void) const { return true; }
};

// Validates that iteration visits keys in strictly ascending order and
// the count matches the expected size.
template<bool multi_t, typename deleter_t>
static void hxtest_check_order(
	const hxrbtree<hxtest_rbtree_node_t, multi_t, deleter_t>& tree,
	size_t expected_size) {
	size_t count = 0u;
	int prev = -2147483647 - 1;
	for(const hxtest_rbtree_node_t& n : tree) {
		EXPECT_LT(prev, n.rbtree_key());
		prev = n.rbtree_key();
		++count;
	}
	EXPECT_EQ(count, expected_size);
	EXPECT_EQ(tree.size(), expected_size);
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
	const int keys[7] = { 4, 2, 6, 1, 3, 5, 7 };
	hxtest_rbtree_buf_t<7> nodes(keys);
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
	size_t count = 0u;
	for(const hxtest_rbtree_node_t& n : tree) {
		EXPECT_EQ(n.rbtree_key(), 3);
		++count;
	}
	EXPECT_EQ(count, (size_t)3);
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
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>& ct = tree;
	EXPECT_EQ(ct.find(7), &a);
	EXPECT_EQ(ct.find(8), (const hxtest_rbtree_node_t*)hxnull);
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
	EXPECT_EQ(tree.front()->rbtree_key(), 1);
	EXPECT_EQ(tree.back()->rbtree_key(), 5);
	tree.release_all();
}

// const front and back return the correct nodes.
TEST(hxrbtree_test, front_back_const) {
	hxtest_rbtree_node_t a(2), b(8);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>& ct = tree;
	EXPECT_EQ(ct.front()->rbtree_key(), 2);
	EXPECT_EQ(ct.back()->rbtree_key(), 8);
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
	hxtest_rbtree_node_t* popped = tree.pop_front();
	EXPECT_EQ(popped, &a);
	EXPECT_EQ(tree.size(), (size_t)2);
	EXPECT_EQ(tree.front()->rbtree_key(), 2);
	tree.release_all();
}

// pop_back removes and returns the maximum; tree shrinks.
TEST(hxrbtree_test, pop_back_removes_maximum) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	hxtest_rbtree_node_t* popped = tree.pop_back();
	EXPECT_EQ(popped, &c);
	EXPECT_EQ(tree.size(), (size_t)2);
	EXPECT_EQ(tree.back()->rbtree_key(), 2);
	tree.release_all();
}

// pop_front the last node leaves tree empty and returns null on the next call.
TEST(hxrbtree_test, pop_front_to_empty) {
	hxtest_rbtree_node_t a(1);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	hxtest_rbtree_node_t* popped = tree.pop_front();
	EXPECT_EQ(popped, &a);
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
	const hxsystem_allocator_scope scope(hxsystem_allocator_temporary_stack);
	s_hxtest_destructor_count = 0;
	hxtest_rbtree_node_t* n = hxnew<hxtest_rbtree_node_t>(1);
	hxrbtree<hxtest_rbtree_node_t> tree;
	tree.insert(n);
	hxtest_rbtree_node_t* ex = tree.extract(n);
	EXPECT_EQ(ex, n);
	EXPECT_EQ(s_hxtest_destructor_count, 0);
	EXPECT_TRUE(tree.empty());
	hxdelete(n);
}

// erase

// erase with default deleter calls destructor exactly once.
TEST(hxrbtree_test, erase_calls_destructor) {
	const hxsystem_allocator_scope scope(hxsystem_allocator_temporary_stack);
	s_hxtest_destructor_count = 0;
	hxrbtree<hxtest_rbtree_node_t> tree;
	tree.insert(hxnew<hxtest_rbtree_node_t>(1));
	tree.insert(hxnew<hxtest_rbtree_node_t>(2));
	tree.insert(hxnew<hxtest_rbtree_node_t>(3));
	hxtest_rbtree_node_t* front = tree.front();
	tree.erase(front);
	EXPECT_EQ(s_hxtest_destructor_count, 1);
	EXPECT_EQ(tree.size(), (size_t)2);
	EXPECT_EQ(tree.front()->rbtree_key(), 2);
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
	const hxsystem_allocator_scope scope(hxsystem_allocator_temporary_stack);
	s_hxtest_custom_deleter_count = 0;
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	hxtest_rbtree_node_t* n = hxnew<hxtest_rbtree_node_t>(7);
	tree.insert(n);
	tree.erase(n, hxtest_rbtree_custom_deleter_t());
	EXPECT_EQ(s_hxtest_custom_deleter_count, 1);
	EXPECT_TRUE(tree.empty());
}

// Erase a node with two children: successor splice and rebalance.
TEST(hxrbtree_test, erase_node_with_two_children) {
	const int keys[6] = { 4, 2, 6, 1, 3, 5 };
	hxtest_rbtree_buf_t<6> nodes(keys);
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
	const hxsystem_allocator_scope scope(hxsystem_allocator_temporary_stack);
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
	const hxsystem_allocator_scope scope(hxsystem_allocator_temporary_stack);
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
	EXPECT_EQ(tree.front()->rbtree_key(), 3);
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
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it =
		tree.lower_bound(4);
	EXPECT_EQ(&(*it), &b);
	tree.release_all();
}

// lower_bound for a key one below an existing node returns that node.
TEST(hxrbtree_test, lower_bound_one_below_existing) {
	hxtest_rbtree_node_t a(2), b(4), c(6);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it =
		tree.lower_bound(3);
	EXPECT_EQ(it->rbtree_key(), 4);
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
	EXPECT_EQ(tree.lower_bound(1)->rbtree_key(), 2);
	tree.release_all();
}

// const lower_bound returns the same result.
TEST(hxrbtree_test, lower_bound_const) {
	hxtest_rbtree_node_t a(3), b(7);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>& ct = tree;
	EXPECT_EQ(ct.lower_bound(4)->rbtree_key(), 7);
	EXPECT_EQ(ct.lower_bound(8), ct.end());
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
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it =
		tree.upper_bound(4);
	EXPECT_EQ(it->rbtree_key(), 6);
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
	EXPECT_EQ(tree.upper_bound(1)->rbtree_key(), 2);
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
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>& ct = tree;
	EXPECT_EQ(ct.upper_bound(3)->rbtree_key(), 7);
	EXPECT_EQ(ct.upper_bound(7), ct.end());
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
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it =
		tree.begin();
	EXPECT_EQ(it->rbtree_key(), 1);
	++it;
	EXPECT_EQ(it->rbtree_key(), 2);
	++it;
	EXPECT_EQ(it->rbtree_key(), 3);
	++it;
	EXPECT_EQ(it, tree.end());
	tree.release_all();
}

// Post-increment returns the prior position and advances the iterator.
TEST(hxrbtree_test, iterator_post_increment) {
	hxtest_rbtree_node_t a(1), b(2);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it =
		tree.begin();
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator old = it++;
	EXPECT_EQ(old->rbtree_key(), 1);
	EXPECT_EQ(it->rbtree_key(), 2);
	tree.release_all();
}

// Pre-decrement from end() yields the maximum, and stepping back reaches begin().
TEST(hxrbtree_test, iterator_pre_decrement_from_end) {
	hxtest_rbtree_node_t a(1), b(2), c(3);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it =
		tree.end();
	--it;
	EXPECT_EQ(it->rbtree_key(), 3);
	--it;
	EXPECT_EQ(it->rbtree_key(), 2);
	--it;
	EXPECT_EQ(it->rbtree_key(), 1);
	EXPECT_EQ(it, tree.begin());
	tree.release_all();
}

// Post-decrement returns the prior position and retreats the iterator.
TEST(hxrbtree_test, iterator_post_decrement) {
	hxtest_rbtree_node_t a(1), b(2);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it =
		tree.end();
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator old = it--;
	EXPECT_EQ(old, tree.end());
	EXPECT_EQ(it->rbtree_key(), 2);
	tree.release_all();
}

// Iterator equality and inequality operators.
TEST(hxrbtree_test, iterator_equality) {
	hxtest_rbtree_node_t a(1), b(2);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it1 =
		tree.begin();
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it2 =
		tree.begin();
	EXPECT_TRUE(it1 == it2);
	EXPECT_FALSE(it1 != it2);
	++it2;
	EXPECT_FALSE(it1 == it2);
	EXPECT_TRUE(it1 != it2);
	tree.release_all();
}

// Arrow operator on mutable iterator returns a mutable pointer.
TEST(hxrbtree_test, iterator_arrow_operator) {
	hxtest_rbtree_node_t a(1);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::iterator it =
		tree.begin();
	EXPECT_EQ(it->rbtree_key(), 1);
	tree.release_all();
}

// Post-increment and post-decrement on const_iterator return the prior position.
TEST(hxrbtree_test, const_iterator_post_increment_and_decrement) {
	hxtest_rbtree_node_t a(1), b(2);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::const_iterator it =
		tree.cbegin();
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::const_iterator old = it++;
	EXPECT_EQ(old->rbtree_key(), 1);
	EXPECT_EQ(it->rbtree_key(), 2);
	old = it--;
	EXPECT_EQ(old->rbtree_key(), 2);
	EXPECT_EQ(it->rbtree_key(), 1);
	tree.release_all();
}

// Dereference operator on const_iterator returns a const reference.
TEST(hxrbtree_test, const_iterator_dereference) {
	hxtest_rbtree_node_t a(5);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>& ct = tree;
	const hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete>::const_iterator it =
		ct.begin();
	EXPECT_EQ((*it).rbtree_key(), 5);
	tree.release_all();
}

// hxrbtree_set_node and hxrbtree_map_node

// hxrbtree_set_node used directly as node_t: insert, find, iterate in order.
TEST(hxrbtree_test, set_node_as_node_type) {
	hxrbtree_set_node<int> a(1), b(2), c(3);
	hxrbtree<hxrbtree_set_node<int>, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	EXPECT_EQ(tree.find(2), &b);
	EXPECT_EQ(tree.find(4), (hxrbtree_set_node<int>*)hxnull);
	int expected = 1;
	for(const hxrbtree_set_node<int>& n : tree) {
		EXPECT_EQ(n.rbtree_key(), expected++);
	}
	tree.release_all();
}

// map_node stores and retrieves key and value independently.
TEST(hxrbtree_test, map_node_key_and_value) {
	using map_node_t = hxrbtree_map_node<int, int>;
	// key-only constructor: value is default-initialized.
	map_node_t d(5);
	EXPECT_EQ(d.rbtree_key(), 5);
	EXPECT_EQ(d.value(), 0);
	// key+value constructor.
	map_node_t n(3, 99);
	EXPECT_EQ(n.rbtree_key(), 3);
	EXPECT_EQ(n.value(), 99);
	// const value() accessor.
	const map_node_t& cn = n;
	EXPECT_EQ(cn.value(), 99);
}

// Map tree preserves insertion order, find, and mutable value access.
TEST(hxrbtree_test, map_node_mutable_value) {
	using map_node_t = hxrbtree_map_node<int, int>;
	map_node_t a(1, 10), b(2, 20), c(3, 30);
	hxrbtree<map_node_t, false, hxdo_not_delete> tree;
	tree.insert(&a);
	tree.insert(&b);
	tree.insert(&c);
	EXPECT_EQ(tree.find(2), &b);
	EXPECT_EQ(tree.find(4), (map_node_t*)hxnull);
	int expected_key = 1;
	int expected_val = 10;
	for(map_node_t& n : tree) {
		EXPECT_EQ(n.rbtree_key(), expected_key++);
		n.value() = expected_val + 1;
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
	const int keys[15] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	hxtest_rbtree_buf_t<15> nodes(keys);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	for(size_t i = 0u; i < 15u; ++i) {
		tree.insert(&nodes[i]);
	}
	hxtest_check_order(tree, 15u);
	EXPECT_EQ(tree.front()->rbtree_key(), 1);
	EXPECT_EQ(tree.back()->rbtree_key(), 15);
	tree.release_all();
}

// Insert 15 nodes in descending order and verify all are in order.
// Descending insertion exercises the left-leaning red uncle recolor path
// and the right-left zig-zag rotation.
TEST(hxrbtree_test, insert_15_descending_check_order) {
	const int keys[15] = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
	hxtest_rbtree_buf_t<15> nodes(keys);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	for(size_t i = 0u; i < 15u; ++i) {
		tree.insert(&nodes[i]);
	}
	hxtest_check_order(tree, 15u);
	EXPECT_EQ(tree.front()->rbtree_key(), 1);
	EXPECT_EQ(tree.back()->rbtree_key(), 15);
	tree.release_all();
}

// Erase all nodes from a 7-node tree one at a time verifying order after each.
// Covers erase rebalance loops with black sibling, red sibling, and
// two-black-nephews cases on both left and right sides.
TEST(hxrbtree_test, erase_all_nodes_one_by_one) {
	const int keys[7] = { 4, 2, 6, 1, 3, 5, 7 };
	hxtest_rbtree_buf_t<7> nodes(keys);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	for(size_t i = 0u; i < 7u; ++i) {
		tree.insert(&nodes[i]);
	}
	// Erase in key order to exercise the left-child rebalance cases.
	const int erase_order[] = { 1, 2, 3, 4, 5, 6, 7 };
	for(size_t i = 0u; i < 7u; ++i) {
		hxtest_rbtree_node_t* n = tree.find(erase_order[i]);
		EXPECT_NE(n, static_cast<hxtest_rbtree_node_t*>(hxnull));
		tree.erase(n, hxdo_not_delete());
		hxtest_check_order(tree, static_cast<size_t>(6 - static_cast<int>(i)));
	}
	EXPECT_TRUE(tree.empty());
}

// Erase in reverse key order to exercise the right-child rebalance cases.
TEST(hxrbtree_test, erase_all_nodes_reverse_order) {
	const int keys[7] = { 4, 2, 6, 1, 3, 5, 7 };
	hxtest_rbtree_buf_t<7> nodes(keys);
	hxrbtree<hxtest_rbtree_node_t, false, hxdo_not_delete> tree;
	for(size_t i = 0u; i < 7u; ++i) {
		tree.insert(&nodes[i]);
	}
	const int erase_order[] = { 7, 6, 5, 4, 3, 2, 1 };
	for(size_t i = 0u; i < 7u; ++i) {
		hxtest_rbtree_node_t* n = tree.find(erase_order[i]);
		EXPECT_NE(n, static_cast<hxtest_rbtree_node_t*>(hxnull));
		tree.erase(n, hxdo_not_delete());
		hxtest_check_order(tree, static_cast<size_t>(6 - static_cast<int>(i)));
	}
	EXPECT_TRUE(tree.empty());
}

// copy/move construct produce insertable unlinked nodes; copy/move assign leave
// tree linkage of the destination unchanged.
TEST(hxrbtree_node_test, copy_move_construct_and_assign) {
	const hxtest_rbtree_base_node_t a(1), b(2), c(3), d(4);
	hxrbtree<hxtest_rbtree_base_node_t, false, hxdo_not_delete> tree;
	hxtest_rbtree_base_node_t e(a);
	tree.insert(&e);
	hxtest_rbtree_base_node_t f(hxmove(b));
	tree.insert(&f);
	EXPECT_EQ(tree.size(), (size_t)2);
	// copy-assign: e stays linked, source c is unchanged.
	e = c;
	EXPECT_EQ(tree.front(), &e);
	// move-assign: f stays linked.
	f = hxmove(d);
	EXPECT_EQ(tree.back(), &f);
	EXPECT_EQ(tree.size(), (size_t)2);
	tree.release_all();
}

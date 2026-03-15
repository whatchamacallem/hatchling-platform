// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxlist.hpp>
#include <hx/hxtest.hpp>

namespace {

struct hxtest_list_node_t : hxlist_node {
	explicit hxtest_list_node_t(int v) : value(v) { }
	int value;
};

// A node type that tracks destructor calls to verify deleter behavior.
static int s_hxtest_destructor_count = 0;

struct hxtest_list_counted_node_t : hxlist_node {
	explicit hxtest_list_counted_node_t(int v) : value(v) { }
	~hxtest_list_counted_node_t(void) { ++s_hxtest_destructor_count; }
	int value;
};

// A custom deleter that tracks how many times it is called.
static int s_hxtest_custom_deleter_count = 0;

struct hxtest_list_custom_deleter_t {
	void operator()(hxtest_list_counted_node_t* ptr) const {
		++s_hxtest_custom_deleter_count;
		hxdelete(ptr);
	}
	operator bool(void) const { return true; }
};

} // namespace


// Newly constructed list is empty, size is 0, begin equals end.
TEST(hxlist_test, empty_on_construction) {
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	EXPECT_TRUE(list.empty());
	EXPECT_EQ(list.size(), (size_t)0);
	EXPECT_EQ(list.begin(), list.end());
}

// push_back preserves insertion order and size over three nodes.
TEST(hxlist_test, push_back_and_iterate) {
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	hxtest_list_node_t a(1), b(2), c(3);
	list.push_back(&a);
	list.push_back(&b);
	list.push_back(&c);
	EXPECT_EQ(list.size(), (size_t)3);
	EXPECT_FALSE(list.empty());
	int expected = 1;
	for(const hxtest_list_node_t& n : list) {
		EXPECT_EQ(n.value, expected++);
	}
	EXPECT_EQ(expected, 4);
	list.release_all();
}

// push_back two nodes: front is first pushed, back is second pushed.
TEST(hxlist_test, push_back_two_nodes_order) {
	hxtest_list_node_t a(10), b(20);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	EXPECT_EQ(list.front(), &a);
	EXPECT_EQ(list.back(), &b);
	list.release_all();
}

// push_front reverses insertion order so iteration is 1,2,3.
TEST(hxlist_test, push_front_and_iterate) {
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	hxtest_list_node_t a(1), b(2), c(3);
	list.push_front(&c);
	list.push_front(&b);
	list.push_front(&a);
	EXPECT_EQ(list.size(), (size_t)3);
	int expected = 1;
	for(const hxtest_list_node_t& n : list) {
		EXPECT_EQ(n.value, expected++);
	}
	EXPECT_EQ(expected, 4);
	list.release_all();
}

// push_front two nodes: most-recently pushed is front, first pushed is back.
TEST(hxlist_test, push_front_two_nodes_order) {
	hxtest_list_node_t a(10), b(20);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_front(&a);
	list.push_front(&b);
	EXPECT_EQ(list.front(), &b);
	EXPECT_EQ(list.back(), &a);
	list.release_all();
}

// pop_front on empty list returns null (documented behavior).
TEST(hxlist_test, pop_front_empty_returns_null) {
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	EXPECT_EQ(list.pop_front(), (hxtest_list_node_t*)hxnull);
}

// pop_back on empty list returns null (documented behavior).
TEST(hxlist_test, pop_back_empty_returns_null) {
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	EXPECT_EQ(list.pop_back(), (hxtest_list_node_t*)hxnull);
}

// pop_front and pop_back remove from the correct ends, leaving the middle intact,
// and return null once the list is empty.
TEST(hxlist_test, pop_front_and_pop_back) {
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	hxtest_list_node_t a(1), b(2), c(3);
	list.push_back(&a);
	list.push_back(&b);
	list.push_back(&c);
	EXPECT_EQ(list.pop_front()->value, 1);
	EXPECT_EQ(list.size(), (size_t)2);
	EXPECT_EQ(list.front()->value, 2);
	EXPECT_EQ(list.pop_back()->value, 3);
	EXPECT_EQ(list.size(), (size_t)1);
	EXPECT_EQ(list.pop_front()->value, 2);
	EXPECT_TRUE(list.empty());
	EXPECT_EQ(list.pop_front(), (hxtest_list_node_t*)hxnull);
}

// insert_before the front places new node first.
TEST(hxlist_test, insert_before_front) {
	hxtest_list_node_t a(2), b(3), c(1);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	list.insert_before(&a, &c);
	EXPECT_EQ(list.size(), (size_t)3);
	EXPECT_EQ(list.front(), &c);
	int expected = 1;
	for(const hxtest_list_node_t& n : list) {
		EXPECT_EQ(n.value, expected++);
	}
	list.release_all();
}

// insert_before a middle node places the new node between its neighbors.
TEST(hxlist_test, insert_before_middle) {
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	hxtest_list_node_t a(1), b(3), mid(2);
	list.push_back(&a);
	list.push_back(&b);
	list.insert_before(&b, &mid);
	EXPECT_EQ(list.size(), (size_t)3);
	int expected = 1;
	for(const hxtest_list_node_t& n : list) {
		EXPECT_EQ(n.value, expected++);
	}
	EXPECT_EQ(expected, 4);
	list.release_all();
}

// insert_after the back places new node at the tail.
TEST(hxlist_test, insert_after_back) {
	hxtest_list_node_t a(1), b(2), c(3);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	list.insert_after(&b, &c);
	EXPECT_EQ(list.size(), (size_t)3);
	EXPECT_EQ(list.back(), &c);
	int expected = 1;
	for(const hxtest_list_node_t& n : list) {
		EXPECT_EQ(n.value, expected++);
	}
	list.release_all();
}

// insert_after a middle node places the new node between its neighbors.
TEST(hxlist_test, insert_after_middle) {
	hxtest_list_node_t a(1), b(3), c(2);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	list.insert_after(&a, &c);
	EXPECT_EQ(list.size(), (size_t)3);
	int expected = 1;
	for(const hxtest_list_node_t& n : list) {
		EXPECT_EQ(n.value, expected++);
	}
	list.release_all();
}

// extract the only node leaves list empty and returns that node.
TEST(hxlist_test, extract_single_node) {
	hxtest_list_node_t a(1);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	hxtest_list_node_t* result = list.extract(&a);
	EXPECT_EQ(result, &a);
	EXPECT_TRUE(list.empty());
}

// extract front, back, and middle nodes verify neighbor relinking in all cases.
TEST(hxlist_test, extract_and_erase) {
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	hxtest_list_node_t a(1), b(2), c(3);
	list.push_back(&a);
	list.push_back(&b);
	list.push_back(&c);
	list.extract(&b);
	EXPECT_EQ(list.size(), (size_t)2);
	EXPECT_EQ(list.front()->value, 1);
	EXPECT_EQ(list.back()->value, 3);
	list.extract(&a);
	EXPECT_EQ(list.size(), (size_t)1);
	EXPECT_EQ(list.front()->value, 3);
	list.extract(&c);
	EXPECT_TRUE(list.empty());
}

// erase with hxdefault_delete calls destructor on the erased node and on clear.
TEST(hxlist_test, erase_with_default_deleter_calls_destructor) {
	const hxsystem_allocator_scope scope(hxsystem_allocator_temporary_stack);
	s_hxtest_destructor_count = 0;
	hxlist<hxtest_list_counted_node_t> list;
	list.push_back(hxnew<hxtest_list_counted_node_t>(1));
	list.push_back(hxnew<hxtest_list_counted_node_t>(2));
	list.push_back(hxnew<hxtest_list_counted_node_t>(3));
	list.erase(list.front());
	EXPECT_EQ(s_hxtest_destructor_count, 1);
	EXPECT_EQ(list.size(), (size_t)2);
	EXPECT_EQ(list.front()->value, 2);
	list.clear();
	EXPECT_EQ(s_hxtest_destructor_count, 3);
}

// erase with hxdo_not_delete override does not call destructor.
TEST(hxlist_test, erase_with_do_not_delete_override) {
	s_hxtest_destructor_count = 0;
	hxtest_list_counted_node_t a(1);
	hxlist<hxtest_list_counted_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.erase(&a);
	EXPECT_EQ(s_hxtest_destructor_count, 0);
	EXPECT_TRUE(list.empty());
}

// erase with a custom deleter override is called exactly once per node.
TEST(hxlist_test, erase_with_custom_deleter_override) {
	const hxsystem_allocator_scope scope(hxsystem_allocator_temporary_stack);
	s_hxtest_custom_deleter_count = 0;
	hxtest_list_counted_node_t* n = hxnew<hxtest_list_counted_node_t>(42);
	hxlist<hxtest_list_counted_node_t, hxdo_not_delete> list;
	list.push_back(n);
	list.erase(n, hxtest_list_custom_deleter_t());
	EXPECT_EQ(s_hxtest_custom_deleter_count, 1);
	EXPECT_TRUE(list.empty());
}

// clear on an empty list is a no-op (m_size != 0 branch not taken).
TEST(hxlist_test, clear_empty_list) {
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.clear();
	EXPECT_TRUE(list.empty());
	EXPECT_EQ(list.size(), (size_t)0);
}

// clear with hxdo_not_delete override unlinks nodes without calling destructor.
TEST(hxlist_test, clear_with_do_not_delete_override) {
	s_hxtest_destructor_count = 0;
	hxtest_list_counted_node_t a(1), b(2);
	hxlist<hxtest_list_counted_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	list.clear(hxdo_not_delete());
	EXPECT_EQ(s_hxtest_destructor_count, 0);
	EXPECT_TRUE(list.empty());
	EXPECT_EQ(list.size(), (size_t)0);
}

// clear with custom deleter override is called for each node.
TEST(hxlist_test, clear_with_custom_deleter_override) {
	const hxsystem_allocator_scope scope(hxsystem_allocator_temporary_stack);
	s_hxtest_custom_deleter_count = 0;
	hxlist<hxtest_list_counted_node_t, hxdo_not_delete> list;
	list.push_back(hxnew<hxtest_list_counted_node_t>(1));
	list.push_back(hxnew<hxtest_list_counted_node_t>(2));
	list.clear(hxtest_list_custom_deleter_t());
	EXPECT_EQ(s_hxtest_custom_deleter_count, 2);
	EXPECT_TRUE(list.empty());
}

// release_all resets size and sentinel linkage without invoking deleter.
TEST(hxlist_test, release_all_resets_state) {
	hxtest_list_node_t a(1), b(2);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	list.release_all();
	EXPECT_TRUE(list.empty());
	EXPECT_EQ(list.size(), (size_t)0);
	EXPECT_EQ(list.begin(), list.end());
}

// The list can be reused after release_all.
TEST(hxlist_test, reuse_after_release_all) {
	hxtest_list_node_t a(1), b(2), c(3);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	list.release_all();
	list.push_back(&c);
	EXPECT_EQ(list.size(), (size_t)1);
	EXPECT_EQ(list.front()->value, 3);
	list.release_all();
}

// Destructor calls clear() which invokes deleter on remaining nodes.
TEST(hxlist_test, destructor_calls_clear) {
	const hxsystem_allocator_scope scope(hxsystem_allocator_temporary_stack);
	s_hxtest_destructor_count = 0;
	{
		hxlist<hxtest_list_counted_node_t> list;
		list.push_back(hxnew<hxtest_list_counted_node_t>(1));
		list.push_back(hxnew<hxtest_list_counted_node_t>(2));
	}
	EXPECT_EQ(s_hxtest_destructor_count, 2);
}

// Pre-increment steps through all nodes and reaches end() exactly.
TEST(hxlist_test, iterator_pre_increment) {
	hxtest_list_node_t a(1), b(2), c(3);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	list.push_back(&c);
	hxlist<hxtest_list_node_t, hxdo_not_delete>::iterator it = list.begin();
	EXPECT_EQ(it->value, 1);
	++it;
	EXPECT_EQ(it->value, 2);
	++it;
	EXPECT_EQ(it->value, 3);
	++it;
	EXPECT_EQ(it, list.end());
	list.release_all();
}

// Post-increment returns old position and advances iterator.
TEST(hxlist_test, iterator_post_increment) {
	hxtest_list_node_t a(1), b(2);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	hxlist<hxtest_list_node_t, hxdo_not_delete>::iterator it = list.begin();
	const hxlist<hxtest_list_node_t, hxdo_not_delete>::iterator old = it++;
	EXPECT_EQ(old->value, 1);
	EXPECT_EQ(it->value, 2);
	list.release_all();
}

// Pre-decrement steps backward through all nodes and reaches begin() exactly.
TEST(hxlist_test, iterator_pre_decrement) {
	hxtest_list_node_t a(1), b(2), c(3);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	list.push_back(&c);
	hxlist<hxtest_list_node_t, hxdo_not_delete>::iterator it = list.end();
	--it;
	EXPECT_EQ(it->value, 3);
	--it;
	EXPECT_EQ(it->value, 2);
	--it;
	EXPECT_EQ(it->value, 1);
	EXPECT_EQ(it, list.begin());
	list.release_all();
}

// Post-decrement returns old position and retreats iterator.
TEST(hxlist_test, iterator_post_decrement) {
	hxtest_list_node_t a(1), b(2);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	hxlist<hxtest_list_node_t, hxdo_not_delete>::iterator it = list.end();
	const hxlist<hxtest_list_node_t, hxdo_not_delete>::iterator old = it--;
	EXPECT_EQ(old, list.end());
	EXPECT_EQ(it->value, 2);
	list.release_all();
}

// Iterator equality and inequality operators.
TEST(hxlist_test, iterator_equality) {
	hxtest_list_node_t a(1), b(2);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	const hxlist<hxtest_list_node_t, hxdo_not_delete>::iterator it1 = list.begin();
	hxlist<hxtest_list_node_t, hxdo_not_delete>::iterator it2 = list.begin();
	EXPECT_TRUE(it1 == it2);
	EXPECT_FALSE(it1 != it2);
	++it2;
	EXPECT_FALSE(it1 == it2);
	EXPECT_TRUE(it1 != it2);
	list.release_all();
}

// Arrow operator on mutable iterator returns mutable pointer.
TEST(hxlist_test, iterator_arrow_operator) {
	hxtest_list_node_t a(1);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	const hxlist<hxtest_list_node_t, hxdo_not_delete>::iterator it = list.begin();
	it->value = 99;
	EXPECT_EQ(a.value, 99);
	list.release_all();
}

// Post-increment and post-decrement on const_iterator return the prior position.
TEST(hxlist_test, const_iterator_post_increment_and_decrement) {
	hxtest_list_node_t a(1), b(2);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	hxlist<hxtest_list_node_t, hxdo_not_delete>::const_iterator it = list.begin();
	hxlist<hxtest_list_node_t, hxdo_not_delete>::const_iterator old = it++;
	EXPECT_EQ(old->value, 1);
	EXPECT_EQ(it->value, 2);
	old = it--;
	EXPECT_EQ(old->value, 2);
	EXPECT_EQ(it->value, 1);
	list.release_all();
}

// Interleaved push_front and push_back produce correct order.
TEST(hxlist_test, mixed_push_front_and_push_back) {
	hxtest_list_node_t a(1), b(4), c(2), d(3);
	hxlist<hxtest_list_node_t, hxdo_not_delete> list;
	list.push_back(&a);
	list.push_back(&b);
	list.push_front(&c); // front: c(2), a(1), b(4)
	list.insert_before(&b, &d); // front: c(2), a(1), d(3), b(4)
	// Expected order by value: 2, 1, 3, 4.
	const int expected[] = { 2, 1, 3, 4 };
	int idx = 0;
	for(const hxtest_list_node_t& n : list) {
		EXPECT_EQ(n.value, expected[idx++]);
	}
	EXPECT_EQ(idx, 4);
	list.release_all();
}

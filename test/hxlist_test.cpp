// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxlist.hpp>
#include <hx/hxtest.hpp>

namespace {

struct hxtest_list_node : hxlist_node<hxtest_list_node> {
	explicit hxtest_list_node(int value_) : value(value_) { }
	int value;
};

} // namespace

TEST(hxlist_test, push_back_and_iterate) {
	hxlist<hxtest_list_node> list_;
	hxtest_list_node a_(1), b_(2), c_(3);
	list_.push_back(&a_);
	list_.push_back(&b_);
	list_.push_back(&c_);
	EXPECT_EQ(list_.size(), (size_t)3);
	int expected_ = 1;
	for(const hxtest_list_node& n_ : list_) {
		EXPECT_EQ(n_.value, expected_++);
	}
	list_.release_all();
}

TEST(hxlist_test, push_front_and_iterate) {
	hxlist<hxtest_list_node> list_;
	hxtest_list_node a_(1), b_(2), c_(3);
	list_.push_front(&c_);
	list_.push_front(&b_);
	list_.push_front(&a_);
	EXPECT_EQ(list_.size(), (size_t)3);
	int expected_ = 1;
	for(const hxtest_list_node& n_ : list_) {
		EXPECT_EQ(n_.value, expected_++);
	}
	list_.release_all();
}

TEST(hxlist_test, pop_front_and_pop_back) {
	hxlist<hxtest_list_node> list_;
	hxtest_list_node a_(1), b_(2), c_(3);
	list_.push_back(&a_);
	list_.push_back(&b_);
	list_.push_back(&c_);
	EXPECT_EQ(list_.pop_front()->value, 1);
	EXPECT_EQ(list_.pop_back()->value, 3);
	EXPECT_EQ(list_.size(), (size_t)1);
	EXPECT_EQ(list_.pop_front()->value, 2);
	EXPECT_TRUE(list_.empty());
	EXPECT_EQ(list_.pop_front(), (hxtest_list_node*)hxnull);
}

TEST(hxlist_test, front_and_back) {
	hxlist<hxtest_list_node> list_;
	hxtest_list_node a_(10), b_(20);
	list_.push_back(&a_);
	list_.push_back(&b_);
	EXPECT_EQ(list_.front()->value, 10);
	EXPECT_EQ(list_.back()->value, 20);
	list_.release_all();
}

TEST(hxlist_test, extract_and_erase) {
	hxlist<hxtest_list_node, hxdo_not_delete> list_;
	hxtest_list_node a_(1), b_(2), c_(3);
	list_.push_back(&a_);
	list_.push_back(&b_);
	list_.push_back(&c_);
	list_.extract(&b_);
	EXPECT_EQ(list_.size(), (size_t)2);
	EXPECT_EQ(list_.front()->value, 1);
	EXPECT_EQ(list_.back()->value, 3);
	list_.erase(&a_);
	EXPECT_EQ(list_.size(), (size_t)1);
	list_.erase(&c_);
	EXPECT_TRUE(list_.empty());
}

TEST(hxlist_test, insert_before_and_after) {
	hxlist<hxtest_list_node> list_;
	hxtest_list_node a_(1), b_(3), mid_(2);
	list_.push_back(&a_);
	list_.push_back(&b_);
	list_.insert_before(&b_, &mid_);
	EXPECT_EQ(list_.size(), (size_t)3);
	int expected_ = 1;
	for(const hxtest_list_node& n_ : list_) {
		EXPECT_EQ(n_.value, expected_++);
	}
	list_.release_all();
}

TEST(hxlist_test, reverse_iterate) {
	hxlist<hxtest_list_node> list_;
	hxtest_list_node a_(1), b_(2), c_(3);
	list_.push_back(&a_);
	list_.push_back(&b_);
	list_.push_back(&c_);
	hxlist<hxtest_list_node>::iterator it_ = list_.end();
	--it_;
	EXPECT_EQ(it_->value, 3);
	--it_;
	EXPECT_EQ(it_->value, 2);
	--it_;
	EXPECT_EQ(it_->value, 1);
	list_.release_all();
}

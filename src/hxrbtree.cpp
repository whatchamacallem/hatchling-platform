// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hxrbtree.hpp"

// rb_first_ / rb_last_ / rb_next_ / rb_prev_

hxrbtree_node* hxrbtree_rb_first_(hxrbtree_node* root_) {
	if(root_ == hxnull) {
		return hxnull;
	}
	while(root_->m_rb_left_ != hxnull) {
		root_ = root_->m_rb_left_;
	}
	return root_;
}

hxrbtree_node* hxrbtree_rb_last_(hxrbtree_node* root_) {
	if(root_ == hxnull) {
		return hxnull;
	}
	while(root_->m_rb_right_ != hxnull) {
		root_ = root_->m_rb_right_;
	}
	return root_;
}

hxrbtree_node* hxrbtree_rb_next_(hxrbtree_node* node_) {
	if(node_->m_rb_right_ != hxnull) {
		return hxrbtree_rb_first_(node_->m_rb_right_);
	}
	hxrbtree_node* parent_ = node_->rb_parent_();
	while(parent_ != hxnull && node_ == parent_->m_rb_right_) {
		node_   = parent_;
		parent_ = parent_->rb_parent_();
	}
	return parent_;
}

hxrbtree_node* hxrbtree_rb_prev_(hxrbtree_node* node_) {
	if(node_->m_rb_left_ != hxnull) {
		return hxrbtree_rb_last_(node_->m_rb_left_);
	}
	hxrbtree_node* parent_ = node_->rb_parent_();
	while(parent_ != hxnull && node_ == parent_->m_rb_left_) {
		node_   = parent_;
		parent_ = parent_->rb_parent_();
	}
	return parent_;
}

// rb_rotate_left_ / rb_rotate_right_

void hxrbtree_rb_rotate_left_(hxrbtree_node* node_, hxrbtree_node*& root_) {
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

void hxrbtree_rb_rotate_right_(hxrbtree_node* node_, hxrbtree_node*& root_) {
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

void hxrbtree_rb_insert_color_(hxrbtree_node* node_, hxrbtree_node*& root_) {
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
					hxrbtree_rb_rotate_left_(parent_, root_);
					hxrbtree_node* tmp_ = parent_;
					parent_ = node_;
					node_   = tmp_;
				}
				parent_->rb_set_color_(1);
				gparent_->rb_set_color_(0);
				hxrbtree_rb_rotate_right_(gparent_, root_);
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
					hxrbtree_rb_rotate_right_(parent_, root_);
					hxrbtree_node* tmp_ = parent_;
					parent_ = node_;
					node_   = tmp_;
				}
				parent_->rb_set_color_(1);
				gparent_->rb_set_color_(0);
				hxrbtree_rb_rotate_left_(gparent_, root_);
			}
		}
	}
	root_->rb_set_color_(1);
}

// rb_erase_

void hxrbtree_rb_erase_(hxrbtree_node* node_, hxrbtree_node*& root_) {
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
		hxrbtree_node* successor_ = hxrbtree_rb_first_(node_->m_rb_right_);
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
			successor_->m_rb_right_ = node_->m_rb_right_;
			node_->m_rb_right_->rb_set_parent_(successor_);
		}
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
				hxrbtree_rb_rotate_left_(parent_, root_);
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
					hxrbtree_rb_rotate_right_(sibling_, root_);
					sibling_ = parent_->m_rb_right_;
				}
				sibling_->rb_set_color_(parent_->rb_color_());
				parent_->rb_set_color_(1);
				sibling_->m_rb_right_->rb_set_color_(1);
				hxrbtree_rb_rotate_left_(parent_, root_);
				child_ = root_;
				break;
			}
		}
		else {
			hxrbtree_node* sibling_ = parent_->m_rb_left_;
			if(sibling_->rb_is_red_()) {
				sibling_->rb_set_color_(1);
				parent_->rb_set_color_(0);
				hxrbtree_rb_rotate_right_(parent_, root_);
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
					hxrbtree_rb_rotate_left_(sibling_, root_);
					sibling_ = parent_->m_rb_left_;
				}
				sibling_->rb_set_color_(parent_->rb_color_());
				parent_->rb_set_color_(1);
				sibling_->m_rb_left_->rb_set_color_(1);
				hxrbtree_rb_rotate_right_(parent_, root_);
				child_ = root_;
				break;
			}
		}
	}
	if(child_ != hxnull) {
		child_->rb_set_color_(1);
	}
}

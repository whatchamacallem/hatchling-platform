// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hxrbtree.hpp"

// hxrbtree_insert_color_

hxattr_hot void hxrbtree_insert_color_(hxrbtree_node* node_, hxrbtree_node*& root_) {
	hxrbtree_node* parent_ = hxrbtree_red_parent_(node_);
	while(true) {
		if(parent_ == hxnull) {
			// Node is root: paint it black.
			node_->rb_set_parent_color_(hxnull, 1);
			break;
		}
		if(parent_->rb_is_black_()) {
			break;
		}
		hxrbtree_node* grand_parent_ = hxrbtree_red_parent_(parent_);
		hxrbtree_node* temp_ = grand_parent_->m_right_;
		if(parent_ != temp_) {
			// parent_ == grand_parent_->m_left_
			if(temp_ != hxnull && temp_->rb_is_red_()) {
				// Case 1: uncle is red — recolor and recurse at grandparent.
				temp_->rb_set_parent_color_(grand_parent_, 1);
				parent_->rb_set_parent_color_(grand_parent_, 1);
				node_   = grand_parent_;
				parent_ = node_->rb_parent_();
				node_->rb_set_parent_color_(parent_, 0);
				continue;
			}
			temp_ = parent_->m_right_;
			if(node_ == temp_) {
				// Case 2: node is inner grandchild — left-rotate at parent,
				// then fall through to Case 3 with roles swapped.
				temp_ = node_->m_left_;
				parent_->m_right_ = temp_;
				node_->m_left_    = parent_;
				if(temp_ != hxnull) {
					temp_->rb_set_parent_color_(parent_, 1);
				}
				parent_->rb_set_parent_color_(node_, 0);
				parent_ = node_;
				temp_    = node_->m_right_;
			}
			// Case 3: node is outer grandchild — right-rotate at grandparent.
			grand_parent_->m_left_ = temp_;
			parent_->m_right_ = grand_parent_;
			if(temp_ != hxnull) {
				temp_->rb_set_parent_color_(grand_parent_, 1);
			}
			hxrbtree_rotate_set_parents_(grand_parent_, parent_, root_, 0);
			break;
		}
		else {
			// parent_ == grand_parent_->m_right_
			temp_ = grand_parent_->m_left_;
			if(temp_ != hxnull && temp_->rb_is_red_()) {
				// Case 1: uncle is red — recolor and recurse at grandparent.
				temp_->rb_set_parent_color_(grand_parent_, 1);
				parent_->rb_set_parent_color_(grand_parent_, 1);
				node_   = grand_parent_;
				parent_ = node_->rb_parent_();
				node_->rb_set_parent_color_(parent_, 0);
				continue;
			}
			temp_ = parent_->m_left_;
			if(node_ == temp_) {
				// Case 2: node is inner grandchild — right-rotate at parent,
				// then fall through to Case 3 with roles swapped.
				temp_ = node_->m_right_;
				parent_->m_left_ = temp_;
				node_->m_right_  = parent_;
				if(temp_ != hxnull) {
					temp_->rb_set_parent_color_(parent_, 1);
				}
				parent_->rb_set_parent_color_(node_, 0);
				parent_ = node_;
				temp_    = node_->m_left_;
			}
			// Case 3: node is outer grandchild — left-rotate at grandparent.
			grand_parent_->m_right_ = temp_;
			parent_->m_left_   = grand_parent_;
			if(temp_ != hxnull) {
				temp_->rb_set_parent_color_(grand_parent_, 1);
			}
			hxrbtree_rotate_set_parents_(grand_parent_, parent_, root_, 0);
			break;
		}
	}
}

// hxrbtree_erase_

hxattr_hot void hxrbtree_erase_(hxrbtree_node* node_, hxrbtree_node*& root_) {
	hxrbtree_node* child_  = hxnull;
	hxrbtree_node* parent_ = hxnull;
	int color_ = 0;

	if(node_->m_left_ == hxnull) {
		child_ = node_->m_right_;
	}
	else if(node_->m_right_ == hxnull) {
		child_ = node_->m_left_;
	}
	else {
		hxrbtree_node* successor_ = hxrbtree_first_(node_->m_right_);
		child_  = successor_->m_right_;
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
			parent_->m_left_      = child_;
			successor_->m_right_  = node_->m_right_;
			node_->m_right_->rb_set_parent_(successor_);
		}
		successor_->m_left_ = node_->m_left_;
		successor_->rb_set_parent_color_(node_->rb_parent_(), node_->rb_color_());
		if(node_->rb_parent_() != hxnull) {
			if(node_->rb_parent_()->m_left_ == node_) {
				node_->rb_parent_()->m_left_ = successor_;
			}
			else {
				node_->rb_parent_()->m_right_ = successor_;
			}
		}
		else {
			root_ = successor_;
		}
		node_->m_left_->rb_set_parent_(successor_);
		goto rebalance_;
	}

	parent_ = node_->rb_parent_();
	color_  = node_->rb_color_();
	if(child_ != hxnull) {
		child_->rb_set_parent_(parent_);
	}
	if(parent_ != hxnull) {
		if(parent_->m_left_ == node_) {
			parent_->m_left_ = child_;
		}
		else {
			parent_->m_right_ = child_;
		}
	}
	else {
		root_ = child_;
	}

rebalance_:
	if(color_ == 0 || parent_ == hxnull) {
		return;
	}
	while(true) {
		hxrbtree_node* sibling_ = parent_->m_right_;
		if(child_ != sibling_) {
			// child_ == parent_->m_left_
			hxrbtree_node* tmp1_;
			hxrbtree_node* tmp2_;
			if(sibling_->rb_is_red_()) {
				// Case 1: left-rotate at parent to make sibling black.
				tmp1_ = sibling_->m_left_;
				parent_->m_right_ = tmp1_;
				sibling_->m_left_ = parent_;
				tmp1_->rb_set_parent_color_(parent_, 1);
				hxrbtree_rotate_set_parents_(parent_, sibling_, root_, 0);
				sibling_ = tmp1_;
			}
			tmp1_ = sibling_->m_right_;
			if(tmp1_ == hxnull || tmp1_->rb_is_black_()) {
				tmp2_ = sibling_->m_left_;
				if(tmp2_ == hxnull || tmp2_->rb_is_black_()) {
					// Case 2: both sibling children black — flip sibling red.
					sibling_->rb_set_parent_color_(parent_, 0);
					if(parent_->rb_is_red_()) {
						// Parent was red: just blacken it and we are done.
						// Use addition to set the black bit without a mask,
						// valid because the bit is known to be 0.
						parent_->m_parent_color_ += static_cast<uintptr_t>(1u);
					}
					else {
						child_  = parent_;
						parent_ = child_->rb_parent_();
						if(parent_ != hxnull) {
							continue;
						}
					}
					break;
				}
				// Case 3: sibling's right child black, left red — right-rotate
				// at sibling to promote the left child.  tmp1_ is saved for
				// Case 4 so we avoid re-fetching after the rotation.
				tmp1_                = tmp2_->m_right_;
				sibling_->m_left_ = tmp1_;
				tmp2_->m_right_   = sibling_;
				parent_->m_right_ = tmp2_;
				if(tmp1_ != hxnull) {
					tmp1_->rb_set_parent_color_(sibling_, 1);
				}
				tmp1_    = sibling_;
				sibling_ = tmp2_;
			}
			// Case 4: left-rotate at parent.  sibling_ takes parent's color;
			// parent_ and the outer red nephew both become black.
			tmp2_ = sibling_->m_left_;
			parent_->m_right_ = tmp2_;
			sibling_->m_left_ = parent_;
			tmp1_->rb_set_parent_color_(sibling_, 1);
			if(tmp2_ != hxnull) {
				tmp2_->rb_set_parent_(parent_);
			}
			hxrbtree_rotate_set_parents_(parent_, sibling_, root_, 1);
			break;
		}
		else {
			sibling_ = parent_->m_left_;
			hxrbtree_node* tmp1_;
			hxrbtree_node* tmp2_;
			if(sibling_->rb_is_red_()) {
				// Case 1: right-rotate at parent to make sibling black.
				tmp1_ = sibling_->m_right_;
				parent_->m_left_   = tmp1_;
				sibling_->m_right_ = parent_;
				tmp1_->rb_set_parent_color_(parent_, 1);
				hxrbtree_rotate_set_parents_(parent_, sibling_, root_, 0);
				sibling_ = tmp1_;
			}
			tmp1_ = sibling_->m_left_;
			if(tmp1_ == hxnull || tmp1_->rb_is_black_()) {
				tmp2_ = sibling_->m_right_;
				if(tmp2_ == hxnull || tmp2_->rb_is_black_()) {
					// Case 2: both sibling children black — flip sibling red.
					sibling_->rb_set_parent_color_(parent_, 0);
					if(parent_->rb_is_red_()) {
						parent_->m_parent_color_ += static_cast<uintptr_t>(1u);
					}
					else {
						child_  = parent_;
						parent_ = child_->rb_parent_();
						if(parent_ != hxnull) {
							continue;
						}
					}
					break;
				}
				// Case 3: sibling's left child black, right red — left-rotate
				// at sibling.  tmp1_ is saved for Case 4.
				tmp1_                 = tmp2_->m_left_;
				sibling_->m_right_ = tmp1_;
				tmp2_->m_left_     = sibling_;
				parent_->m_left_   = tmp2_;
				if(tmp1_ != hxnull) {
					tmp1_->rb_set_parent_color_(sibling_, 1);
				}
				tmp1_    = sibling_;
				sibling_ = tmp2_;
			}
			// Case 4: right-rotate at parent.
			tmp2_ = sibling_->m_right_;
			parent_->m_left_   = tmp2_;
			sibling_->m_right_ = parent_;
			tmp1_->rb_set_parent_color_(sibling_, 1);
			if(tmp2_ != hxnull) {
				tmp2_->rb_set_parent_(parent_);
			}
			hxrbtree_rotate_set_parents_(parent_, sibling_, root_, 1);
			break;
		}
	}
}

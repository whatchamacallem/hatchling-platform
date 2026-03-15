#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
//

#include "../hxsettings.h"

class hxrbtree_node;

// hxrbtree_rb_red_parent_ / hxrbtree_rb_rotate_set_parents_

// Reads the parent pointer from a node known to be red (color bit == 0),
// skipping the mask.
hxattr_hot inline hxrbtree_node* hxrbtree_rb_red_parent_(const hxrbtree_node* node_) {
	return reinterpret_cast<hxrbtree_node*>(node_->m_rb_parent_color_);
}

// Transfers old_'s parent+color word to new_, sets old_'s parent to new_ with
// color_, and redirects old_'s parent's child pointer to new_.  Fuses the
// three writes that classic separate rotation code performs individually.
hxattr_hot inline void hxrbtree_rb_rotate_set_parents_(
	hxrbtree_node* old_, hxrbtree_node* new_, hxrbtree_node*& root_, int color_) {
	hxrbtree_node* parent_ = old_->rb_parent_();
	new_->m_rb_parent_color_ = old_->m_rb_parent_color_;
	old_->rb_set_parent_color_(new_, color_);
	if(parent_ != hxnull) {
		if(parent_->m_rb_left_ == old_) {
			parent_->m_rb_left_ = new_;
		}
		else {
			parent_->m_rb_right_ = new_;
		}
	}
	else {
		root_ = new_;
	}
}

// hxrbtree_rb_first_ / hxrbtree_rb_last_ / hxrbtree_rb_next_ / hxrbtree_rb_prev_

hxattr_hot inline hxrbtree_node* hxrbtree_rb_first_(hxrbtree_node* root_) {
	if(root_ == hxnull) {
		return hxnull;
	}
	while(root_->m_rb_left_ != hxnull) {
		root_ = root_->m_rb_left_;
	}
	return root_;
}

hxattr_hot inline hxrbtree_node* hxrbtree_rb_last_(hxrbtree_node* root_) {
	if(root_ == hxnull) {
		return hxnull;
	}
	while(root_->m_rb_right_ != hxnull) {
		root_ = root_->m_rb_right_;
	}
	return root_;
}

hxattr_hot inline hxrbtree_node* hxrbtree_rb_next_(hxrbtree_node* node_) {
	if(node_->m_rb_right_ != hxnull) {
		node_ = node_->m_rb_right_;
		while(node_->m_rb_left_ != hxnull) {
			node_ = node_->m_rb_left_;
		}
		return node_;
	}
	hxrbtree_node* parent_ = node_->rb_parent_();
	while(parent_ != hxnull && node_ == parent_->m_rb_right_) {
		node_   = parent_;
		parent_ = parent_->rb_parent_();
	}
	return parent_;
}

hxattr_hot inline hxrbtree_node* hxrbtree_rb_prev_(hxrbtree_node* node_) {
	if(node_->m_rb_left_ != hxnull) {
		node_ = node_->m_rb_left_;
		while(node_->m_rb_right_ != hxnull) {
			node_ = node_->m_rb_right_;
		}
		return node_;
	}
	hxrbtree_node* parent_ = node_->rb_parent_();
	while(parent_ != hxnull && node_ == parent_->m_rb_left_) {
		node_   = parent_;
		parent_ = parent_->rb_parent_();
	}
	return parent_;
}

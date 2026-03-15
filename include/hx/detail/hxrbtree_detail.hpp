#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

class hxrbtree_node;

void hxrbtree_rb_rotate_left_(hxrbtree_node* node_, hxrbtree_node*& root_);
void hxrbtree_rb_rotate_right_(hxrbtree_node* node_, hxrbtree_node*& root_);
void hxrbtree_rb_insert_color_(hxrbtree_node* node_, hxrbtree_node*& root_);
void hxrbtree_rb_erase_(hxrbtree_node* node_, hxrbtree_node*& root_);
hxrbtree_node* hxrbtree_rb_first_(hxrbtree_node* root_);
hxrbtree_node* hxrbtree_rb_last_(hxrbtree_node* root_);
hxrbtree_node* hxrbtree_rb_next_(hxrbtree_node* node_);
hxrbtree_node* hxrbtree_rb_prev_(hxrbtree_node* node_);

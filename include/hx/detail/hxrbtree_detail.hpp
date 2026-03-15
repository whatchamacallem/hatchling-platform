#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
//

#include "../hxsettings.h"

class hxrbtree_node;

void hxrbtree_rotate_left_(hxrbtree_node* node_, hxrbtree_node*& root_);
void hxrbtree_rotate_right_(hxrbtree_node* node_, hxrbtree_node*& root_);
void hxrbtree_insert_color_(hxrbtree_node* node_, hxrbtree_node*& root_);
void hxrbtree_erase_(hxrbtree_node* node_, hxrbtree_node*& root_);
hxrbtree_node* hxrbtree_first_(hxrbtree_node* root_);
hxrbtree_node* hxrbtree_last_(hxrbtree_node* root_);
hxrbtree_node* hxrbtree_next_(hxrbtree_node* node_);
hxrbtree_node* hxrbtree_prev_(hxrbtree_node* node_);

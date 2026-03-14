#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hxkey.hpp"
#include "hxutility.h"

#if HX_CPLUSPLUS >= 202002L
template<typename node_t_>
concept hxlist_concept_ =
	requires(node_t_& node_, const node_t_& const_node_) {
		sizeof(node_t_);
		{ node_.list_prev() = static_cast<void*>(hxnull) } -> hxconvertible_to<void*&>;
		{ const_node_.list_prev() } -> hxconvertible_to<void*>;
		{ node_.list_next() = static_cast<void*>(hxnull) } -> hxconvertible_to<void*&>;
		{ const_node_.list_next() } -> hxconvertible_to<void*>;
	};
#else
#define hxlist_concept_ typename
#endif

class hxlist_node {
public:
	hxlist_node(void) : m_list_prev_(hxnull), m_list_next_(hxnull) { }

	void* list_prev(void) const { return m_list_prev_; }
	void*& list_prev(void) { return m_list_prev_; }

	void* list_next(void) const { return m_list_next_; }
	void*& list_next(void) { return m_list_next_; }

private:
	hxlist_node(const hxlist_node&) = delete;
	void operator=(const hxlist_node&) = delete;

	void* m_list_prev_;
	void* m_list_next_;
};

template<hxlist_concept_ node_t_,
	typename deleter_t_=hxdefault_delete>
class hxlist {
public:
	using node_t = node_t_;

	class const_iterator {
	public:
		const_iterator(hxlist_node* current_, hxlist_node* sentinel_)
			: m_current_node_(current_), m_sentinel_(sentinel_) { }

		const_iterator(void) : m_current_node_(hxnull), m_sentinel_(hxnull) { }

		const_iterator& operator++(void) {
			hxassertmsg(m_current_node_ != m_sentinel_, "invalid_iterator");
			m_current_node_ = static_cast<hxlist_node*>(m_current_node_->list_next());
			return *this;
		}

		const_iterator operator++(int) { const_iterator t_(*this); operator++(); return t_; }

		const_iterator& operator--(void) {
			m_current_node_ = static_cast<hxlist_node*>(m_current_node_->list_prev());
			return *this;
		}

		const_iterator operator--(int) { const_iterator t_(*this); operator--(); return t_; }

		bool operator==(const const_iterator& x_) const { return m_current_node_ == x_.m_current_node_; }
		bool operator!=(const const_iterator& x_) const { return m_current_node_ != x_.m_current_node_; }

		const node_t_& operator*(void) const { return *static_cast<node_t_*>(m_current_node_); }
		const node_t_* operator->(void) const { return static_cast<node_t_*>(m_current_node_); }

	protected:
		/// \cond HIDDEN
		hxlist_node* m_current_node_;
		hxlist_node* m_sentinel_;
		/// \endcond
	};

	class iterator : public const_iterator {
	public:
		iterator(hxlist_node* current_, hxlist_node* sentinel_)
			: const_iterator(current_, sentinel_) { }

		iterator(void) { }

		iterator& operator++(void) { const_iterator::operator++(); return *this; }
		iterator operator++(int) { iterator t_(*this); const_iterator::operator++(); return t_; }
		iterator& operator--(void) { const_iterator::operator--(); return *this; }
		iterator operator--(int) { iterator t_(*this); const_iterator::operator--(); return t_; }

		node_t_& operator*(void) const { return *static_cast<node_t_*>(this->m_current_node_); }
		node_t_* operator->(void) const { return static_cast<node_t_*>(this->m_current_node_); }
	};

	explicit hxlist(void) {
		m_size_ = 0u;
		m_head_.list_prev() = &m_head_;
		m_head_.list_next() = &m_head_;
	}

	~hxlist(void) { this->clear(); }

	iterator begin(void) { return iterator(static_cast<hxlist_node*>(m_head_.list_next()), &m_head_); }
	const_iterator begin(void) const { return const_iterator(static_cast<hxlist_node*>(m_head_.list_next()), const_cast<hxlist_node*>(&m_head_)); }
	iterator end(void) { return iterator(&m_head_, &m_head_); }
	const_iterator end(void) const { return const_iterator(const_cast<hxlist_node*>(&m_head_), const_cast<hxlist_node*>(&m_head_)); }
	const_iterator cbegin(void) const { return begin(); }
	const_iterator cend(void) const { return end(); }

	size_t size(void) const { return m_size_; }
	bool empty(void) const { return m_head_.list_next() == &m_head_; }

	node_t_* front(void) {
		hxassertmsg(!empty(), "empty_list");
		return static_cast<node_t_*>(m_head_.list_next());
	}

	const node_t_* front(void) const {
		hxassertmsg(!empty(), "empty_list");
		return static_cast<const node_t_*>(m_head_.list_next());
	}

	node_t_* back(void) {
		hxassertmsg(!empty(), "empty_list");
		return static_cast<node_t_*>(m_head_.list_prev());
	}

	const node_t_* back(void) const {
		hxassertmsg(!empty(), "empty_list");
		return static_cast<const node_t_*>(m_head_.list_prev());
	}

	void push_front(node_t_* ptr_) {
		hxassertmsg(ptr_ != hxnull, "null_node");
		hxlist_node* old_first_ = static_cast<hxlist_node*>(m_head_.list_next());
		ptr_->list_next() = old_first_;
		ptr_->list_prev() = &m_head_;
		old_first_->list_prev() = ptr_;
		m_head_.list_next() = ptr_;
		++m_size_;
	}

	void push_back(node_t_* ptr_) {
		hxassertmsg(ptr_ != hxnull, "null_node");
		hxlist_node* old_last_ = static_cast<hxlist_node*>(m_head_.list_prev());
		ptr_->list_prev() = old_last_;
		ptr_->list_next() = &m_head_;
		old_last_->list_next() = ptr_;
		m_head_.list_prev() = ptr_;
		++m_size_;
	}

	node_t_* pop_front(void) {
		if(empty()) {
			return hxnull;
		}
		node_t_* ptr_ = static_cast<node_t_*>(m_head_.list_next());
		extract_(ptr_);
		return ptr_;
	}

	node_t_* pop_back(void) {
		if(empty()) {
			return hxnull;
		}
		node_t_* ptr_ = static_cast<node_t_*>(m_head_.list_prev());
		extract_(ptr_);
		return ptr_;
	}

	void insert_before(node_t_* pos_, node_t_* ptr_) {
		hxassertmsg(pos_ != hxnull && ptr_ != hxnull, "null_node");
		hxlist_node* prev_ = static_cast<hxlist_node*>(pos_->list_prev());
		ptr_->list_prev() = prev_;
		ptr_->list_next() = pos_;
		prev_->list_next() = ptr_;
		pos_->list_prev() = ptr_;
		++m_size_;
	}

	void insert_after(node_t_* pos_, node_t_* ptr_) {
		hxassertmsg(pos_ != hxnull && ptr_ != hxnull, "null_node");
		hxlist_node* next_ = static_cast<hxlist_node*>(pos_->list_next());
		ptr_->list_next() = next_;
		ptr_->list_prev() = pos_;
		next_->list_prev() = ptr_;
		pos_->list_next() = ptr_;
		++m_size_;
	}

	node_t_* extract(node_t_* ptr_) {
		hxassertmsg(ptr_ != hxnull, "null_node");
		extract_(ptr_);
		return ptr_;
	}

	template<typename deleter_override_t_>
	void erase(node_t_* ptr_, const deleter_override_t_& deleter_) {
		extract_(ptr_);
		if(deleter_) {
			deleter_(ptr_);
		}
	}

	void erase(node_t_* ptr_) { this->erase(ptr_, deleter_t_()); }

	template<typename deleter_override_t_>
	void clear(const deleter_override_t_& deleter_) {
		if(m_size_ != 0u) {
			if(deleter_) {
				hxlist_node* n_ = static_cast<hxlist_node*>(m_head_.list_next());
				while(n_ != &m_head_) {
					hxlist_node* next_ = static_cast<hxlist_node*>(n_->list_next());
					deleter_(static_cast<node_t_*>(n_));
					n_ = next_;
				}
				release_all();
			}
			else {
				release_all();
			}
		}
	}

	void clear(void) { this->clear(deleter_t_()); }

	void release_all(void) {
		m_head_.list_prev() = &m_head_;
		m_head_.list_next() = &m_head_;
		m_size_ = 0u;
	}

private:
	hxlist(const hxlist&) = delete;

	void extract_(hxlist_node* ptr_) {
		static_cast<hxlist_node*>(ptr_->list_prev())->list_next() = ptr_->list_next();
		static_cast<hxlist_node*>(ptr_->list_next())->list_prev() = ptr_->list_prev();
		--m_size_;
	}

	size_t m_size_;
	hxlist_node m_head_;
};

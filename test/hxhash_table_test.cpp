// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxhash_table_nodes.hpp>
#include <hx/hxhash_table.hpp>
#include <hx/hxtest.hpp>

static class hxhash_table_test_f* s_hxtest_current = 0;

class hxhash_table_test_f :
	public testing::Test
{
public:
	class hxtest_object {
	public:
		hxtest_object(void) {
			++s_hxtest_current->m_constructed;
			id = s_hxtest_current->m_next_id++;
		}
		~hxtest_object(void) {
			++s_hxtest_current->m_destructed;
			id = -1;
		}

		void operator=(const hxtest_object& x) { id = x.id; }
		void operator=(int32_t x) { id = x; }
		bool operator==(const hxtest_object& x) const { return id == x.id; }
		bool operator==(int32_t x) const { return id == x; }

		// XXX???
		operator float(void) const { return static_cast<float>(id); }

		int32_t id;
	};

	class hxtest_integer : public hxhash_table_node_integer<int32_t> {
	public:
		hxtest_integer(int32_t k) : hxhash_table_node_integer(k) { }
		hxtest_object value;
	};

	class hxtest_string : public hxhash_table_node_string<hxsystem_allocator_temporary_stack> {
	public:
		hxtest_string(const char* k) : hxhash_table_node_string(k) { }
		hxtest_object value;
	};

	class hxtest_string_literal : public hxhash_table_node_string_literal {
	public:
		hxtest_string_literal(const char* k) : hxhash_table_node_string_literal(k) { }
	};

	hxhash_table_test_f(void) {
		hxassert(s_hxtest_current == hxnull);
		m_constructed = 0;
		m_destructed = 0;
		m_next_id = 0;
		s_hxtest_current = this;
	}
	~hxhash_table_test_f(void) override {
		s_hxtest_current = 0;
	}

	int32_t m_constructed;
	int32_t m_destructed;
	int32_t m_next_id;
};

TEST_F(hxhash_table_test_f, null) {
	{
		using table_t = hxhash_table<hxtest_integer, 4>;
		table_t table;
		EXPECT_EQ(table.size(), 0u);
		const table_t& const_table = table;

		// "Returns an iterator pointing to the beginning of the hash table." Empty table => begin == end and load factor 0.
		EXPECT_EQ(table.begin(), table.end());
		EXPECT_EQ(table.cbegin(), table.cend());
		EXPECT_EQ(const_table.begin(), const_table.cend());
		EXPECT_EQ(table.begin(), table.end());
		EXPECT_EQ(table.cbegin(), table.cend());
		EXPECT_EQ(const_table.begin(), const_table.cend());

		// "Removes all nodes and calls deleter_t::operator() on every node." Clearing untouched table keeps load factor { 0.0 }.
		table.clear();
		EXPECT_EQ(table.load_factor(), 0.0f);

	}
	EXPECT_EQ(m_constructed, 0);
	EXPECT_EQ(m_destructed, 0);
}

TEST_F(hxhash_table_test_f, single) {
const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	static const int k = 77;
	{
		using table_t = hxhash_table<hxtest_integer, 4>;
		table_t table;
		const table_t& const_table = table;
		hxtest_integer* node = hxnew<hxtest_integer>(k);
		// "Inserts a node_t into the hash table, allowing duplicate keys." Seed table with manual node.
		table.insert_node(node);

		// Iterator + count checks confirm single-entry semantics.
		EXPECT_NE(table.begin(), table.end());
		EXPECT_NE(table.cbegin(), table.cend());
		EXPECT_EQ(++table.begin(), table.end());
		EXPECT_EQ(++table.cbegin(), table.cend());
		EXPECT_EQ(table.size(), 1u);
		EXPECT_EQ(table.count(k), 1u);

		// insert_unique with a duplicate key returns the existing node. Caller discards the rejected ptr.
		hxtest_integer* dup = hxnew<hxtest_integer>(k);
		EXPECT_EQ(table.insert_unique(dup), node);
		hxdelete(dup);

		// find() hit and miss, both mutable and const.
		EXPECT_EQ(table.find(k), node);
		EXPECT_EQ(table.find(k, node), hxnullptr);
		EXPECT_EQ(const_table.find(k), node);
		EXPECT_EQ(const_table.find(k, node), hxnullptr);

		// extract() hit and miss.
		EXPECT_EQ(table.extract(k), node);
		EXPECT_EQ(table.extract(k), hxnullptr);

		table.insert_node(node);
		// release_all() removes all nodes without deleting them.
		table.release_all();
		EXPECT_EQ(table.find(k), hxnullptr);
		EXPECT_EQ(table.size(), 0u);
		EXPECT_EQ(table.count(k), 0u);

		// insert_unique on an empty table inserts and returns the node.
		hxtest_integer* new_node = hxnew<hxtest_integer>(k);
		EXPECT_EQ(table.insert_unique(new_node), new_node);
		EXPECT_NE(new_node->value.id, node->value.id);
		EXPECT_EQ(table.size(), 1u);

		// Destructor frees new_node. Node was already extracted above.
		hxdelete(node);
	}
	EXPECT_EQ(m_constructed, 3);
	EXPECT_EQ(m_destructed, 3);
}

TEST_F(hxhash_table_test_f, map_node_usage) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	using map_node_t = hxhash_table_map_node<int32_t, hxtest_object>;
	using table_t = hxhash_table<map_node_t, 4>;
	{
		table_t table;

		// insert_unique inserts a new node and returns it when key is absent.
		map_node_t* n10 = hxnew<map_node_t>(10);
		map_node_t* via_insert = table.insert_unique(n10);
		EXPECT_EQ(via_insert, n10);
		EXPECT_EQ(via_insert->key(), 10);
		via_insert->value().id = 123;

		map_node_t* manual = hxnew<map_node_t>(20);
		manual->value().id = 321;
		// Link external allocation through insert_node to co-exist with first entry.
		table.insert_node(manual);

		EXPECT_EQ(table.size(), 2u);
		EXPECT_EQ(table.count(10), 1u);
		EXPECT_EQ(table.count(20), 1u);

		const table_t& const_table = table;
		const map_node_t* const_lookup = const_table.find(10);
		EXPECT_NE(const_lookup, hxnullptr);
		if(const_lookup != hxnullptr) {
			EXPECT_EQ(const_lookup->value().id, 123);
		}

		map_node_t* manual_lookup = table.find(20);
		EXPECT_NE(manual_lookup, hxnullptr);
		if(manual_lookup != hxnullptr) {
			EXPECT_EQ(manual_lookup->value().id, 321);
		}

		EXPECT_EQ(table.size(), 2u);
	}

	EXPECT_EQ(m_constructed, 2);
	EXPECT_EQ(m_destructed, 2);
}

TEST_F(hxhash_table_test_f, multiple) {
	static const int size_i = 78; // Used to do range checking modulo size.
	static const unsigned int size_u = 78u;
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	{
		// Table will be overloaded.
		using table_t = hxhash_table<hxtest_integer>;
		table_t table;
		const table_t& const_table = table;
		// "Use set_table_size_bits to configure hash bits dynamically." Force 2^5 buckets before load test.
		table.set_table_size_bits(5);

		// Insert keys { 0..size-1 } with value.id mirroring the key.
		for(int i = 0; i < size_i; ++i) {
			hxtest_integer* n = hxnew<hxtest_integer>(i);
			hxtest_integer* inserted = table.insert_unique(n);
			EXPECT_EQ(inserted, n);
			EXPECT_EQ(inserted->value.id, i);
			EXPECT_EQ(inserted->key(), i);
		}

		// Check properties of size unique keys.
		int id_histogram[size_u] = {};
		EXPECT_EQ(table.size(), size_u);
		table_t::iterator it = table.begin();
		table_t::iterator cit = table.begin();
		for(int i = 0; i < size_i; ++i) {
			hxtest_integer* ti = table.find(i);
			EXPECT_EQ(ti->value, i);
			EXPECT_EQ(table.find(i, ti), hxnullptr);

			// Iteration over.
			EXPECT_NE(it, table.end());
			EXPECT_NE(cit, table.cend());
			EXPECT_EQ(it, cit);
			EXPECT_LT(static_cast<unsigned int>(it->value.id), size_u);
			id_histogram[it->value.id]++;
			EXPECT_LT(static_cast<unsigned int>(cit->value.id), size_u);
			id_histogram[cit->value.id]++;
			++cit;
			it++;
		}
		EXPECT_EQ(table.end(), it);
		EXPECT_EQ(table.cend(), cit);
		for(int i = 0; i < size_i; ++i) {
			EXPECT_EQ(id_histogram[i], 2);
		}

		// insert second size elements
		for(int i = 0; i < size_i; ++i) {
			hxtest_integer* ti = hxnew<hxtest_integer>(i);
			EXPECT_EQ(ti->value.id, i+size_i);
			table.insert_node(ti);
		}

		// Check properties of 2*size duplicate keys.
		int key_histogram[size_u] = {};
		EXPECT_EQ(table.size(), size_u * 2u);
		it = table.begin();
		cit = table.begin();
		for(int i = 0; i < size_i; ++i) {
			hxtest_integer* ti = table.find(i);
			EXPECT_EQ(ti->key(), i);
			const hxtest_integer* ti2 = const_table.find(i, ti); // test const version
			EXPECT_EQ(ti2->key(), i);
			EXPECT_EQ(table.find(i, ti2), hxnullptr);

			EXPECT_EQ(table.count(i), 2u);

			EXPECT_LT(static_cast<unsigned int>(it->key()), size_u);
			key_histogram[it->key()]++;
			++it;
			EXPECT_LT(static_cast<unsigned int>(it->key()), size_u);
			key_histogram[it->key()]++;
			it++;
			EXPECT_LT(static_cast<unsigned int>(cit->key()), size_u);
			key_histogram[cit->key()]++;
			++cit;
			EXPECT_LT(static_cast<unsigned int>(cit->key()), size_u);
			key_histogram[cit->key()]++;
			cit++;
		}
		EXPECT_EQ(table.end(), it);
		EXPECT_EQ(table.cend(), cit);
		for(int i = 0; i < size_i; ++i) {
			EXPECT_EQ(key_histogram[i], 4);
		}

		// load_max() should be within 4x the mean for a heavily loaded table.
		EXPECT_GT((table.load_factor() * 4.0f), (float)table.load_max());

		// Erase keys [0..size/2), remove 1 of 2 of keys [size/2..size)
		for(int i = 0; i < (size_i/2); ++i) {
			EXPECT_EQ(table.erase(i), 2);
		}
		for(int i = (size_i/2); i < size_i; ++i) {
			hxtest_integer* ti = table.extract(i);
			EXPECT_EQ(ti->key(), i);
			hxdelete(ti);
		}

		// Check properties of size_i/2 remaining keys.
		for(int i = 0; i < (size_i/2); ++i) {
			EXPECT_EQ(table.release_key(i), 0);
			EXPECT_EQ(table.find(i), hxnullptr);
		}
		for(int i = (size_i/2); i < size_i; ++i) {
			hxtest_integer* ti = table.find(i);
			EXPECT_EQ(ti->key(), i);
			EXPECT_EQ(table.find(i, ti), hxnullptr);
			EXPECT_EQ(table.count(i), 1u);
		}

		it = table.begin();
		cit = table.begin();
		for(int i = 0; i < (size_i/2); ++i) {
			++it;
			cit++;
		}
		EXPECT_EQ(table.end(), it);
		EXPECT_EQ(table.cend(), cit);
	}
	EXPECT_EQ(m_constructed, 2*size_i);
	EXPECT_EQ(m_destructed, 2*size_i);
}

TEST_F(hxhash_table_test_f, strings) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	static const char* colors[] = {
		"Red","Orange","Yellow",
		"Green","Cyan","Blue",
		"Indigo","Violet" };
	const size_t sz = hxsize(colors);

	{
		using table_t = hxhash_table<hxtest_string, 4>;
		table_t table;

		// Insert colors in reverse. insert_unique inserts each new key and returns it.
		for(size_t i = sz; i-- != 0;) {
			hxtest_string* n = hxnew<hxtest_string>(colors[i]);
			hxtest_string* inserted = table.insert_unique(n);
			EXPECT_EQ(inserted, n);
			EXPECT_STREQ(inserted->key(), colors[i]);
		}
		EXPECT_NE(table.find("Cyan"), hxnullptr);
		EXPECT_EQ(table.find("Pink"), hxnullptr);

	}
	EXPECT_EQ(m_constructed, sz);
	EXPECT_EQ(m_destructed, sz);
}

TEST_F(hxhash_table_test_f, string_literal_nodes) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	static const char* const literals[] = {
		"Crimson", "Teal", "Magenta", "Gold"
	};

	using table_t = hxhash_table<hxtest_string_literal, 4>;
	table_t table;

	for(unsigned int i = 0; i < hxsize(literals); ++i) {
		// String literal keys are owned externally. The node stores only the pointer.
		hxtest_string_literal* n = hxnew<hxtest_string_literal>(literals[i]);
		hxtest_string_literal* inserted = table.insert_unique(n);
		EXPECT_EQ(inserted, n);
		EXPECT_EQ(inserted->key(), literals[i]);
		EXPECT_EQ(inserted->hash(), hxkey_hash(literals[i]));
	}

	EXPECT_EQ(table.size(), (size_t)hxsize(literals));
	EXPECT_NE(table.find("Crimson"), hxnullptr);
	EXPECT_EQ(table.find("Cerulean"), hxnullptr);
}

#include "lru_cache.hpp"

namespace gen {


template<typename K, typename V>
lru_cache<K, V>::lru_cache(size_t max_size) :
	m_values(),
	m_key_iter_table(),
	m_max_size(max_size),
	m_mutex()
{
}


template<typename K, typename V>
lru_cache<K, V>::lru_cache(const lru_cache& other) :
	m_mutex()
{
	std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lk(this->m_mutex, other.m_mutex);
	lru_cache(other.capacity());

	// put other's key-value pairs in the correct order (from the last to the first)
	for (auto it = other.m_values.rbegin(); it != other.m_values.rend(); it++)
	{
		put(it->first, it->second);
	}
}

template<typename K, typename V>
lru_cache<K, V>& lru_cache<K, V>::operator=(const lru_cache& other)
{
	std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lk(m_mutex, other.m_mutex);
	
	clear();
	for (auto it = other.m_values.rbegin(); it != other.m_values.rend(); it++)
	{
		put(it->first, it->second);
	}
	return *this;
}

template<typename K, typename V>
lru_cache<K, V>::lru_cache(lru_cache&& other) :
	m_mutex()
{
	std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lk(m_mutex, other.m_mutex);

	m_values.swap(other.m_values);
	m_key_iter_table.swap(other.m_key_iter_table);
}

template<typename K, typename V>
lru_cache<K, V>& lru_cache<K, V>::operator=(lru_cache&& other)
{
	std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lk(m_mutex, other.m_mutex);

	m_values.swap(other.m_values);
	m_key_iter_table.swap(other.m_key_iter_table);
	return *this;
}


template<typename K, typename V>
inline void lru_cache<K, V>::put(const K& key, const V& value)
{
	std::lock_guard<std::recursive_mutex> lck(m_mutex);
	if (capacity() == 0)
	{
		return;
	}

	auto key_elem = m_key_iter_table.find(key);
	if (key_elem == m_key_iter_table.end())
	{ // element not found
		// if cache is full -- move last element to the front
		if (size() == capacity())
		{
			if (size() > 1) {
				// move last element to the beginning
				m_values.splice(m_values.begin(), m_values, m_values.rbegin().base());
			}

			// erase old data from the table
			auto top = m_values.begin();
			m_key_iter_table.erase(top->first);
			top->first = key;
			top->second = value;
		}
		else
		{
			m_values.emplace_front(key, value);
		}
		m_key_iter_table.insert_or_assign( key, m_values.begin());
	}
	else
	{ // element with the provided key is already cached, so the value is updated
		key_elem->second->second = value;
		m_values.splice(m_values.begin(), m_values, key_elem->second);
	}
}

template<typename K, typename V>
std::optional<V> lru_cache<K, V>::get(const K& key)
{
	std::lock_guard<std::recursive_mutex> lck(m_mutex);
	auto found_elem = m_key_iter_table.find(key);
	// if it is a cache-miss then return optional without value
	if (found_elem == m_key_iter_table.end())
	{
		return std::optional<V>();
	}

	// move the found element to the front of the list
	m_values.splice(m_values.begin(), m_values, found_elem->second);
	return std::make_optional(found_elem->second->second);
}


template<typename K, typename V>
void lru_cache<K, V>::reserve(size_t new_max_size)
{
	std::lock_guard<std::recursive_mutex> lck(m_mutex);
	if (new_max_size < size())
	{
		size_t diff = size() - new_max_size;
		for (size_t i = 0; i < diff; i++)
		{
			m_key_iter_table.erase(m_values.rbegin()->first);
			m_values.pop_back();
		}
	}
	m_max_size = new_max_size;
}


template<typename K, typename V>
void lru_cache<K, V>::clear()
{
	std::lock_guard<std::recursive_mutex> lck(m_mutex);
	m_values.clear();
	m_key_iter_table.clear();
}

}

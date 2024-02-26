#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <unordered_map>
#include <optional>
#include <mutex>


namespace gen {
template<typename K, typename V>
class lru_cache {
	using key_value_list = std::list<std::pair<K, V>>;
	using key_iter_list = std::unordered_map<K, typename key_value_list::iterator>;

	/// <summary>
	/// A list of values. It is always pre-allocated to have the maximum size of nodes
	/// to avoid unneccssary allocations
	/// </summary>
	key_value_list m_values;

	/// <summary>
	/// Maping from keys to index in the content
	/// </summary>
	key_iter_list m_key_iter_table;

	size_t m_max_size;

	mutable std::recursive_mutex m_mutex;

public:
	using key_type = K;
	using value_type = V;

	/// <summary>
	/// Constructor with max-size provided
	/// </summary>
	lru_cache(size_t max_size);

	lru_cache(const lru_cache& other);

	lru_cache& operator=(const lru_cache& other);

	lru_cache(lru_cache&& other);

	lru_cache& operator=(lru_cache&& other);

	~lru_cache()
	{
		// clear at destruction
		clear();
	}


	void put(const K& key, const V& value);

	/// <summary>
	/// Get optional value by providing key
	/// Value is returned by copying, to avoid invalid references in case of multi-threaded app
	/// </summary>
	/// <param name="key">Key to look for</param>
	/// <returns>nullopt in case if no key-value pairs with the provided key is stored,
	/// optional with a value otherwise</returns>
	std::optional<V> get(const K& key);

	/// <summary>
	/// set capacity for the cache
	/// </summary>
	void reserve(size_t new_max_size);

	/// <summary>
	/// Removes all cached items
	/// </summary>
	void clear();

	inline size_t size() const
	{
		return m_key_iter_table.size();
	}

	inline size_t capacity() const
	{
		return m_max_size;
	}

	inline bool empty() const
	{
		return m_key_iter_table.empty();
	}

	inline std::optional<std::pair<K, V>> front() const
	{
		return empty() ? std::optional<std::pair<K, V>>() : std::make_optional(m_values.front());
	}

	inline bool cached(const K& key) const
	{
		return m_key_iter_table.find(key) != m_key_iter_table.end();
	}
};

}

#include "lru_cache.ipp"

#endif



#pragma once

namespace CacheCpp{
	template<typename Key, typename Value>
	class Node
	{
	public:
		Node(const Key& key, const Value& value)
			:m_key(key), m_value(value), m_accessCount(1),
			m_prev(nullptr), m_next(nullptr)
		{
		}
		~Node() = default;

		void SetValue(const Value& value) { m_value = value; }
		const Key GetKey() const { return m_key; }
		const Value GetValue() const { return m_value; }
		const size_t GetAccessCount() const { return m_accessCount; }
		void IncrementAccessCount() { m_accessCount++; }

		void SetPrev(std::shared_ptr<Node<Key, Value>>prev) { m_prev = prev; }
		void SetNext(std::shared_ptr<Node<Key, Value>>next) { m_next = next; }
		const std::shared_ptr<Node<Key, Value>> GetPrev() const { return m_prev; }
		const std::shared_ptr<Node<Key, Value>> GetNext() const { return m_next; }

	private:
		Key m_key;
		Value m_value;
		size_t m_accessCount;
		std::shared_ptr<Node<Key, Value>> m_prev;
		std::shared_ptr<Node<Key, Value>> m_next;
	};

	template<typename Key, typename Value>
	struct CacheTraits
	{

	};
}
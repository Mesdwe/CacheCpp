#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include "Node.h"
#include "CachePolicy.h"

namespace CacheCpp {

	template<typename Key, typename Value>
	class LRUCache : public ICachePolicy<Key, Value>
	{
	public:
		LRUCache(int capacity) : m_capacity(capacity), m_list(std::make_unique<LinkedList<Key,Value>>())
		{
		}

		virtual ~LRUCache() override = default;

		void Put(const Key& key, const Value& value) override
		{
			if (m_capacity < 0)
				return;

			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_caches.find(key);
			if (it != m_caches.end())
			{
				it->second->SetValue(value);
				_MoveToMostRecent(it->second);
				return;
			}

			_AddNewNode(key, value);
		}

		bool Get(const Key& key, Value& value) override
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_caches.find(key);
			if (it != m_caches.end())
			{
				value = it->second->GetValue();
				_MoveToMostRecent(it->second);
				return true;
			}
			return false;
		}

		virtual void Remove(const Key& key) override
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_caches.find(key);
			if (it != m_caches.end())
			{
				m_list->RemoveNode(it->second);
				m_caches.erase(it);
			}
		}

		virtual size_t Size() const override { return m_caches.size(); }

		virtual size_t Capacity() const override { return m_capacity; }

	private:
		void _AddNewNode(const Key& key, const Value& value)
		{
			if (m_caches.size() >= m_capacity)
				_EvictNode();
			NodePtr new_node = std::make_shared<NodeType>(key, value);
			m_list->InsertNode(new_node);
			m_caches[key] = new_node;
		}

		void _MoveToMostRecent(const NodePtr& node)
		{
			m_list->RemoveNode(node);
			m_list->InsertNode(node);
		}


		void _EvictNode()
		{
			NodePtr least_recent = m_list->GetLastNode();
			if (!least_recent) return;

			m_list->RemoveNode(least_recent);
			m_caches.erase(least_recent->GetKey());
		}

	private:
		int m_capacity;
		NodeMap m_caches;    // value: Node<Key,Value>
		std::mutex m_mutex;
		std::unique_ptr<CacheCpp::LinkedList<Key, Value>> m_list;   // m_list->GetLastNode() is the node to evict
	};

	// optimisation: LRU-K
	template<typename Key, typename Value>
	class LRUKCache : public LRUCache<Key, Value>
	{
	public:
		LRUKCache(int capacity, int historyCapacity, int k)
			: LRUCache<Key, Value>(capacity),
			m_accessHistory(std::make_unique<LRUCache<Key, size_t>>(historyCapacity)), m_k(k)
		{
		}

		void Put(const Key& key, const Value& value) override
		{

			size_t history_count = _UpdateAccessCount(key);

			if (history_count >= m_k)
			{
				m_accessHistory->Remove(key);
				LRUCache<Key, Value>::Put(key, value);
			}

		}

		bool Get(const Key& key, Value& value) override
		{
			_UpdateAccessCount(key);

			return LRUCache<Key, Value>::Get(key, value);
		}

	private:
		size_t _UpdateAccessCount(const Key& key)
		{
			size_t history_count = 0;
			m_accessHistory->Get(key, history_count);
			m_accessHistory->Put(key, ++history_count);

			return history_count;
		}
	private:
		int m_k;
		std::unique_ptr<LRUCache<Key, size_t>> m_accessHistory;
	};


	// Optimisation: 
	template<typename Key, typename Value>
	class LRUHashCache : public ICachePolicy<Key, Value>
	{
	public:
		LRUHashCache(int capacity, int sliceNum)
			: m_capacity(capacity),
			m_sliceNum(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency())
		{
			size_t slice_size = std::ceil(capacity / static_cast<double>(m_sliceNum));
			for (int i = 0; i < m_sliceNum; ++i)
			{
				m_sliceCaches.emplace_back(std::make_unique<LRUCache<Key, Value>>(slice_size));
			}
		}

		void Put(const Key& key, const Value& value) override
		{
			size_t slice_index = _Hash(key) % m_sliceNum;
			m_sliceCaches[slice_index]->Put(key, value);
		}

		bool Get(const Key& key, Value& value) override
		{
			size_t slice_index = _Hash(key) % m_sliceNum;
			return m_sliceCaches[slice_index]->Get(key, value);
		}

		virtual void Remove(const Key& key) override
		{
			size_t slice_index = _Hash(key) % m_sliceNum;
			return m_sliceCaches[slice_index]->Remove(key);
		}

		virtual size_t Size() const override
		{ 
			size_t size = 0;
			for (int i = 0; i < m_sliceNum; ++i)
			{
				size += m_sliceCaches[i]->Size();
			}
			return size;
		}

		virtual size_t Capacity() const override { return m_capacity; }

	private:
		size_t _Hash(const Key& key)
		{
			std::hash<Key> hash_func;
			return hash_func(key);
		}

	private:
		int m_capacity;
		int m_sliceNum;
		std::vector<std::unique_ptr<LRUCache<Key, Value>>> m_sliceCaches;
	};

}
#pragma once


#include <memory>
#include <mutex>
#include <unordered_map>

#include "Node.h"
#include "CachePolicy.h"


namespace CacheCpp {


	template<typename Key, typename Value>
	class FrequencyList
	{
	public:
		explicit FrequencyList(int freq)
			: m_freq(freq), m_list(std::make_unique<LinkedList<Key, Value>>())
		{
		}

		const std::unique_ptr<CacheCpp::LinkedList<Key, Value>>& GetList() const { return m_list; }

	private:
		int m_freq;   // access frequency
		std::unique_ptr<CacheCpp::LinkedList<Key, Value>> m_list;
	};


	template<typename Key, typename Value>
	class LFUCache : public ICachePolicy<Key, Value>
	{
	public:
		LFUCache(int capacity, int maxAverageNum = 10)
			: m_capacity(capacity), m_minFreq(INT_MAX), m_maxAverageNum(maxAverageNum),
			m_avgFreq(0), m_totalFreq(0)
		{
		}

		virtual ~LFUCache() override = default;

		void Put(const Key& key, const Value& value) override
		{
			if (m_capacity < 0)
				return;

			std::lock_guard<std::mutex> lock(m_mutex);

			auto it = m_caches.find(key);
			if (it != m_caches.end())
			{
				it->second->SetValue(value);
				_UpdateExistingNode(it->second);
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
				_UpdateExistingNode(it->second);
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
				_RemoveFromFreqList(it->second);
				m_caches.erase(it->second->GetKey());
				_UpdateFreqStats(false, it->second->GetAccessCount());
			}
		}

		void Clear()
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_caches.clear();
			m_freqLists.clear();
		}

		virtual size_t Size() const override { return m_caches.size(); }

		virtual size_t Capacity() const override { return m_capacity; }

	private:
		void _AddNewNode(const Key& key, const Value& value)
		{
			if (m_caches.size() >= m_capacity)
				_EvictNode();

			NodePtr new_node = std::make_shared<NodeType>(key, value);
			m_caches[key] = new_node;
			_AddToFreqList(new_node);
			_UpdateFreqStats(true, 1);
			m_minFreq = std::min(m_minFreq, 1);
		}

		void _UpdateExistingNode(const NodePtr& node)
		{
			int old_freq = node->GetAccessCount();
			// remove from freq list
			_RemoveFromFreqList(node);
			// update freq
			node->IncrementAccessCount();
			// add back to freq list
			_AddToFreqList(node);
			// update min freq if neccessary
			if (old_freq == m_minFreq && m_freqLists.find(old_freq) == m_freqLists.end())
				++m_minFreq;

			_UpdateFreqStats(true, 1);
		}


		void _EvictNode()
		{
			if (m_freqLists.find(m_minFreq) == m_freqLists.end()) return;
			auto& list = m_freqLists[m_minFreq];
			if (!list || list->IsEmpty()) return;

			NodePtr node = list->GetLastNode();
			_RemoveFromFreqList(node);
			m_caches.erase(node->GetKey());
			_UpdateFreqStats(false, node->GetAccessCount());
		}

		void _RemoveFromFreqList(const NodePtr& node)
		{
			int freq = node->GetAccessCount();
			auto& list = m_freqLists[freq];
			list->RemoveNode(node);
			if (list->IsEmpty()) 
			{
				m_freqLists.erase(freq);
			}
		}

		void _AddToFreqList(const NodePtr& node)
		{
			size_t freq = node->GetAccessCount();
			if (m_freqLists.find(freq) == m_freqLists.end())
			{
				m_freqLists[freq] = std::make_unique<LinkedList<Key, Value>>();
			}
			m_freqLists[freq]->InsertNode(node);
		}

		void _UpdateFreqStats(bool increase, int num)
		{
			if (increase)
				m_totalFreq += num;
			else
				m_totalFreq -= num;


			if (m_caches.empty())
				m_avgFreq = 0;
			else
				m_avgFreq = m_totalFreq / m_caches.size();

			if (m_avgFreq > m_maxAverageNum)
			{
				// halve all access counts
				m_minFreq = INT_MAX;
				for (auto it = m_caches.begin(); it != m_caches.end(); ++it)
				{
					NodePtr node = it->second;
					if (node == nullptr)
						continue;

					_RemoveFromFreqList(node);

					int target = node->GetAccessCount() / 2;
					if (target < 1) target = 1;

					m_minFreq = std::min(m_minFreq, target);

					node->SetAccessCount(target);

					_AddToFreqList(node);
				}

				if (m_minFreq == INT_MAX)
					m_minFreq = 1;
			}
		}

	private:
		int m_capacity;
		int m_minFreq;
		int m_maxAverageNum;
		int m_avgFreq;
		int m_totalFreq;

		std::mutex m_mutex;
		NodeMap m_caches;
		std::unordered_map<int, std::unique_ptr<LinkedList<Key, Value>>> m_freqLists;
	};
}
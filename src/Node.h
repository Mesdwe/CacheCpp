#pragma once

namespace CacheCpp{
	template<typename Key, typename Value>
	class Node
	{
	public:
		Node(const Key key, const Value value)
			:m_key(std::move(key)), m_value(std::move(value)), m_accessCount(1),
			m_next(nullptr)
		{
		}
		~Node() = default;

		void SetValue(const Value& value) { m_value = value; }
		const Key& GetKey() const { return m_key; }
		const Value& GetValue() const { return m_value; }
		const size_t GetAccessCount() const { return m_accessCount; }
		void IncrementAccessCount() { m_accessCount++; }
		void SetAccessCount(size_t value) { m_accessCount = value; }

		void SetPrev(std::shared_ptr<Node<Key, Value>>prev) { m_prev = prev; }
		void SetNext(std::shared_ptr<Node<Key, Value>>next) { m_next = next; }
		const std::shared_ptr<Node<Key, Value>> GetPrev() const { return m_prev.lock(); }
		const std::shared_ptr<Node<Key, Value>> GetNext() const { return m_next; }

	private:
		Key m_key;
		Value m_value;
		size_t m_accessCount;
		std::weak_ptr<Node<Key, Value>> m_prev;
		std::shared_ptr<Node<Key, Value>> m_next;
	};

	template<typename Key, typename Value>
	class LinkedList
	{
	public:
		using NodeType = Node<Key, Value>;
		using NodePtr = std::shared_ptr<NodeType>;

		explicit LinkedList()
		{
			_InitialiseList();
		}

		// If head's next is tail, the list is empty.
		// This logic assumes dummy head and tail nodes are always present.
		bool IsEmpty() const
		{
			return m_head->GetNext() == m_tail;
		}

		void InsertNode(const NodePtr& node)
		{
			// Insert at the beginning of the list
			node->SetPrev(m_head);
			node->SetNext(m_head->GetNext());
			node->GetNext()->SetPrev(node);
			m_head->SetNext(node);
		}

		void RemoveNode(const NodePtr& node)
		{
			if (!node || node == m_head || node == m_tail)
				return;

			auto prev = node->GetPrev();
			auto next = node->GetNext();
			if (prev && next) 
			{
				prev->SetNext(next);
				next->SetPrev(prev);
			}

			node->SetPrev(nullptr);
			node->SetNext(nullptr);
		}

		NodePtr GetLastNode() const// get node to evict
		{
			if (IsEmpty()) return nullptr;
			return m_tail->GetPrev();
		}
	private:
		void _InitialiseList()
		{
			m_head = std::make_shared<NodeType>(Key(), Value());
			m_tail = std::make_shared<NodeType>(Key(), Value());
			m_head->SetNext(m_tail);
			m_tail->SetPrev(m_head);
		}

	private:
		NodePtr m_head;
		NodePtr m_tail;
	};
}
#pragma once  
#include <memory>  
#include <mutex>  
#include <unordered_map>  

#include "Node.h"  
#include "CachePolicy.h"  
#include "LRU.h" 

namespace CacheCpp {  
    template<typename Key, typename Value>
    class ArcLFUCache
    {
    public:
        using NodeType = Node<Key, Value>;
        using NodePtr = std::shared_ptr<NodeType>;
        ArcLFUCache(int capacity, int ghostCapacity)
            : m_capacity(capacity), m_ghostCapacity(ghostCapacity),
            m_lfuMain(std::make_unique<LFUCache<Key,Value>>(capacity)),
            m_lfuGhost(std::make_unique<LFUCache<Key,Value>>(ghostCapacity))
        {
        }

        bool Put(const Key& key, const Value& value)
        {
            if (m_capacity == 0) return false;
            if (m_lfuMain->Size() >= m_capacity)
            {
                m_lfuGhost->Put(key, value);
            }
            m_lfuMain->Put(key, value);
            return true;
        }

        bool Get(const Key& key, Value& value)
        {
            return m_lfuMain->Get(key, value);
        }

        bool CheckGhost(Key key)
        {
            if (m_lfuGhost->Contains(key))
            {
                m_lfuGhost->Remove(key);
                return true;
            }
            return false;
        }

        void IncreaseCapacity() 
        { 
            m_capacity++; 
            m_lfuMain->IncreaseCapacity();
        }

        bool DecreaseCapacity()
        {
            if (m_capacity <= 0) return false;
            --m_capacity;
            if (m_lfuMain->Size() >= m_capacity) {
                NodePtr node = m_lfuMain->GetNodeToEvict();
                if (node != nullptr)
                {
                    m_lfuGhost->Put(node->GetKey(), node->GetValue());
                }
                m_lfuMain->DecreaseCapacity();
            }
            return true;
        }

        size_t Size() { return m_lfuMain->Size(); }
    private:
        int m_capacity;
        int m_ghostCapacity;
        std::unique_ptr<LFUCache<Key, Value>> m_lfuMain;
        std::unique_ptr<LFUCache<Key, Value>> m_lfuGhost;
    };

    template<typename Key, typename Value>
    class ArcLRUCache
    {
    public:
        using NodeType = Node<Key, Value>;
        using NodePtr = std::shared_ptr<NodeType>;
        ArcLRUCache(int capacity, int ghostCapacity, int transformThreshold)
            : m_capacity(capacity), m_ghostCapacity(ghostCapacity), m_transformThershold(transformThreshold),
            m_lruMain(std::make_unique<LRUCache<Key, Value>>(capacity)),
            m_lruGhost(std::make_unique<LRUCache<Key, Value>>(ghostCapacity))
        {
        }

        bool Put(const Key& key, const Value& value)
        {
            if (m_capacity == 0) return false;
            if (m_lruMain->Size() >= m_capacity)
            {
                m_lruGhost->Put(key, value);
            }
            m_lruMain->Put(key, value);
            return true;
        }

        bool Get(const Key& key, Value& value, bool& shouldTransform)
        {
            if (m_lruMain->Get(key, value))
            {
                NodePtr node = m_lruMain->Find(key);
                if (node != nullptr)
                    shouldTransform = node->GetAccessCount() >= m_transformThershold;
                return true;
            }
            return false;
        }

        bool CheckGhost(Key key)
        {
            if (m_lruGhost->Contains(key))
            {
                m_lruGhost->Remove(key);
                return true;
            }
            return false;
        }

        void IncreaseCapacity()
        {
            m_capacity++;
            m_lruMain->IncreaseCapacity();
        }

        bool DecreaseCapacity()
        {
            if (m_capacity <= 0) return false;
            --m_capacity;
            if (m_lruMain->Size() >= m_capacity) {
                NodePtr node = m_lruMain->GetNodeToEvict();
                if (node != nullptr)
                {
                    m_lruMain->Put(node->GetKey(), node->GetValue());
                }
                m_lruMain->DecreaseCapacity();
            }
            return true;
        }

        size_t Size() { return m_lruMain->Size(); }
    private:
        int m_capacity;
        int m_ghostCapacity;
        int m_transformThershold;
        std::unique_ptr<LRUCache<Key, Value>> m_lruMain;
        std::unique_ptr<LRUCache<Key, Value>> m_lruGhost;

    };

   template<typename Key, typename Value>  
   class ARCCache : public ICachePolicy<Key, Value>  
   {  
   public:  
       ARCCache(int capacity, int transformThreshold = 10)  
           : m_capacity(capacity), m_ghostCapacity(capacity), m_transformThreshold(transformThreshold),  
           m_lfu(std::make_unique<ArcLFUCache<Key, Value>>(capacity,capacity)),
           m_lru(std::make_unique<ArcLRUCache<Key, Value>>(capacity,capacity, transformThreshold))
       {  
       }  

       void Put(const Key& key, const Value& value) override  
       {  
           std::lock_guard<std::mutex> lock(m_mutex);
           bool in_ghost = _CheckInGhost(key);  
           if (in_ghost)
           {
               m_lru->Put(key, value);
           }
           else  
           {  
               if (m_lru->Put(key, value))
                   m_lfu->Put(key, value);
           }  
       }  

       virtual bool Get(const Key& key, Value& value) override  
       {  
           std::lock_guard<std::mutex> lock(m_mutex);
           _CheckInGhost(key);

           bool shouldTransform = false;
           if (m_lru->Get(key, value, shouldTransform))
           {
               if (shouldTransform)
               {
                   m_lfu->Put(key, value);
               }
               return true;
           }
           return m_lfu->Get(key, value);
       }  

       virtual void Remove(const Key& key) override
       {
           std::lock_guard<std::mutex> lock(m_mutex);
       }
       virtual size_t Size() const override { return m_lfu->Size()+m_lru->Size(); }  

       virtual size_t Capacity() const override { return m_capacity; }  

   private:  
       bool _CheckInGhost(const Key& key)  
       {  
           bool result = false;  

           if (m_lru->CheckGhost(key))  
           {  
               if (m_lfu->DecreaseCapacity())
               {
                   m_lru->IncreaseCapacity();
               }
               result = true;  
           }  
           else if (m_lfu->CheckGhost(key))  
           {  
               if (m_lru->DecreaseCapacity())
               {
                   m_lfu->IncreaseCapacity();
               }
               result = true;
           }  
           return result;  
       }  

       int m_capacity;  
       int m_ghostCapacity;  
       int m_transformThreshold;  
       std::mutex m_mutex;  

       std::unique_ptr<ArcLFUCache<Key, Value>> m_lfu;  
       std::unique_ptr<ArcLRUCache<Key, Value>> m_lru;  
   };  
}
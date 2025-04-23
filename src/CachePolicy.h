#pragma once

#include <unordered_map>
#include "Node.h"

namespace CacheCpp {

template<typename Key, typename Value>
class ICachePolicy
{
public:
    using NodeType = Node<Key, Value>;
    using NodePtr = std::shared_ptr<NodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    virtual ~ICachePolicy() {};

    virtual void Put(const Key& key, const Value& value) = 0;

    virtual bool Get(const Key& key, Value& value) = 0;

    virtual void Remove(const Key& key) = 0;

    virtual size_t Size() const = 0;

    virtual size_t Capacity() const = 0;
};

}
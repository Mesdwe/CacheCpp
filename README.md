A Thread-safe, STL-compatible C++ caching framework designed for flexible experimentation with multiple cache replacement strategies.

## Caching Strategies

### 1. `LRU`

- Classic LRU cache that evicts the **least recently used** item when full.
- An improved LRU that promotes entries to the main cache only after being accessed **K times**.
- A **sharded** LRUCache that splits entries across N slices using a hash function.

### 2. `LFU (WIP)`

### 3. `ARC (WIP)`

### Extensible for more policies

//
// YOU IMPLEMENT (this whole file; it is a template). Spec §4.2.
//
// A fixed-capacity cache that evicts the Least Recently Used entry.
// Both get() and put() must be O(1) (expected), so it is built from two
// structures that point at the same nodes:
//
//   std::unordered_map<Key, Node*>   finds a key's node in O(1)
//   a doubly linked list of Nodes    keeps recency order:
//        head_ = most recently used ... tail_ = least recently used
//
// Rules for this file:
//   * Write the doubly linked list yourself (Node with prev/next pointers,
//     new/delete inside this class only). Do NOT use std::list, std::deque
//     or std::forward_list (don't even #include their headers), or a
//     std::vector you shift around (spec §7.3).
//   * std::unordered_map is allowed (and expected) for the key -> Node* index.
//   * No leaks, no double frees: ASan runs on every test.

#pragma once
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class LruCache
{
public:
    struct Stats
    {
        std::size_t hits = 0;      // get() found the key
        std::size_t misses = 0;    // get() did not find the key
        std::size_t evictions = 0; // put() removed the LRU entry to make room
    };

    // A cache holding at most `capacity` entries. Capacity 0 is legal: such a
    // cache never stores anything (every get() is a miss).
    explicit LruCache(std::size_t capacity) : capacity_(capacity) {}

    // Frees every node.
    ~LruCache() { clear(); }

    // Copying a cache of raw-pointer nodes is not needed; forbid it.
    LruCache(const LruCache &) = delete;
    LruCache &operator=(const LruCache &) = delete;

    // Hit: marks the entry most recently used, counts a hit, and returns a
    //      pointer to its value (valid until the next put/erase/clear).
    // Miss: counts a miss and returns nullptr.
    const Value *get(const Key &key)
    {
        auto it = map_.find(key);
        if (it != map_.end())
        {
            stats_.hits++;
            move_to_front(it->second);
            return &(it->second->value);
        }
        else
        {
            stats_.misses++;
            return nullptr;
        }
    }

    // Key present: replaces its value and marks it most recently used (no
    //   eviction, no hit/miss counted).
    // Key absent: if the cache is full, evicts the least recently used entry
    //   (counting one eviction), then inserts the new entry as most recently
    //   used. With capacity 0, does nothing.
    void put(const Key &key, Value value)
    {
        // 1. Capacity is 0: return immediately. Nothing is ever stored.
        if (capacity_ == 0)
        {
            return;
        }

        auto it = map_.find(key);
        if (it != map_.end())
        {
            // 2. Key already present: replace the node's value, move to front, done.
            // No eviction, no stats, even if the cache is full.
            it->second->value = std::move(value);
            move_to_front(it->second);
        }
        else
        {
            // 3. Key absent and the cache is full (size() == capacity_): evict the tail.
            if (map_.size() == capacity_)
            {
                Node *victim = tail_;
                // Order matters: unlink, erase from map, then delete.
                unlink(victim);
                map_.erase(victim->key);
                delete victim;
                stats_.evictions++;
            }

            // 4. Create the node, push it front, and record it in the map.
            Node *new_node = new Node{key, std::move(value), nullptr, nullptr};
            push_front(new_node);
            map_[key] = new_node;
        }
    }

    // True if the key is cached. Does NOT change recency or stats.
    bool contains(const Key &key) const
    {
        return map_.find(key) != map_.end();
    }

    // Removes the key if present (not an eviction). Returns whether it was.
    bool erase(const Key &key)
    {
        auto it = map_.find(key);
        if (it != map_.end())
        {
            Node *target = it->second;
            unlink(target);
            map_.erase(it);
            delete target;
            return true;
        }
        return false;
    }

    // Removes every entry. Stats are kept.
    void clear()
    {
        // Walk from the head saving each node’s next in a local before deleting it
        Node *current = head_;
        while (current != nullptr)
        {
            Node *next_node = current->next;
            delete current;
            current = next_node;
        }
        // Empty the map and reset head_ and tail_ to null
        map_.clear();
        head_ = nullptr;
        tail_ = nullptr;
    }

    std::size_t size() const noexcept { return map_.size(); }
    std::size_t capacity() const noexcept { return capacity_; }
    Stats stats() const noexcept { return stats_; }

    // Keys from most to least recently used, by walking the list from head_.
    // Used by the tests to check your recency order.
    std::vector<Key> keys_mru_to_lru() const
    {
        std::vector<Key> out;
        for (Node *n = head_; n != nullptr; n = n->next)
            out.push_back(n->key);
        return out;
    }

private:
    struct Node
    {
        Key key;
        Value value;
        Node *prev;
        Node *next;
    };

    // ===== YOUR HELPERS AND MEMBERS =====

    // Unlinks a node from its current position in the doubly linked list
    void unlink(Node *n)
    {
        // Left side: if n has a prev, prev's next skips over n; otherwise n was head
        if (n->prev)
        {
            n->prev->next = n->next;
        }
        else
        {
            head_ = n->next;
        }

        // Right side: if n has a next, next's prev skips n; otherwise n was tail
        if (n->next)
        {
            n->next->prev = n->prev;
        }
        else
        {
            tail_ = n->prev;
        }
    }

    // Pushes a node to the front (head) of the doubly linked list
    void push_front(Node *n)
    {
        n->next = head_;
        n->prev = nullptr;

        if (head_)
        {
            head_->prev = n;
        }
        else
        {
            tail_ = n; // The easy-to-forget case: list was empty, n is also tail
        }

        head_ = n;
    }

    // Moves an existing node to the front of the list (makes it MRU)
    void move_to_front(Node *n)
    {
        if (n == head_)
        {
            return; // Already at the front, do nothing
        }
        unlink(n);
        push_front(n);
    }

    std::unordered_map<Key, Node *, Hash> map_;
    Node *head_ = nullptr;
    Node *tail_ = nullptr;
    std::size_t capacity_;
    Stats stats_;
};

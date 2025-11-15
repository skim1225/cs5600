# Practicum 1 — Memory Hierarchy Simulation Writeup
## Sooji Kim

Practicum 1 is an extension of the Part I message store which adds an in-memory cache, fixed-size messages, page replacement algorithms, and performance evaluation.
This writeup explains my design decisions, testing methodology, and evaluation results.

---

# **Part 1 — Cache Design**

## **1. Fixed-Size Message Structure**
The assignment required every message to occupy a fixed-size page regardless of content length.
To satisfy this requirement I created a wrapper for the message which pads messages to the fixed size.

```c
typedef struct {
    msg_content_t content;
    char padding[MSG_SIZE - sizeof(msg_content_t)];
} message_t;
```

- `msg_content_t` contains the meaningful fields (ID, timestamp, sender, receiver, payload, delivered flag).  
- `padding[]` fills the remaining bytes so each message is exactly `MSG_SIZE` bytes (1024 bytes in my configuration).  
- Using this wrapper ensures `sizeof(message_t) == MSG_SIZE` at compile time, satisfying fixed-page semantics.

---

## **2. Cache Structure**
The cache struct is implemented as:

```c
typedef struct {
    message_t msg;
    bool occupied;
    unsigned long long last_used;
} cache_entry_t;

typedef struct {
    cache_entry_t entries[CACHE_SIZE];
    unsigned long long use_counter;
} cache_t;
```

I decided on an array-based cache because I thought that it was a simplest way to index and access entries.
The memory layout is contiguous, like a real page in memory.
I decided against using another data structure such as hashmaps, binary search trees, or linked lists because I felt that the added complexities did not justify an increase in performance from O(N) to O(lgN), especially for the purposes of this practicum with a small cache size and small message set.

The last-used counter keeps track of when the cache entry was last used, where a greater number signifies a more recent time. This is used for the MRU replacement algorithm.

Since my cache is implemented as an array, the lookup strategy is a linear scan of O(N) performance as stated previously.
```c
message_t *cache_lookup(cache_t *cache, int id);
```

---

# **Part 3 — Testing Strategy**

My strategy was to test correctness of the functions as well as the behavior of the cache.

I did this via the following tests:

---

## **1. Correctness Tests**
- Validate messages are succesfully created and written to disk
- Validate messages are correctly retrieved 

---

## **2. Cache Behavior Tests**
- Fill cache and ensure all entries become `occupied`  
- Test retrieving messages that are known to be in cache (hits)
- Test retrieving messages not in cache so we have to retrieve it from disk (miss)
- Test that random replacement works correctly
- Test that MRU replacement works correctly

---

# **Part 4 — Evaluation**


## **Metrics**

### Random Replacement
- Hits ≈ 500  
- Misses ≈ 500  
- Hit Ratio ≈ 0.5  

### MRU Replacement
- Hits ≈ 500  
- Misses ≈ 500  
- Hit Ratio ≈ 0.5
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <functional>
#include "thread_safety.hpp"

/// \brief default_max_size Default maximum capacity of cache
static const size_t default_max_size = 100;

/// \brief default_log_level Default log options
static const bool default_log_level = false;

/// \brief The cache_key_hash_function struct Hash function for default cache Key type
struct cache_key_hash_function
{
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1,T2>& t) const
    {
        return std::hash<T1>{}(t.first) ^ std::hash<T2>{}(t.second);
    }
};

template<
    class Key,
    class Value,
    class HashFunction=std::hash<Key>,
    class KeyEqual=std::equal_to<Key>,
    class Allocator=std::allocator<std::pair<const Key, Value>>
>
/// \brief The Cache class This templated class consists a Cache that functions at an LRU manner.
/// The insertion and look up complexity is O(1).
/// An insertion implies the insertion to three data structures:
/// 1. hashmap from Keys to Values
/// 2. hashmap from Keys to insertion rounds
/// 3. hashmap from insertion round to Keys.
/// Key features:
/// 1. Keys and Values can be of arbitrary type.
/// 2. User can provide a maximum capacity.
/// 3. Multithreaded functionality is provided.
class Cache
{
public:
    /// \brief Cache        Constructor of the LRU cache
    /// \param max_size     The maximum capacity of the cache
    /// \param enable_logs  Enables/disables verbosity
    Cache(int max_size = default_max_size, bool enable_logs = default_log_level)
        : m_round_counter(0),
          m_max_size(max_size),
          m_oldest_insertion(1),
          m_enable_logs(enable_logs)
          // m_writers_counter(0),
          // m_readers_counter(0)
    {}

    /// \brief Disable copy constructor
    Cache(const Cache&) = delete;
    /// \brief Disable copy assignment operator
    Cache& operator=(const Cache&) = delete;

    /// \brief Move constructor
    Cache(Cache&& other)
        : Cache()
    {
        swap(*this, other);
    }

    friend void swap(Cache& first, Cache& second)
    {
        using std::swap;
        swap(first.m_cache, second.m_cache);
        swap(first.m_rounds, second.m_rounds);
        swap(first.m_reverse_rounds, second.m_reverse_rounds);
        swap(first.m_round_counter, second.m_round_counter);
        swap(first.m_oldest_insertion, second.m_oldest_insertion);
        swap(first.m_max_size, second.m_max_size);
        swap(first.m_enable_logs, second.m_enable_logs);
        swap(first.m_thread_safety, second.m_thread_safety);
    }

    /// \brief size Returns the amount of inserted key-value pairs
    /// \return     The amount of inserted key-value pairs
    size_t size()
    {
        return m_cache.size();
    }

    /// \brief find         Finds the value of corresponding key, if exists.
    /// \param key          The Key
    /// \param sleeptime    Optiion to cause delays (for testing multithreading functionalities)
    /// \return             Returns an std::pair<Value, bool>.
    ///                     If the  Kes exists, the corresponding value is returned and the bool is
    ///                     set to true
    ///                     If the  Key does
    ///                     not exist, a default-constructed Value object is returned and bool is set to
    ///                     false
    std::pair<Value, bool> find(const Key& key, int sleeptime = 0)
    {
        m_thread_safety.enter_as_reader();
        if (sleeptime) {
            std::this_thread::sleep_for(std::chrono::seconds(sleeptime));
        }
        auto item = m_cache.find(key);
        if (item == m_cache.end()) {
            m_thread_safety.exit_as_reader();
            return std::make_pair(Value{}, false);
        }
        auto previously_inserted_round = inserted_round(key);

        m_thread_safety.exit_as_reader();
        return std::make_pair(item->second, true);
    }

    /// \brief insert       Inserts a key-value pair in the cache. If max capacity is reached, the oldest
    ///                     key-value pair is evicted.
    /// \param key          The Key
    /// \param value        The Value
    /// \param sleeptime    Optiion to cause delays (for testing multithreading functionalities)
    /// \return             0 if  Key already existed, 1 if  Key is newly added
    size_t insert(Key key, Value value, int sleeptime = 0)
    {
        m_thread_safety.enter_as_writer();

        if (sleeptime > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(sleeptime));
        }

        auto previously_inserted_round = inserted_round(key);

        // in case of new insertion
        if (previously_inserted_round == 0) {
            // increment rounds
            m_round_counter++;

            // in case of max capacity
            if (m_cache.size() == m_max_size) {
                delete_least_recent();
            }

            insert_new_record(key, value);
            m_thread_safety.exit_as_writer();
            return 1;
        }

        // in case of already inserted item
        if (previously_inserted_round != m_round_counter) {
            update_inserted_round(key, previously_inserted_round);
        }
        m_thread_safety.exit_as_writer();
        return 0;
    }

    /// \brief print    Prints the contents of the cache (for debbugging purposes)
    void print(
            const std::function<void(Key k)>& print_key
            = [](Key k){std::cout << k;},
            const std::function<void(Value v)>& print_value
            = [](Value v){std::cout << v;}
            )
    {
        for (auto& c : m_cache) {
            std::cout << "[";
            print_key(c.first);
            std::cout << "] -> ";
            print_value(c.second);
            std::cout << " (at round " << m_rounds[c.first] << ")" << std::endl;
        }
        std::cout << "Contents of cache (" << m_cache.size() << "):" << std::endl;
    }

private:
    /// \brief inserted_round   Return the round a  Key was inserted
    /// \param key              The key
    /// \return                 The round if the  Key exists, 0 otherwise
    size_t inserted_round(const Key& key) const
    {
        auto round = m_rounds.find(key);
        if (round == m_rounds.end()) {
            return 0;
        }
        return round->second;
    }

    /// \brief update_inserted_round        Updates the round a pair is lastly retrieved/inserted
    /// \param key                          The key
    /// \param previously_inserted_round    The round that the pair was orinally inserted or updated
    void update_inserted_round(const Key& key, size_t previously_inserted_round)
    {
        m_round_counter++;
        m_rounds[key] = m_round_counter;
        m_reverse_rounds[m_round_counter] = key;
        m_reverse_rounds.erase(previously_inserted_round);
        if (previously_inserted_round == m_oldest_insertion) {
            increase_oldest_round();
        }

        if (m_enable_logs) {
            std::cout << "Key insertion round updated from " << previously_inserted_round << " to " << m_round_counter << std::endl;
            std::cout << "Oldest round is " << m_oldest_insertion << std::endl;
            std::cout << "Current round is " << m_round_counter << std::endl;
        }
    }

    /// \brief insert_new_record            Inserts a new key-value pair in the look up structures
    /// \param key                          The key
    /// \param value                        The value
    void insert_new_record(Key& key, Value& value)
    {
        // insert key-value pair
        m_cache[key] = value;
        m_rounds[key] = m_round_counter;
        m_reverse_rounds[m_round_counter] = key;

        if (m_enable_logs) {
            std::cout << "New key inserted at round " << m_round_counter << std::endl;
        }
    }

    /// \brief delete_least_recent          Evicts a key-value pair from the look up structures
    void delete_least_recent()
    {
        auto lru_key = m_reverse_rounds[m_oldest_insertion];
        m_cache.erase(lru_key);
        m_rounds.erase(lru_key);
        m_reverse_rounds.erase(m_oldest_insertion);
        increase_oldest_round();

        if (m_enable_logs) {
            std::cout << "Max capacity reached, deleted key from previous round " << std::endl;
            std::cout << "New oldest round is " << m_oldest_insertion << std::endl;
        }
    }

    /// \brief Finds the next oldest inserted round
    void increase_oldest_round()
    {
        m_oldest_insertion++;
        while(m_reverse_rounds.find(m_oldest_insertion) == m_reverse_rounds.end()) {
            m_oldest_insertion++;
        }
    }

    template<class T>
    /// \brief default_print                Default print method (for debugging purposes)
    /// \param t                            A simple type to by printed
    void default_print(T t)
    {
        std::cout << t;
    }

    /// \brief m_cache                      The Key->Value hashmap
    std::unordered_map<Key, Value, HashFunction, KeyEqual, Allocator>   m_cache;
    /// \brief m_rounds                     The Key->insertion/lookup round hashmap
    std::unordered_map<Key, size_t, HashFunction, KeyEqual>             m_rounds;
    /// \brief m_reverse_rounds             The insertion/lookup round ->Key hashmap
    std::unordered_map<size_t, Key>                                     m_reverse_rounds;
    /// \brief m_round_counter              The total amound of insertions/updates
    size_t m_round_counter;
    /// \brief m_oldest_insertion           The inseriton round of the oldest pair
    size_t m_oldest_insertion;
    /// \brief m_max_size                   The maximum capacity of the cache
    size_t m_max_size;
    /// \brief m_enable_logs                Enable/disable verbocity
    bool m_enable_logs;
    /// \brief thread_safety_t              Helper class to handle reads/writes of multiple threads
    thread_safety_t m_thread_safety;

};

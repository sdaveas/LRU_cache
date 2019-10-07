#include "cache.hpp"

// Default alias for cache's key and value type
using cache_key_t       = std::pair<std::string, std::string>;
using cache_value_t     = size_t;

// Tests
int main()
{
    // Create a accepting records with capacity 10;
    // key      std::pair<"std::string", int>, and
    // value    <int>
    Cache<cache_key_t, cache_value_t, cache_key_hash_function> cache(10);

    // Insert some different items in the cache
    cache.insert({"BTCUSD", "2019-01-01"}, 10000);
    cache.insert({"BTCUSD", "2019-01-02"}, 10001);
    cache.insert({"BTCUSD", "2019-01-03"}, 10002);
    cache.insert({"BTCUSD", "2019-01-04"}, 10003);
    cache.insert({"BTCUSD", "2019-01-05"}, 10004);
    cache.insert({"BTCUSD", "2019-01-06"}, 10005);
    cache.insert({"BTCUSD", "2019-01-07"}, 10006);
    cache.insert({"BTCUSD", "2019-01-08"}, 10007);
    cache.insert({"BTCUSD", "2019-01-09"}, 10008);
    cache.insert({"BTCUSD", "2019-01-09"}, 10008);

    {
        // Request an existing item
        auto res = cache.find({"BTCUSD", "2019-01-05"});
        if (res.second == true) {
            std::cout << "Item found. Value : " << res.first << std::endl;
        }
        else {
            std::cout << "Item not found in cache." << std::endl;
        }
    }

    {
        // Request an non-existing item
        auto res = cache.find({"BTCUSD", "2019-02-05"});
        if (res.second == true) {
            std::cout << "Item found. Value : " << res.first << std::endl;
        }
        else {
            std::cout << "Item not found in cache." << std::endl;
        }
    }

    std::cout << std::endl;

    // Print all existing items
    cache.print(
            []
            (cache_key_t v){
                std::cout << v.first << "," << v.second;
            }
            );
    return 0;
}


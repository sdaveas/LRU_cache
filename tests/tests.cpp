#define CATCH_CONFIG_MAIN // tell catch to provide a main function

#include "../tests/catch/catch.hpp"
#include "../src/cache.hpp"
#include <tuple>

TEST_CASE("Move construction test") {
    SECTION("Move cache") {
        size_t max_size = 10;
        Cache<int, int> cache(10);
        for (int i=0; i<100; i++) {
            cache.insert(i, i);
        }
        REQUIRE(cache.size() == max_size);

        Cache<int, int> cache_2(std::move(cache));
        REQUIRE(cache.size() == 0);
        REQUIRE(cache_2.size() == max_size);
    }
}

TEST_CASE("Functionality tests") {
    SECTION("Maximum capacity preserved") {
        size_t max_size = 10;
        Cache<int, int> cache(10);
        for (int i=0; i<100; i++) {
            cache.insert(i, i);
        }
        REQUIRE(cache.size() == max_size);
    }
    SECTION("Mutiple same insertions") {
        Cache<int, int> cache;
        cache.insert(1, 42);

        REQUIRE(cache.size() == 1);
        REQUIRE(cache.find(1).first == 42);
        // attempt same insertion. Nothing must change
        cache.insert(1, 42);
        REQUIRE(cache.size() == 1);
        REQUIRE(cache.find(1).first == 42);
    }
    SECTION("Correct evicting with inserts") {
        int max_size = 3;
        Cache<std::string, int> cache(max_size);    //  Current round   | Keys(round)
        cache.insert("A", 1);                       //          1       | A(1)
        cache.insert("A", 1);                       //          1       | A(1)
        cache.insert("A", 1);                       //          1       | A(1)
        cache.insert("B", 1);                       //          2       | A(1), B(2)
        cache.insert("A", 1);                       //          3       | B(2), A(3)
        cache.insert("B", 1);                       //          4       | A(3), B(4)
        cache.insert("C", 1);                       //          5       | A(3), B(4), C(5)
        cache.insert("D", 1);                       //          6       | B(4), C(5), D(6)

        // Cache must hold B, C, D
        REQUIRE(cache.size() == max_size);
        REQUIRE(cache.find("B").second == true);
        REQUIRE(cache.find("C").second == true);
        REQUIRE(cache.find("D").second == true);
    }
    SECTION("Correct evicting with updates") {
        int max_size = 3;
        Cache<std::string, int> cache(max_size);    //  Current round   | Keys(round)
        cache.insert("A", 1);                       //          1       | A(1)
        cache.insert("B", 1);                       //          2       | A(1), B(2)
        cache.insert("C", 1);                       //          3       | C(3) ,B(2), A(1)
        cache.insert("B", 1);                       //          4       | B(4), C(3), A(1)
        cache.insert("D", 1);                       //          5       | D(5), B(4), C(3)
        cache.insert("E", 1);                       //          6       | E(6), D(5), B(4)

        // Cache must hold B, C, D
        REQUIRE(cache.size() == max_size);
        REQUIRE(cache.find("E").second == true);
        REQUIRE(cache.find("D").second == true);
        REQUIRE(cache.find("B").second == true);
    }
    SECTION("Big inserts") {
        int max_size = 1000000;
        Cache<int, int> cache(max_size);
        for (int i=0; i<max_size*2; i++) {
            cache.insert(i, i);
        }
        REQUIRE(cache.size() == max_size);
    }
}

TEST_CASE("Templated polymorphism tests") {
    SECTION("<Key, Value> are <int, int>") {
        Cache<int, int> cache(3);
        cache.insert(1, 1);
        cache.insert(2, 2);
        cache.insert(3, 3);
        cache.insert(4, 4);
        REQUIRE(cache.find(1).second == false);
        REQUIRE(cache.find(2).first == 2);
        REQUIRE(cache.find(3).first == 3);
        REQUIRE(cache.find(4).first == 4);
    }
    SECTION("<Key, Value> are <std::string, int>") {
        Cache<std::string, int> cache(3);
        cache.insert("1", 1);
        cache.insert("2", 2);
        cache.insert("3", 3);
        cache.insert("4", 4);
        REQUIRE(cache.find("1").second == false);
        REQUIRE(cache.find("2").first == 2);
        REQUIRE(cache.find("3").first == 3);
        REQUIRE(cache.find("4").first == 4);
    }
    SECTION("<Key, Value> are <std::pair<std::string, int> , <int>>") {
        Cache<std::pair<std::string, int>, int, cache_key_hash_function> cache(3);
        cache.insert({"1", 1}, 1);
        cache.insert({"2", 2}, 2);
        cache.insert({"3", 3}, 3);
        cache.insert({"4", 4}, 4);
        REQUIRE(cache.find({"1", 1}).second == false);
        REQUIRE(cache.find({"2", 2}).first == 2);
        REQUIRE(cache.find({"3", 3}).first == 3);
        REQUIRE(cache.find({"4", 4}).first == 4);
    }
    SECTION("<Key, Value> are <std::pair<std::string, int> , std::tuple<int, int, int>>") {
        Cache<std::pair<std::string, int>, std::tuple<int, int, int>, cache_key_hash_function> cache(3);
        cache.insert({"1", 1}, {1, 1, 1});
        cache.insert({"2", 2}, {2, 2, 2});
        cache.insert({"3", 3}, {3, 3, 3});
        cache.insert({"4", 4}, {4, 4, 4});
        REQUIRE(cache.find({"1", 1}).second == false);
        REQUIRE(cache.find({"2", 2}).first == std::make_tuple(2, 2, 2));
        REQUIRE(cache.find({"3", 3}).first == std::make_tuple(3, 3, 3));
        REQUIRE(cache.find({"4", 4}).first == std::make_tuple(4, 4, 4));
    }
}

TEST_CASE("Multithreaded tests") {
    SECTION("Multiple writers write all data") {
        Cache<int, int> cache(10000);

        std::thread t1(
                [&cache]() {
                    for (int i=0; i<10000; i+=4)
                        cache.insert(i, i);
                });
        std::thread t2(
                [&cache]() {
                    for (int i=1; i<10000; i+=4)
                        cache.insert(i, i);
                });
        std::thread t3(
                [&cache]() {
                    for (int i=2; i<10000; i+=4)
                        cache.insert(i, i);
                });
        std::thread t4(
                [&cache]() {
                    for (int i=3; i<10000; i+=4)
                        cache.insert(i, i);
                });

        t1.join();
        t2.join();
        t3.join();
        t4.join();

        REQUIRE(cache.size() == 10000);
    }
    SECTION("Multiple writers prevent max capacity") {
        Cache<int, int> cache(500);

        std::thread t1(
                [&cache]() {
                    for (int i=0; i<10000; i+=4)
                        cache.insert(i, i);
                });
        std::thread t2(
                [&cache]() {
                    for (int i=1; i<10000; i+=4)
                        cache.insert(i, i);
                });
        std::thread t3(
                [&cache]() {
                    for (int i=2; i<10000; i+=4)
                        cache.insert(i, i);
                });
        std::thread t4(
                [&cache]() {
                    for (int i=3; i<10000; i+=4)
                        cache.insert(i, i);
                });

        t1.join();
        t2.join();
        t3.join();
        t4.join();

        REQUIRE(cache.size() == 500);
    }
    SECTION("Writers block readers") {
        Cache<int, int> cache(1);

        int sleeptime = 1;

        // Make writer sleep 1 second inside critical section;
        std::thread writer(
                [&cache, sleeptime]() {
                    cache.insert(1, 1, sleeptime);
                });

        auto readers_start = std::chrono::system_clock::now();

        // Start polling unti writer write the data
        std::vector<std::thread> readers;
        std::function<void()> read =
            [&cache, sleeptime]() {
                while(cache.find(1).second == false) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            };

        for (int i=0; i<100; i++) {
            readers.push_back(std::thread(read));
        }

        writer.join();
        for (int i=0; i<100; i++) {
            readers[i].join();
        }

        auto readers_end = std::chrono::system_clock::now();
        std::chrono::duration<double> readers_duration = readers_end - readers_start;

        REQUIRE(readers_duration.count() > 1.f);
    }
    SECTION("Writers block writers and readers") {
        Cache<int, int> cache(2);
        int sleeptime = 1;

        // Make writers sleep 1 second inside critical section;
        std::thread writer1(
                [&cache, sleeptime]() {
                cache.insert(1, 1, sleeptime);
                });
        std::thread writer2(
                [&cache, sleeptime]() {
                cache.insert(2, 2, sleeptime);
                });

        const auto readers_start = std::chrono::system_clock::now();

        // Start polling until writers write the data
        std::vector<std::thread> readers;
        std::function<void()> read =
            [&cache, sleeptime]() {
                while(cache.find(1).second == false || cache.find(2).second == false) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            };

        for (int i=0; i<100; i++) {
            readers.push_back(std::thread(read));
        }

        writer1.join();
        writer2.join();
        for (int i=0; i<100; i++) {
            readers[i].join();
        }

        const auto readers_end = std::chrono::system_clock::now();
        std::chrono::duration<double> readers_duration = readers_end - readers_start;

        // If writers are blocking writers, the execution cannot be completed in less than 2 seconds
        REQUIRE(readers_duration.count() > 2.0f);
        // If readers are not blocking other readers, the executions of readers should be parallel.
        // That is 0.5sec for each reader
        REQUIRE(readers_duration.count() < 2.5f);
    }

}

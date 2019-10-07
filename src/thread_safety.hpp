#include <thread>
#include <mutex>
#include <condition_variable>

struct thread_safety_t
{
    /// \brief Constructor
    thread_safety_t()
        :m_readers_counter(0), m_writers_counter(0)
    {}

    /// \brief Disable copy constructor
    thread_safety_t(const thread_safety_t&) = delete;

    /// \brief Disable assignment operator
    thread_safety_t operator=(const thread_safety_t&) = delete;

    /// \brief Move constructor
    thread_safety_t(thread_safety_t&& other)
        : thread_safety_t()
    {
        swap(*this, other);
    }

    friend void swap(thread_safety_t& first, thread_safety_t& second)
    {
        using std::swap;
        swap(first.m_writers_counter, second.m_writers_counter);
        swap(first.m_readers_counter, second.m_readers_counter);
    }


    /// \brief can_write                    Indicates whether a thread can proceed to writing
    /// \return                             True no other writers or readers are using the cache
    bool can_write()
    {
        return m_writers_counter < 1 && m_readers_counter < 1;
    }

    /// \brief enter_as_writer              Lock resources as writer
    void enter_as_writer()
    {
        std::unique_lock<std::mutex> lock(m_writers);
        m_cv.wait(lock,
                [this]()
                {
                    return can_write();
                });
        m_writers_counter++;
    }

    /// \brief exit_as_writer               Unlock resources as writer
    void exit_as_writer()
    {
        std::lock_guard<std::mutex> lock(m_writers);
        m_writers_counter--;
        m_cv.notify_all();
    }

    /// \brief can_read                     Indicates whether a thread can proceed to reading
    /// \return                             True no other writers or readers are using the cache
    bool can_read()
    {
        return m_writers_counter < 1;
    }

    /// \brief enter_as_reader              Lock resources as readed
    void enter_as_reader()
    {
        std::unique_lock<std::mutex> lock(m_readers);
        m_cv.wait(lock,
                [this]()
                {
                    return can_read();
                });
        m_readers_counter++;
    }

    /// \brief exit_as_reader               Unlock resources as readed
    void exit_as_reader()
    {
        std::lock_guard<std::mutex> lock(m_readers);
        m_readers_counter--;
        m_cv.notify_all();
    }

    /// \brief m_writers_counter            Total number of writes inside the critical section
    size_t m_writers_counter;
    /// \brief m_readers_counter            Total number of writes inside the critical section
    size_t m_readers_counter;

    /// \brief m_readers                    Mutex for readers
    std::mutex m_readers;
    /// \brief m_readers                    Mutex for readers
    std::mutex m_writers;

    /// \brief m_cv                         Condition variable for reading and writing
    std::condition_variable m_cv;
};

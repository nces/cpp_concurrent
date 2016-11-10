#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <string>
#include <memory>

template<typename T>
class TestChannel {
public:
	virtual ~TestChannel() {}
	virtual void test_read(const T& i) = 0;
	virtual void test_write(const T& i) = 0;
	virtual bool test() = 0;
};

/**
 * Aims to be similar to a "Go channel" where only reading and writing to the channel happens in serial.
 */
template<typename T>
class channel {
public:
    // Put an element in the channel
    void write(T i) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_test) m_test->test_read(i);

        m_queue.push_front(i);
        m_cond.notify_one();
    }

    // Read an element in the channel. If the channel is empty and not closed, this call will block the current thread
    // until an item is put in the channel. If there are multiple threads blocked by read, it is up to the OS's thread
    // scheduler to determine which thread will get the next item in the channel.
    //
    // @return true if the channel is still open, false otherwise
    bool read(T& i) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_test) m_test->test_write(i);

        if (!m_queue.empty()) {
            i = m_queue.back();
            m_queue.pop_back();
            return !m_closed;
        } else {
            if (m_closed) {
                return false;
            } else {
                //TODO conditional wait here for input
                m_cond.wait(lock, [this]{ return !m_queue.empty(); });
                i = m_queue.back();
                m_queue.pop_back();
                return true;
            }
        }
    }

    // Closes the channel.
    void close() {
        m_closed = true;
        m_cond.notify_all();
    }

    bool empty() const { return m_queue.empty(); }

    // Add a test via dependency injection
    void setTest(TestChannel<T>* t) { m_test = t; }

protected:
    std::deque<T> m_queue;
    std::mutex m_mutex;
    bool m_closed = false;
    std::condition_variable m_cond;

    TestChannel<T>* m_test = nullptr; // non-managed pointer
};

// A simple usage of channel to make sure messages from multiple threads are readable (i.e., do not overlap one another)
class serial_writer {
public:
    serial_writer() {
        m_wThread = std::make_unique<std::thread>([this]{
            std::string msg;
            while (m_messages.read(msg)) {
                std::cout << msg << std::flush;
            }
        });
    }
    ~serial_writer() {
        m_messages.close();
        std::cout << "m_messages closed" << std::endl;
        m_wThread->join();
        std::cout << "writer thread finished" << std::endl;
    }

    void print(std::string s) {
        m_messages.write(s);
    }

protected:
    channel<std::string> m_messages;
    std::unique_ptr<std::thread> m_wThread;
};

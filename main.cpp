#include <iostream>
#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <string>
#include <memory>
#include <vector>
#include <chrono>

template<typename T>
class channel {
public:
    void write(T i) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_front(i);
        m_cond.notify_one();
    }

    bool read(T& i) {
        std::unique_lock<std::mutex> lock(m_mutex);
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

    void close() { m_closed = true; }

    bool empty() const { return m_queue.empty(); }

protected:
    std::deque<T> m_queue;
    std::mutex m_mutex;
    bool m_closed = false;
    std::condition_variable m_cond;
};

int main()
{
    channel<std::string> chan;
    std::vector<std::unique_ptr<std::thread>> threads;
    constexpr int N = 10;
    for (int i = 0; i < N; ++i) {
        threads.push_back(std::make_unique<std::thread>([&chan,i]{
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Writing " << i << " to channel" << std::endl;
            chan.write(std::to_string(i));
        }));
    }
    //std::thread t([&chan]{
    //    for (int i = 0; i < N; ++i) {
    //        std::this_thread::sleep_for(std::chrono::seconds(1));
    //        std::cout << "Writing " << i << " to channel" << std::endl;
    //        chan.write(std::to_string(i));
    //    }
    //});
    std::string tmp;
    int itemsRead = 0;
    while (chan.read(tmp)) {
        std::cout << "Read " << tmp << " from channel" << std::endl;
        ++itemsRead;
        if (N == itemsRead) {
            chan.close();
        }
    }
    return 0;
}

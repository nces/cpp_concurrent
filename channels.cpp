#include <iostream>
#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <string>
#include <memory>
#include <vector>
#include <chrono>

#include "channels.h"

class Test : public TestChannel<std::string> {
public:
	Test(int n) : N(n) {}
	void test_read(const std::string&) {
		++nRead;
	}

	void test_write(const std::string&) {
		++nWrite;
	}

	bool test() {
		bool status = nRead == nWrite;
		if (!status) {
			std::cerr << "nRead = " << nRead << ", nWrite = " << nWrite << std::endl;
		}
		return nRead == nWrite;
	}

protected:
	const int N;
	int nRead = 0;
	int nWrite = 0;
};

int main()
{
    channel<std::string> chan;
    serial_writer w;
    std::vector<std::unique_ptr<std::thread>> threads;
    constexpr int N = 10;
    Test tester(N);
    chan.setTest(&tester);
    for (int i = 0; i < N; ++i) {
        threads.push_back(std::make_unique<std::thread>([&chan,i,&w]{
            std::this_thread::sleep_for(std::chrono::seconds(1));
            w.print("Writing " + std::to_string(i) + " to channel\n");
            chan.write(std::to_string(i));
            return;
        }));
    }
    std::string tmp;
    int itemsRead = 0;
    while (chan.read(tmp)) {
        w.print("Read " + tmp + " from channel\n");
        ++itemsRead;
        if (N == itemsRead) {
        	std::cout << "closing chan" << std::endl;
            chan.close();
        }
    }
    for (auto& t : threads) { t->join(); }
    std::cout << "threads finished" << std::endl;
    std::cout << "Test status: " << ((tester.test()) ? "SUCCESS" : "FAIL") << std::endl;
    return 0;
}

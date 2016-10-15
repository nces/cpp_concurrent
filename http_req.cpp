#include <string>
#include <sstream>
#include <future>
#include <functional>
#include <thread>
#include <iostream>
#include <vector>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include "channels.h"

using http_res = std::string;

class http_req {
public:
    http_req(std::string url) : m_url(url) {}

    std::future<http_res> run() {
        return std::async(std::launch::async, [this] () -> http_res {
            curlpp::Easy req;
            req.setOpt(new curlpp::options::WriteStream(&m_bodyStream));
            req.setOpt(new curlpp::options::Url(m_url.c_str()));
            req.perform();

            return m_bodyStream.str();
        });
    }

    std::future<http_res> run(std::function<void(http_res)> fn) {
        return std::async(std::launch::async, [this,fn] () -> http_res {
            curlpp::Easy req;
            req.setOpt(new curlpp::options::WriteStream(&m_bodyStream));
            req.setOpt(new curlpp::options::Url(m_url.c_str()));
            req.perform();

            std::string body = m_bodyStream.str();
            std::cout << "finished request" << std::endl;
            fn(body);

            return body;
        });
    }

protected:
    std::string m_url;
    std::stringstream m_bodyStream;
};

int main(int argc, char** argv) {
    curlpp::Cleanup cleaner;
    std::vector<std::future<http_res>> threads;

    serial_writer writer;

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " url [url...]" << std::endl;
    }

    for (char** itr = argv + 1, **argvEnd = argv + argc; itr != argvEnd; itr++) {
        http_req req{std::string(*itr)};
        threads.push_back(req.run([&writer] (http_res res) {
            writer.print("response body = " + res);
        }));
    }

    std::cout << "Waiting for requests to finish..." << std::endl;
    for (auto& t : threads) { t.wait(); }
    std::cout << "Finished waiting for requests" << std::endl;

    return 0;
}

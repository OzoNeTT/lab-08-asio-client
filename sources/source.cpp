#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <vector>
#include <boost/shared_ptr.hpp>
using namespace boost::asio;
io_service service;

struct talk_to_svr {
private:
    ip::tcp::socket sock_;
    enum { max_msg = 1024 };
    int already_read_;
    char buff_[max_msg];
    bool started_;
    std::string username_;

    void write_request() {
        write("ping\n");
    }
    void read_answer() {
        already_read_ = 0;
        read(sock_, buffer(buff_), boost::bind(&talk_to_svr::read_complete, this, _1, _2));
        process_msg();
    }
    void process_msg() {
        std::string msg(buff_, already_read_);
        if ( msg.find("login ") == 0)
            on_login();
        else if ( msg.find("ping") == 0)
            on_ping(msg);
        else if ( msg.find("clients ") == 0)
            on_clients(msg);
    }

    void on_login() {
        std::cout << username_ << " logged in" << std::endl;
        do_ask_clients();
    }
    void on_ping(const std::string & msg) {
        std::istringstream in(msg);
        std::string answer;
        in >> answer >> answer;
        if ( answer == "client_list_changed")
            do_ask_clients();
    }
    void on_clients(const std::string & msg) {
        std::string clients = msg.substr(8);
        std::cout << username_ << ", new client list:" << clients;
    }
    void do_ask_clients() {
        write("clients\n");
        read_answer();
    }

    void write(const std::string & msg) {
        sock_.write_some(buffer(msg));
    }
    size_t read_complete(const boost::system::error_code & err, size_t bytes) {
        if ( err) return 0;
        already_read_ = bytes;
        bool found = std::find(buff_, buff_ + bytes, '\n') < buff_ + bytes;
        return found ? 0 : 1;
    }
public:
    talk_to_svr(const std::string & username) : sock_(service), started_(true), username_(username) {}
    void connect(ip::tcp::endpoint ep) {
        sock_.connect(ep);
    }
    void loop() {
        write("login " + username_ + "\n");
        read_answer();
        while ( started_) {
            write_request();
            read_answer();
            int millis = rand() % 7000;
            std::cout << username_ << " postpone ping " << millis << " ms" << std::endl;
            boost::this_thread::sleep(boost::posix_time::millisec(millis));
        }
    }
    std::string username() const {
        return username_;
    }
};

ip::tcp::endpoint ep( ip::address::from_string("127.0.0.1"), 8001);
void run_client(const std::string & client_name) {
    talk_to_svr client(client_name);
    try {
        client.connect(ep);
        client.loop();
    } catch(boost::system::system_error & err) {
        std::cout << "client terminated " << client.username() << std::endl;
    }
}

int main() {
    std::string names[6] = { "John", "James", "Lucy", "Tracy", "Frank", "Abby"};
    std::vector<boost::thread> threads;
    threads.reserve(6);
    for ( size_t i = 0; i < 6 ; i++) {
        threads.emplace_back(run_client, names[i]);
        boost::this_thread::sleep( boost::posix_time::millisec(100));
    }
    for(boost::thread &thread : threads){
        thread.join();
    }
}
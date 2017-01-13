#define _WEBSOCKETPP_CPP11_STL_

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include <iostream>
#include <map>
#include <string>
#include "connection_metadata.cpp"

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

// creates a new thread for a connection, close the connection and emit messages to a specified thread.   
class websocket_endpoint {
public:
    websocket_endpoint () : m_next_id(0) {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

        m_endpoint.init_asio();
        m_endpoint.start_perpetual();
        // shared pointer to a new thread object pointer  
        m_thread = std::make_shared<std::thread>(&client::run, &m_endpoint);
		
    }

    ~websocket_endpoint() {
        m_endpoint.stop_perpetual();
        
        for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
            if (it->second->get_status() != "Open") {
                // Only close open connections
                continue;
            }
            
            std::cout << "> Closing connection " << it->second->get_id() << std::endl;
            
            std::error_code ec;
            m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
            if (ec) {
                std::cout << "> Error closing connection " << it->second->get_id() << ": "  
                          << ec.message() << std::endl;
            }
        }
        // Resource Acquisition Is Initialization
        m_thread->join();
    }

    int connect(std::string const & uri) {
        std::error_code ec;
		// enable ssl connection settings
		m_endpoint.set_tls_init_handler(connection_metadata::on_tls_init);

        client::connection_ptr con = m_endpoint.get_connection(uri, ec);

        if (ec) {
            std::cout << "> Connect initialization error: " << ec.message() << std::endl;
            return -1;
        }

        int new_id = m_next_id++;
        connection_metadata::ptr metadata_ptr = std::make_shared<connection_metadata>(new_id, con->get_handle(), uri);
        m_connection_list[new_id] = metadata_ptr;
		
        con->set_open_handler(std::bind(
            &connection_metadata::on_open,
            metadata_ptr,
            &m_endpoint,
            std::placeholders::_1
        ));
        con->set_fail_handler(std::bind(
            &connection_metadata::on_fail,
            metadata_ptr,
            &m_endpoint,
            std::placeholders::_1
        ));
        con->set_close_handler(std::bind(
            &connection_metadata::on_close,
            metadata_ptr,
            &m_endpoint,
            std::placeholders::_1
        ));
        con->set_message_handler(std::bind(
            &connection_metadata::on_message,
            metadata_ptr,
            std::placeholders::_1,
            std::placeholders::_2
        ));
		
        m_endpoint.connect(con);

        return new_id;
    }

    void close(int id, websocketpp::close::status::value code, std::string reason) {
        std::error_code ec;
        
        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }
        
        m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
        if (ec) {
            std::cout << "> Error initiating close: " << ec.message() << std::endl;
        }
    }

    void send(int id, std::string message) {
        std::error_code ec;

        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }
        m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
        if (ec) {
            std::cout << "> Error sending message: " << ec.message() << std::endl;
            return;
        }
        
        metadata_it->second->record_sent_message(message);
    }

    connection_metadata::ptr get_metadata(int id) const {
        con_list::const_iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            return connection_metadata::ptr();
        } else {
            return metadata_it->second;
        }
    }
private:
    typedef std::map<int,connection_metadata::ptr> con_list;

    client m_endpoint;
    std::shared_ptr<std::thread> m_thread;

    con_list m_connection_list;
    int m_next_id;
};

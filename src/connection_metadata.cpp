
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include <jansson.h>
#include <iostream>
#include <map>
#include <cmath>
#include <string>
#include <sys/time.h>
#include <unordered_map>

typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef std::multimap<time_t, double> volMMap;

/*
*	Once the websocket connection has been established we maintain the last hour of trades in 2 multimaps.  
*/

class connection_metadata {
public:
	typedef std::shared_ptr<connection_metadata> ptr;
	typedef std::shared_ptr<timeMMap> tickPtr;
	typedef std::shared_ptr<volMMap> volPtr;

	connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
		: m_id(id), m_hdl(hdl), m_status("Connecting"), m_uri(uri), m_server("N/A") {}

	void on_open(client * c, websocketpp::connection_hdl hdl) {
		m_status = "Open";
		client::connection_ptr con = c->get_con_from_hdl(hdl);
		m_server = con->get_response_header("Server");
	}

	void on_fail(client * c, websocketpp::connection_hdl hdl) {
		m_status = "Failed";

		client::connection_ptr con = c->get_con_from_hdl(hdl);
		m_server = con->get_response_header("Server");
		m_error_reason = con->get_ec().message();
	}

	void on_close(client * c, websocketpp::connection_hdl hdl) {
		m_status = "Closed";
		client::connection_ptr con = c->get_con_from_hdl(hdl);
		std::stringstream s;
		s << "close code: " << con->get_remote_close_code() << " ("
		  << websocketpp::close::status::get_string(con->get_remote_close_code())
		  << "), close reason: " << con->get_remote_close_reason();
		m_error_reason = s.str();
	}
	// Handle incoming messages 
	void on_message(websocketpp::connection_hdl, client::message_ptr msg) {

		if (msg->get_opcode() == websocketpp::frame::opcode::text)
		{
			// retrieve message from message pointer
			std::string message = msg->get_payload();
			json_error_t error;
			time_t rawtime;
			time(&rawtime);
			size_t startingSet;
			// Convert message string to a json array
			json_t *jObject = json_loads(message.c_str() , JSON_DECODE_INT_AS_REAL, &error);
			if (!jObject)
			{
				std::cout << "ERROR: config.json incorrect (" << error.text << ")\n" << std::endl;
				return;
			}

			// If we subscribed to either trades or books retrieve corresponding channel id
			if (json_is_object(jObject) && std::string(json_string_value(json_object_get(jObject, "event"))).compare("subscribed") == 0)
			{
				if (std::string(json_string_value(json_object_get(jObject, "channel"))).compare("trades") == 0)
				{
					tradeChanId = json_real_value(json_object_get(jObject, "chanId"));
					std::cout << "Successfully subscribed to trade channel" << std::endl;
				}
				else if (std::string(json_string_value(json_object_get(jObject, "channel"))).compare("book") == 0)
				{
					orderChanId = json_real_value(json_object_get(jObject, "chanId"));
					std::cout << "Successfully subscribed to order channel" << std::endl;
				}
			} else if (json_real_value(json_array_get(jObject, 0))  == tradeChanId && tradeChanId != -1)
			{
				json_array_remove(jObject, 0L);
				// check for initial set
				if (json_is_array(json_array_get(jObject, 0)) != 0) {

					startingSet = json_array_size(json_array_get(jObject, 0));
					for (size_t i = 0; i < startingSet; ++i) {
						if (json_real_value(json_array_get(json_array_get(json_array_get(jObject, 0), i), 3)) < 0.0) {
							json_array_append(json_array_get(json_array_get(jObject, 0), i), json_integer(-1));
						} else {
							json_array_append(json_array_get(json_array_get(jObject, 0), i), json_integer(1));
						}
					}
				}
				else if (!json_is_array(json_array_get(jObject, 0)))
				{
					const char* comparison = json_string_value(json_array_get(jObject, 0));
					int tick = 0;

					if (strcmp(comparison, "te") == 0)
					{

						recentVolume.insert ( std::pair<time_t, double>( rawtime, json_real_value( json_array_get( jObject, 4 ) ) ) );
						recentPrice.insert 	( std::pair<time_t, double>( rawtime, json_real_value( json_array_get( jObject, 3 ) ) ) );
						auto vit = recentVolume.begin();
						auto pit = recentPrice.begin();
						while (difftime(rawtime, vit->first) > 3600.0) {
							recentVolume.erase(vit++);
						}
						while (difftime(rawtime, pit->first) > 3600.0) {
							recentPrice.erase(pit++);
						}
					}
				}
			}
			json_decref(jObject);
		}
	}

	static context_ptr on_tls_init(websocketpp::connection_hdl hdl) {
		// establishes a SSL connection
		context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

		try {
			ctx->set_options(boost::asio::ssl::context::default_workarounds |
			                 boost::asio::ssl::context::no_sslv2 |
			                 boost::asio::ssl::context::no_sslv3 |
			                 boost::asio::ssl::context::single_dh_use);
		} catch (std::exception& e) {
			std::cout << "Error in context pointer: " << e.what() << std::endl;
		}
		return ctx;
	}

	websocketpp::connection_hdl get_hdl() const {
		return m_hdl;
	}

	int get_id() const {
		return m_id;
	}

	std::string get_status() const {
		return m_status;
	}

	volPtr getVolume() {
		volPtr ptr(&recentVolume);
		return ptr;
	}

	volPtr getPrice() {
		volPtr ptr(&recentPrice);
		return ptr;
	}

	tickPtr getTrades() {
		tickPtr ptr(&recentTrades);
		return ptr;
	}

	std::unordered_map<double, double> *getHashTable() {
		return &orderHashTable;
	}

	void record_sent_message(std::string message) {
		m_messages.push_back(">> " + message);
	}

	//friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);
private:

	int m_id;
	websocketpp::connection_hdl m_hdl;
	std::string m_status;
	std::string m_uri;
	std::string m_server;
	std::string m_error_reason;
	double tradeChanId = -1;
	double orderChanId = -1;
	//hashTable orderbook implementation
	std::unordered_map<double, double> orderHashTable;
	std::vector<std::string> m_messages;
	volMMap recentVolume;
	volMMap recentPrice;
	double tickData = 0.0;

};
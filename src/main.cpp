#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <cstdlib>
#include <map>
#include <utility>
#include "websocket_endpoint.cpp"
#include "base64.h"
#include "bitfinex.h"

using namespace std;

// Connects to the Bitfinex endpoint and subscribes to the trades channel.
connection_metadata::ptr connectEndpoint(websocket_endpoint &endpoint) {

	int id = endpoint.connect("wss://api2.bitfinex.com:3000/ws");
	if (id != -1) {
		std::cout << "Connection created with ID: " <<  id << std::endl;
	}
	
	// shared pointer to a connection_metadata object element in the con_list map
	connection_metadata::ptr conData = endpoint.get_metadata(id);
	std::string status = conData->get_status();
	std::cout << "Status of webSocket: " << status << std::endl;

	std::string tradeEvent  = "{ \"event\": \"subscribe\", \"channel\": \"trades\", \"pair\": \"BTCUSD\"}"; //,\"prec\": \"P0\"}";

	while (status != "Open") {
		sleep(0.1);
		status = conData->get_status();
	}
	endpoint.send(id, tradeEvent);

	return conData;
}


int main(int argc, char *argv[]) {

	//initialize websocket connection
	websocket_endpoint endpoint;
	connection_metadata::ptr conData = connectEndpoint(endpoint);

	/*	
		Through the shared pointers below we can access the real-time updated volume and price maps.
	*/
	
	connection_metadata::tickPtr tradesMap;
	connection_metadata::volPtr volMap;

	tradesMap = conData->getTrades();
	volMap = conData->getVolume();

	// Do something with your data. 
	while (true)
	{
		sleep(1.0);
	}

	return 0;
}

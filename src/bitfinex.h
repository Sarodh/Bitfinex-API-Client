#ifndef BITFINEX_H
#define BITFINEX_H

#include <curl/curl.h>
#include <string>
#include <map>

typedef std::pair<std::string, double> Order;

std::string doubleToString(double d);

double stringToDouble(std::string s);

namespace Bitfinex {

// get quote
double getQuote(CURL* curl, bool isBid);

// get the current availability for usd or btc
double getAvail(CURL* curl, std::string currency);

// send order to the exchange and return order ID
// DEPRECATED int sendOrder(CURL *curl, Parameters params, std::string direction, double quantity, double price);
std::pair<std::string, double> sendOrder(CURL* curl, std::string direction, double quantity);

// check the status of the order
bool isOrderComplete(CURL* curl, std::string orderId);

// retrive current trade position if any
double getActivePos(CURL *curl);

// get the limit price according to the requested volume
double getLimitPrice(CURL *curl, double volume, bool isBid);

// send a request to the exchange and return a JSON object
json_t* authRequest(CURL* curl, string url, std::string request, std::string options);

double getLPrice(CURL* curl);

std::pair<std::string, double> sendOCOOrder(CURL* curl, std::string direction, double quantity, double upperBound, double lowerBound);

std::pair<std::string, double> sendMarketOrder(CURL* curl, std::string direction, double quantity);

bool isExitOrderComplete(CURL *curl, Order &orderId);

std::string getActiveOrderID(CURL *curl);

double getActivePrice(CURL *curl);

double getActivePL(CURL *curl);

}

#endif
// Bitfinex API client
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <sys/time.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include "base64.h"
#include <jansson.h>
#include <cmath>
#include "bitfinex.h"
#include "curlFunc.h"
#include <iomanip>

using namespace std;


std::string doubleToString(double d)
{
	std::ostringstream ss;
	ss << d;
	return ss.str();
}

double stringToDouble(std::string s)
{
	return atof(s.c_str());
}

namespace Bitfinex {
double getQuote(CURL* curl, bool isBid) {
	json_t* root = getJsonFromUrl(curl, "https://api.bitfinex.com/v1/ticker/btcusd", "");
	const char* quote;
	double quoteValue;
	if (isBid) {
		quote = json_string_value(json_object_get(root, "bid"));
	} else {
		quote = json_string_value(json_object_get(root, "ask"));
	}
	if (quote != NULL) {
		quoteValue = atof(quote);
	} else {
		quoteValue = 0.0;
	}
	json_decref(root);
	return quoteValue;
}

double getLPrice(CURL* curl)
{
	json_t* root = getJsonFromUrl(curl, "https://api.bitfinex.com/v1/pubticker/btcusd", "");
	const char* price;
	double priceValue;

	price = json_string_value(json_object_get(root, "last_price"));

	if (price != NULL)
		priceValue = atof(price);
	else
		priceValue = 0.0;

	json_decref(root);
	return priceValue;
}

double getAvail(CURL *curl, string currency) {
	json_t *root = authRequest(curl, "https://api.bitfinex.com/v1/balances", "balances", "");

	//returns -1 if the apikey is invalid
	if (json_integer_value(root) == -1)
		return -1;

	while (json_object_get(root, "message") != NULL) {
		sleep(1.0);
		cout << "<Bitfinex> Error with JSON: " << json_dumps(root, 0) << ". Retrying..." << endl;
		root = authRequest(curl, "https://api.bitfinex.com/v1/balances", "balances", "");
	}

	// go through the list (order not preserved for some reason)
	size_t arraySize = json_array_size(root);
	double availability = 0.0;
	for (size_t i = 0; i < arraySize; i++) {
		string tmpType = json_string_value(json_object_get(json_array_get(root, i), "type"));
		string tmpCurrency = json_string_value(json_object_get(json_array_get(root, i), "currency"));
		if (tmpType.compare("trading") == 0 && tmpCurrency.compare(currency.c_str()) == 0) {
			availability = atof(json_string_value(json_object_get(json_array_get(root, i), "amount")));
		}
	}
	json_decref(root);
	return availability;
}

pair<string, double> sendOrder(CURL* curl, string direction, double quantity) {
	double limPrice;  // define limit price to be sure to be executed
	if (direction.compare("buy") == 0) {
		limPrice = getLimitPrice(curl, quantity, false);
		cout << "<Bitfinex> Buy LimPrice: " << limPrice << endl;
	}
	else if (direction.compare("sell") == 0) {
		limPrice = getLimitPrice(curl, quantity, true);
		cout << "<Bitfinex> Sell LimPrice: " << limPrice << endl;
	}

	cout << "<Bitfinex> Trying to send a \"" << direction << "\" limit order: " << quantity << "@$" << limPrice << "..." << endl;
	ostringstream oss;
	oss << "\"symbol\":\"btcusd\", \"amount\":\"" << quantity << "\", \"price\":\"" << limPrice << "\", \"exchange\":\"bitfinex\", \"side\":\"" << direction << "\", \"type\":\"limit\"";
	string options = oss.str();

	json_t *root = authRequest(curl, "https://api.bitfinex.com/v1/order/new", "order/new", options);
	if (root == json_integer(-1)) {
		return make_pair("none", -1.0);
	}
	int orderId = json_integer_value(json_object_get(root, "order_id"));
	std::cout << "<Bitfinex> Done (order ID: " << orderId << ")\n" << endl;
	json_decref(root);
	return make_pair(std::to_string(orderId), limPrice);
}

pair<string, double> sendOCOOrder(CURL* curl, string direction, double quantity, double upperBound, double lowerBound) {

	cout << "<Bitfinex> Trying to send a \"" << direction << "\" OCO limit order: "
	     << quantity << "@" << " Upper Bound: $" << upperBound << "..." << " Lower Bound: $" << lowerBound << "..." << endl;
	ostringstream oss;
	oss << "\"symbol\":\"btcusd\", \"amount\":\"" << quantity
	    << "\", \"price\":\"" << upperBound
	    << "\", \"exchange\":\"bitfinex\", \"side\":\"" << direction
	    << "\", \"type\":\"limit"
	    << "\", \"ocoorder\":true, \""
	    << direction << "_price_oco\":\"" << lowerBound << "\"";
	string options = oss.str();

	json_t *root = authRequest(curl, "https://api.bitfinex.com/v1/order/new", "order/new", options);
	if (root == json_integer(-1)) {
		return make_pair("none", -1.0);
	}
	int orderId = json_integer_value(json_object_get(root, "order_id"));
	std::cout << "<Bitfinex> Done (order ID: " << orderId << ")\n" << endl;
	json_decref(root);
	return make_pair(std::to_string(orderId), upperBound);
}

pair<string, double> sendMarketOrder(CURL* curl, string direction, double quantity) {

	cout << "<Bitfinex> Trying to send a \"" << direction << "\" Market order: " << quantity << "@$" << getLPrice(curl) << "..." << endl;
	ostringstream oss;
	oss << "\"symbol\":\"btcusd\", \"amount\":\"" << quantity << "\", \"price\":\"" << 1 << "\", \"exchange\":\"bitfinex\", \"side\":\"" << direction << "\", \"type\":\"market\"";
	string options = oss.str();

	json_t *root = authRequest(curl, "https://api.bitfinex.com/v1/order/new", "order/new", options);
	if (root == json_integer(-1)) {
		return make_pair("none", -1.0);
	}
	int orderId = json_integer_value(json_object_get(root, "order_id"));
	double orderPrice = json_real_value(json_object_get(root, "avg_execution_price"));
	std::cout << "<Bitfinex> Done (order ID: " << orderId << ":" << orderPrice << ")\n" << endl;
	json_decref(root);
	return make_pair(std::to_string(orderId), orderPrice);
}

bool isOrderComplete(CURL* curl, string orderId) {
	if (orderId.empty() || orderId == "0") {
		return true;
	}
	std::stringstream ss;
	ss << "\"order_id\":" << orderId;
	std::string options = ss.str();

	json_t *root = authRequest(curl, "https://api.bitfinex.com/v1/order/status", "order/status", options);
	bool isComplete = !json_boolean_value(json_object_get(root, "is_live"));
	if (isComplete) {
		conLog << logger::ILOG << "ORDER COMPLETED";
	}
	json_decref(root);
	return isComplete;
}

bool isExitOrderComplete(CURL *curl, Order &orderId) {
	if (orderId.first.empty() || orderId.first == "0") {
		return true;
	}
	std::stringstream ss;
	ss << "\"order_id\":" << orderId.first;
	std::string options = ss.str();

	json_t *root = authRequest(curl, "https://api.bitfinex.com/v1/order/status", "order/status", options);
	bool isComplete = !json_boolean_value(json_object_get(root, "is_live"));
	if (isComplete) {
		conLog << logger::ILOG << "ORDER COMPLETED";
		orderId.second = stringToDouble(json_string_value(json_object_get(root, "avg_execution_price")));
	}
	json_decref(root);
	return isComplete;
}

string getActiveOrderID(CURL *curl) {
	json_t *root = authRequest(curl, "https://api.bitfinex.com/v1/orders", "orders", "");

	string id = "";
	if (json_array_size(root) == 0) {
		cout << "<Bitfinex> WARNING: No orders found" << endl;
		id = "none";
	}
	else
		id = to_string(json_integer_value(json_object_get(json_array_get(root, 0), "id")));
	json_decref(root);
	return id;
}

double getActivePos(CURL *curl) {
	json_t *root = authRequest(curl, "https://api.bitfinex.com/v1/positions", "positions", "");
	double position;
	if (json_array_size(root) == 0) {
		cout << "<getActivePos()> WARNING: BTC position not available, return 0.0" << endl;
		position = 0.0;
	} else {
		position = atof(json_string_value(json_object_get(json_array_get(root, 0), "amount")));
	}
	json_decref(root);
	return position;
}

double getActivePrice(CURL *curl) {
	json_t *root = authRequest(curl, "https://api.bitfinex.com/v1/positions", "positions", "");
	double position;
	if (json_array_size(root) == 0) {
		cout << "<getActivePrice()> WARNING: BTC position not available, return 0.0" << endl;
		position = 0.0;
	} else {
		position = atof(json_string_value(json_object_get(json_array_get(root, 0), "base")));
	}
	json_decref(root);
	return position;
}

double getActivePL(CURL *curl) {
	json_t *root = authRequest(curl, "https://api.bitfinex.com/v1/positions", "positions", "");
	double position;
	double base;
	double swap;
	double amount;
	if (json_array_size(root) == 0) {
		cout << "<Bitfinex> WARNING: BTC position not available, return 0.0" << endl;
		position = 0.0;
	} else {

		position = atof(json_string_value(json_object_get(json_array_get(root, 0), "pl")));
		base = atof(json_string_value(json_object_get(json_array_get(root, 0), "base")));
		swap = atof(json_string_value(json_object_get(json_array_get(root, 0), "swap")));
		amount = atof(json_string_value(json_object_get(json_array_get(root, 0), "amount")));
	}
	
	std::cout << "---------------------------------" << std::endl;
	std::cout << "Amount Traded: " << std::setprecision(4) << amount << std::endl;
	std::cout << "PL in USD: " << std::setprecision(4) << position << std::endl;
	std::cout << "Trade Price: " << std::setprecision(4) << base << std::endl;
	std::cout << "Swap in USD: " << std::setprecision(4) << swap << std::endl;
	std::cout << "---------------------------------" << std::endl;

	json_decref(root);
	return (((position + swap) / (base * std::abs(amount))) * 100);
}


double getLimitPrice(CURL *curl, double volume, bool isBid) {

	json_t *root;
	if (isBid) {
		root = json_object_get(getJsonFromUrl(curl, "https://api.bitfinex.com/v1/book/btcusd", ""), "bids");
	} else {
		root = json_object_get(getJsonFromUrl(curl, "https://api.bitfinex.com/v1/book/btcusd", ""), "asks");
	}

	// loop on volume
	double tmpVol = 0.0;
	int i = 0;
	while (tmpVol < volume) {
		// volumes are added up until the requested volume is reached
		tmpVol += atof(json_string_value(json_object_get(json_array_get(root, i), "amount")));
		i++;
	}
	// return the second next offer
	double limPrice = atof(json_string_value(json_object_get(json_array_get(root, i + 1), "price")));
	json_decref(root);
	return limPrice;
}

json_t* authRequest(CURL* curl, string url, string request, string options) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long nonce = (tv.tv_sec * 1000.0) + (tv.tv_usec * 0.001) + 1.5;

	string bitfinexSecret = std::getenv("BFX-KEY");
	string bitfinexApi = "X-BFX-APIKEY: " + std::getenv("BFX-SECRET");

	ostringstream oss;
	if (options.empty()) {
		oss << "{\"request\":\"/v1/" << request << "\",\"nonce\":\"" << nonce << "\"}";
	}
	else {
		oss << "{\"request\":\"/v1/" << request << "\",\"nonce\":\"" << nonce << "\", " << options << "}";
	}
	string tmpPayload = base64_encode(reinterpret_cast<const unsigned char*>(oss.str().c_str()), oss.str().length());

	oss.clear();
	oss.str("");
	oss << "X-BFX-PAYLOAD:" << tmpPayload;
	string payload;
	payload = oss.str();

	oss.clear();
	oss.str("");

	// build the signature
	unsigned char* digest;

	// Using sha384 hash engine
	digest = HMAC(EVP_sha384(), bitfinexSecret.c_str(), bitfinexSecret.length(), (unsigned char*)tmpPayload.c_str(), tmpPayload.length(), NULL, NULL);

	char mdString[SHA384_DIGEST_LENGTH + 100];
	for (int i = 0; i < SHA384_DIGEST_LENGTH; ++i) {
		sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);
	}
	oss.clear();
	oss.str("");
	oss << "X-BFX-SIGNATURE:" << mdString;

	// cURL headers
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, bitfinexApi.c_str());
	headers = curl_slist_append(headers, payload.c_str());
	headers = curl_slist_append(headers, oss.str().c_str());

	// cURL request
	CURLcode resCurl;
	if (curl) {
		int httpCode(0);
		string readBuffer;
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
		resCurl = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
		json_t *root;
		json_error_t error;


		while (resCurl != CURLE_OK) {
			cout << "<Bitfinex> Error with cURL. Retry in 2 sec...\n" << endl;
			sleep(2.0);
			readBuffer = "";
			resCurl = curl_easy_perform(curl);
		}
		//DEBUG
		/*if (url == "https://api.bitfinex.com/v1/order/status")
			cout << "Result of order request: " << readBuffer << endl;*/
		while (httpCode != 200) {
			sleep(2.0);
			cout << "<Bitfinex> Invalid authorization credentials, http code: " << httpCode << ". Retrying..." << endl;
			cout << readBuffer << endl;
			readBuffer = "";
			resCurl = curl_easy_perform(curl);
		}
		root = json_loads(readBuffer.c_str(), 0, &error);
		while (!root) {
			cout << "<Bitfinex> Error with JSON:\n" << error.text << ". Retrying..." << endl;
			readBuffer = "";
			resCurl = curl_easy_perform(curl);
			while (resCurl != CURLE_OK) {
				cout << "<Bitfinex> Error with cURL. Retry in 2 sec...\n" << endl;
				sleep(2.0);
				readBuffer = "";
				resCurl = curl_easy_perform(curl);
			}
			root = json_loads(readBuffer.c_str(), 0, &error);
		}
		curl_slist_free_all(headers);
		curl_easy_reset(curl);
		return root;
	} else {
		cout << "<Bitfinex> Error with cURL init." << endl;
		return NULL;
	}
}
}
#ifndef CURLFUNC_H
#define CURLFUNC_H

#include <string>
#include <jansson.h>
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <unistd.h>


// general curl callback
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	((std::string*)userp)->append(static_cast<const char*>(contents), size * nmemb);
	return size * nmemb;
}

// return JSON data from not authentificated address
json_t* getJsonFromUrl(CURL *curl, std::string url, std::string postFields) {
	struct curl_slist *headers = NULL;
	std::string useragent = "User-agent: bitfinex-client";
	headers = curl_slist_append(headers, useragent.c_str());

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	if (!postFields.empty()) {
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
	}
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	std::string readBuffer;
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 3600);
	CURLcode resCurl = curl_easy_perform(curl);

	while (resCurl != CURLE_OK) {
		std::cout << "Error with cURL. Retry in 2 sec...\n" << std::endl;
		sleep(2.0);
		readBuffer = "";
		curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 0);
		resCurl = curl_easy_perform(curl);
	}

	json_t *root;
	json_error_t error;
	root = json_loads(readBuffer.c_str(), 0, &error);
	while (!root) {
		std::cout << "Error with JSON:\n" << error.text << ". Retrying..." << std::endl;
		readBuffer = "";
		resCurl = curl_easy_perform(curl);
		while (resCurl != CURLE_OK) {
			std::cout << "Error with cURL. Retry in 2 sec...\n" << std::endl;
			sleep(2.0);
			readBuffer = "";
			resCurl = curl_easy_perform(curl);
		}
		root = json_loads(readBuffer.c_str(), 0, &error);
	}

	curl_easy_reset(curl);
	return root;
}

#endif
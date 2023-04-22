#pragma once
#include "TableStorageEntry.h"
#include "ProjectStructures.h"
#include <string>

class TableStorageRequestHandler
{
public:
	TableStorageRequestHandler();
	enum Verb { GET, DELETE, POST, PUT };
	HttpObject SendRequest(Verb, TableStorageEntry);
	void getAzureConfiguration();
	std::string CalcHmacSHA256(std::string_view decodedKey, std::string_view msg);
	static size_t WriteCallback(char* contents, size_t size, size_t nmemb, void* userp);
private:
	std::string accountName;
	std::string storageAccountKey;
	std::string tableName;
	std::string authHeaderKey;
	struct curl_slist* slist1;
};



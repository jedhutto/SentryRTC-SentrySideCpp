#pragma once
#include <string>

class TableStorageEntry
{
public:
	TableStorageEntry(std::string, std::string, std::string, std::string);
	TableStorageEntry(std::string);
	TableStorageEntry();
	bool isInitialized();
	std::string toJsonString();
	void fromJsonString(std::string);
	std::string getJsonProperty(std::string, std::string);
	std::string description;
	std::string candidate;
	std::string status;
	std::string RowKey;
	std::string Timestamp;
	std::string PartitionKey;
private:
	bool isInit;
};
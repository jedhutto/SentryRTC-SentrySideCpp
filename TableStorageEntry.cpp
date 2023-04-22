#include "TableStorageEntry.h"
#include <string>
#include "base64.hpp"

TableStorageEntry::TableStorageEntry(std::string name, std::string description, std::string candidate, std::string status)
{
	this->description = description;
	this->status = status;
	this->candidate = candidate;
	this->PartitionKey = "unibottable";
	this->RowKey = name;
	this->isInit = true;
}
TableStorageEntry::TableStorageEntry(std::string name)
{
	this->RowKey = name;
	this->PartitionKey = "unibottable";
}

std::string TableStorageEntry::toJsonString()
{
	std::string entry = "{\"PartitionKey\":\"" + PartitionKey + "\",\"RowKey\":\"" + RowKey + "\"";
	if (description != "") {
		entry += ",\"description\":\"" + base64::to_base64(description) + "\"";
	}
	if (status != "") {
		entry += ",\"status\":\"" + status + "\"";
	}
	if (candidate != "") {
		entry += ",\"candidate\":\"" + base64::to_base64(candidate) + "\"";
	}
	if (Timestamp != "") {
		entry += ",\"Timestamp\":\"" + Timestamp + "\"";
	}

	entry += "}";
	//fprintf(stderr, "\nBODY:\n%s\n\n", entry.c_str());
	return std::string(entry);
}

void TableStorageEntry::fromJsonString(std::string entryJson)
{
	PartitionKey = getJsonProperty(entryJson, "PartitionKey\":\"");
	RowKey = getJsonProperty(entryJson, "RowKey\":\"");
	description = base64::from_base64(getJsonProperty(entryJson, "description\":\""));
	status = getJsonProperty(entryJson, "status\":\"");
	candidate = base64::from_base64(getJsonProperty(entryJson, "candidate\":\""));
	Timestamp = getJsonProperty(entryJson, "Timestamp\":\"");
}

std::string TableStorageEntry::getJsonProperty(std::string entryJson, std::string property)
{
	//"{\"PartitionKey\":\"unibottable\",\"RowKey\":\"0000003\",\"Timestamp\":\"2022-09-06T13:25:49.8604891Z\",\"status\":\"ready\"}"
	int keyLength = std::string(property).length();
	int startIndex = entryJson.find(property);
	if (startIndex < 0) {
		return "";
	}
	startIndex += keyLength;
	int endIndex = entryJson.find("\",\"", startIndex);
	if (endIndex < 0) {
		endIndex = entryJson.find("\"}", startIndex);
		if (endIndex < 0) {
			return "";
		}
	}
	std::string value = entryJson.substr(startIndex, endIndex - startIndex);
	return value;
}

TableStorageEntry::TableStorageEntry()
{
	this->isInit = false;
}

bool TableStorageEntry::isInitialized()
{
	return this->isInit;
}

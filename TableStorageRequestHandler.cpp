#include "TableStorageRequestHandler.h"
#include "TableStorageEntry.h"
#include "ProjectStructures.h"
#include <string>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <curl/curl.h>
#include "base64.hpp"
#include <sys/stat.h>
#include <string.h>
#include <array>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <filesystem>

TableStorageRequestHandler::TableStorageRequestHandler()
{
	getAzureConfiguration();
}

HttpObject TableStorageRequestHandler::SendRequest(Verb verb, TableStorageEntry entry = TableStorageEntry())
{
	struct curl_slist* slist;
	CURL* curl;
	CURLcode res;
	struct stat file_info;
	std::string url;
	std::string params = "";
	time_t rawtime;
	struct tm* timeinfo;
	char timeBuffer[80];
	std::string jsonstr; //TODO Build Body

	time(&rawtime);
	timeinfo = gmtime(&rawtime);

	strftime(timeBuffer, 80, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	char* timeHeader = (char*)malloc(strlen(timeBuffer) + strlen("x-ms-date:") + 1);
	strcpy(timeHeader, "x-ms-date:");
	strcat(timeHeader, timeBuffer);
	entry.Timestamp = timeBuffer;

	curl_global_init(CURL_GLOBAL_ALL);

	slist1 = NULL;
	slist1 = curl_slist_append(slist1, "Content-Type:application/json");
	slist1 = curl_slist_append(slist1, "Accept:application/json");//;odata=nometadata
	slist1 = curl_slist_append(slist1, timeHeader);
	slist1 = curl_slist_append(slist1, "x-ms-version:2020-04-08");
	slist1 = curl_slist_append(slist1, "DataServiceVersion:3.0;NetFx");
	slist1 = curl_slist_append(slist1, "MaxDataServiceVersion:3.0;NetFx");
	//fprintf(stderr, "\nTIME HEADER%s\n\n", timeHeader);


	//Authorization Section
	uint8_t hashBuffer[250];
	for (int i = 0; i < 250; i++) {
		hashBuffer[i] = '\0';
	}

	params = "(PartitionKey='" + entry.PartitionKey + "',RowKey='" + entry.RowKey + "')";

	url = "https://unibottable.table.core.windows.net/DescCanExchangeTable" + params;

	std::string stringToSign;
	stringToSign = std::string(timeBuffer) + '\n' + '/' + accountName + '/' + tableName + params;

	std::string hash = CalcHmacSHA256(base64::from_base64(storageAccountKey), stringToSign.c_str());

	uint8_t hashPre64[33];
	hashPre64[32] = '\0';
	for (int i = 0; i < 33; i++) {
		hashPre64[i] = (uint8_t)hash[i];
	}

	std::string stringHash((char*)hashPre64, 32);
	std::string base64Auth = base64::to_base64(stringHash);
	std::string authHeaderComplete = authHeaderKey + base64Auth;
	slist1 = curl_slist_append(slist1, authHeaderComplete.c_str());

	//Authorization Section End

	char bodyLength[80];
	char contentLength[80];
	curl = curl_easy_init();
	std::string readBuffer;
	std::string entryJson;
	long code = 0;
	if (curl) {
		switch (verb) {
		case GET:
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
			break;
		case DELETE:
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;
		case POST:
			entryJson = entry.toJsonString();
			sprintf(bodyLength, "%d", entryJson.length());
			strcpy(contentLength, "Content-Length:");
			strcat(contentLength, bodyLength);
			slist1 = curl_slist_append(slist1, contentLength);

			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, entryJson.c_str());
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
			break;
		case PUT:
			entryJson = entry.toJsonString();
			//sprintf(bodyLength, "%d", entryJson.length());
			//strcpy(contentLength, "Content-Length:");
			//strcat(contentLength, bodyLength);
			//slist1 = curl_slist_append(slist1, contentLength);

			curl_easy_setopt(curl, CURLOPT_POST, true);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, entryJson.c_str());
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
			curl_easy_setopt(curl, CURLOPT_HEADER, true);
			break;
		default:
			return HttpObject{ entry, -1 };
		}

		//fprintf(stderr, "\nURL:\n%s\n\n", url.c_str());
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		res = curl_easy_perform(curl);
		
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		if (code != 200 && code != 204) {
			fprintf(stderr, "%s\n", (&readBuffer)->c_str());
		}
		//else {
		//	entry.fromJsonString(readBuffer);
		//	return HttpObject{ entry, code };
		//}
		curl_easy_cleanup(curl);
	}

	free(timeHeader);
	curl_global_cleanup();
	//fprintf(stderr, "%s\n", (&readBuffer)->c_str());
	entry.fromJsonString(readBuffer);

	return HttpObject{ entry, (int)code };
}

void TableStorageRequestHandler::getAzureConfiguration()
{
	std::ifstream azureConfigTxt("/home/pi/azure_configuration.txt");
	if (azureConfigTxt.is_open())
	{
		std::getline(azureConfigTxt, accountName);
		std::getline(azureConfigTxt, storageAccountKey);
		std::getline(azureConfigTxt, tableName);
		std::getline(azureConfigTxt, authHeaderKey);
	}
	else 
	{
		throw std::invalid_argument("azure_configuration.txt not found at: " + std::string("/home/pi") + std::string(std::filesystem::current_path()));
	}
}

std::string TableStorageRequestHandler::CalcHmacSHA256(std::string_view decodedKey, std::string_view msg)
{
	std::array<unsigned char, EVP_MAX_MD_SIZE> hash;
	unsigned int hashLen;
	unsigned char* thistemp = HMAC(
		EVP_sha256(),
		decodedKey.data(),
		static_cast<int>(decodedKey.size()),
		reinterpret_cast<unsigned char const*>(msg.data()),
		static_cast<int>(msg.size()),
		hash.data(),
		&hashLen
	);

	return std::string{ reinterpret_cast<char const*>(hash.data()), hashLen };
}

size_t TableStorageRequestHandler::WriteCallback(char* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}
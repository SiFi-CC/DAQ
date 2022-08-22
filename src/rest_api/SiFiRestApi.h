#ifndef SIFIRESTAPI_H
#define SIFIRESTAPI_H

#include <algorithm>
#include <curl/curl.h>
#include <iostream>
#include <string>
#include "nlohmann/json.hpp"

class SiFiRestApi {
    private:
        std::string endpoint_url;
        std::string username_and_password;
        void Download(std::string &readBuffer);
        void Upload(const std::string &holder);
    public:
        SiFiRestApi();
        SiFiRestApi(const std::string endpoint);
        ~SiFiRestApi();
        void SetUsernameAndPassword(std::string username, std::string password);
        void Fetch(std::map<std::string, std::string> &container);
        void Fetch(std::vector<std::map<std::string, std::string> > &container);
        void Write(const std::vector<std::map<std::string, std::string> > &data);
        void Write(const std::map<std::string, std::string> &data);
        void Print();
};

#endif /* SIFIRESTAPI_H */

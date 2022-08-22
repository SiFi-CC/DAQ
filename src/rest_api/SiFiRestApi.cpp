#include "SiFiRestApi.h"
SiFiRestApi::SiFiRestApi() : endpoint_url(""), username_and_password("") {}
SiFiRestApi::~SiFiRestApi() {}
SiFiRestApi::SiFiRestApi(const std::string endpoint) {
    endpoint_url = "https://bragg.if.uj.edu.pl/SiFiCCData/Prototype/api/" + endpoint;
}
void SiFiRestApi::SetUsernameAndPassword(std::string username, std::string password) {
    username_and_password = username + ":" + password;
}
void SiFiRestApi::Download(std::string &readBuffer) {
    CURL *curl;
    CURLcode res = CURLE_FAILED_INIT;
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, endpoint_url.c_str() );
        curl_easy_setopt(curl, CURLOPT_USERPWD, username_and_password.c_str() );
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t {
            static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    if(res != CURLE_OK) {
        fprintf(stderr, "%s\n", curl_easy_strerror(res) );
    }
}
void SiFiRestApi::Upload(const std::string &holder) {
    CURLcode res = CURLE_FAILED_INIT;
    CURL* curl = curl_easy_init();
    std::string url(endpoint_url);
    struct curl_slist* headers = NULL;
    if(curl){
        headers = curl_slist_append(headers, "Expect:");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string header = "Content-Type: application/json";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERPWD, username_and_password.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, holder.c_str());
        res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    if(res != CURLE_OK) {
        fprintf(stderr, "%s\n", curl_easy_strerror(res) );
    }
}
void SiFiRestApi::Fetch(std::map<std::string, std::string> &container) {
    std::string readBuffer;
    Download(readBuffer);
    nlohmann::json j = nullptr;
    try {
        j = nlohmann::json::parse(readBuffer);
    } catch (nlohmann::json::parse_error &e) {
        fprintf(stderr, e.what() );
    }
    for(nlohmann::json::iterator it = j.begin(); it != j.end(); ++it) {
        try {
            container[it.key() ] = it.value();
        } catch (nlohmann::json::type_error &e) {
            //doesn't fit the initial data type so dump it into string
            container[it.key() ] = it.value().dump();
        }
    }
}
void SiFiRestApi::Fetch(std::vector<std::map<std::string, std::string> > &container) {
    std::string readBuffer;
    Download(readBuffer);
    nlohmann::json j = nlohmann::json::parse(readBuffer);
    for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it) {
        std::map<std::string, std::string> m;
        for(nlohmann::json::iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2) {
            try {
                m[it2.key() ] = it2.value();
            } catch (nlohmann::json::type_error &e) {
                //doesn't fit the initial data type so dump it into string
                m[it2.key() ] = it2.value().dump();
            }
        }
        container.push_back(m);
    }
}
/*
 * Writes data to the REST API endpoint
 */
void SiFiRestApi::Write(const std::vector<std::map<std::string, std::string> > &data) { 
    nlohmann::json jout;
    for(int i = 0; i < data.size(); ++i){
        nlohmann::json holder;
        for(std::map<std::string, std::string>::const_iterator it = data[i].begin(); it != data[i].end(); ++it){
            holder[it->first] = it->second;
        }
        jout.push_back(holder);
    }
    Upload(jout.dump() );
}
void SiFiRestApi::Write(const std::map<std::string, std::string> &data) { 
    nlohmann::json holder;
    for(std::map<std::string, std::string>::const_iterator it = data.begin(); it != data.end(); ++it){
        holder[it->first] = it->second;
    }
    Upload(holder.dump() );
}
void SiFiRestApi::Print() {
//    std::cout << "[ " << std::endl;
//	for(std::vector<std::map<std::string, std::string> >::const_iterator it = container.begin(); it != container.end(); ++it) {
//		std::cout << "{ ";
//		for(std::map<std::string, std::string>::const_iterator it2 = it->begin(); it2 != it->end(); ++it2) {
//			std::cout << it2->first << ": " << it2->second << ", ";
//		}
//		std::cout << " }, ";
//	}
//	std::cout << " ]" << std::endl;
}

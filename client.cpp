//https://gist.github.com/alghanmi/c5d7b761b2c9ab199157 
#include <iostream>
#include <string>
#include <curl/curl.h>


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string getMaster(std::string host){ //TODO

    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        host.append("/get_master");
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res == 0){ // we got a response TODO add response codes or something for each error case
            return readBuffer;
        }
    }else{
        fprintf(stderr,"Could not init curl\n");
        return "failure";
    }
    return "failure";
}

std::string getWorker(std::string host){ //TODO
    
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        host.append("/get_worker");
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res == 0){ // we got a response TODO add response codes or something for each error case
            return readBuffer;
        }
    }else{
        fprintf(stderr,"Could not init curl\n");
        return "failure";
    }
    return "failure";
}

void getProblem(){ //TODO
    throw "getProblem is not implemented yet.";
    return;
}

void heartbeat(){ //TODO
    throw "heartbeat is not implemented yet.";
    return;
}

void sendResult(){ //TODO
    throw "sendResult is not implemented yet.";
    return;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "should be called as ./client [connectionpoint]" << std::endl;
        return -1;
    }
    
    std::string host = argv[1];
    std::string master = getMaster(host);
    if (master == "failure")
    {
        std::cout << "tsja TODO" << std::endl;
    }
    
    std::cout << master << std::endl;

    std::string worker = getWorker(master);

    if (worker == "")
    {
        std::cout << "empty worker assignment" << std::endl;
    }
    else
    {
        std::cout << worker << std::endl;
    }
    
    return 0;
}

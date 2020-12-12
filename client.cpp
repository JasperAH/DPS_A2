//https://gist.github.com/alghanmi/c5d7b761b2c9ab199157 
#include <iostream>
#include <string>
#include <curl/curl.h>


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void getMaster(){ //TODO
    throw "getMaster is not implemented yet.";
    return;
}

void getWorker(){ //TODO
    throw "getWorker is not implemented yet.";
    return;
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

    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "localhost");
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        std::cout << readBuffer << std::endl;
    }
    return 0;
}

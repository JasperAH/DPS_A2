//https://gist.github.com/alghanmi/c5d7b761b2c9ab199157 
#include <iostream>
#include <sstream>
#include <string>
#include <curl/curl.h>
#include <fstream>
#include <vector>

std::string line1;
std::string line2;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string getMaster(std::string host){

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

std::string getWorker(std::string host){
    
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

int getProblem(std::string host){ //TODO
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        host.append("/get_problem");
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        std::cout << "readBuffer: " << readBuffer << std::endl;
        if(res == 0){ // we got a response TODO add response codes or something for each error case
            std::ofstream out("tmp_data.csv"); //TODO magic name
            out << readBuffer;
            out.close();
            return 1;
        }
    }else{
        fprintf(stderr,"Could not init curl\n");
        return 0;
    }
    return -1;
}

void heartbeat(){ //TODO
    throw "heartbeat is not implemented yet.";
    return;
}

int computeProblem(int &lineNumber){
    std::ifstream file("tmp_data.csv");
    std::string line;
    if (file.is_open()) {
        std::getline(file, line);
        std::cout << "line: " << line << std::endl;
        lineNumber = std::stoi(line);
        std::vector<std::vector<int>> data;
        while (std::getline(file, line))
        {
            std::stringstream lineStream(line);
            std::string val;
            std::vector<int> row;
            while (std::getline(lineStream, val, 'c'))
            {
                std::cout << "val: " << val << std::endl;
                row.push_back(std::stoi(val));
            }
            data.push_back(row);
        }
        file.close();

        int result = 0;
        for (int i = 0; i < data[0].size(); i++)
        {
            result += data[0][i] * data[1][i];
        }
        return result;
        
    }
    
    return -1;
}

void sendResult(std::string host, int result, int lineNumber){
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    curl = curl_easy_init();
    if(curl) {
        std::string tmp = "q=uploadFromClient&index=";
        host = host.append("/?q=uploadFromClient&index=");
        host = host.append(std::to_string(lineNumber));
        host = host.append("&result=");
        host = host.append(std::to_string(result)); //TODO 4 is arbitrary and magic, find optimum
        char *data = &tmp[0];
        std::cout << "host: " << host << std::endl;
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if(res == 0){ // we got a response
            std::cout << "result succesfully uploaded." << std::endl;
        }
    }else{
        fprintf(stderr,"Could not init curl\n");
    }
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

    getProblem(worker);

    int lineNumber;

    int result = computeProblem(lineNumber); //TODO do somethin with error (result == -1)

    sendResult(worker, result, lineNumber);
    
    return 0;
}

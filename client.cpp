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
        if(res == 0 && readBuffer[0] != 'X'){ // we got a response TODO add response codes or something for each error case
            std::ofstream out("tmp_data.csv"); //TODO magic name
            out << readBuffer;
            out.close();
            return 1;
        }
        else if(res == 503 || readBuffer[0] == 'X') //Pistache::Http::Code::Service_Unavailable
        {
            std::cout << "Data unavailable" << std::endl;
            return -1;
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
        lineNumber = std::stoi(line);
        std::vector<std::vector<int>> data;
        while (std::getline(file, line))
        {
            std::stringstream lineStream(line);
            std::string val;
            std::vector<int> row;
            while (std::getline(lineStream, val, ','))
            {
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
        host = host.append("/?q=uploadFromClient&index=");
        host = host.append(std::to_string(lineNumber));
        host = host.append("&result=");
        host = host.append(std::to_string(result));
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
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

void checkout(std::string host, int localID){
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        host = host.append("/?q=checkout&clientID=");
        host = host.append(std::to_string(localID));
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res == 0){ // we got a response TODO add response codes or something for each error case
            std::cout << "Succesfull checkout" << std::endl;
        }
        else
        {
            std::cout << "checkout failure";
        }
    }else{
        fprintf(stderr,"Could not init curl\n");
    }
    return;
}

int signup(std::string host){
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        host.append("/signup");
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res == 0){ // we got a response TODO add response codes or something for each error case
            if(readBuffer == "error - No available client spot"){
                return -1;
            }
            else{
                std::cout << "Succesfull signup" << std::endl;
                return stoi(readBuffer);
            }
        }
        else
        {
            std::cout << "signup failure " << std::endl;
        }
    }else{
        fprintf(stderr,"Could not init curl\n");
    }
    return -1;
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

    int clientID = signup(worker);
    int x = 0;
    while (clientID < 0 && x < 5) // signup failed
    {
        clientID = signup(worker);
        x++;
    }
    if (x == 5)
    {
        std::cout << "No viable connection was made, exit program" << std::endl;
        return -1;
    }
    
    

    if(getProblem(worker) == -1){
        checkout(worker, clientID);
        return -1;
    }
    else{
        int lineNumber;
        int result = computeProblem(lineNumber); //TODO do somethin with error (result == -1)
        sendResult(worker, result, lineNumber);
        checkout(worker, clientID);
    }

    

    return 0;
}

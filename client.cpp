//https://gist.github.com/alghanmi/c5d7b761b2c9ab199157 
#include <iostream>
#include <sstream>
#include <string>
#include <curl/curl.h>
#include <fstream>
#include <vector>

std::string line1;
std::string line2;

std::string dataPath = "/local/ddps2008/";

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

int getProblem(std::string host, int clientID, std::string ID){ //TODO
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        std::string tmp = "q=get_problem&clientID=";
        host.append("/?");
        tmp.append(std::to_string(clientID));
        host.append(tmp);
        char* data = &tmp[0];
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if(res == 0 && readBuffer[0] != 'X'){ // we got a response TODO add response codes or something for each error case
            std::ofstream out(dataPath+"tmp_data.csv" + ID); //TODO magic name
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

void heartbeat(std::string host, int clientID){ //TODO
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        std::string tmp = "q=clientHeartbeat&clientID=";
        host.append("/?");
        tmp.append(std::to_string(clientID));
        host.append(tmp);
        char* data = &tmp[0];
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if(res == 0 ){
            return; //TODO more implementation?? Don't think it is necessary
        }        
    }else{
        fprintf(stderr,"Could not init curl\n");
        return;
    }
    return;
}

int computeProblem(int &lineNumber, std::string ID){
    std::ifstream file(dataPath+"tmp_data.csv" + ID);
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

void sendResult(std::string host, int result, int lineNumber, int clientID){
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    curl = curl_easy_init();
    if(curl) {
        std::string tmp = "q=uploadFromClient&index=";
        host.append("/?");
        tmp.append(std::to_string(lineNumber));
        tmp.append("&result=");
        tmp.append(std::to_string(result));
        tmp.append("&clientID=");
        tmp.append(std::to_string(clientID));
        host.append(tmp);
        char* data = &tmp[0];
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

void checkout(std::string host, int localID){
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        std::string tmp = "q=checkout&clientID=";
        host.append("/?");
        tmp.append(std::to_string(localID));
        host.append(tmp);
        char* data = &tmp[0];
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_PORT, 9080);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
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
    if (argc < 3)
    {
        std::cout << "should be called as ./client [connectionpoint] [unique id]" << std::endl;
        return -1;
    }
    
    std::string host = argv[1];
    std::string ID = argv[2];

    int numProblems = 10;
    int numCheckins = 10;

    for(int i = 0; i < numCheckins; i++){
        std::string master = getMaster(host);
        if (master == "failure")
        {
            std::cout << "No master assigned" << std::endl;
            continue;
        }
        std::string worker = getWorker(master);
        if (worker == "")
        {
            std::cout << "empty worker assignment" << std::endl;
            continue;
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
            continue;
        }
        for (int j = 0; j < numProblems; j++){
            if(getProblem(worker, clientID, ID) == -1){
                checkout(worker, clientID);
                continue;
            }
            else{
                int lineNumber;
                int result = computeProblem(lineNumber, ID); //TODO do somethin with error (result == -1)
                sendResult(worker, result, lineNumber, clientID);
                numCheckins++; // keep going as long as there are problems to solve
            }
        }
        checkout(worker, clientID);
    }
    std::cout << "Client " << ID << " has finished all checkins and problems." << std::endl;

    return 0;
}

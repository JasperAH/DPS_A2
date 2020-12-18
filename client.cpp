//https://gist.github.com/alghanmi/c5d7b761b2c9ab199157 
#include <iostream>
#include <sstream>
#include <string>
#include <curl/curl.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>

std::string line1;
std::string line2;

bool stop_client = false;
bool stop_calc = false;
int stop_calc_counter = 0;

std::string dataPath = "/local/ddps2008/";

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string getMaster(std::string host){
    fprintf(stderr,"getting master\n");
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
        if(res == 7){
            stop_client = true;
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
        curl_easy_setopt(curl,CURLOPT_TIMEOUT,5);
        std::chrono::time_point<std::chrono::system_clock> getProblemTimer = std::chrono::system_clock::now();
        res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        if(res == 0 && readBuffer[0] != 'X'){ // we got a response TODO add response codes or something for each error case
            std::chrono::duration<double> diff = std::chrono::system_clock::now() - getProblemTimer;
            fprintf(stdout,"CID %d client getProblem in %f\n",clientID,diff.count());
            std::ofstream out(dataPath+"tmp_data.csv" + ID); //TODO magic name
            out << readBuffer;
            out.close();
            return 1;
        } else if(res == 7){
            stop_calc_counter ++;
            return -1;
        }
        else if(res == 503 || readBuffer[0] == 'X') //Pistache::Http::Code::Service_Unavailable
        {
            fprintf(stderr,"Data unavailable\n");
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
        lineNumber = (line == "") ? -1 : std::stoi(line);
        std::vector<std::vector<int>> data;
        while (std::getline(file, line))
        {
            std::stringstream lineStream(line);
            std::string val;
            std::vector<int> row;
            while (std::getline(lineStream, val, ','))
            {
                if(val != ""){ // protect against I/O failure during either network or drive write/read
                    try{
                        row.push_back(std::stoi(val));                        
                    }    
                    catch (const std::invalid_argument& e) {
                        return -1;
                    }
                    catch (const std::out_of_range& e) {
                        return -1;
                    }
                    catch (const std::exception& e)
                    {
                        return -1;
                    }
                }
            }
            data.push_back(row);
        }
        file.close();

        int result = 0;
        if(data.size() == 2){
            for (int i = 0; i < data[0].size(); i++)
            {
                result += data[0][i] * data[1][i];
            }
            return result;
        }else{
            fprintf(stderr,"tmp_data.csv is malformed\n");
            return -1;
        }
    }else{
        fprintf(stderr,"couldn't open tmp_data.csv\n");
    }
    
    return -1;
}

bool sendResult(std::string host, int result, int lineNumber, int clientID){
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
        curl_easy_setopt(curl,CURLOPT_TIMEOUT,5);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if(res == 0){ // we got a response
            fprintf(stderr,"result succesfully uploaded.\n");
            return true;
        }
    }else{
        fprintf(stderr,"Could not init curl\n");
        return false;
    }
    return false;
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
            fprintf(stderr,"Succesfull checkout\n");
            
        } else if(res == 7){
            stop_client = true;
        }
        else
        {
            fprintf(stderr,"checkout failure\n");
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
                try{
                    fprintf(stderr,"Succesfull signup %s\n",readBuffer.c_str());
                    if(readBuffer == "")
                        return -1;
                    else
                        return stoi(readBuffer);
                } catch(int e){
                    fprintf(stderr,"error: %d\n",e);
                    return -1;
                }
            }
        } else if(res == 7){
            stop_client = true;
        }
        else
        {
            fprintf(stderr,"signup failure \n");
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
    while(!stop_client){
    //for(int i = 0; i < numCheckins; i++){
        if(numCheckins <= 0){
            stop_client = true;
        }
        std::string master = getMaster(host);
        if (master == "failure")
        {
            fprintf(stderr,"No master assigned\n");
            numCheckins--;
            continue;
        }
        std::string worker = getWorker(master);
        if (worker == "")
        {
            fprintf(stderr,"empty worker assignment\n");
            numCheckins--;
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
            fprintf(stderr,"No viable connection was made, exit program\n");
            numCheckins--;
            continue;
        }
        numProblems = 10;
        //for (int j = 0; j < numProblems; j++){
        stop_calc = false;
        while(!stop_calc){
            if(getProblem(worker, clientID, ID) == -1){
                checkout(worker, clientID);
                numCheckins--;
            }
            else{
                int lineNumber;
                int result = computeProblem(lineNumber, ID); //TODO do somethin with error (result == -1)
                srand(clientID);
                while(!sendResult(worker, result, lineNumber, clientID)){
                    stop_calc_counter++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(rand()%1900 + 100)); // sleep for a random time
                    if(stop_calc_counter>100)
                        break;
                }
                //numProblems++; // keep going as long as there are problems to solve
                numCheckins = 10;
                stop_calc_counter = 0;
            }
            if(stop_calc_counter > 100)
                stop_calc = true;
        }
        checkout(worker, clientID);
    }
    fprintf(stderr,"Client %s has finished all checkins and problems.\n",ID.c_str());

    return 0;
}

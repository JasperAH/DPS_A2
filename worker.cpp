#include <iostream>
#include <string>
#include <pistache/endpoint.h> //https://github.com/pistacheio/pistache
#include <curl/curl.h>
#include <chrono>

#include <sstream>
#include <vector>
#include <fstream>
#include <mutex>
//edit this depending on device
const std::string dataPath = "/var/scratch/ddps2008/";
//const std::string dataPath = "/home/thomaswink/Documents/Studie/DDPS/DPS_A2/";
//const std::string dataPath = "/home/user/Documents/DPS/A2/";

bool stop_server = false;

int worker_id;
int master_id;
int n_workers;
char** hostnames;

int * workload;
std::chrono::time_point<std::chrono::system_clock> * worker_heartbeats;

bool dataAvailableInServer = true;
std::vector<std::pair<int, std::pair<int, std::string>>> localData; //clientID, line, data
std::vector<std::pair<int,int>> storedResults; //lineIndex, Result
static int MINIMUM_STORE = 100; //TODO find optimum

static const int MAXCLIENTS = 50; //TODO find optimum
bool clientActive[MAXCLIENTS];
int numLocalClientsServer;
int numLocalClients;
//Client heartbeat:
std::chrono::time_point<std::chrono::system_clock> clientHeartbeat[MAXCLIENTS];

// MASTER stuff
std::vector<std::vector<int>> inputData; // 2d matrix of inputdata
std::vector<std::pair<std::pair<int,bool>,std::pair<int,int>>> distributedData; // list of worker_id, has_returned_result, start_index, end_index (excl)
long long output = 0;
int curIndex = 0;
int getProblemDataSize = 2; //amount of data rows to return when /getproblemdata is called
bool rollback = false;
std::mutex output_lock;
std::chrono::time_point<std::chrono::system_clock> start_time;
std::chrono::time_point<std::chrono::system_clock> end_time;
bool started = false;

// WORKER stuff


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/******************************************Master functions*******************************/

std::string assignWorker(){
  for (int i = 0; i < n_workers; i++)
  {
    if (workload[i] < MAXCLIENTS)
    {
      return hostnames[i];
    }
  }
  return "";
}

/******************************************Worker functions*******************************/

std::string activateClient(){
  for (int i = 0; i < MAXCLIENTS; i++)
  {
    if (!clientActive[i])
    {
      clientActive[i] = true;
      numLocalClients++;
      clientHeartbeat[i] = std::chrono::system_clock::now();
      return std::to_string(i);
    }
  }
  return "error - No available client spot";
}

/**********************************************Handler**********************************/
struct HelloHandler : public Pistache::Http::Handler {
  HTTP_PROTOTYPE(HelloHandler);
  void onRequest(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter writer) override{
    if(request.resource().compare("/get_id") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok, std::to_string(worker_id)); // return own ID for master election
    }
    else if(request.resource().compare("/signup") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok, activateClient()); // return OK
    }
    else if(request.resource().compare("/get_master") == 0 && request.method() == Pistache::Http::Method::Get){
      if(!started){
        start_time = std::chrono::system_clock::now();
        started = true;
      } 
      writer.send(Pistache::Http::Code::Ok, hostnames[master_id]); // return hostname of master
    }
    else if(request.resource().compare("/get_worker") == 0 && request.method() == Pistache::Http::Method::Get){
      if(!started){
        start_time = std::chrono::system_clock::now();
        started = true;
      } 
      writer.send(Pistache::Http::Code::Ok, assignWorker()); // return hostname of assigned worker
    }
    else if(request.resource().compare("/stop") == 0 && request.method() == Pistache::Http::Method::Get){
      stop_server = true;
      writer.send(Pistache::Http::Code::Ok,"stop"); // return OK
    }
    else {

      //if(request.resource().compare("/heartbeat") == 0 && request.method() == Pistache::Http::Method::Get){
      // usage: <host>:<port>/?q=heartbeat\&workerID=<workerID>
      if(master_id == worker_id && request.method() == Pistache::Http::Method::Post && request.resource().compare("/?q=")){
        if (request.query().get("q").get() == "heartbeat"){
          int worker = atoi(request.query().get("workerID").get().c_str());
          worker_heartbeats[worker] = std::chrono::system_clock::now();
          writer.send(Pistache::Http::Code::Ok); // return OK
        }
      }

      // usage: <host>:<port>/?q=get_problem\&clientID=<clientID>
      if(request.method() == Pistache::Http::Method::Post && request.resource().compare("/?q=")){
        if (request.query().get("q").get() == "get_problem"){
          int client = atoi(request.query().get("clientID").get().c_str());
          clientHeartbeat[client] = std::chrono::system_clock::now();
          Pistache::Http::ResponseStream rStream = writer.stream(Pistache::Http::Code::Ok);
          std::string data;
          std::string lineNumber;
          int found = -1;
          for (int i = 0; i < localData.size(); i++)
          {
            if (localData[i].first == -1)
            {
              data = localData[i].second.second;
              lineNumber = std::to_string (localData[i].second.first);
              found = i;
              break;
            }
          }
          if(found == -1 && localData.size()>0){ // resend data from potential stragglers
            data = localData[0].second.second;
            lineNumber = std::to_string(localData[0].second.first);
            found = 0;
          }
          if (found == -1)
          {
            std::string tmp("X");
            rStream << tmp.c_str() << "\n";
            //rStream << std::to_string(Pistache::Http::Code::Service_Unavailable).c_str();
            rStream << Pistache::Http::ends;
            /* TODO send error that data was not available ofzo, mss meer data ophalen?, mss niet ivm blocking */
          }
          else
          {
            rStream << lineNumber.c_str() << "\n";
            rStream << Pistache::Http::flush;
            rStream << localData[found].second.second.c_str() << "\n";
            rStream << Pistache::Http::flush;
            rStream << localData[found+1].second.second.c_str() << "\n";
            rStream << Pistache::Http::flush;
            localData[found].first = client;
            localData[found+1].first = client;
            rStream << Pistache::Http::ends; // also flushes and ends the stream
            writer.send(Pistache::Http::Code::Ok);
          }
        }
      }
      // usage: <host>:<port>/?q=uploadFromClient\&index=<vector_index>\&result=<result>\&clientID=<clientID>
      if(request.method() == Pistache::Http::Method::Post && request.resource().compare("/?q=")){ //TODO deassign worker in master so more clients can be assigned
        if (request.query().get("q").get() == "uploadFromClient"){
          int res = atoi(request.query().get("result").get().c_str());
          int ind = atoi(request.query().get("index").get().c_str());
          int client = atoi(request.query().get("clientID").get().c_str());
          clientHeartbeat[client] = std::chrono::system_clock::now();
          storedResults.push_back({ind, res});
          writer.send(Pistache::Http::Code::Ok); // return OK
        }
      }
      // usage: <host>:<port>/?q=clientHeartbeat\&clientID=<clientID>
      if(request.method() == Pistache::Http::Method::Post && request.resource().compare("/?q=")){ //TODO deassign worker in master so more clients can be assigned
        if (request.query().get("q").get() == "clientHeartbeat"){
          int client = atoi(request.query().get("clientID").get().c_str());
          clientHeartbeat[client] = std::chrono::system_clock::now();
          writer.send(Pistache::Http::Code::Ok); // return OK
        }
      }

      // usage: <host>:<port>/?q=result\&index=<vector_index>\&result=<result>
      if(master_id == worker_id && request.resource().compare("/?q=") && request.method() == Pistache::Http::Method::Post){
        if (request.query().get("q").get() == "result")
        {
          int res = atoi(request.query().get("result").get().c_str());
          int ind = atoi(request.query().get("index").get().c_str());
          if(distributedData.at(ind).first.second == false){
            output_lock.lock();
            output += res;
            output_lock.unlock();
            distributedData.at(ind).first.second = true;
            distributedData.at(ind+1).first.second = true;
          }
          writer.send(Pistache::Http::Code::Ok); // return OK
        }
      }

      // call using <host>:<port>/?q=numClientsChange\&workerID=<worker_id>\&numLocalClients=<numLocalClients>
      if(master_id == worker_id && request.method() == Pistache::Http::Method::Post && request.resource().compare("/?q=")){
        if (request.query().get("q").get() == "numClientsChange"){
          int wID = atoi(request.query().get("workerID").get().c_str());
          int numLocal = atoi(request.query().get("numLocalClients").get().c_str());
          workload[wID] = numLocal;
          writer.send(Pistache::Http::Code::Ok); // return OK
        }
      }

      // call using <host>:<port>/?q=checkout\&clientID=<client_id>
      if(request.method() == Pistache::Http::Method::Post && request.resource().compare("/?q=")){
        if (request.query().get("q").get() == "checkout"){
          numLocalClients--;
          int cID = atoi(request.query().get("clientID").get().c_str());
          clientActive[cID] = false;
          for (int x = localData.size()-1; x >= 0 ; x--)
          {
            if (localData[x].first == cID)
            {
              localData[x].first == -1;
            }
            
          }
          writer.send(Pistache::Http::Code::Ok); // return OK
        }
      }
      // call using <host>:<port>/?q=getproblemdata\&workerID=<worker_id>\&datasize=<datasize>
      if(master_id == worker_id && request.method() == Pistache::Http::Method::Post && request.resource().compare("/?q=")){
        if (request.query().get("q").get() == "getproblemdata"){
          int wID = atoi(request.query().get("workerID").get().c_str());
          int datasize = atoi(request.query().get("datasize").get().c_str());
        //}
        //if(master_id == worker_id && request.resource().compare("/getproblemdata") == 0 && request.method() == Pistache::Http::Method::Get){
          Pistache::Http::ResponseStream rStream = writer.stream(Pistache::Http::Code::Ok); // open datastream for response
          // first send index in vector:
          // return data as CSV, amount is controlled by getProblemDataSize
          int maxIndex = ((curIndex + datasize) > inputData.at(0).size()) ? inputData.at(0).size() : (curIndex + datasize);
          if (maxIndex == curIndex)
          {
            bool foundData = false;
            int foundIndex = -1;
            for(int i = 0; i < distributedData.size(); ++i){
              if(distributedData.at(i).first.first == -1){ //worker died
                foundData = true;
                foundIndex = i;
                break;
              }
            }
            if(foundData){
              distributedData.at(foundIndex).first.first = wID;

              rStream << std::to_string(foundIndex).c_str() << "\n";
              rStream << Pistache::Http::flush;

              for(int i = distributedData.at(foundIndex).second.first; i < distributedData.at(foundIndex).second.second; ++i){
                std::string line = "";
                for(int j = 0; j < inputData.size(); ++j){
                  line += std::to_string(inputData.at(j).at(i));
                  if(j < (inputData.size()-1)) line += ",";
                }
                rStream << line.c_str() << "\n";
                rStream << Pistache::Http::flush; //optional? should ensure sending data more often
                writer.send(Pistache::Http::Code::Ok);      
              }   
              rStream << Pistache::Http::ends;
            }else{
              /*
              rStream << std::to_string(-10).c_str() << "\n"; //Means no more data available in server to distribute
              rStream << Pistache::Http::flush;
              rStream << Pistache::Http::ends;
              writer.send(Pistache::Http::Code::Ok); // return OK, default behaviour
              */
              //find data with not-yet returned results:
              bool findOldData=false;
              int dataIndex = -1;
              for(int i = 0; i < distributedData.size(); ++i){
                if(distributedData.at(i).first.second == false){
                  findOldData = true;
                  dataIndex = i;
                }
              }
              if(findOldData){
                rStream << std::to_string(dataIndex).c_str() << "\n";
                rStream << Pistache::Http::flush;

                for(int i = distributedData.at(dataIndex).second.first; i < distributedData.at(dataIndex).second.second; ++i){
                  std::string line = "";
                  for(int j = 0; j < inputData.size(); ++j){
                    line += std::to_string(inputData.at(j).at(i));
                    if(j < (inputData.size()-1)) line += ",";
                  }
                  rStream << line.c_str() << "\n";
                  rStream << Pistache::Http::flush; //optional? should ensure sending data more often
                  writer.send(Pistache::Http::Code::Ok);      
                }   

                rStream << Pistache::Http::ends;
              }else{ // we should be done now
                rStream << std::to_string(-10).c_str() << "\n"; //Means no more data available in server to distribute
                rStream << Pistache::Http::flush;
                rStream << Pistache::Http::ends;
                writer.send(Pistache::Http::Code::Ok); // return OK, default behaviour
              }
            }
          }
          else{
            rStream << std::to_string(distributedData.size()).c_str() << "\n";
            rStream << Pistache::Http::flush;
            for(int i = curIndex; i < maxIndex; ++i){
              std::string line = "";
              for(int j = 0; j < inputData.size(); ++j){
                line += std::to_string(inputData.at(j).at(i));
                if(j < (inputData.size()-1)) line += ",";
              }
              rStream << line.c_str() << "\n";
              rStream << Pistache::Http::flush; //optional? should ensure sending data more often
              distributedData.push_back({{wID,false},{i,i+(getProblemDataSize-1)}});
              
            }     

            // TODO vervang 0 met worker_id en maak deze if een POST met request.query()
            curIndex += maxIndex - curIndex;

            rStream << Pistache::Http::ends; // also flushes and ends the stream
            writer.send(Pistache::Http::Code::Ok); // return OK, default behaviour
          }
        }
      }
    }
    
  }
};

bool sendHeartBeat(){
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl = curl_easy_init();
  if(curl) {
      std::string host(hostnames[master_id]);
      std::string tmp = "q=heartbeat&workerID=";
      host.append("/?");
      tmp.append(std::to_string(worker_id));
      host.append(tmp);
      char* data = &tmp[0];
      curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
      curl_easy_setopt(curl, CURLOPT_PORT, 9080);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
      res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);

      return res == 0; // CURLE_OK (0) All fine. Proceed as usual.
  }else{
    fprintf(stderr,"Could not init curl\n");
    return false;
  }
}

void checkpoint_data(){
  fprintf(stderr,"snapshotting current progress\n");
  std::ofstream csv_file(dataPath+"snapshot.csv");
  csv_file << output << "\n"; // snapshot partial result

  for(int i = 0; i < inputData.at(0).size(); ++i){
    for(int col = 0; col < inputData.size(); ++col){
      csv_file << inputData.at(col).at(i);
      if(col < (inputData.size() -1)){
        csv_file << ",";
      }
    }
    csv_file << "\n";
  }
  csv_file.close();
  if(distributedData.size()>0){
    csv_file.open(dataPath+"snapshot_distributed.csv");
    for(int i = 0; i < distributedData.size(); ++i){
      csv_file << std::to_string(distributedData.at(i).first.first) << ",";
      csv_file << std::to_string(distributedData.at(i).first.second) << ",";
      csv_file << std::to_string(distributedData.at(i).second.first) << ",";
      csv_file << std::to_string(distributedData.at(i).second.second) << "\n";
    }
    csv_file.close();
  }
}

void read_input_data(){
  // open file, checking for a snapshot first
  std::ifstream csv_file(dataPath+"snapshot.csv");
  if(!csv_file.good()){
    fprintf(stderr,"no snapshot found, opening data\n");
    csv_file.close();
    csv_file.open(dataPath+"data.csv");
  } else{
    fprintf(stderr,"reading snapshot data from file\n");
    rollback = true;
  }

  bool initialized = false;
  std::string line;

  if(csv_file.is_open()){
    std::getline(csv_file,line);
    output = atoll(line.c_str()); // first line in the file is the snapshotted result
    while(std::getline(csv_file,line)){
        int column = 0;
        std::string value;
        std::stringstream ss2(line);

        while(std::getline(ss2,value,',')){
          if(!initialized) inputData.push_back(std::vector<int> {});
          inputData.at(column).push_back(atoi(value.c_str()));
          column++;
        }
        initialized = true;
    }
  } else {
    fprintf(stderr,"couldn't open file to read\n");
  }
  csv_file.close();
  
  if(rollback){
    fprintf(stderr,"reading distributed snapshot data from file\n");
    csv_file.open(dataPath+"snapshot_distributed.csv");
    if(csv_file.is_open()){
      while(std::getline(csv_file,line)){
        int column = 0;
        std::string value;
        std::stringstream ss2(line);

        int wid, start_ind, end_ind;
        bool has_returned;
        while(std::getline(ss2,value,',')){
          if(column == 0) wid = atoi(value.c_str());
          else if(column == 1) has_returned = (atoi(value.c_str()) == 1);
          else if(column == 2) start_ind = atoi(value.c_str());
          else if(column == 3) end_ind = atoi(value.c_str());
          column++;
        }
        distributedData.push_back({{wid,has_returned},{start_ind,end_ind}});
      }
      if(distributedData.size() > 0){
        curIndex = distributedData.at(distributedData.size()-1).second.second;
      } else{
        curIndex = 0;
      }
    }
    csv_file.close();
  }
}

void elect_master(){
  fprintf(stderr,"Electing new master\n");

  int lowest_id = worker_id;
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  for(int i = 0; i < n_workers; ++i){
    if(i != worker_id){
      curl = curl_easy_init();
      if(curl) {
          std::string host(hostnames[i]);
          host.append("/get_id");
          curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
          curl_easy_setopt(curl, CURLOPT_PORT, 9080);
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
          res = curl_easy_perform(curl);
          curl_easy_cleanup(curl);
          
          if(res == 0){ // we got a response
            int other_id = atoi(readBuffer.c_str());
            if(other_id < lowest_id)
              lowest_id = other_id;
          }
      }else{
        fprintf(stderr,"Could not init curl\n");
      }
    }
  }
  fprintf(stderr,"new master_id: %d\n",lowest_id);
  master_id = lowest_id;
  if(worker_id == master_id)
    read_input_data();
}

void ask_data(){ // call using <host>:<port>/?q=getproblemdata\&workerID=<worker_id>\&datasize=<datasize>
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl = curl_easy_init();
  if(curl) {
      std::string host(hostnames[master_id]);
      std::string tmp = "q=getproblemdata&workerID=";
      host.append("/?");
      tmp.append(std::to_string(worker_id));
      tmp.append("&datasize=");
      tmp.append(std::to_string((MINIMUM_STORE - localData.size()) + 4)); //TODO 4 is arbitrary and magic, find optimum
      host.append(tmp);
      char* data = &tmp[0];
      curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
      curl_easy_setopt(curl, CURLOPT_PORT, 9080);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
      res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);
      
      if(res == 0){ // we got a response
        std::stringstream lineStream(readBuffer);
        std::string row;
        std::vector<std::string> lines;
        std::getline(lineStream, row, '\n');
        if(stoi(row) == -10){
          //TODO do something that means that there is no more data in server to distribute
          dataAvailableInServer = false;
          return;
        }
        int lineIndex = std::stoi(row);
        while (std::getline(lineStream, row, '\n'))
        {
            localData.push_back({-1,{lineIndex,row}});
            lineIndex++;
        }
        
      }
  }else{
    fprintf(stderr,"Could not init curl\n");
  }
  return;
}

int send_result(int i){//TODO better variable name
  CURL *curl;
  CURLcode res;
  std::string readBuffer;
  curl = curl_easy_init();
  if(curl) {
      std::string host(hostnames[master_id]);
      std::string tmp = "q=result&index=";
      host.append("/?");
      tmp.append(std::to_string(storedResults[i].first));
      tmp.append("&result=");
      tmp.append(std::to_string(storedResults[i].second));
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
        int lineNumber = storedResults[i].first;
        storedResults.erase( storedResults.begin() + i );
        return lineNumber;
      }
  }else{
    fprintf(stderr,"Could not init curl\n");
  }
  return -1;

}

void updateNumClients(){ // call using <host>:<port>/?q=numClientsChange\&workerID=<worker_id>\&numLocalClients=<numLocalClients>
  CURL *curl;
  CURLcode res;
  std::string readBuffer;
  curl = curl_easy_init();
  if(curl) {
      int freezeValue = numLocalClients;
      std::string host(hostnames[master_id]);
      std::string tmp = "q=numClientsChange&workerID=";
      host.append("/?");
      tmp.append(std::to_string(worker_id));
      tmp.append("&numLocalClients=");
      tmp.append(std::to_string(freezeValue));
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
        numLocalClientsServer = freezeValue;
        fprintf(stderr,"number of clients updated in master node\n");
        return ;
      }
  }else{
    fprintf(stderr,"Could not init curl\n");
  }
  fprintf(stderr, "client update to master failed\n");
  return;
}


void stop_servers(){
  fprintf(stderr,"Stopping other servers\n");

  int lowest_id = worker_id;
  CURL *curl;
  CURLcode res;
  std::string readBuffer;
  for(int i = 0; i < n_workers; ++i){
    if(i != worker_id){
      curl = curl_easy_init();
      if(curl) {
          std::string host(hostnames[i]);
          host.append("/stop");
          curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
          curl_easy_setopt(curl, CURLOPT_PORT, 9080);
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
          res = curl_easy_perform(curl);
          curl_easy_cleanup(curl);
      }else{
        fprintf(stderr,"Could not init curl to stop servers\n");
      }
    }
  }

}

int main(int argc, char **argv) {
  // parse input
  if (argc < 3){
      fprintf(stderr, "usage: %s <worker_id> <worker_hostnames>\n" 
      "The worker's own hostname is at index worker_id in worker_hostnames.\n", argv[0]);
      return -1;
  }

  worker_id = atoi(argv[1]);
  n_workers = argc - 2;
  hostnames = (char**) malloc(sizeof(char*)*(argc-2));
  workload = (int*) malloc(sizeof(int)*(argc-2));
  worker_heartbeats = (std::chrono::time_point<std::chrono::system_clock>*) malloc(sizeof(std::chrono::time_point<std::chrono::system_clock>)*(argc-2));
  for(int i = 0; i < n_workers; ++i){
    hostnames[i] = argv[i+2];
    workload[i] = 0;
    worker_heartbeats[i] = std::chrono::system_clock::now();
  }
  for (int i = 0; i < MAXCLIENTS; i++)
  {
    clientActive[i] = false;
  }
  numLocalClientsServer = 0;
  numLocalClients = 0;

  // INIT pistache
  Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(9080));

  auto opts = Pistache::Http::Endpoint::options().threads(8);
  Pistache::Http::Endpoint server(addr);
  server.init(opts);
  server.setHandler(std::make_shared<HelloHandler>());
  server.serveThreaded();

  fprintf(stderr,"Server running at %s:%s\n",addr.host().c_str(),addr.port().toString().c_str());

  // INIT master
  elect_master();
  if(master_id == worker_id) checkpoint_data();


  // INIT clocks & heartbeat
  std::chrono::time_point<std::chrono::system_clock> heartbeat_time = std::chrono::system_clock::now();
  std::chrono::time_point<std::chrono::system_clock> snapshot_time = std::chrono::system_clock::now();
  std::chrono::time_point<std::chrono::system_clock> numclients_time = std::chrono::system_clock::now();
  std::chrono::time_point<std::chrono::system_clock> client_heartbeat_time = std::chrono::system_clock::now();
  std::chrono::duration<double> diff;
  double heartbeat_interval = 5.0; // heartbeat once every 5 sec
  double master_interval = 10.0; // check every 10 sec if every worker did heartbeat
  double snapshot_interval = 300.0; // snapshot every 5 min
  double numclients_interval = 5.0;
  double client_heartbeat_interval = 3.0;
  int master_down_counter = 0;
  double clientTimeout = 60.0;

  while(!stop_server){
    // HEARTBEAT
    diff = std::chrono::system_clock::now() - heartbeat_time;
    if((master_id != worker_id) && (diff.count() > heartbeat_interval)){ // every 5 seconds, heartbeat
      if(!sendHeartBeat()){ // master is down, check every second
        master_down_counter++;
        heartbeat_interval = 1.0;
      } else if(master_down_counter > 0){ // if master recovers, go back to 5 seconds heartbeat
        master_down_counter = 0;
        heartbeat_interval = 5.0;
      }
      if(master_down_counter >= 5){ // if master stays down, elect new master
        elect_master();
        master_down_counter = 0;
        heartbeat_interval = 5.0;
      }
      heartbeat_time = std::chrono::system_clock::now();
    }
    if ((master_id == worker_id) && n_workers > 1 && (diff.count() > master_interval))
    {
      for (int i = 0; i < n_workers; i++)
      {
        std::chrono::duration<double> timediff = std::chrono::system_clock::now() - worker_heartbeats[i];
        if (i != master_id && timediff.count() > master_interval)
        {
          fprintf(stderr,"worker node %s appears to be down, making data available for redistribution: \n", std::to_string(i).c_str());
          for(int j = 0; j < distributedData.size(); ++j){
            if(distributedData.at(j).first.first == i && distributedData.at(j).first.second == false){
              distributedData.at(j).first.first = -1;
            }
          }
          //TODO workernode is down
          /*
          {
            contact node and check if alive?
            what if node restarts before this check is done, some data is never delivered as
          }
          */
        }
      }
      heartbeat_time = std::chrono::system_clock::now();
    }
    


    if (localData.size() < MINIMUM_STORE){// && dataAvailableInServer){
      ask_data();
    }

    if(storedResults.size()>0){
      std::vector<int> deletedLines;
      for (int i = storedResults.size()-1; i >= 0; i--)//TODO This is needed as long as the POST result only takes one result at a time
      {
        deletedLines.push_back(send_result(i));
      }
      for (int x = deletedLines.size() - 1; x >= 0 ; x--)
      {
        for (int y = localData.size() - 1; y >= 0 ; y--) //TODO does not scale well
        {
          if(localData[y].second.first == deletedLines[x]){
            localData.erase(localData.begin() + y + 1 );
            localData.erase(localData.begin() + y );
            deletedLines.erase(deletedLines.begin() + x);
          }
        } 
      }
    }
    
    diff = std::chrono::system_clock::now() - numclients_time;
    if (numLocalClientsServer != numLocalClients && diff.count() > numclients_interval)
    {
      updateNumClients();
      numclients_time = std::chrono::system_clock::now();
    }
    

    //Client heartbeat, only when number of clients gets to high
    diff = std::chrono::system_clock::now() - client_heartbeat_time;
    if(diff.count() > client_heartbeat_interval){ // numLocalClients > (int)(0.8 * numLocalClients)){ //TODO magic number
      for (int i = 0; i < numLocalClients; i++)
      {
        fprintf(stderr,"checking heartbeat of client %d, active: %d\n",i,clientActive[i]);
        if (clientActive[i])
        {
          diff = std::chrono::system_clock::now() - clientHeartbeat[i];
          fprintf(stderr,"time since last heartbeat of client %d: %f\n",i,diff.count());
          if (diff.count() > clientTimeout)
          {
            fprintf(stderr,"deactivating client %d\n",i);
            numLocalClients--;
            clientActive[i] = false;
            for (int x = localData.size()-1; x >= 0 ; x--)
            {
              if (localData[x].first == i)
              {
                localData[x].first == -1;
              }
              
            }
            fprintf(stderr,"client %d deactivated\n",i);
          }
        }
      }
      client_heartbeat_time = std::chrono::system_clock::now();
    }



    // SNAPSHOT periodically
    diff = std::chrono::system_clock::now() - snapshot_time;
    if(master_id == worker_id && (diff.count() > snapshot_interval)){
      checkpoint_data();
      snapshot_time = std::chrono::system_clock::now();
    }

    // check if we're done when all data has been distributed
    if(!stop_server && master_id == worker_id && distributedData.size() > 0 && distributedData.at(distributedData.size()-1).second.second >= inputData.at(0).size()){
      bool done = true;
      for(int i = 0; i < distributedData.size(); ++i){
        if(distributedData.at(i).first.second == false)
          done = false;
      }

      stop_server = done;
    }
    //fprintf(stderr,"%d",stop_server);
  }
  fprintf(stderr,"stopping server\n");
  if(master_id == worker_id){
    end_time = std::chrono::system_clock::now();
    diff = end_time - start_time;
    checkpoint_data();
    fprintf(stdout, "result: %lld\n",output);
    fprintf(stdout,"time: %f\n",diff.count());
    stop_servers();
  }
  free(hostnames);
}
#include <iostream>
#include <string>
#include <pistache/endpoint.h> //https://github.com/pistacheio/pistache
#include <curl/curl.h>
#include <chrono>

#include <sstream>
#include <vector>
#include <fstream>

//edit this depending on device
const std::string dataPath = "/home/user/Documents/DPS/A2/";

bool stop_server = false;

int worker_id;
int master_id;
int n_workers;
char** hostnames;

// MASTER stuff
std::vector<std::vector<int>> inputData; // 2d matrix of inputdata
std::vector<std::pair<std::pair<int,bool>,std::pair<int,int>>> distributedData; // list of worker_id, has_returned_result, start_index, end_index (excl)
int output = 0;
int curIndex = 0;
int getProblemDataSize = 2; //amount of data rows to return when /getproblemdata is called
bool rollback = false;

// WORKER stuff
// stores MASTER client_id, result, bHas_returned_result, vector_index, string:<data>
std::vector<std::pair<int,std::pair<std::pair<int,bool>,std::pair<int,std::string>>>> clientData;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// sends every item in clientData with has_returned_result == true to master,
// master checks whether the result has been added or not
bool sendResultsToMaster(){
  for(int i = 0; i < clientData.size(); ++i){
    if(clientData.at(i).second.first.second){ // has_returned_result == true
      CURL *curl;
      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
          std::string host(hostnames[master_id]);
          host.append("/?q=mResult");
          host.append("&index="+std::to_string(clientData.at(i).second.second.first));
          host.append("&result="+std::to_string(clientData.at(i).second.first.first));
          curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
          curl_easy_setopt(curl, CURLOPT_PORT, 9080);
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
          res = curl_easy_perform(curl);
          curl_easy_cleanup(curl);
      }else{
        fprintf(stderr,"Could not init curl\n");
        return false;
      }
      if(res != 0){
        fprintf(stderr,"curl sending results to master failed\n");
        return false;
      }
    } 
  }
  return true;
}

std::string getProblemDataFromMaster(int clientID){
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl = curl_easy_init();
  if(curl) {
      std::string host(hostnames[master_id]);
      host.append("/?q=getproblemdata");
      host.append("&workerID="+std::to_string(worker_id));
      curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
      curl_easy_setopt(curl, CURLOPT_PORT, 9080);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
      res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);
  }else{
    fprintf(stderr,"Could not init curl\n");
    return "";
  }
  if(res == 0){
    std::stringstream ss(readBuffer);
    std::string line;
    std::getline(ss,line);
    int vIndex = atoi(line.c_str());
    std::string sData;
    while(std::getline(ss,line))
      sData.append(line+"\n");
    clientData.push_back({clientID,{{0,false},{vIndex,sData}}});
    return sData;
  }else{
    fprintf(stderr,"curl getting problem data from master failed\n");
    return "";
  }
}

struct HelloHandler : public Pistache::Http::Handler {
  HTTP_PROTOTYPE(HelloHandler);
  void onRequest(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter writer) override{
    if(request.query().get("q").get() == "result" && request.method() == Pistache::Http::Method::Get){
      bool sendRes = sendResultsToMaster();
      if(sendRes){
        writer.send(Pistache::Http::Code::Ok); // return OK
      }else{
        writer.send(Pistache::Http::Code::Internal_Server_Error);
      }
    }
    if(request.query().get("q").get() == "getproblem" && request.method() == Pistache::Http::Method::Get){
      int clientID = atoi(request.query().get("clientID").get().c_str());
      std::string r = getProblemDataFromMaster(clientID);
      Pistache::Http::ResponseStream rStream = writer.stream(Pistache::Http::Code::Ok); // open datastream for response
      rStream << r.c_str();
      rStream << Pistache::Http::ends;
    }
    if(request.resource().compare("/get_id") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok, std::to_string(worker_id)); // return own ID for master election
    }
    if(request.resource().compare("/heartbeat") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok); // return OK
    }
    if(request.resource().compare("/stop") == 0 && request.method() == Pistache::Http::Method::Get){
      stop_server = true;
      writer.send(Pistache::Http::Code::Ok); // return OK
    }
    // usage: <host>:<port>/?q=result\&index=<vector_index>\&result=<result>
    if(master_id == worker_id && request.query().get("q").get() == "mResult" && request.method() == Pistache::Http::Method::Get){
      int res = atoi(request.query().get("result").get().c_str());
      int ind = atoi(request.query().get("index").get().c_str());
      if(distributedData.at(ind).first.second == false){
        output += res;
        distributedData.at(ind).first.second = true;
      }
      writer.send(Pistache::Http::Code::Ok); // return OK
    }
    // call using <host>:<port>/?q=getproblemdata\&workerID=<worker_id>
    // returns vector_id in the first line
    // other lines are data in csv format
    if(master_id == worker_id && request.query().get("q").get() == "getproblemdata" && request.method() == Pistache::Http::Method::Get){
      int wID = atoi(request.query().get("workerID").get().c_str());
    //}
    //if(master_id == worker_id && request.resource().compare("/getproblemdata") == 0 && request.method() == Pistache::Http::Method::Get){
      Pistache::Http::ResponseStream rStream = writer.stream(Pistache::Http::Code::Ok); // open datastream for response
      // first send index in vector:
      rStream << std::to_string(distributedData.size()).c_str() << "\n";
      rStream << Pistache::Http::flush;
      // return data as CSV, amount is controlled by getProblemDataSize
      int maxIndex = ((curIndex + getProblemDataSize) > inputData.at(0).size()) ? inputData.at(0).size() : (curIndex + getProblemDataSize);
      for(int i = curIndex; i < maxIndex; ++i){
        std::string line = "";
        for(int j = 0; j < inputData.size(); ++j){
          line += std::to_string(inputData.at(j).at(i));
          if(j < (inputData.size()-1)) line += ",";
        }
        rStream << line.c_str() << "\n";
        rStream << Pistache::Http::flush; //optional? should ensure sending data more often
      }     

      // TODO vervang 0 met worker_id en maak deze if een POST met request.query()
      distributedData.push_back({{wID,false},{curIndex,curIndex+getProblemDataSize}});
      curIndex += getProblemDataSize;

      rStream << Pistache::Http::ends; // also flushes and ends the stream
    }
    writer.send(Pistache::Http::Code::Ok); // return OK, default behaviour
  }
};

bool sendHeartBeat(){
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl = curl_easy_init();
  if(curl) {
      std::string host(hostnames[master_id]);
      host.append("/heartbeat");
      curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
      curl_easy_setopt(curl, CURLOPT_PORT, 9080);
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

  csv_file.open(dataPath+"snapshot_distributed.csv");
  for(int i = 0; i < distributedData.size(); ++i){
    csv_file << std::to_string(distributedData.at(i).first.first) << ",";
    csv_file << std::to_string(distributedData.at(i).first.second) << ",";
    csv_file << std::to_string(distributedData.at(i).second.first) << ",";
    csv_file << std::to_string(distributedData.at(i).second.second) << "\n";
  }
  csv_file.close();
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
    output = atoi(line.c_str()); // first line in the file is the snapshotted result
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
  }
  csv_file.close();
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
  for(int i = 0; i < n_workers; ++i){
    hostnames[i] = argv[i+2];
  }

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
  std::chrono::duration<double> diff;
  double heartbeat_interval = 5.0; // heartbeat once every 5 sec
  double snapshot_interval = 300.0; // snapshot every 5 min
  int master_down_counter = 0;

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
  }
  fprintf(stderr,"stopping server\n");
  if(master_id == worker_id){
    checkpoint_data();
    fprintf(stdout, "result: %d\n",output);
    stop_servers();
  }
  free(hostnames);
}
#include <iostream>
#include <string>
#include <pistache/endpoint.h> //https://github.com/pistacheio/pistache
#include <curl/curl.h>
#include <chrono>

#include <sstream>
#include <vector>
#include <fstream>

//edit this depending on device
const std::string dataPath = "/home/thomaswink/Documents/Studie/DDPS/DPS_A2/";

bool stop_server = false;

int worker_id;
int master_id;
int n_workers;
char** hostnames;

int * workload;

std::vector<std::pair<int, std::pair<int, std::string>>> localData; //clientID, line, data
std::vector<std::pair<int,int>> storedResults; //lineIndex, Result
static int MINIMUM_STORE = 10; //TODO find optimum

static int MAXCLIENTS = 5; //TODO find optimum

// MASTER stuff
std::vector<std::vector<int>> inputData; // 2d matrix of inputdata
std::vector<std::pair<std::pair<int,bool>,std::pair<int,int>>> distributedData; // list of worker_id, has_returned_result, start_index, end_index (excl)
int output = 0;
int curIndex = 0;
int getProblemDataSize = 2; //amount of data rows to return when /getproblemdata is called
bool rollback = false;


// WORKER stuff


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/******************************************Master functions*******************************/

std::string assignWorker(){
  //throw "getProblemData is not implemented yet.";
  for (int i = 0; i < n_workers; i++)
  {
    if (workload[i] < MAXCLIENTS)
    {
      return hostnames[i];
    }
  }
  return "";
}

/**************************************Client serving  functions**************************/
void getProblemData(void *ptr, int & size){ //TODO
    throw "getProblemData is not implemented yet.";
    return;
}

void postResultInternally(){ //TODO
    throw "postResultInternally is not implemented yet.";
    return;
}

/**********************************************Handler**********************************/
struct HelloHandler : public Pistache::Http::Handler {
  HTTP_PROTOTYPE(HelloHandler);
  void onRequest(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter writer) override{
    if(request.resource().compare("/get_id") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok, std::to_string(worker_id)); // return own ID for master election
    }
    if(request.resource().compare("/heartbeat") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok); // return OK
    }
    if(request.resource().compare("/get_master") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok, hostnames[master_id]); // return hostname of master
    }
    if(request.resource().compare("/get_worker") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok, assignWorker()); // return hostname of assigned worker
    }
    if(request.resource().compare("/get_problem") == 0 && request.method() == Pistache::Http::Method::Get){
      Pistache::Http::ResponseStream rStream = writer.stream(Pistache::Http::Code::Ok);
      std::string data;
      std::string line;
      int found = -1;
      for (int i = 0; i < localData.size(); i++)
      {
        if (localData[i].first == -1)
        {
          data = localData[i].second.second;
          line = localData[i].second.first;
          found = true;
          break;
        }
      }
      if (found == -1)
      {
        /* TODO send error that data was not available ofzo, mss meer data ophalen?, mss niet ivm blocking */
      }
      else
      {
        rStream << line.c_str() << "\n";
        rStream << Pistache::Http::flush;
        rStream << localData[found].second.second.c_str() << "\n";
        rStream << Pistache::Http::flush;
        rStream << localData[found+1].second.second.c_str() << "\n";
        rStream << Pistache::Http::flush;
        localData[found].first = 1;
        localData[found+1].first = 1;
        rStream << Pistache::Http::ends; // also flushes and ends the stream
        writer.send(Pistache::Http::Code::Ok);
      }
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
    // usage: <host>:<port>/?q=resultClient\&index=<vector_index>\&result=<result>
    if(request.query().get("q").get() == "resultClient" && request.method() == Pistache::Http::Method::Post){
      int res = atoi(request.query().get("result").get().c_str());
      int ind = atoi(request.query().get("index").get().c_str());
      storedResults.push_back({ind, res});
      writer.send(Pistache::Http::Code::Ok); // return OK
    }
    // call using <host>:<port>/?q=getproblemdata\&workerID=<worker_id>\&datasize=<datasize>
    if(master_id == worker_id && request.query().get("q").get() == "getproblemdata" && request.method() == Pistache::Http::Method::Post){
      int wID = atoi(request.query().get("workerID").get().c_str());
      int datasize = atoi(request.query().get("datasize").get().c_str());
    //}
    //if(master_id == worker_id && request.resource().compare("/getproblemdata") == 0 && request.method() == Pistache::Http::Method::Get){
      Pistache::Http::ResponseStream rStream = writer.stream(Pistache::Http::Code::Ok); // open datastream for response
      // first send index in vector:
      rStream << std::to_string(distributedData.size()).c_str() << "\n";
      rStream << Pistache::Http::flush;
      // return data as CSV, amount is controlled by getProblemDataSize
      int maxIndex = ((curIndex + datasize) > inputData.at(0).size()) ? inputData.at(0).size() : (curIndex + datasize);
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
      distributedData.push_back({{wID,false},{curIndex,curIndex+datasize}});
      curIndex += datasize;

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
  fprintf(stderr,"snapshotting temp\n");

  csv_file.open(dataPath+"snapshot_distributed.csv");
  for(int i = 0; i < distributedData.size(); ++i){
    csv_file << std::to_string(distributedData.at(i).first.first) << ",";
    csv_file << std::to_string(distributedData.at(i).first.second) << ",";
    csv_file << std::to_string(distributedData.at(i).second.first) << ",";
    csv_file << std::to_string(distributedData.at(i).second.second) << "\n";
  }
  csv_file.close();
  fprintf(stderr,"snapshotting ended\n");
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

void ask_data(){
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl = curl_easy_init();
  if(curl) {
      std::string host(hostnames[master_id]);
      std::string tmp = "q=getgetproblemdata&workerID=";
      tmp = tmp.append(std::to_string(worker_id));
      tmp = tmp.append("&datasize=");
      tmp = tmp.append(std::to_string((MINIMUM_STORE - localData.size()) + 4)); //TODO 4 is arbitrary and magic, find optimum
      char *data = &tmp[0];
      curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
      curl_easy_setopt(curl, CURLOPT_PORT, 9080);
      curl_easy_setopt(curl, CURLOPT_POST, 1);
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

void send_result(int i){//TODO better variable name
  CURL *curl;
  CURLcode res;
  std::string readBuffer;
  curl = curl_easy_init();
  if(curl) {
      std::string host(hostnames[master_id]);
      std::string tmp = "q=result&index=";
      tmp = tmp.append(std::to_string(storedResults[i].first));
      tmp = tmp.append("&result=");
      tmp = tmp.append(std::to_string(storedResults[i].second));
      char *data = &tmp[0];
      curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
      curl_easy_setopt(curl, CURLOPT_PORT, 9080);
      curl_easy_setopt(curl, CURLOPT_POST, 1);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
      res = curl_easy_perform(curl);
      curl_easy_cleanup(curl);
      
      if(res == 0){ // we got a response
        storedResults.erase( storedResults.begin() + i );
      }
  }else{
    fprintf(stderr,"Could not init curl\n");
  }
  return;

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
  for(int i = 0; i < n_workers; ++i){
    hostnames[i] = argv[i+2];
    workload[i]++;
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
  checkpoint_data();


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

    if (localData.size() < MINIMUM_STORE)
    {
      ask_data();
    }

    if(storedResults.size()>0){
      for (int i = storedResults.size()-1; i >= 0; i--)//TODO dit moet zolang de post maar 1 result per keer kan verwerken
      {
        send_result(i);
      }
    }
    

    // SNAPSHOT periodically
    diff = std::chrono::system_clock::now() - snapshot_time;
    if(master_id == worker_id && (diff.count() > snapshot_interval)){
      checkpoint_data();
      snapshot_time = std::chrono::system_clock::now();
    }

    // check if we're done when all data has been distributed
    if(master_id == worker_id && distributedData.size() > 0 && distributedData.at(distributedData.size()-1).second.second >= inputData.at(0).size()){
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
  }
  free(hostnames);
}
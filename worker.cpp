#include <iostream>
#include <string>
#include <pistache/endpoint.h> //https://github.com/pistacheio/pistache
#include <curl/curl.h>
#include <chrono>

int worker_id;
int master_id;
int n_workers;
char** hostnames;
int * workload;

static int MAXCLIENTS = 5; //TODO find optimum
//using namespace Pistache;

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
    else if(request.resource().compare("/heartbeat") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok); // return OK
    }
    else if(request.resource().compare("/get_master") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok, hostnames[master_id]); // return hostname of master
    }
    else if(request.resource().compare("/get_worker") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok, assignWorker()); // return hostname of assigned worker
    }
  }
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

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

int elect_master(){
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
  return lowest_id;
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

  auto opts = Pistache::Http::Endpoint::options().threads(1);
  Pistache::Http::Endpoint server(addr);
  server.init(opts);
  server.setHandler(std::make_shared<HelloHandler>());
  server.serveThreaded();

  fprintf(stderr,"Server running at %s:%s\n",addr.host().c_str(),addr.port().toString().c_str());

  // INIT master
  master_id = elect_master();

  // INIT clock & heartbeat
  std::chrono::time_point<std::chrono::system_clock> prev_time = std::chrono::system_clock::now();
  double heartbeat_interval = 5.0;
  int master_down_counter = 0;

  while(true){
    // HEARTBEAT
    if((master_id != worker_id) && ((std::chrono::system_clock::now() - prev_time).count() > heartbeat_interval)){ // every 5 seconds, heartbeat
      if(!sendHeartBeat()){ // master is down, check every second
        master_down_counter++;
        heartbeat_interval = 1.0;
      } else if(master_down_counter > 0){ // if master recovers, go back to 5 seconds heartbeat
        master_down_counter = 0;
        heartbeat_interval = 5.0;
      }
      if(master_down_counter >= 5){ // if master stays down, elect new master
        master_id = elect_master();
        master_down_counter = 0;
        heartbeat_interval = 5.0;
      }
      prev_time = std::chrono::system_clock::now();
    }



  }

  free(hostnames);
}
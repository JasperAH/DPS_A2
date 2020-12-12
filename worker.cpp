#include <iostream>
#include <string>
#include <pistache/endpoint.h> //https://github.com/pistacheio/pistache
#include <curl/curl.h>
#include <chrono>


//using namespace Pistache;

struct HelloHandler : public Pistache::Http::Handler {
  HTTP_PROTOTYPE(HelloHandler);
  void onRequest(const Pistache::Http::Request&, Pistache::Http::ResponseWriter writer) override{
    writer.send(Pistache::Http::Code::Ok, "Hello, World!\n");
  }
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool sendHeartBeat(char* host){
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl = curl_easy_init();
  if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL, host);
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

int elect_master(int worker_id, int n_workers, char** hostnames){
  int lowest_id = worker_id;

}

int main(int argc, char **argv) {
  // parse input
  if (argc < 3){
      fprintf(stderr, "usage: %s <worker_id> <worker_hostnames>\n" 
      "The worker's own hostname is at index worker_id in worker_hostnames.\n", argv[0]);
      return -1;
  }

  int worker_id = atoi(argv[1]);
  int master_id;
  int n_workers = argc - 2;
  char* hostnames[argc-2];
  for(int i = 0; i < n_workers; ++i){
    hostnames[i] = argv[i+2];
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
  master_id = elect_master(worker_id,n_workers,hostnames);

  // INIT clock & heartbeat
  std::chrono::time_point<std::chrono::system_clock> prev = std::chrono::system_clock::now();
  double heartbeat_interval = 5.0;
  int master_down_counter = 0;

  while(true){
    // HEARTBEAT
    if((std::chrono::system_clock::now() - prev).count() > heartbeat_interval){ // every 5 seconds, heartbeat
      if(!sendHeartBeat(hostnames[master_id])){ // master is down, check every second
        master_down_counter++;
        heartbeat_interval = 1.0;
      } else if(master_down_counter > 0){ // if master recovers, go back to 5 seconds heartbeat
        master_down_counter = 0;
        heartbeat_interval = 5.0;
      }
      if(master_down_counter >= 5){ // if master stays down, elect new master
        master_id = elect_master(worker_id,n_workers,hostnames);
        master_down_counter = 0;
        heartbeat_interval = 5.0;
      }
    }



  }
}
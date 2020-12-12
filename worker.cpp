#include <iostream>
#include <string>
#include <pistache/endpoint.h> //https://github.com/pistacheio/pistache
#include <curl/curl.h>

//using namespace Pistache;

struct HelloHandler : public Pistache::Http::Handler {
  HTTP_PROTOTYPE(HelloHandler);
  void onRequest(const Pistache::Http::Request&, Pistache::Http::ResponseWriter writer) override{
    writer.send(Pistache::Http::Code::Ok, "Hello, World!\n");
  }
};

int main(int argc, char **argv) {
  Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(9080));

  auto opts = Pistache::Http::Endpoint::options().threads(1);
  Pistache::Http::Endpoint server(addr);
  server.init(opts);
  server.setHandler(std::make_shared<HelloHandler>());
  server.serveThreaded();

  fprintf(stderr,"Server running at %s:%s\n",addr.host().c_str(),addr.port().toString().c_str());
  while(true){

  }
}
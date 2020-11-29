#include <iostream>
#include <string>
#include <pistache/endpoint.h>

//using namespace Pistache;

struct HelloHandler : public Pistache::Http::Handler {
  HTTP_PROTOTYPE(HelloHandler)
  void onRequest(const Pistache::Http::Request&, Pistache::Http::ResponseWriter writer) override{
    writer.send(Pistache::Http::Code::Ok, "Hello, World!");
  }
};

int main() {
  Pistache::Http::listenAndServe<HelloHandler>(Pistache::Address("*:9080"));
}
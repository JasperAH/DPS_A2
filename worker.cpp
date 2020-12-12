#include <iostream>
#include <string>
#include <pistache/endpoint.h> //https://github.com/pistacheio/pistache

//using namespace Pistache;

/******************************************Master functions*******************************/

/*****************************************Internal functions******************************/
void getProblemData(void *ptr, int & size){ //TODO
    throw "myFunction is not implemented yet.";
    return;
}

void postResultInternally(){ //TODO
    throw "myFunction is not implemented yet.";
    return;
}

void getWorker(){ //TODO
    throw "myFunction is not implemented yet.";
    return;
}

/**********************************************Handlers**********************************/
struct HelloHandler : public Pistache::Http::Handler {
  HTTP_PROTOTYPE(HelloHandler)
  void onRequest(const Pistache::Http::Request&, Pistache::Http::ResponseWriter writer) override{
    writer.send(Pistache::Http::Code::Ok, "Hello, World!");
  }
};



int main() {
  Pistache::Http::listenAndServe<HelloHandler>(Pistache::Address("*:9080"));
}
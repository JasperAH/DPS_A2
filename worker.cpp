#include <iostream>
#include <string>
#include <pistache/endpoint.h> //https://github.com/pistacheio/pistache

//using namespace Pistache;

/******************************************Master functions*******************************/

std::string assignWorker(){
  //throw "getProblemData is not implemented yet.";
  return "";
}

/******************************************Internal functions*******************************/


/**************************************Client serving  functions**************************/
void getProblemData(void *ptr, int & size){ //TODO
    throw "getProblemData is not implemented yet.";
    return;
}

void postResultInternally(){ //TODO
    throw "postResultInternally is not implemented yet.";
    return;
}

/**********************************************Handlers**********************************/
struct HelloHandler : public Pistache::Http::Handler {
  HTTP_PROTOTYPE(HelloHandler)
  void onRequest(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter writer) override{
    if(request.resource().compare("/get_master") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok, "localhost"); // return own ID for master election
    }
    else if(request.resource().compare("/get_worker") == 0 && request.method() == Pistache::Http::Method::Get){
      writer.send(Pistache::Http::Code::Ok, assignWorker()); // return own ID for master election
    }
    
  }
};



int main() {
  Pistache::Http::listenAndServe<HelloHandler>(Pistache::Address("*:9080"));
}
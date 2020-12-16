#include <iostream>
#include <sstream>
#include <string>
#include <curl/curl.h>
#include <fstream>
#include <vector>
#include <ctime>

int main(){

    std::ofstream outfile ("data.csv");
    int arraylength = 10000;
    int numArrays = 1000;

    srand(static_cast <unsigned int> (time(0)));

    outfile << "0" << std::endl;
    for (int i = 0; i < numArrays; i++)
    {
        outfile << rand() % 101;
        for (int j = 1; j < arraylength; j++)
        {
            outfile << ", " << rand() % 101;
        }
        outfile << std::endl;
        if (i%2 == 0)
        {
            std::cout << i << std::endl;
        }
        
    }

    outfile.close();
}
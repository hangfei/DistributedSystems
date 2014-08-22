#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <vector>

using namespace std;

/**
 * Helper module
 */ 
class utils
{

public :

  void send_request(string, int, string);

  vector<string> splitEx(const string&, string);

  int getCurrentTime();

  string getIpAdress();
};
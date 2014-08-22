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
#include <ifaddrs.h>

#include "utils.h"

#define BUFSIZE 2048

/**
 * Send request using UDP protocol
 */
void utils::send_request(string server_name, int port_num, string content)
{

  //Initialize parameters
  string server (server_name);
  int SERVICE_PORT = port_num;

  struct sockaddr_in myaddr, remaddr;
  int fd, i, slen=sizeof(remaddr);
  socklen_t addrlen = sizeof(remaddr);
  int recvlen;
  char buf[BUFSIZE];

  /*bind socket*/
  if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
    printf("socket created\n");

  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(0);

  if (::bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
  {
    perror("bind failed");
  }
  /*bind socket*/
  memset((char *) &remaddr, 0, sizeof(remaddr));
  remaddr.sin_family = AF_INET;
  remaddr.sin_port = htons(SERVICE_PORT);
  if (inet_aton(server.c_str(), &remaddr.sin_addr)==0)
  {
    fprintf(stderr, "inet_aton() failed\n");
    exit(1);
  }
  /*Send message to sever*/
  sprintf(buf, "%s", content.c_str());
  if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1)
    perror("sendto");
}



/**
 * Split the string
 */
vector<string> utils::splitEx(const string& src, string separate_character)
{
  vector<string> strs;

  int separate_characterLen = separate_character.size();
  int lastPosition = 0,index = -1;
  while (-1 != (index = src.find(separate_character,lastPosition)))
  {
    strs.push_back(src.substr(lastPosition,index - lastPosition));
    lastPosition = index + separate_characterLen;
  }
  string lastString = src.substr(lastPosition);
  if (!lastString.empty())
    strs.push_back(lastString);
  return strs;
}



/**
 * Get current time.
 */
int utils::getCurrentTime()
{
  time_t timer;
  time(&timer);  /* get current time; same as: timer = time(NULL)  */
  return (int)timer;
}



/**
 * Get IP Address
 */
string utils::getIpAdress()
{
  struct ifaddrs *ifaddr, *ifa;
  int family, s, n;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1)
  {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  /* Walk through linked list, maintaining head pointer so we
   can free list later */

  for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
  {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;

    /* For an AF_INET* interface address, display the address */

    if (family == AF_INET)
    {
      s = getnameinfo(ifa->ifa_addr,
                      (family == AF_INET) ? sizeof(struct sockaddr_in) :
                      sizeof(struct sockaddr_in6),
                      host, NI_MAXHOST,
                      NULL, 0, NI_NUMERICHOST);
      if (s != 0)
      {
        printf("getnameinfo() failed: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
      }

      if(strcmp(ifa->ifa_name,"eth0") == 0)
      {
        freeifaddrs(ifaddr);
        return string(host);
      }
    }
  }
}

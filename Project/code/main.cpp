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
#include <vector>
#include <thread>

#include "leader.h"
#include "client.h"
#include "utils.h"
#include "LeaderElection.h"

#define BUFSIZE 2048

using namespace std;

vector<string> splitEx(const string& src, string separate_character);
void leaderStartListeningOnPort(leader leader);
void clientStartListeningOnPort(client client);
void check_is_alive(client &client);


/**
 * A Distributed Chatting system.
 * @Usage: it's developed under UNIX environment. To run the code, you need to
 * use terminal. Type the following command in the terminal.
 * There are two typcial situations:
 *     Leader: ./dchat username
 *     Client: ./dchat username leader_ip:leader_port
 * @Description: It's implemented with UDP protocol. This system handle various
 *               common situations in a distributed environment, such as client crash/leave,
 *               leader/server crash/leave.
 * @Author: Hangfei Lin, Di Wu, Bofei Wang
 * @Date: April
 * @Version: 4.2(Final Version)
 */
int main(int argc, char *argv[])
{
  //initial parameter
  string leader_info;

  //check the number of parameters
  if(argc != 2 && argc != 3)
  {
    printf("./dchat <name> or ./dchat <name> <host:portNum>\n");
    exit(1);
  }

  //If the user is trying to start a new group
  if(argc == 2)
  {
    // set a random server port
    srand((unsigned)time(NULL));
    int servic_port = rand() % 2000 + 8000;

    utils u;
    string name(argv[1]);
    string server = u.getIpAdress();
    cout << name << " started a new chat, listening on " \
         << server << ":" << servic_port << "\n";

    //Initialize leader
    leader_info = server + ":" + to_string(servic_port);

    cout << "Succeeded, current users: \n" <<name << " " <<leader_info
         << " (Learder) \n" << "Waiting for others to join... \n";

    //do_leader
    leader leader;
    leader.init_group_info(name, server, servic_port);
    leaderStartListeningOnPort(leader);
  }

  //If the user is trying to join a new group
  if(argc == 3)
  {

    utils u;
    string name(argv[1]);

    //parse server ip:port
    string server_ip_port(argv[2]);
    vector<string> split_server_ip_port = splitEx(server_ip_port, ":");
    string server_ip(split_server_ip_port[0].c_str());
    int server_port = atoi(split_server_ip_port[1].c_str());

    // parse local port
    srand((unsigned)time(NULL));
    int local_port = rand() % 2000 + 8000;
    string local_ip = u.getIpAdress();

    //do_client
    client client;
    int res = client.join(server_ip, server_port, name, local_ip, local_port);
    if(res > 0)
    {
      clientStartListeningOnPort(client);
    }
    else
    {
      cout << name << " joining a new chat on " << server_ip<< ":" << server_port \
           << ", listening on " << local_ip << ":" << local_port << endl;
      cout << "Sorry, no chat is active on " << server_ip<< ":" << server_port \
           << ", try again later." <<endl;
      cout << "Bye." << endl;
    }
    //client.do_client(server_ip, server_port);
  }
}



/**
 * Split the string
 */
vector<string> splitEx(const string& src, string separate_character)
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
  if (!lastString.empty()) strs.push_back(lastString);
  return strs;
}



/**
 * Starts the chatting system as a leader.
 * The leader would be in charge of the logistics of the chatting
 * system. If the leader leaves or crashes, the chatting system would
 * automatically start a leader election process.
 */
void leaderStartListeningOnPort(leader leader)
{
  struct sockaddr_in myaddr;	/* our address */
  struct sockaddr_in remaddr;	/* remote address */
  socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
  int recvlen;			/* # bytes received */
  int fd, slen=sizeof(remaddr);				/* our socket */
  char buf[BUFSIZE];	/* receive buffer */

  /* create a UDP socket */

  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("cannot create socket\n");
  }

  /* bind the socket to any valid IP address and a specific port */

  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(leader.leader_port);

  if (::bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
  {
    perror("bind failed"); 
  }

  // temp variables
  fd_set rfds;
  int retval;

  // Thread for checking clients status
  std::thread first (&leader::check_is_alive, &leader);
  leader.current_leader_stamp = 0;
  leader.my_stamp = 0;

  while (1)
  {

    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    FD_SET(fd, &rfds);
    retval = select(fd + 1, &rfds, NULL, NULL, NULL);
    if (retval == -1)
    {
      printf("ERROR");
    }

    //when the input is from socket

    if (FD_ISSET(fd, &rfds))
    {
      //printf("waiting on port %d\n", this->leader_port);
      recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
      //printf("received %d bytes\n", recvlen);
      if (recvlen > 0)
      {
        buf[recvlen] = 0;
        //Start a new timer thread------------------------------

        leader.do_leader(buf, fd, slen, remaddr);
      }
    }

    //when the input is from command line
    if (FD_ISSET(STDIN_FILENO, &rfds))
    {

      char* line = (char*) malloc(10000);
      //string line;
      size_t len = 0;
      ssize_t read;
      if ((read = getline(&line, &len, stdin)) != -1)
      {
        //line = getchar();
        //cout << string("command line input: ") << &line == EOF << string("??") <<endl;
        utils u;
        string content = string("NORMAL,") + \
                         to_string(leader.my_stamp)+"," + leader.leader_name + "," + string(line);
        leader.my_stamp++;
        //string content = string("NORMAL,") + string(line);
        u.send_request(leader.leader_ip, leader.leader_port, content);
      }
    }
  }
}



/**
 * Join the chatting system as a client.
 * If the client detects that the leader leaves or crashes, the chatting
 * room would pause and enter the leader election process. The chatting room
 * would continue as normal after leader election and leader inform.
 */
void clientStartListeningOnPort(client client)
{
  struct sockaddr_in myaddr;	/* our address */
  struct sockaddr_in remaddr;	/* remote address */
  socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
  int recvlen;			/* # bytes received */
  int fd, slen=sizeof(remaddr);				/* our socket */
  char buf[BUFSIZE];	/* receive buffer */
  int is_leader = 0; /*flag for client's role state*/

  /* create a UDP socket */

  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("cannot create socket\n");
  }

  /* bind the socket to any valid IP address and a specific port */

  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(client.local_port);

  if (::bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
  {
    perror("bind failed");
  }

  // temp variables
  fd_set rfds;
  int retval;

  client.my_stamp = 0;
  deque<vector<string>> emptyDeque (10);
  client.client_side_queue = emptyDeque;
  LeaderElection leaderElection;

  // TODO ??
  client.leader_alive = 1;
  string response = "";
  while (1)
  {

    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    FD_SET(fd, &rfds);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;     // setting waiting period

    // Thread for checking clients status
    retval = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (retval == -1)
    {
      printf("ERROR");
    }


    if(is_leader)
    {
      check_is_alive(client);
    }

    // TODO if get input, will timeout be triggered?
    if(retval == 0 && !is_leader)
    {
      utils u;
      if(client.leader_last_response_time != 0 && (u.getCurrentTime() - client.leader_last_response_time > 3))
      {
        std::cout << u.getCurrentTime() << endl;
        std::cout << client.leader_last_response_time << endl;
        //erase old leader info
        client.group_member_info.erase(client.leader_name);
        cout<< "ELECTION: leader leaves or crashes." << endl;
        client.leader_alive = false;
      }

      // Report client status
      time_t timer;
      time(&timer);  /* get current time; same as: timer = time(NULL)  */
      int currenttime = timer;
      string content = string("CLIENTSTATUS,") + \
                       string(to_string(currenttime)) + string(",") + \
                       string(client.local_name) + string(",") + \
                       string(client.local_ip)+ string(",") + \
                       string(to_string(client.local_port));
      u.send_request(client.leader_ip, client.leader_port, content);
    }

    //when the input is from socket
    if (FD_ISSET(fd, &rfds))
    {
      recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
      if (recvlen > 0)
      {
        buf[recvlen] = 0;

        response = string(buf);
        utils u;
        vector<string> split_res = u.splitEx(response, ",");
        // If not ELECTION message, handle via the clients or leader module
        if(!split_res[0].compare(string("ELECTION")) == 0)
        {
          if(is_leader == 0)
          {
            client.do_client(buf, fd, slen, remaddr);
          }
          else
          {
            client.do_leader(buf, fd, slen, remaddr);
          }
        }
      }
    }

    //when the input is from command line
    if (FD_ISSET(STDIN_FILENO, &rfds))
    {
      char* line = (char*) malloc(10000);
      size_t len = 0;
      ssize_t read;
      if ((read = getline(&line, &len, stdin)) != -1)
      {
        utils u;
        string content = string("NORMAL,") + \
                         to_string(client.my_stamp)+"," + client.local_name + "," + string(line);
        client.my_stamp++;
        u.send_request(client.leader_ip, client.leader_port, content);
      }
    }

    // If leader is not alive, start election
    if(!client.leader_alive)
    {
      utils u;
      std::cout << "ELECTION: leader crashes or leaves. Start election..." << endl;
      // enter election loop
      string leader_result = leaderElection.hold_election(client.local_name, fd, client.group_member_info);
      std::cout << "ELECTION: new leader is " << leader_result << endl;
      if(leader_result.compare(client.local_name) == 0)
      {
        std::cout << "         you are the leader now." << endl;
        // TODO change to true?
        is_leader = 1;
        client.init_leader();
      }
      else
      {
        client.init_client();
      }
      // update leader information for clients and new leader
      client.leader_alive = 1;
      string my_info = client.group_member_info[leader_result];
      vector<string> split_ip_port = u.splitEx(my_info, ":");
      client.leader_ip = split_ip_port[0];
      client.leader_port = atoi(split_ip_port[1].c_str());
      client.leader_last_response_time = u.getCurrentTime();
    }

    // TODO
    if(response.empty())
    {
      continue;
    }
    // parse the message
    utils u;
    vector<string> split_res = u.splitEx(response, ",");
    response = "";    // reset response string
    // handle ELECTION request(for participatation)
    if(split_res[0].compare(string("ELECTION")) == 0)
    {
      if(split_res.size() != 5)
      {
        cout << "ELECTION: FORMAT WRONG!" << "\n";
        continue;
      }
      string command = split_res[0];
      string timestamp = split_res[1];
      string candidate_name = split_res[2];
      string ip = split_res[3];
      string port_num = split_res[4];
      // ELECTION, <timestamp>, <username>, <IP>, <portnum>
      cout << "ELECTION: received election request from " << candidate_name <<"\n";

      // participate the election
      if(leaderElection.participate_election(client.local_name, candidate_name, ip, port_num))
      {
        // Hold its own election
        cout << "ELECTION: Deny the election. Hold your own election" << "\n";
        string leader_result = leaderElection.hold_election(client.local_name, fd, client.group_member_info);
        std::cout << "ELECTION: new leader is " << leader_result << endl;
        if(leader_result.compare(client.local_name) == 0)
        {
          std::cout << "         you are the leader now." << endl;
          // TODO change to true?
          is_leader = 1;
          client.init_leader();
        }
        else
        {
          client.init_client();
        }
        // update leader information for clients and new leader
        client.leader_alive = 1;
        string my_info = client.group_member_info[leader_result];
        vector<string> split_ip_port = u.splitEx(my_info, ":");
        client.leader_ip = split_ip_port[0];
        client.leader_port = atoi(split_ip_port[1].c_str());
        client.leader_last_response_time = u.getCurrentTime();
      }
      else
      {
        // TODO ???? wait for leader inform?
        // do i need client.init_client();
        // and init other params?
        // do nothing
        cout << "ELECTION: bigger candidate. Waiting for election result." << "\n";
        
      }
      cout << "ELECTION: finished." << "\n";
    } // Finish handle election
  }
}



/**
 * Check if it's alive
 */
void check_is_alive(client &client)
{
    for (auto it=client.group_member_last_visit.cbegin(); it!=client.group_member_last_visit.cend(); )
    {
      utils u;
      if(u.getCurrentTime()-it->second > 2)
      {
        cout<< "NOTICE that " << it->first << " has crashed or leave" << endl;
        string content = string("CLIENTLEAVE")+string(",")+string("time")+string(",")+it->first;
        client.group_member_info.erase(it->first);
        client.group_member_last_visit.erase(it++);

        for (std::map<string,string>::iterator it = client.group_member_info.begin(); it!=client.group_member_info.end(); ++it)
        {
          vector<string> split_ip_port = u.splitEx(it->second, ":");
          if(atoi(split_ip_port[1].c_str()) != client.leader_port)
          {
            u.send_request(string(split_ip_port[0].c_str()), atoi(split_ip_port[1].c_str()), content);
          }
        }
      }
      else
      {
        ++it;
      }
    }
}


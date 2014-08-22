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
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <map>
#include <vector>

#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "LeaderElection.h"
#include "utils.h"
#define BUFSIZE 2048

using namespace std;

int systemCheck = 1, isNew;
pthread_mutex_t systemCheckLock, isNewLock, ardMsgLock;



/**
 * Bully compare based on alphetic order of the string
 */
bool bully_compare(string me, string candidate)
{
  if(me.compare(candidate) > 0)
  {
    // refuse election
    return true;
  }
  else
  {
    // agree the election
    return false;
  }
}



/**
 * Send leader election signal to other members.
 */
void sendElectionSignal(string my_name, map<string, string> group_member_info) 
{
  utils u;
  string my_info = group_member_info[my_name];
  vector<string> split_ip_port = u.splitEx(my_info, ":");
  string my_ip = split_ip_port[0];
  string my_port_str = split_ip_port[1];

  for (std::map<string,string>::iterator it=group_member_info.begin(); it!=group_member_info.end(); ++it)
  {
    // get info
    vector<string> split_ip_port = u.splitEx(it->second, ":");
    string member_name = it->first;
    string member_ip = split_ip_port[0];
    int port_num = atoi(split_ip_port[1].c_str());

    // bully compare
    if(my_name.compare(member_name) == 0)
    {
      // cout << "Local_name in map. Correct." << it->first << endl;
      continue;
    }
    else if(bully_compare(my_name, member_name))
    {
      // cout << it->first << " smaller. Do not send election." << endl;
      continue;
    }
    else
    {
      // send "ELECTION" signal
      std::cout << "Send signal:" << member_name << " " << member_ip << " " << port_num << endl;
      string content = "ELECTION";
      content += ",";
      content += "fake_timestamp";
      content += ",";
      content += my_name;
      content += ",";
      content += my_ip;
      content += ",";
      content += my_port_str;
      u.send_request(member_ip, port_num, content);
    }
  }

}

/**
 * Hold leader election
 */
string LeaderElection::hold_election(string my_name, int fd, map<string, string> group_member_info)
{
  utils u;
  bool election_result = true;
  bool election_over = false;
  string my_info = group_member_info[my_name];
  vector<string> split_ip_port = u.splitEx(my_info, ":");
  string my_ip = split_ip_port[0];
  string my_port_str = split_ip_port[1];
  map<string, string> election_vote; // map for votes

  // Step1. Send election signal to participants
  sendElectionSignal(my_name, group_member_info);

  // Step2. Wait for responses
  struct sockaddr_in myaddr;	/* our address */
  struct sockaddr_in remaddr;	/* remote address */
  socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
  int recvlen;			/* # bytes received */
  int slen=sizeof(remaddr);				/* our socket */
  char buf[BUFSIZE];	/* receive buffer */

  fd_set rfds;
  int retval;

  int countdonw_vote = 5;
  struct timeval tv;

  while(1)
  {
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    FD_SET(fd, &rfds);

    tv.tv_sec = 1;
    tv.tv_usec = 0;     // setting waiting period

    retval = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (retval == -1)
    {
      printf("ERROR");
    }

    // Election timer
    if(retval == 0)
    {
      std::cout << "ELECTION: waiting for votes... "  << countdonw_vote-- << " seconds left" << endl;
      if(election_over)
      {
        std::cout << "Election: finished: " << election_over << endl;
        break;
      }
      if(countdonw_vote == 0)
      {
        // TODO 
        std::cout << "Election: timeout." << election_over << endl;
        sendElectionSignal(my_name, group_member_info);
        break;
      }
    }

    //when the input is from socket
    if (FD_ISSET(fd, &rfds))
    {
      recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
      if (recvlen > 0)
      {
        buf[recvlen] = 0;
        printf("ELECTION: received message: \"%s\"\n", buf);
        utils u;
        vector<string> split_res = u.splitEx(buf, ",");

        // Handle Election
        if(split_res[0].compare(string("ELECTION")) == 0)
        {
          if(split_res.size() != 5 && split_res.size() != 6)
          {
            cout << "ELECTION: FORMAT WRONG" << "\n";
            continue;
          }
          // vote message
          if(split_res.size() == 6)
          {
            string command = split_res[0];
            string timestamp = split_res[1];
            string username = split_res[2];
            string ip = split_res[3];
            string port_num = split_res[4];
            string vote = split_res[5];
            cout << "ELECTION: vote " << vote << " from " << username << endl;
            if(vote.compare("OK") == 0)
            {
              cout << "ELECTION: failed" << endl;
              election_vote[username] = "OK";
              election_result = false;
              election_over = true;
              break;
            }
          }

          // hold election message
          if(split_res.size() == 5)
          {
            string command = split_res[0];
            string timestamp = split_res[1];
            string candidate_name = split_res[2];
            string ip = split_res[3];
            string port_num = split_res[4];
            if(bully_compare(my_name, candidate_name))
            {
              // if my_name bigger than candidate_name(true), deny election
              // send "OK"
              string deny_string = "OK";
              std::cout << "Send OK ==>" << candidate_name << " "
                        << ip << " " << port_num << endl;
              string content = "ELECTION";
              content += ",";
              content += "fake_timestamp";
              content += ",";
              content += my_name;
              content += ",";
              content += ip;
              content += ",";
              content += port_num;
              content += ",";
              content += deny_string;
              u.send_request(ip, atoi(port_num.c_str()), content);
              // TODO hold its own election, if not already holding, necessary?
            }
            else
            {
              // do nothing
              // TODO not use timer yet
              std::cout << my_name << "ELECTION: lose election to " << candidate_name << endl;
              election_result = 0;
              election_over = true;
              break;
            }
          }
        }
        // TODO Handle other if other message?
      }
    }

    // Interact with user input
    if (FD_ISSET(STDIN_FILENO, &rfds))
    {
      char* line = (char*) malloc(10000);
      size_t len = 0;
      ssize_t read;
      if ((read = getline(&line, &len, stdin)) != -1)
      {
        cout << "Election: please wait." << "\n";
      }
    }
  } // End waiting for response

  // Step3. Send results or wait for results.
  std::cout << "Election: over. Sending results or waiting for results..." << endl;
  if(election_result)
  {
    // Win, send result to clients
    cout << "ELECTION: win the election." << endl;
    for (std::map<string,string>::iterator it=group_member_info.begin(); it!=group_member_info.end(); ++it)
    {
      // get info
      vector<string> split_ip_port = u.splitEx(it->second, ":");
      string member_name = it->first;
      string member_ip = split_ip_port[0];
      int port_num = atoi(split_ip_port[1].c_str());

      // bully compare ??
      if(my_name.compare(member_name) == 0)
      {
        // cout << "Local_name in map. Assertion true." << it->first << endl;
        continue;
      }
      else
      {
        // send "LEADERINFORM" result
        // std::cout << "Send signal:" << member_name << " " << member_ip << " " << port_num << endl;
        string content = "LEADERINFORM";
        content += ",";
        content += "fake_timestamp";
        content += ",";
        content += my_name;
        content += ",";
        content += my_ip;
        content += ",";
        content += my_port_str;
        u.send_request(member_ip, port_num, content);
      }
    }
    // do i need ACK from clients?
    return my_name;
  }
  else
  {
    // Lose, wait for LEADERINFORM
    cout << "Lose the election. Wait for LEADERINFORM..." << "\n";
    int countdown_leaderinform = 10;
    struct timeval tv;
    while(1)
    {
      FD_ZERO(&rfds);
      FD_SET(STDIN_FILENO, &rfds);
      FD_SET(fd, &rfds);

      tv.tv_sec = 1;
      tv.tv_usec = 0;     // setting waiting period

      retval = select(fd + 1, &rfds, NULL, NULL, &tv);
      if (retval == -1)
      {
        printf("ERROR");
      }

      // LEADERINFORM countdown
      if(retval == 0)
      {
        countdown_leaderinform--;
        std::cout << " Waiting for LEADERINFORM..." << countdown_leaderinform << " seconds left" << endl;
        if(countdown_leaderinform == 0)
        {
          // TODO do something
          std::cout << "LEADERINFORM timeout!." << endl;
          sendElectionSignal(my_name, group_member_info);
          break;
        }
      }

      // when the input is from socket. wait for LEADERINFORM
      if (FD_ISSET(fd, &rfds))
      {
        recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
        if (recvlen > 0)
        {
          buf[recvlen] = 0;
          printf("Received message: \"%s\"\n", buf);
          utils u;
          vector<string> split_res = u.splitEx(buf, ",");

          // Handle LEADERINFORM
          if(split_res[0].compare(string("LEADERINFORM")) == 0)
          {
            if(split_res.size() != 5)
            {
              cout << "ELECTION FORMAT WRONG" << "\n";
              continue;
            }
            // vote message
            if(split_res.size() == 5)
            {
              string command = split_res[0];
              string timestamp = split_res[1];
              string leader_name = split_res[2];
              string ip = split_res[3];
              string port_num = split_res[4];
              return leader_name;
            }
          }
        }
      }

      if (FD_ISSET(STDIN_FILENO, &rfds))
      {
        char* line = (char*) malloc(10000);
        size_t len = 0;
        ssize_t read;
        if ((read = getline(&line, &len, stdin)) != -1)
        {
          cout << "Leader Election - waiting for LEADERINFORM. Please wait..." << "\n";
        }
      }
    } // End waiting for response
    return "ERROR_NO_LEADER";
  }
}



/**
 * Participate the election
 */
bool LeaderElection::participate_election(string my_name, string candidate_name, string ip, string port_string)
{
  utils u;
  string deny_string = "OK";
  cout << "participate_election";
  cout << "bully_compare:" << bully_compare(my_name, candidate_name) << endl;
  // if my_name bigger than candidate_name(true), send OK
  if(bully_compare(my_name, candidate_name))
  {
    // send "OK"
    // ELECTION, <timestamp>, <"OK">
    std::cout << "Send OK" << endl;
    std::cout << ip << port_string << endl;
    string content = "ELECTION";
    content += ",";
    content += "fake_timestamp";
    content += ",";
    content += my_name;
    content += ",";
    content += ip;
    content += ",";
    content += port_string;
    content += ",";
    content += deny_string;
    u.send_request(ip, atoi(port_string.c_str()), content);
    return true;
  }
  else
  {
    // if my_name bigger than candidate_name(true), do nothing
    // do nothing
    // TODO not use timer yet
    return false;
  }
}


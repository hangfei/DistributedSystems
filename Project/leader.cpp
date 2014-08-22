#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <string>
#include <iostream>
#include <map>
#include <vector>

#include "utils.h"
#include "leader.h"

#define BUFSIZE 2048

using namespace std;


/**
 * Initialize the group member info.
 * When the program starts to function as a leader, it would
 * initialize the group member info.
 */
void leader::init_group_info(string name, string server_name, int port_num)
{
  this->leader_name = name;
  this->leader_ip = string(server_name);
  this->leader_port = port_num;
  group_member_info[name] = string(server_name) + string(":") + to_string(port_num);

  deque<string> emptyDeque (10);
  multi_queue[name] = emptyDeque;
  current_client_stamp[name] = 0;
}



/**
 * Start role as a leader
 * The function would handle incoming request and send messages back to the clients.
 */
void leader::do_leader(char buf[BUFSIZE], int fd, int slen, struct sockaddr_in remaddr)
{
  char *token = strtok(buf, ",");

  //handle JOINREQUEST
  if(strcmp(token,"JOINREQUEST") == 0)
  {

    //parse time;
    token = strtok(NULL, ",");
    char time[80];
    strcpy(time, token);
    //parse name
    token = strtok(NULL, ",");
    char name[80];
    strcpy(name, token);
    //parse ip
    token = strtok(NULL, ",");
    char ip[80];
    strcpy(ip, token);
    //parse port
    token = strtok(NULL, ",");
    char port[80];
    strcpy(port, token);

    //check if username exist
    int is_exist = 1;
    for (std::map<string,string>::iterator it=group_member_info.begin(); it!=group_member_info.end(); ++it)
    {
    	if(it->first.compare(string(name)) == 0) {
            is_exist = 0;
    	}
    }
	
	  // if username exist
  	if(is_exist == 1) {
  			
  		//add the new member to maps
  		group_member_info[name] = string(ip) + string(":") + string(port);
  		deque<string> emptyDeque (10);
  		multi_queue[name] = emptyDeque;
  		current_client_stamp[name] = 0;
  		cout << "NOTICE " << name <<" joined on " << string(ip) + string(":") + string(port) <<endl;

  		//response for the client that request join
  		string res_for_join ("JOINRESPONSE,");
  		res_for_join += to_string(current_leader_stamp) + ",";
  		res_for_join += leader_name + ",";
  		res_for_join += leader_ip + ",";
  		res_for_join += to_string(leader_port) + ",";
  		res_for_join += to_string(group_member_info.size()) + ",";
  		utils u;
  		for (std::map<string,string>::iterator it=group_member_info.begin(); it!=group_member_info.end(); ++it)
  		{
  		  vector<string> split_ip_port = u.splitEx(it->second, ":");
  		  res_for_join += it->first + ",";
  		  res_for_join += split_ip_port[0] + ",";
  		  res_for_join += split_ip_port[1] + ",";
  		}
  		sprintf(buf, "%s", res_for_join.c_str());
  		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1)
  		  perror("sendto");

  		//inform each memeber in the map

  		for (std::map<string,string>::iterator it=group_member_info.begin(); it!=group_member_info.end(); ++it)
  		{
  		  utils u;
  		  string content = string("JOININFORM,time,") + string(name) + string(",")
  						   + string(ip) + string(",") + string(port);
  		  vector<string> split_ip_port = u.splitEx(it->second, ":");
  		  u.send_request(string(split_ip_port[0].c_str()), atoi(split_ip_port[1].c_str()), content);

  		}
  	}
  	else {
  		string res_for_join ("JOINRESPONSE,");
  		res_for_join += to_string(current_leader_stamp) + ",";
  		res_for_join += to_string(-1);
  		sprintf(buf, "%s", res_for_join.c_str());
  		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1)
  		  perror("sendto");
  	}

  }

  //handle request: NORMAL
  if(strcmp(token,"NORMAL") == 0)
  {
    //send message to each member in the map
    utils u;
    //parse time;
    token = strtok(NULL, ",");
    int	time = stoi(string(token));
    //parse name
    token = strtok(NULL, ",");
    char name[80];
    strcpy(name, token);
    //parse ip
    token = strtok(NULL, "");
    char text[80];
    strcpy(text, token);
    doSequence(time, string(name), string(text));
  }

  // handle requst: report status
  // send message to each member in the map
  if(strcmp(token,"CLIENTSTATUS") == 0)
  {
    
    utils u;
    //parse reportTime;
    token = strtok(NULL, ",");
    char reportTime[80];
    strcpy(reportTime, token);
    //parse name
    token = strtok(NULL, ",");
    char name[80];
    strcpy(name, token);
    //parse ip
    token = strtok(NULL, ",");
    char ip[80];
    strcpy(ip, token);
    //parse port
    token = strtok(NULL, "");
    char port[80];
    strcpy(port, token);
    string text("leader\n");

    int currenttime = u.getCurrentTime();
    this->group_member_last_visit[string(name)] = atoi(string(reportTime).c_str());
    string content = string("LEADERSTATUS")+string(",")+string(to_string(currenttime));

    // send response to client
    u.send_request(string(ip), atoi(string(port).c_str()), content);
  }

}



/**
 * Check if all the client is alive. If not, remove dead client.
 * This function runs every 3 seconds.
 */
void leader::check_is_alive()
{
  while(true)
  {
    usleep(3000000);
    for (auto it=group_member_last_visit.cbegin(); it!=group_member_last_visit.cend(); )
    {
      utils u;
      if(u.getCurrentTime()-it->second>2)
      {

        cout<< "NOTICE that " << it->first << " has crashed or leave" << endl;
        string content = string("CLIENTLEAVE")+string(",")+string("time")+string(",")+it->first;
        group_member_info.erase(it->first);
        group_member_last_visit.erase(it++);

        for (std::map<string,string>::iterator it=group_member_info.begin(); it!=group_member_info.end(); ++it)
        {
          vector<string> split_ip_port = u.splitEx(it->second, ":");
          if(atoi(split_ip_port[1].c_str()) != leader_port)
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
}



/**
 * Guarantees that all messsages are received on the same order(based on leader timeline) on the
 * clinet side.
 */
void leader::doSequence(int time, string name, string text)
{
  utils u;
  if(current_client_stamp[name]==time)
  {
    multi_queue[name].at(0) = text;
    int i ;
    for(i = 0; multi_queue[name].at(i).empty()!=1; i++)
    {
      string content = string("NORMAL,")+ to_string(current_leader_stamp) + string(",") + name + string(",") + multi_queue[name].at(i);

      current_leader_stamp++;
      current_client_stamp[name]++;
      //print the message on the leader's board
      cout <<name<<":: "<<multi_queue[name].at(i);
      //
      for (std::map<string,string>::iterator it=group_member_info.begin(); it!=group_member_info.end(); ++it)
      {
        vector<string> split_ip_port = u.splitEx(it->second, ":");
        if(atoi(split_ip_port[1].c_str()) != leader_port)
        {
          u.send_request(string(split_ip_port[0].c_str()), atoi(split_ip_port[1].c_str()), content);
        }
      }
    }

    for(; i!=0; i--)
    {
      multi_queue[name].pop_front();
    }
    multi_queue[name].resize(10);

  }
  else if(current_client_stamp[name]<time)
  {
    int sub = time - current_client_stamp[name];
    multi_queue[name].at(sub) = text;

  }
}

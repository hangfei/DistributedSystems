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
#include <map>
#include <thread>

#include "client.h"
#include "utils.h"
#define BUFSIZE 2048

using namespace std;




/**
 * Initialized the parameters of a leader functionality.
 * If the client is elected as leader, the clients would start running
 * as a leader. The parameters would be initialized.
 */
void client::init_leader()
{
  for (std::map<string,string>::iterator it=group_member_info.begin(); it!=group_member_info.end(); ++it)
  {
    deque<string> emptyDeque (10);
    multi_queue[it->first] = emptyDeque;
    current_client_stamp[it->first] = 0;
  }
  current_leader_stamp = 0;
  my_stamp = 0;
  my_leader_stamp = 0;
}



/**
 * Initialized the parameters of a leader functionality.
 */
void client::init_client()
{
  deque<vector<string>> emptyDeque (10);
  client_side_queue = emptyDeque;
  my_stamp = 0;
  my_leader_stamp = 0;
}



/**
 * Run as a client.
 * Handles the incoming message.
 */
void client::do_client(char buf[BUFSIZE], int fd, int slen, struct sockaddr_in remaddr)
{

  string response = string(buf);
  utils u;
  vector<string> split_res = u.splitEx(response, ",");

  //handle JOININFORM
  if(split_res[0].compare(string("JOININFORM")) == 0)
  {
    cout<<"NOTICE "<<split_res[2]<<" joined on " <<string(split_res[3])<<string(":")<<string(split_res[4])<<endl;
    group_member_info[split_res[2]] = string(split_res[3]) + string(":") + string(split_res[4]);
  }

  //handle JOINREQUEST
  if(split_res[0].compare(string("JOINREQUEST")) == 0)
  {
  	//check if username exist
	int is_exist = 1;
	for (std::map<string,string>::iterator it=group_member_info.begin(); it!=group_member_info.end(); ++it)
	{
		if(it->first.compare(split_res[2]) == 0) {
	        is_exist = 0;
		}
	}
	
	if(is_exist == 1) {
		//add the new member to map
		group_member_info[split_res[2]] = string(split_res[3]) + string(":") + string(split_res[4]);

		//response for the client that requests join
		string res_for_join ("JOINRESPONSE,");
		res_for_join += to_string(my_leader_stamp) + ",";
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

		//forward the request to leader
		u.send_request(leader_ip, leader_port, response);
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

  //handle NORMAL
  if(split_res[0].compare(string("NORMAL")) == 0)
  {
    doSequence(split_res);
  }

  //handle LEADERSTATUS
  if(split_res[0].compare(string("LEADERSTATUS")) == 0)
  {
    this->leader_last_response_time = atoi(split_res[1].c_str());
  }

  //handle CLIENTLEAVE
  if(split_res[0].compare(string("CLIENTLEAVE")) == 0)
  {
    this->group_member_info.erase(split_res[2]);
    cout<< "NOTICE that" <<split_res[2] << " has crashed or leave" << endl;
  }

  //handle LEADERINFORM
  if(split_res[0].compare(string("LEADERINFORM")) == 0)
  {
    current_leader_stamp = 0;
    my_stamp = 0;
  }
}



/**
 * Handle JOINREQUEST
 */
int client::join(string server_name, int port_num, string username, string local_ip, int local_port)
{

  //Initialize parameters
  this->local_name = string(username);
  this->local_ip = string(local_ip);
  this->local_port = local_port;

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
  sprintf(buf, "JOINREQUEST,time,%s,%s,%d", username.c_str(), local_ip.c_str(), local_port);
  if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1)
  {
    perror("sendto");
    return -1;
  }

  /*Receive message from sever*/
  while(1)
  {

    //set the time out for join request
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 900000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
    {
      perror("Error");
    }
    recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);

    //if there is any response within the time out range
    if (recvlen > 0)
    {
      buf[recvlen] = 0;

      //parse response
      string response = string(buf);
      utils u;
      vector<string> split_info = u.splitEx(response, ",");

      if (split_info[0].compare(string("JOINRESPONSE")) == 0)
      {

        my_leader_stamp = atoi(split_info[1].c_str());
        
        leader_name = split_info[2];
		
		//check if username exist
		if(atoi(leader_name.c_str()) == -1) {
		    cout << "Username exists, try again, Bye!" << endl;
			exit(1);
		}
		
        leader_ip = split_info[3];
        leader_port = atoi(split_info[4].c_str());

        int member_num = atoi(split_info[5].c_str());
        for(int i = 0; i < member_num; i++)
        {
          group_member_info[split_info[6 + i*3]] = string(split_info[7 + i*3]) + string(":") + string(split_info[8 + i*3]);
        }
        //print welcome message
        cout << "Welcome joined on " << string(leader_ip) + string(":") + to_string(leader_port) <<endl;
      }


      return 1;
    }
    close(fd);
    return -1;
    //break;
  }
}



/**
 *
 */
void client::do_leader(char buf[BUFSIZE], int fd, int slen, struct sockaddr_in remaddr)
{


  //printf("received message: \"%s\"\n", buf);
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

  //handle NORMAL
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
    doLeaderSequence(time, string(name), string(text));
  }

  //handle report status
  if(strcmp(token,"CLIENTSTATUS") == 0)
  {
    //send message to each member in the map
    utils u;
    //parse reportTime;
    token = strtok(NULL, ",");
    char reportTime[80];
    strcpy(reportTime, token);
    //cout<<"reportTime:"<<string(reportTime)<<endl;
    //parse name
    token = strtok(NULL, ",");
    char name[80];
    strcpy(name, token);
    //cout<<"name"<<string(name)<<endl;
    //parse ip
    token = strtok(NULL, ",");
    char ip[80];
    strcpy(ip, token);
    //cout<<"ip"<<string(ip)<<endl;
    //parse port
    token = strtok(NULL, "");
    char port[80];
    strcpy(port, token);
    //cout<<"port"<<string(port)<<endl;
    string text("leader\n");

    int currenttime = u.getCurrentTime();
    this->group_member_last_visit[string(name)] = atoi(string(reportTime).c_str());
    string content = string("LEADERSTATUS")+string(",")+string(to_string(currenttime));//+string("\n") ;
    // send response to client
    u.send_request(string(ip), atoi(string(port).c_str()), content);
  }

}



/**
 *
 */
void client::doLeaderSequence(int time, string name, string text)
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




/**
 *
 */
void client::doSequence(vector<string> message)
{
  int leader_time_stamp = stoi(message[1]);
  if(leader_time_stamp == my_leader_stamp)
  {
    client_side_queue.at(0)=message;
    int i = 0;
    for(i = 0; client_side_queue[i].empty()!=1; i++)
    {
      my_leader_stamp++;
      cout<< client_side_queue[i][2] << ":: ";
      for(int j = 3; j < client_side_queue[i].size(); j++)
      {
        if(j != client_side_queue[i].size() - 1)
        {
          cout<< client_side_queue[i][j] << ",";
        }
        else
        {
          cout<< client_side_queue[i][j];
        }
      }
    }
    for(; i!=0; i--)
    {
      client_side_queue.pop_front();
    }
    client_side_queue.resize(10);
  }
  else if(leader_time_stamp>my_leader_stamp)
  {
    client_side_queue.at(leader_time_stamp-my_leader_stamp) = message;
  }






}

/**
 * 
 */
void client::check_is_alive()
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
 * Report and check leader status in interval.
 * 
 */
void client::report()
{
  utils u;
  while(true)
  {
    usleep(1000000);
    // Report client status
    time_t timer;
    time(&timer);  /* get current time; same as: timer = time(NULL)  */
    int currenttime = timer;

    utils u;
    string content = string("CLIENTSTATUS,") + \
                     string(to_string(currenttime)) + string(",") + \
                     string(this->local_name) + string(",") + \
                     string(this->local_ip)+ string(",") + \
                     string(to_string(this->local_port));
  }
}

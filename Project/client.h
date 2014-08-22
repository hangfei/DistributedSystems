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
#include <map>
#include <vector>
#include <deque>

using namespace std;

/**
 * This module(class) performs as a client. It receives user input, and send to
 * the leader and receives messages from leader.
 */
class client
{
  public :
    string local_name;
    string local_ip;
    int local_port;

    string leader_name = "defautl_leader";
    string leader_ip;
    int leader_port;
    int leader_last_response_time = 0;
    bool leader_alive = true;

    int my_stamp; // when update -> 0
    int my_leader_stamp; // when update -> 0
    deque<vector<string>> client_side_queue; //when update -> clear

    int current_leader_stamp; //when update -> 0
    map<string, int> current_client_stamp; //when update -> value -> 0 (leader)
    map<string, int> group_member_last_visit;
    map<string,deque<string>> multi_queue;  //when update -> init (leader)
    map<string, string> group_member_info;

    int join(string, int, string, string, int);
    void do_client(char[], int, int, struct sockaddr_in);
    void do_leader(char[], int, int, struct sockaddr_in);
    void report();
    void check_is_alive();
    void doSequence(vector<string>);
    void doLeaderSequence(int, string, string);
    void init_leader();
    void init_client();
    void check_leader_alive();
};

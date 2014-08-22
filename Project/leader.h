#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <deque>
#include <string>
#include <iostream>
#include <map>

using namespace std;

/**
 * This module implemente a leader role in a distributed system.
 * It receives messages from the clients, and send the messages to the 
 * client(the sequence is guaranteed by Sequencer). And it checks periodically 
 * if all clients are alive.
 */
class leader {

  public :
    // 
    string leader_name;
    string leader_ip;
    int leader_port;
    int current_leader_stamp;
    int my_stamp;

    // 
    map<string, string> group_member_info;
    map<string, int> group_member_last_visit;
    map<string, int> current_client_stamp;
    map<string,deque<string>> multi_queue;

    // initialize chat group information
    // each time the group members or leader is changed, init is needed
    void init_group_info(string, string, int);
    // start role as leader
    void do_leader(char[], int, int, struct sockaddr_in);
    // check if alive
    void check_is_alive();
    // sequencer
    void doSequence(int, string, string);
};

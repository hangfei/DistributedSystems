#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>

using namespace std;


/**
 * This modules handles leader election in face of leader failure or quit.
 * It uses Bully Algorithm to elect the new leader.
 */
class LeaderElection
{
public :
  string hold_election(string, int, map<string, string>);
  bool participate_election(string, string, string, string);
};

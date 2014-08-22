A Chat Room System in a Distributed Environment
==================

##Description
A chat room system. Functions including setting up chat room, join the chat room, and send and receive message. The chat room is a centralized system with leader election.
- Developed a message handling and coordination module as the basis for the chatting system
- Implemented an ordered message delivery module with sequencer on top of UDP protocol(of unordered delivery characteristic)
- Implemented leader election using Bully Algorithm in case of leader failure
- Fault Tolerance

##Project Architectrue
- Leader Module
- Client Module
- Leader Election Module
- Sequencer

####Results:


####Supporting Package:


##Installation instructions 
- Run under linux with pthread package.
- Network required.


##Where to get help
TBD



##Contribution guidelines
TBD

##Contributor list
Bofei Wang, Hangfei Lin, Di Wu

##Credits, Inspiration, Alternatives



- - - -
####Illustrations
Finite State Machine of Chat Room System
![picture alt](https://github.com/hangfei/Software-Systems/blob/master/Project/image/cis505_FSM.jpg "Title is optional")

User Interface
![picture alt](https://github.com/hangfei/Software-Systems/blob/master/Project/image/ui.png "Title is optional")

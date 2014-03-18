Software Systems
==================

Description

This course is an advanced introduction to the theory and practice of modern software systems. We will focus on fundamental concepts of distributed systems and the design principles for building large scale computational systems. The course will involve written assignments, two programming projects, a midterm, and a final examination.

Prerequisites

A good undergraduate operating systems course. Working knowledge of C/C++ programming in the Unix/Linux environment. Reading knowledge of Java.

Textbook, Readings, Web Site

The required text for the course (available from the Penn Bookstore either now or soon) is:

Andrew S. Tanenbaum and Maarten van Steen, Distributed Systems: Principles and Paradigms (2/e). Prentice Hall, 2007.
Most of the readings will be from the textbook, but we will also refer to other sources from time to time. In particular, we will study technical manuals, system source code, and research papers. The additional readings will generally be available in online form; check the course web page for URLs or pointers.

We will also draw extensively from basic concepts of computer operating systems, at the level of an undergraduate OS course (such as CIS-380). We assume you are familiar with this material already; a good reference is

A.S. Tanenbaum. Modern Operating Systems (3/e). Prentice Hall. (You can use the 2nd edition as well).
A copy of this book will be espeically helpful in the first half of the class.
An additional book may be helpful in completing the projects:

W. Richard Stevens and Stephen A. Rago. Advanced Programming in the UNIX Environment (2/e). Addison-Wesley Professional. 2005. (available online, but you probably want a printed copy).
The course web site (with the current version of this text, updated office hours, schedule, etc) can be found online (right here!) at:

http://www.crypto.com/courses/spring14/cis505/


## Processes and Threads##

####Race Condition####
  - Execution result depend on the interleaving of two programs
  - 

####Synchronization####



####Mutual Exclusion####

- Guard Critical Section
  - Possible Implementations  
    - Turning Off Interrupts
    - Use shared variables
    - Use the OS
  - Requirements
    - Safety
    - Generality
    - Deadlock Freedom
    - Starvation Freedom

####Deadlock####
- Deadlock Theory
  - The four preconditions for deadlock
- Deadlock Appraoch
  - Ignore (Ostrich Algorithm)
  - Prevention
  - Detection
  - Avoidance


####Stravation####
- Algorithms that take into account starvation
- “Fair” schedules
- Random system behavior that perturbs “bad” cycles

####Abstract Problems####
- Dining Philosopher
- Readers and Writers
- Sleeping Barber


##Abstraction##
###Top###
###Bottom###
###Middle###

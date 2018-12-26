# computer-networks-gg
Repository for Computer Networks project. 

Requirement are specified here: 
http://www.cs.put.poznan.pl/mboron/prez/zasady_projektow.pdf

We've decided to implement simple chat web app. 

# Technologies:

C/C++ with API BSD Sockets

Electron with React

# Communication protocol
1. User connects to the server, connections is given it's file descriptor. 
2. User sends his username.
3. User receives list of available users ```500;user1;user2;...;```

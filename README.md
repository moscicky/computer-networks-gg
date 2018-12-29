# computer-networks-gg
Repository for Computer Networks project. 

Requirement are specified here: 
http://www.cs.put.poznan.pl/mboron/prez/zasady_projektow.pdf

We've decided to implement simple chat web app. 

# Technologies

C/C++ with API BSD Sockets

Electron with React

# Start-up
```CmakeLists``` is provided so that the code can be easily imported (i.e. to CLion). 

Server:
1. ```cd gg-server```
2. ```g++ -pthread main.cpp -o server -std=c++11```
3. ```./server [port] [adres]```

**Note: -Wc++11-extensions used** 

Test client:

```nc [address] [port]```



# Communication protocol
1. User connects to the server, connections is given it's file descriptor. 
2. User sends his username.
3. User receives list of available users ```500;user1;user2;...;```
4. Other users receive new user's username ```200;new_user;```
5. User sends message ```receiver;message;```
6. User receives the message ```400;sender;message;```
7. User quits, others receive ```300;user_that_left;```


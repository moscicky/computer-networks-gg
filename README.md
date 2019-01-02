# computer-networks-gg :thinking:
Repository for Computer Networks project. 

Requirement are specified here: 
http://www.cs.put.poznan.pl/mboron/prez/zasady_projektow.pdf

We've decided to implement simple chat web app. 

## Technologies

C/C++ with API BSD Sockets

Electron with React

## Start-up
```CmakeLists``` is provided so that the code can be easily imported (i.e. to CLion). 

Server:
1. ```cd gg-server```
2. ```g++ -pthread main.cpp -o server -std=c++11```
3. ```./server [port] [adres]```

**Note: -Wc++11-extensions used** 

Test client:

```nc [address] [port]```


## Communication protocol
1. User connects to the server, connections is given it's file descriptor. 
2. User sends his username.
3. User receives list of available users ```500;user1;user2;...;```
4. Other users receive new user's username ```200;new_user;```
5. User sends message ```receiver;message;```
6. User receives the message ```400;sender;message;```
7. User quits, others receive ```300;user_that_left;```

# Sprawozdanie

**Repozytorium klienta dostępne [tutaj](https://github.com/szczupag/electron-react-client-for-chat-app)**

## Opis protokołu

* Uruchamiając serwer specyfikujemy jego port i adres. 
* Uruchamiając klienta specyfikujemy port i adres serwera.
* Klient po podłączeniu do serwera otrzymuje deskryptor i wysyła nazwę: ```nazwa```
* Podłączony klient otrzymuje listę podłączonych użytkowników: ```500;uzytkownik1;uzytkownik2;...;```
* Pozostali podłączeni klienci otrzymują informację o nowym użytkowniku: ```200;nowy_uzytkownik;```
* Klient wysyła wiadomość: ```adresat;wiadomosc;```
* Klient otrzymuje wiadomość: ```400;nadawca;wiadomosc;```
* Klient wychodzi, reszta klientów otrzymuje informację: ```300;odlaczony_uzytkownik;```

## Struktury projektu

```client_thread_data ```

Dane wątku obsługującego klienta. Przekazywane do nowo powstałego wątku, zawierają id deskryptora gniazda do komunikacji z nowym użytkownikiem - ```int socket_fd```.

```msg_data```

Dane dotyczące przesyłanej wiadomości. Zawierają id deskryptora gniazda odbiorcy - ``` int fd```, nazwę adresata - ```char sender[256]``` oraz wiadomość - ```char msg[256]```

## Uruchamianie projektu

Serwera zawiera plik ```CmakeLists``` umożliwiający łatwy import projektu do np. Cliona.

Serwer:

1. ```git clone https://github.com/moscicky/computer-networks-gg```
2. ```cd gg-server```
3. ```g++ -pthread main.cpp -o server -std=c++11 -Wall```
4. ```./server adres|nazwa_domenowa port``` (np. ```./server 127.0.0.1 1234 lub ./server onet.pl```)

**Uwaga: użyto rozszerzeń -Wc++11-extensions** 

Klient:

1. ```git clone https://github.com/szczupag/electron-react-client-for-chat-app```
2. ```npm install```
3. ```npm run dev```



# SIMPLE TCP GROUP CHAT IN C

## What it does

This project contains two files one simulating the server and the other simulating the client. Server accepts connection from the clients and simulates interaction between multiple clients, supporting broadcasts to every client, direct message to a specific group of clients else exclusion of a group according to the sender's preference.

## How to get it to work

> NOTE: Make sure you have make, git and gcc installed on your linux or a c compiler on your windows if you want to run this on your windows machine.

- **Open a terminal window and do the following**

- *Clone the the repository and go into the directory project.*

```shell
git clone https://github.com/Alpinine99/tcp-group-chat-in-c.git
cd tcp-group-chat-in-c
```

- *Run make to build the project.*

```shell
make
```

> You should see a similar output and addition of some object files, the server and client executable.

![output](/resources/Screenshot%20From%202026-01-10%2016-00-49.png)

- *Now you can run the server executable on  this terminal window.*

```shell
./server 8000
```

> server file takes the a port number as an argument, just make sure the port selected is free on your local machine.

- *Here I'm demonstrating running the client locally check out [this](#running-client-on-another-machine) to see how to run client on different machines. Open a separate terminal window and run the client executable.*

```shell
./client 127.0.0.1 8000
```

> Client takes the ip and port as its arguments, again make sure you use the port number you used to run your server executable. Here I used the loop back address as we are running both server and client locally.

- *Now you should see a message from the server asking you for your alias. Make sure your alias is only **alphabets** and between **5** and **15** characters else you'll receive an error message from the server nad you won't be added to the group chat. The server also logs everything that happens you can check the logs.*

- *Once you are in you can open other terminal windows and run the same client executable, make sure to use a different alias else you won't be allowed into the chat group.*

> Remember you can only run up to 10 clients simultaneously, else the server won't accept any more.

- *You can send messages normally which will be broadcasted to all client.*

- *To include or exclude an alias or aliases.*

>`@alias message`, `@alias1,alias2,alias3 message`, replace the aliases with the actual aliases and message with your message to specify who **should receive** this message.
`!@alias message`, `!@alias1,alias2,alias3 message`, replace the aliases with the actual aliases and message with your message to specify who **should not receive** this message.

- *To quit you can simple type `quit` and you'll close the connection.*

## Running client on another machine

- *Find the Server machine ip address.*

> On server machine open another terminal run: **Windows** `ipconfig`, **Linux/Unix/Termux** `ifconfig` or `ip addr show` and look for the interface that you share with the client machine and look for the line that says `inet` note down that ip address.

- Make sure you can ping each other

> From client machine run `ping 'server ip'` and make sure the address is reachable.

- Build and connect

> From client machine simply clone the project or copy it over and build it, remember copying the executable most probably won't work.

```shell
git clone https://github.com/Alpinine99/tcp-group-chat-in-c.git
cd tcp-group-chat-in-c
make
./client 'server ip' 8000
```

## Modification

- *To change some constants like the **minimum or maximum alias length, maximum number clients** you can tweak the `commons.h` file.*

> Remember this file is contains server specific language the client refer to to interpret some symbols, and so if you are running the client on another machine make sure both server and client `common.h` file similar constants else interpretation will cause undefined behavior.

- *To change the keywords like `@`, `!@` and `quit` change the preferred character or pattern from the `client_consts.h` file.*

> This is a client specific file and each client can have his own defined symbols, no interpretation issues.

- After modification rebuild the project.

```shell
make clean
make
```

## Known issues

1. The text formatting on the client side is kinda untidy, I'll fix that.
2. Server logics is not optimized to the best I'm not well conversant with the `string.h` library I'll look into that, feel free to edit.

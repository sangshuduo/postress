# postress
* compile postress
```
gcc postress.c -o postress
```

Usage: ./postress -h <hostname> -p <port> -r <rest url> -u <username> -P <password> -a <auth str> -s <command>

        -h	specify the host to connect. Default is 127.0.0.1.
        -p	specify The TCP/IP port number to use for the connection. Default is 80.
        -r	specify the RESTful API url to use.
        -u	specify the user's name to use when connecting to the server.
        -P	specify the password to use when connecting to the server.
        -a	authorization string. instead of input user's name and password
        -s	sql command to execute.

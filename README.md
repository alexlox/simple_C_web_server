# Simple C Web Server
A simple web server made in C. Only GET requests and some MIMETYPES are working.

It loads a fully functional HTML, CSS JavaScript site and renders PHP code as long as there is no POST.

Site must be in same directory as the server. Open localhost:PORT in browser to load it after you compile and start the seerver.
To compile and start:

```
gcc SWS.c -o SWS.bin
./SWS.bin
```

You can see the PORT at the begginning of code at #define. If you get a message saying that port is already used then change it. I usually used 8080 and 8081 to play with the server.

All the requests, responses and connections can be seen in the log file created by server or in the console.

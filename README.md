# UDP Ping
## Description
A simple C client/server implementation over UDP aimed to calculate latency and packet loss. Will be using this with future benchmarks I make.

## Client
The client (`ping`) supports the following arguments:

```
--dest -d => The destination host/IP to send to.
--port -p => The destination port.
--timeout -t => How much time to wait for a packet to be sent out before considering it a timeout (in microseconds).
--interval -i => After packet receive, how long to wait in microseconds.
--verbose -v => Verbose mode.
```

## Server
The server (`server`) supports the following arguments:

```
--port -p => The port to bind to.
--verbose -v => Verbose mode.
```

## Credits
* [Christian Deacon](https://github.com/gamemann)
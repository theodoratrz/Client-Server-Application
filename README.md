# System Programming Course

## Compile and Run
use `make` to compile all files. And `make clean` when finish to clean it.

Running the Server:
`./bin/Server [port] [buffersize] [threadpoolsize]`

Running the Client:
`./bin/Client [serverName] [port] [command]`

## Project Structure  
|--> bin (for executables)  
|--> build (for files created in runtime)  
|--> include (header files)  
|--> src (source files) 
makefile  
readme

## File Description
- Client: This file implements the clients of the project. It establish a connection with the given server, asks server to execute requests and waits for the responses.
- Server: This file implements the server of the project. The server is responsible for opening the connection and listen to it for requests. It consists of 3 types of threads: 1) the main, 2) the controller and 3) the worker.  
    1) the main establishes the connection and uses accept() to accept new connections. For each connection it creates a controller thread. Also, it creates threadpoolsize worker threads.
    2) the controller thread, is responsible for reading the command from the socket and do the right action. If it is an "issueJob" command it puts it in the waiting buffer. It returns the response to the client and closes the socket (if it doesn't wait for a worker to run).
    3) the worker thread, is reading the command and forks to a parent and a child process. The child process calls exec() to execute the command and redirects the stdout to a file. The parent waits for child to finish and reads the output from the log file. After reading it and sending the output to the client, it destroys it. 

## Implementation Details
The client and the server communicate by writing (send()) and reading (read())a buffer. In most cases the buffer is sufficiently big, except from when the server sends the output of "issueJob". In that case and only first we calculate the size of the contents of the file and then we make a dynamic buffer of this size. Because of the bigger size, the server first sents the length of the buffer that will follow.

The synchronization is implemented by using mutexes and condition variables for shared structures. The buffer and the concurrency are shared variables protected by 2 pairs of mutexes. The contion variables are used for letting the controllers and workers know if the buffer is empty, has jobs or it's full. When the buffer is empty the workers are blocked. When the buffer is full the controllers are blocked. 

## ADTs 
For the shake of preserving the FIFO, we needed a buffer that is like a queue, but with an extra feature: you can remove a value not only with `pop()` but from wherever in the queue. This was implemented so that we can run `stop <jobID>` successfully. The QueueList has functions such as `pop()`, `push()`, `find()`, `removeValue()`. We also make use of a compare function to make easier comparisons because of the complicated job struct.

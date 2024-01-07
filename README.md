# mympi

Reimplementing MPI from scratch in C++

Build: `g++ -std=c++11 ./mpi_process.cpp -o mpi_process -pthread && chmod a+x mpi_process`

Start a process: `./mpi_process <port>`

Connect to another: `connect [remote_port]`

Send a message: `send [remote_port]` and then input your message

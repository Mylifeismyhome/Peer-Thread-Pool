/* Tobias Staack */

# Peer-Thread-Pool
This Code-Snippet is related to Networking and can be used to avoid the princip of one thread for one socket connection.
Manage Multiplie Peer Connection using Thread Pools n Peers gets handled by one thread.

[What it handles]
- creates additional threads if needed
- moves peer pointer to another thread to reduce the amount of threads
- shutdowns threads that handles zero peers

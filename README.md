/* Tobias Staack */

# Peer-Thread-Pool
Manage Multiplie Peer Connection using Thread Poolin n Peers gets handled by 1 thread

[What it handles]
- creates additional threads if needed
- moves peer pointer to another thread to reduce the amount of threads
- shutdowns threads that handle zero peers

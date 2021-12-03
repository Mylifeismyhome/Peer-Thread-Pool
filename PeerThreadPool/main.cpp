#include "PeerPool.h"

Net::PeerPool::WorkStatus_t DoWork(int* peer)
{
	std::cout << *peer << std::endl;

	if ((rand() % (100 + 1 - 1) + 1) == 5) return Net::PeerPool::WorkStatus_t::STOP;

	return Net::PeerPool::WorkStatus_t::CONTINUE;
}

void DoSleep(DWORD time)
{
	Sleep(time);
}

int main()
{
	Net::PeerPool::PeerPool_t PeerPoolManager = Net::PeerPool::PeerPool_t();
	PeerPoolManager.set_sleep_time(500);
	PeerPoolManager.set_sleep_function(&DoSleep);
	PeerPoolManager.set_max_peers(10);

	while (true)
	{
		if (PeerPoolManager.count_peers_all() < 20)
			PeerPoolManager.add(Net::PeerPool::peerInfo_t(new int(rand()), &DoWork));

		char str[255];
		sprintf_s(str, "Peers: %llu | Threads: %llu", PeerPoolManager.count_peers_all(), PeerPoolManager.count_pools());
		SetConsoleTitleA(str);

		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}

	return 0;
}
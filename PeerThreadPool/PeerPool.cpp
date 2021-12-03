#define DEFAULT_MAX_PEER_COUNT 4

#include "PeerPool.h"

Net::PeerPool::peerInfo_t::peerInfo_t()
{
	this->peer = nullptr;
	this->fncWork = nullptr;
	this->fncCallbackOnDelete = nullptr;
}

Net::PeerPool::peerInfo_t::peerInfo_t(void* peer)
{
	this->peer = peer;
	this->fncWork = nullptr;
	this->fncCallbackOnDelete = nullptr;
}

Net::PeerPool::peerInfo_t::peerInfo_t(void* peer, WorkStatus_t(*fncWork)(void* peer))
{
	this->peer = peer;
	this->fncWork = fncWork;
	this->fncCallbackOnDelete = nullptr;
}

Net::PeerPool::PeerPool_t::PeerPool_t()
{
	peer_mutex = new std::mutex();
	peer_threadpool_mutex = new std::mutex();

	fncSleep = nullptr;
	sleep_time = 100;
	max_peers = DEFAULT_MAX_PEER_COUNT;
}

Net::PeerPool::PeerPool_t::~PeerPool_t()
{
	if (peer_mutex)
	{
		delete peer_mutex;
		peer_mutex = nullptr;
	}

	if (peer_threadpool_mutex)
	{
		delete peer_threadpool_mutex;
		peer_threadpool_mutex = nullptr;
	}
}

void Net::PeerPool::PeerPool_t::set_sleep_time(DWORD sleep_time)
{
	this->sleep_time = sleep_time;
}

void Net::PeerPool::PeerPool_t::set_sleep_function(void (*fncSleep)(DWORD time))
{
	this->fncSleep = fncSleep;
}

void Net::PeerPool::PeerPool_t::set_max_peers(size_t max_peers)
{
	this->max_peers = max_peers;
}

bool Net::PeerPool::PeerPool_t::check_more_threads_needed()
{
	for (const auto& pool : peer_threadpool)
	{
		if (pool->num_peers != max_peers)
			return false;
	}

	return true;
}

void Net::PeerPool::PeerPool_t::threapool_push(peer_threadpool_t* pool)
{
	const std::lock_guard<std::mutex> lock(*peer_threadpool_mutex);
	peer_threadpool.emplace_back(pool);
}

void Net::PeerPool::PeerPool_t::threadpool_manager(peer_threadpool_t* pool)
{
	while (true)
	{
		for (auto& peer : pool->vPeers)
		{
			if (peer == nullptr)
			{
				auto waiting_peer = this->queue_pop();
				if (waiting_peer)
				{
					peer = waiting_peer;
					pool->num_peers++;
				}
				else
				{
					continue;
				}
			}
			
			// Automaticly set to stop if no worker function is set
			WorkStatus_t ret = WorkStatus_t::STOP;
			if (peer->fncWork)
			{
				ret = (*peer->fncWork)(peer->peer);
			}

			if (ret == WorkStatus_t::STOP)
			{
				if (peer->fncCallbackOnDelete)
				{
					(*peer->fncCallbackOnDelete)();
				}

				delete peer;
				peer = nullptr;
				pool->num_peers--;
			}
			else if (ret == WorkStatus_t::CONTINUE)
			{
				const auto p = threadpool_get_free_slot_in_target_pool(pool);
				if (p)
				{
					// move peer pointer
					for (auto& target_peer : p->vPeers)
					{
						if (!target_peer)
						{
							target_peer = peer;
							p->num_peers++;

							peer = nullptr;
							pool->num_peers--;
							break;
						}
					}
				}
			}
		}

		// close this thread
		if (pool->num_peers == 0)
		{
			// erase from vector
			const std::lock_guard<std::mutex> lock(*peer_threadpool_mutex);
			for (auto it = peer_threadpool.begin(); it != peer_threadpool.end(); it++)
			{
				auto p = *it;
				// compare mem address
				if (p == pool)
				{
					peer_threadpool.erase(it);
					break;
				}
			}

			delete pool;
			pool = nullptr;
			return;
		}

		if (fncSleep)
		{
			(*fncSleep)(sleep_time);
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
		}
	}
}

void Net::PeerPool::PeerPool_t::threadpool_add()
{
	peer_threadpool_t* pool = new peer_threadpool_t();

	// add nullptr's to dynamic vector to reserve the memory
	for(size_t i = 0; i < this->max_peers; i++)
		pool->vPeers.push_back(nullptr);

	pool->num_peers = 0;

	// dispatch the thread
	std::thread(&Net::PeerPool::PeerPool_t::threadpool_manager, this, pool).detach();

	// push to vector
	threapool_push(pool);
}

Net::PeerPool::peer_threadpool_t* Net::PeerPool::PeerPool_t::threadpool_get_free_slot_in_target_pool(peer_threadpool_t* current_pool)
{
	if (!peer_queue.empty()) return nullptr;

	for (const auto& pool : peer_threadpool)
	{
		if (pool == current_pool) continue;
		for (const auto& peer : pool->vPeers)
			if (!peer) return pool;
	}

	return nullptr;
}

Net::PeerPool::peerInfo_t* Net::PeerPool::PeerPool_t::queue_pop()
{
	if (peer_queue.empty()) return nullptr;

	auto peer = peer_queue.back();

	const std::lock_guard<std::mutex> lock(*peer_mutex);
	peer_queue.pop_back();
	return peer;
}

void Net::PeerPool::PeerPool_t::add(peerInfo_t info)
{
	if (check_more_threads_needed())
		this->threadpool_add();

	const std::lock_guard<std::mutex> lock(*peer_mutex);
	peer_queue.emplace_back(new peerInfo_t(info));
}

void Net::PeerPool::PeerPool_t::add(peerInfo_t* info)
{
	if (check_more_threads_needed())
		this->threadpool_add();

	const std::lock_guard<std::mutex> lock(*peer_mutex);
	peer_queue.emplace_back(info);
}

size_t Net::PeerPool::PeerPool_t::count_peers_all()
{
	size_t peers = 0;

	for (const auto& pool : peer_threadpool)
		for (const auto& peer : pool->vPeers)
			if (peer) peers++;

	return peers;
}

size_t Net::PeerPool::PeerPool_t::count_peers(peer_threadpool_t* pool)
{
	size_t peers = 0;

		for (const auto& peer : pool->vPeers)
			if (peer) peers++;

	return peers;
}

size_t Net::PeerPool::PeerPool_t::count_pools()
{
	return peer_threadpool.size();
}
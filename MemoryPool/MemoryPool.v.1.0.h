#pragma once

#include "BaseObjectPool.h"
#include "ObjectPool.v.2.1.h"

#include <map>


class MemoryPool {
public:
	MemoryPool() {
		//poolList[3] = new ObjectPool<char[8]>();
		//poolList[4] = new ObjectPool<char[16]>();
		//poolList[5] = new ObjectPool<char[32]>();
		//poolList[6] = new ObjectPool<char[64]>();
		//poolList[7] = new ObjectPool<char[128]>();
		//poolList[8] = new ObjectPool<char[256]>();
		//poolList[9] = new ObjectPool<char[512]>();
		//poolList[10] = new ObjectPool<char[1024]>();
		//poolList[11] = new ObjectPool<char[2048]>();
		//poolList[12] = new ObjectPool<char[4096]>();
		//poolList[13] = new ObjectPool<char[8192]>();
		//poolList[14] = new ObjectPool<char[16384]>();
		//poolList[15] = new ObjectPool<char[32768]>();
		//poolList[16] = new ObjectPool<char[65536]>();
		//poolList[17] = new ObjectPool<char[131072]>();
		//poolList[18] = new ObjectPool<char[262144]>();
		//poolList[19] = new ObjectPool<char[524288]>();
		//poolList[20] = new ObjectPool<char[1048576]>();
		//poolList[21] = new ObjectPool<char[2097152]>();
		//poolList[22] = new ObjectPool<char[4194304]>();
		//poolList[23] = new ObjectPool<char[8388608]>();
		//poolList[24] = new ObjectPool<char[16777216]>();

		poolList[3] = new ObjectPool<char[1 << 3]>();
		poolList[4] = new ObjectPool<char[1 << 4]>();
		poolList[5] = new ObjectPool<char[1 << 5]>();
		poolList[6] = new ObjectPool<char[1 << 6]>();
		poolList[7] = new ObjectPool<char[1 << 7]>();
		poolList[8] = new ObjectPool<char[1 << 8]>();
		poolList[9] = new ObjectPool<char[1 << 9]>();
		poolList[10] = new ObjectPool<char[1 << 10]>();
		poolList[11] = new ObjectPool<char[1 << 11]>();
		poolList[12] = new ObjectPool<char[1 << 12]>();
		poolList[13] = new ObjectPool<char[1 << 13]>();
		poolList[14] = new ObjectPool<char[1 << 14]>();
		poolList[15] = new ObjectPool<char[1 << 15]>();
		poolList[16] = new ObjectPool<char[1 << 16]>();
		poolList[17] = new ObjectPool<char[1 << 17]>();
		poolList[18] = new ObjectPool<char[1 << 18]>();
		poolList[19] = new ObjectPool<char[1 << 19]>();
		poolList[20] = new ObjectPool<char[1 << 20]>();
		poolList[21] = new ObjectPool<char[1 << 21]>();
		poolList[22] = new ObjectPool<char[1 << 22]>();
		poolList[23] = new ObjectPool<char[1 << 23]>();
		poolList[24] = new ObjectPool<char[1 << 24]>();
	}
	~MemoryPool() {

	}

	void* alloc(int n)
	{
		int size = max(3, (int)ceil(log2(n)));
		
		return poolList[size]->Alloc();
	}

	bool free(void* pData, int sizeOfpData)
	{
		int size = max(3, (int)ceil(log2(sizeOfpData)));

		return poolList[size]->Free(pData);
	}
private:
	std::map<int, BaseObjectPool*> poolList;

};





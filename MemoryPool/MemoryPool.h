#pragma once

#if _WIN32 || _WIN64

#include <Windows.h>
#include <heapapi.h>

#ifdef MEMORY_ALLOCATION_ALIGNMENT
#undef MEMORY_ALLOCATION_ALIGNMENT 
#define MEMORY_ALLOCATION_ALIGNMENT 64
#else
#define MEMORY_ALLOCATION_ALIGNMENT 64
#endif


#if _WIN64
#define FLAG				0xFFFF000000000000
#define CHECK				0xCDCD000000000000
#define	PTRSIZEINT			__int64 

#else
#define FLAG				0x80000000
#define CHECK				0x80000000
#define	PTRSIZEINT			__int32

#endif
#endif

#if __GNUC__
#if __x86_64__ || __ppc64__
#define FLAG				0xFFFF000000000000
#define CHECK				0xCDCD000000000000
#define	PTRSIZEINT			__int64 
#else
#define FLAG				0x80000000
#define CHECK				0x80000000
#define	PTRSIZEINT			__int32
#endif
#endif



template <typename T> class ObjectPool;

#pragma pack(push, 1)
template <typename T, typename... param>
struct  memoryBlock
{
	memoryBlock(param... args)
		: Free(nullptr), data(args ...), next(nullptr) {
	}
	memoryBlock(memoryBlock* _next, memoryBlock* _free, param... args)
		: Free(_free), data(args ...), next(_next) {
	}
	~memoryBlock() {
	}

	memoryBlock* Free;
	T data;

	memoryBlock* next;
	friend class ObjectPool<T>;
	friend class MemoryPool;
};
#pragma pack(pop)

#include <iostream>
using namespace std;


class BaseObjectPool {
public:
	BaseObjectPool() {
	}

	~BaseObjectPool() {
	}

	virtual void* alloc() {
		return nullptr;
	}
	virtual bool free(void* v) {
		return true;
	}
};

template <typename T>
class ObjectPool : BaseObjectPool
{

public:
	ObjectPool()
		:maxSize(0), useSize(0), top(nullptr), FreePtr(nullptr), heap(HeapCreate(0, 0, 0)) {

	}
	ObjectPool(int _size)
		:maxSize(_size), useSize(0), top(nullptr), FreePtr(nullptr), heap(HeapCreate(0, 0, 0))
	{
		for (int i = 0; i < _size; i++)
		{
			char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>));// new memoryBlock<T>();
			memoryBlock<T>* newNode = new (allocPtr) memoryBlock<T>(top, (memoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK));

			top = newNode;
			FreePtr = newNode;
		}
	}

	virtual	~ObjectPool() {
		while (FreePtr != nullptr) {
			memoryBlock<T>* temp = FreePtr;
			if (((PTRSIZEINT)(temp->Free) & FLAG) != CHECK)
			{
				cout << "underflow occurrence" << endl;
				temp = NULL;
			}

			FreePtr = (memoryBlock<T>*)((PTRSIZEINT)temp->Free & (~FLAG));
			temp->~memoryBlock();
			HeapFree(heap, 0, temp); // exception, when underflow
		}
	}

	void init(int _size) {
		maxSize = _size;
		useSize = 0;
		top = nullptr;
		FreePtr = nullptr;

		for (int i = 0; i < _size; i++)
		{
			allocNewBlock();
			//char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>));// new memoryBlock<T>();
			//memoryBlock<T>* newNode = new (allocPtr) memoryBlock<T>(top, (memoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK));
			//top = newNode;
			//FreePtr = newNode;
		}
	}

	void clear() {
		maxSize = 0;
		useSize = 0;
		top = nullptr;
		FreePtr = nullptr;

		while (FreePtr != nullptr) {
			memoryBlock<T>* temp = FreePtr;
			if (((PTRSIZEINT)(temp->Free) & FLAG) != CHECK)
			{
				cout << "underflow occurrence" << endl;
				temp = NULL;
			}

			FreePtr = (memoryBlock<T>*)((PTRSIZEINT)temp->Free & (~FLAG));
			temp->~memoryBlock();
			HeapFree(heap, 0, temp); // exception, when underflow
		}
	}

	void* alloc() {
		return (void*)Alloc();
	}

	T* Alloc()
	{
		if (top == nullptr)
		{
			allocNewBlock();
			//char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>));
			//memoryBlock<T>* newNode = new (allocPtr) memoryBlock<T>(nullptr, (memoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK));
			//top = newNode;
			//FreePtr = newNode;

			maxSize++;
		}

		memoryBlock<T>* ret = top;
		top = (memoryBlock<T>*)ret->next;
		ret->next = (memoryBlock<T>*)this;

		useSize++;

		return (T*)&(ret->data);
	}

	bool free(void* v) {
		return Free((T*)v);
	}

	bool Free(T* pData)
	{
		memoryBlock<T>* freeNode = (memoryBlock<T>*)((char*)(pData)-sizeof(FreePtr));

		if ((PTRSIZEINT)(freeNode->next) != (PTRSIZEINT)this)
		{
			cout << "overflow || not legal pool occurrence || reduplication release memory" << endl;
			//
			//error throw
			// overFlow or invalid
			//
			*(char*)nullptr = NULL;
			return false;
		}
		if (((PTRSIZEINT)(freeNode->Free) & FLAG) != CHECK)
		{
			cout << "underflow occurrence" << endl;
			//
			//error throw
			// underFlow
			//
			*(char*)nullptr = NULL;
			return false;
		}

		freeNode->next = top;
		top = freeNode;
		useSize--;

		return true;
	}

	int	GetCapacityCount(void)
	{
		return maxSize;
	}

	int	GetUseCount(void)
	{
		return useSize;
	}

private:
	size_t maxSize;
	size_t useSize;
	memoryBlock<T>* top;
	memoryBlock<T>* FreePtr;
	HANDLE	heap;

	void allocNewBlock()
	{
		char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>));// new memoryBlock<T>();
		memoryBlock<T>* newNode = new (allocPtr) memoryBlock<T>(top, (memoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK));

		top = newNode;
		FreePtr = newNode;
	}

	friend class MemoryPool;
};

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
		
		return poolList[size]->alloc();
	}

	bool free(void* pData, int sizeOfpData)
	{
		int size = max(3, (int)ceil(log2(sizeOfpData)));

		return poolList[size]->free(pData);
	}
private:
	map<int, BaseObjectPool*> poolList;

};




#if _WIN32 || _WIN64
#if defined(_WIN64) || defined(_M_ALPHA)
#undef MEMORY_ALLOCATION_ALIGNMENT 
#define MEMORY_ALLOCATION_ALIGNMENT 16
#else
#undef MEMORY_ALLOCATION_ALIGNMENT 
#define MEMORY_ALLOCATION_ALIGNMENT 8
#endif
#endif
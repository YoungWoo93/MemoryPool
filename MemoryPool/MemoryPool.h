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

template <typename T> class MemoryPool;

#pragma pack(push, 1)
template <typename T>
struct  memoryBlock
{
	memoryBlock()
		: Free(nullptr), data(), next(nullptr), allocPtr(nullptr) {
	}
	memoryBlock(memoryBlock* _next, memoryBlock* _free, char* _allocPtr)
		: Free(_free), data(), next(_next), allocPtr(_allocPtr) {
	}
	~memoryBlock() {
	}

	memoryBlock* Free;
	T data;

	memoryBlock* next;
	char* allocPtr;
	friend class MemoryPool<T>;
};
#pragma pack(pop)

#include <iostream>
using namespace std;

template <typename T>
class MemoryPool
{

public:
	MemoryPool()
		:maxSize(0), useSize(0), top(nullptr), FreePtr(nullptr), heap(HeapCreate(0, 0, 0)) {

	}
	MemoryPool(int _size, size_t _alignSize = 1)
		:maxSize(_size), useSize(0), top(nullptr), FreePtr(nullptr), heap(HeapCreate(0, 0, 0))
	{
		if (_alignSize > 1)
		{
			for (int i = 0; i < _size; i++)
			{
				char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>) + _alignSize - 1);
				char* alignPtr = (char*)((((PTRSIZEINT)allocPtr + sizeof(FreePtr) + _alignSize - 1) & (~(_alignSize - 1))) - sizeof(FreePtr));
				memoryBlock<T>* newNode = new (alignPtr) memoryBlock<T>(top, (memoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK), allocPtr);

				top = newNode;
				FreePtr = newNode;
			}
		}
		else
		{
			for (int i = 0; i < _size; i++)
			{
				char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>));// new memoryBlock<T>();
				memoryBlock<T>* newNode = new (allocPtr) memoryBlock<T>(top, (memoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK), allocPtr);

				top = newNode;
				FreePtr = newNode;
			}
		}
	}

	virtual	~MemoryPool() {
		while (FreePtr != nullptr) {
			memoryBlock<T>* temp = FreePtr;
			char* allocPtr = temp->allocPtr;

			FreePtr = (memoryBlock<T>*)((PTRSIZEINT)temp->Free & (~FLAG));

			//cout << (char*)&(temp->data) << endl;

			temp->~memoryBlock();
			HeapFree(heap, 0, allocPtr);
		}
	}

	T* alloc()
	{
		if (top == nullptr)
		{
			char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>));
			memoryBlock<T>* newNode = new (allocPtr) memoryBlock<T>(top, (memoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK), allocPtr);

			top = newNode;
			FreePtr = newNode;

			maxSize++;
		}

		memoryBlock<T>* ret = top;
		top = (memoryBlock<T>*)ret->next;
		ret->next = (memoryBlock<T>*)(&top);

		useSize++;

		return (T*)&(ret->data);
	}

	T* alignAlloc(const size_t _alignSize)
	{
		if (top == nullptr)
		{
			char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>) + _alignSize - 1);// new memoryBlock<T>();
			//char* alignPtr = (char*)(((long long int)allocPtr + alignSize - 1) & (~(alignSize - 1))); 
			char* alignPtr = (char*)((((PTRSIZEINT)allocPtr + sizeof(FreePtr) + _alignSize - 1) & (~(_alignSize - 1))) - sizeof(FreePtr));
			memoryBlock<T>* newNode = new (alignPtr) memoryBlock<T>(top, (memoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK), allocPtr);

			//cout << "\t" << (long long int)(&(newNode->data)) % alignSize << endl;

			top = newNode;
			FreePtr = newNode;

			maxSize++;
		}

		memoryBlock<T>* ret = top;
		top = (memoryBlock<T>*)ret->next;
		ret->next = (memoryBlock<T>*)(&top);

		useSize++;

		return (T*)&(ret->data);
	}

	bool Free(T* pData)
	{
		memoryBlock<T>* freeNode = (memoryBlock<T>*)((char*)(pData)-sizeof(FreePtr));

		if ((PTRSIZEINT)(freeNode->next) != (PTRSIZEINT)&top)
		{
			//
			//error throw
			// overFlow or invalid
			//
			return false;
		}
		if (((PTRSIZEINT)(freeNode->Free) & FLAG) != CHECK)
		{
			//
			//error throw
			// overFlow or invalid
			//
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
	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.
	size_t maxSize;
	size_t useSize;
	memoryBlock<T>* top;
	memoryBlock<T>* FreePtr;
	HANDLE	heap;
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
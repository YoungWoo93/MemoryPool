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

template <typename T> class MemoryPool_notaling;

#pragma pack(push, 1)
template <typename T>
struct  memoryBlock_control // align 하지 않음, 메모리 풀에 들어가는 얘들 대부분은 어차피 포인터로 들어갈거임 (객체) 포인터가 아닌 value 자체가 들어가는걸 생각해내자
{
	memoryBlock_control()
		: Free(nullptr), data(), next(nullptr), allocPtr(nullptr) {
	}
	memoryBlock_control(memoryBlock_control* _next, memoryBlock_control* _free, char* _allocPtr)
		: Free(_free), data(), next(_next), allocPtr(_allocPtr) {
	}
	~memoryBlock_control() {
	}

	memoryBlock_control* Free;
	T data;

	memoryBlock_control* next;
	char* allocPtr;
	friend class MemoryPool_notaling<T>;
};
#pragma pack(pop)

#include <iostream>
using namespace std;

template <typename T>
class MemoryPool_notaling
{

public:
	MemoryPool_notaling()
		:maxSize(0), useSize(0), top(nullptr), FreePtr(nullptr), heap(HeapCreate(0, 0, 0)) {

	}
	MemoryPool_notaling(int _size)
		:maxSize(_size), useSize(0), top(nullptr), FreePtr(nullptr), heap(HeapCreate(0, 0, 0))
	{
		for (int i = 0; i < _size; i++)
		{
			char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock_control<T>));// new memoryBlock_control<T>();
			memoryBlock_control<T>* newNode = new (allocPtr) memoryBlock_control<T>(top, (memoryBlock_control<T>*)((PTRSIZEINT)FreePtr | CHECK), allocPtr);

			top = newNode;
			FreePtr = newNode;
		}
	}

	virtual	~MemoryPool_notaling() {
		int k = 0;
		while (FreePtr != nullptr) {
			cout << k++ << endl;
			memoryBlock_control<T>* temp = FreePtr;
			if (((PTRSIZEINT)(temp->Free) & FLAG) != CHECK)
			{
				cout << "underflow accurence" << endl;
				temp = NULL;
			}


			FreePtr = (memoryBlock_control<T>*)((PTRSIZEINT)temp->Free & (~FLAG));
			temp->~memoryBlock_control();
			HeapFree(heap, 0, temp); // exception, when underflow
		}
	}

	T* alloc()
	{
		if (top == nullptr)
		{
			char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock_control<T>));
			memoryBlock_control<T>* newNode = new (allocPtr) memoryBlock_control<T>(top, (memoryBlock_control<T>*)((PTRSIZEINT)FreePtr | CHECK), allocPtr);

			top = newNode;
			FreePtr = newNode;

			maxSize++;
		}

		memoryBlock_control<T>* ret = top;
		top = (memoryBlock_control<T>*)ret->next;
		ret->next = (memoryBlock_control<T>*)(&top);

		useSize++;

		return (T*)&(ret->data);
	}

	bool Free(T* pData)
	{
		memoryBlock_control<T>* freeNode = (memoryBlock_control<T>*)((char*)(pData)-sizeof(FreePtr));

		if ((PTRSIZEINT)(freeNode->next) != (PTRSIZEINT)&top)
		{
			cout << "overflow || not legal pool accurence" << endl;
			//
			//error throw
			// overFlow or invalid
			//
			return false;
		}
		if (((PTRSIZEINT)(freeNode->Free) & FLAG) != CHECK)
		{
			cout << "underflow accurence" << endl;
			//
			//error throw
			// underFlow
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
	memoryBlock_control<T>* top;
	memoryBlock_control<T>* FreePtr;
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
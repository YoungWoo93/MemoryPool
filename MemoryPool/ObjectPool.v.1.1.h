#pragma once


#include "BaseObjectPool.h"

#include <iostream>
#include <tuple>
using namespace std;

//
// 독립적인 heap을 사용함
// 데이터를 캐시라인 경계에 맞춤
// 오버, 언더플로우 체크함
// 마지막에 모두 회수함
// 스레드 세이프티 하지 않음
//
//
//


template <typename T>
class AlignObjectPool_single : BaseObjectPool
{

public:
	AlignObjectPool_single()
		:maxSize(0), useSize(0), top(nullptr), FreePtr(nullptr), heap(HeapCreate(0, 0, 0)) {

	}

	template <typename... param>
	AlignObjectPool_single(int _size, param... constructorArgs)
		: maxSize(_size), useSize(0), top(nullptr), FreePtr(nullptr), heap(HeapCreate(0, 0, 0))
	{
		for (int i = 0; i < _size; i++){
			allocNewBlock(constructorArgs...);
		}
	}

	virtual	~AlignObjectPool_single() {
		clear();
	}

	template <typename... param>
	void init(int _size, param... constructorArgs) {
		maxSize = _size;
		useSize = 0;
		top = nullptr;
		FreePtr = nullptr;

		for (int i = 0; i < _size; i++)
		{
			allocNewBlock(constructorArgs...);
			//char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(alignMemoryBlock<T>));// new alignMemoryBlock<T>();
			//alignMemoryBlock<T>* newNode = new (allocPtr) alignMemoryBlock<T>(top, (alignMemoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK));
			//top = newNode;
			//FreePtr = newNode;
		}
	}

	void clear() {

		while (FreePtr != nullptr) {
			alignMemoryBlock<T>* temp = FreePtr;
			if (((PTRSIZEINT)(temp->nodeLink) & FLAG) != CHECK)
			{
				cout << "underflow occurrence" << endl;
				temp = NULL;
			}

			FreePtr = (alignMemoryBlock<T>*)((PTRSIZEINT)temp->nodeLink & (~FLAG));
			temp->~alignMemoryBlock();
			HeapFree(heap, 0, temp->freePtr); // exception, when underflow
		}

		maxSize = 0;
		useSize = 0;
		top = nullptr;
		FreePtr = nullptr;
	}

	void* alloc() {
		return (void*)Alloc();
	}

	template <typename... param>
	T* Alloc(param... constructorArgs)
	{
		if (top == nullptr)
		{
			allocNewBlock(constructorArgs...);
			//char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(alignMemoryBlock<T>));
			//alignMemoryBlock<T>* newNode = new (allocPtr) alignMemoryBlock<T>(nullptr, (alignMemoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK));
			//top = newNode;
			//FreePtr = newNode;

			maxSize++;
		}

		alignMemoryBlock<T>* ret = top;
		top = (alignMemoryBlock<T>*)ret->next;
		ret->next = (alignMemoryBlock<T>*)this;

		useSize++;

		return (T*)&(ret->data);
	}

	bool free(void* v) {
		return Free((T*)v);
	}

	bool overFlowChecker(T* pData, char** ptrDuplicate)
	{
		alignMemoryBlock<T>* freeNode = (alignMemoryBlock<T>*)((char*)(pData)-sizeof(FreePtr));// -(sizeof(char) * 32));
		*ptrDuplicate = (char*)freeNode->next;

		if ((PTRSIZEINT)(freeNode->next) != (PTRSIZEINT)this)
		{
			cout << "overflow || not legal pool occurrence || reduplication release memory" << endl;
			//
			//error throw
			// overFlow or invalid
			//
			return true;
		}

		return false;
	}

	bool Free(T* pData)
	{
		alignMemoryBlock<T>* freeNode = (alignMemoryBlock<T>*)((char*)(pData)-sizeof(FreePtr));// -(sizeof(char) * 32));

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
		if (((PTRSIZEINT)(freeNode->nodeLink) & FLAG) != CHECK)
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

	size_t GetCapacityCount(void)
	{
		return maxSize;
	}

	size_t GetUseCount(void)
	{
		return useSize;
	}

	void printPool()
	{
		alignMemoryBlock<T>* curPtr = FreePtr;
		int count = 1;
		while (curPtr != nullptr) {
			alignMemoryBlock<T>* temp = curPtr;
			if (((PTRSIZEINT)(temp->nodeLink) & FLAG) != CHECK)
			{
				cout << "underflow occurrence" << endl;
				temp = NULL;
			}

			if ((PTRSIZEINT)(temp->next) == (PTRSIZEINT)this)
			{
				printf("[USING] %4d : %p\n", count++, &(temp->data));
				printf("\t aligne as %d (ptr address % 64)\n", (PTRSIZEINT)(&(temp->data)) % 64);

			}

			curPtr = (alignMemoryBlock<T>*)((PTRSIZEINT)temp->nodeLink & (~FLAG));

		}
	}
private:
	size_t maxSize;
	size_t useSize;
	alignMemoryBlock<T>* top;
	alignMemoryBlock<T>* FreePtr;
	HANDLE	heap;

	template <typename... param>
	void allocNewBlock(param... args)
	{
		int cashSize = 64;

		char* allocPtr = (char*)HeapAlloc(heap, 0, cashSize + sizeof(alignMemoryBlock<T>));// new alignMemoryBlock<T>();
		char* constructPtr = allocPtr + (((cashSize - sizeof(alignMemoryBlock<T>*)) - ((PTRSIZEINT)allocPtr % cashSize)) + cashSize) % cashSize;
		alignMemoryBlock<T>* newNode = new (constructPtr) alignMemoryBlock<T>(allocPtr, args...);
		newNode->next = top;
		newNode->nodeLink = (alignMemoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK),
		top = newNode;
		FreePtr = newNode;
	}

	friend class MemoryPool;
};

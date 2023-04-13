#pragma once


#include "BaseObjectPool.h"

#include <iostream>

using namespace std;

//
// 독립적인 heap을 사용함
// 데이터를 캐시라인 경계에 맞추지 않음
// 오버, 언더플로우 체크함
// 마지막에 모두 회수함
// 스레드 세이프티 하지 않음
//
//
//


template <typename T>
class ObjectPool_sigle : BaseObjectPool
{

public:
	ObjectPool_sigle()
		:maxSize(0), useSize(0), top(nullptr), FreePtr(nullptr), heap(HeapCreate(0, 0, 0)) {

	}
	template <typename... param>
	ObjectPool_sigle(int _size, param... constructorArgs)
		: maxSize(_size), useSize(0), top(nullptr), FreePtr(nullptr), heap(HeapCreate(0, 0, 0))
	{
		for (int i = 0; i < _size; i++){
			allocNewBlock(constructorArgs...);
		}
	}

	virtual	~ObjectPool_sigle() {
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
			//char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>));// new memoryBlock<T>();
			//memoryBlock<T>* newNode = new (allocPtr) memoryBlock<T>(top, (memoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK));
			//top = newNode;
			//FreePtr = newNode;
		}
	}

	void clear() {

		while (FreePtr != nullptr) {
			memoryBlock<T>* temp = FreePtr;
			if (((PTRSIZEINT)(temp->nodeLink) & FLAG) != CHECK)
			{
				cout << "underflow occurrence" << endl;
				temp = NULL;
			}

			FreePtr = (memoryBlock<T>*)((PTRSIZEINT)temp->nodeLink & (~FLAG));
			temp->~memoryBlock();
			HeapFree(heap, 0, temp); // exception, when underflow
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

	bool overFlowChecker(T* pData, char** ptrDuplicate)
	{
		memoryBlock<T>* freeNode = (memoryBlock<T>*)((char*)(pData)-sizeof(FreePtr));// -(sizeof(char) * 32));
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
		memoryBlock<T>* freeNode = (memoryBlock<T>*)((char*)(pData)-sizeof(FreePtr));// -(sizeof(char) * 32));

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
		memoryBlock<T>* curPtr = FreePtr;
		int count = 1;
		while (curPtr != nullptr) {
			memoryBlock<T>* temp = curPtr;
			if (((PTRSIZEINT)(temp->nodeLink) & FLAG) != CHECK)
			{
				cout << "underflow occurrence" << endl;
				temp = NULL;
			}

			if ((PTRSIZEINT)(temp->nodeLink) == (PTRSIZEINT)this)
			{
				printf("[USING] %4d : %p\n", count++, &(temp->data));
				printf("\t aligne as %d (ptr address % 64)\n", (PTRSIZEINT)(&(temp->data)) % 64);

			}

			curPtr = (memoryBlock<T>*)((PTRSIZEINT)temp->Free & (~FLAG));

		}
	}
private:
	size_t maxSize;
	size_t useSize;
	memoryBlock<T>* top;
	memoryBlock<T>* FreePtr;
	HANDLE	heap;

	template <typename... param>
	void allocNewBlock(param... args)
	{
		char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>));// new memoryBlock<T>();
		memoryBlock<T>* newNode = new (allocPtr) memoryBlock<T>(args...);
		newNode->next = top;
		newNode->nodeLink = (memoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK),
		top = newNode;
		FreePtr = newNode;
	}

	friend class MemoryPool;
};

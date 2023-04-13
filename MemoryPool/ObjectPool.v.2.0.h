#pragma once

#define		NULLKEY_CHECK(node, key)		((0xFFFF000000000000 & node) == key)	
#define		MAKE_KEY(nodePtr, seqNo)		((UINT64)seqNo << 48) | (UINT64)nodePtr
#define		MAKE_NODEPTR(key)				(0x0000FFFFFFFFFFFF & (UINT64)key)

#include "BaseObjectPool.h"

#include <iostream>

using namespace std;

//
// 독립적인 heap을 사용함
// 데이터를 캐시라인 경계에 맞추지 않음
// 오버, 언더플로우 체크함
// 마지막에 모두 회수함
// 스레드 세이프티함
//
template <typename T>
class ObjectPool : BaseObjectPool
{

public:
	ObjectPool()
		:maxSize(0), useSize(0), top(0), linkPtr(nullptr), useCount(0), heap(HeapCreate(0, 0, 0)) {

	}
	template <typename... param>
	ObjectPool(int _size, param... constructorArgs)
		: maxSize(0), useSize(_size), top(0), linkPtr(nullptr), useCount(0), heap(HeapCreate(0, 0, 0))
	{
		for (int i = 0; i < _size; i++) {
			memoryBlock<T>* newNode = newMemoryBlock(constructorArgs...);
			newNode->next = (memoryBlock<T>*)this;
			Free(newNode);
		}
	}

	virtual	~ObjectPool() {
		clear();
	}

	template <typename... param>
	void init(int _size, param... constructorArgs) {
		maxSize = 0;
		useSize = _size;
		useCount = 0;
		top = 0;
		linkPtr = nullptr;

		for (int i = 0; i < _size; i++) {
			memoryBlock<T>* newNode = newMemoryBlock(constructorArgs...);
			newNode->next = (memoryBlock<T>*)this;
			Free(newNode);
		}
	}

	void clear() {
		while (linkPtr != nullptr){
			memoryBlock<T>* temp = linkPtr;
			char* temp2;
			if (underFlowChecker(temp, &temp2))
			{
				//underFlow occurs
				char* npt = nullptr;
				*npt = 0;
			}

			linkPtr = (memoryBlock<T>*)((PTRSIZEINT)temp->nodeLink & (~FLAG));
			deleteMemoryBlock(temp);
		}
		top = 0;
		useSize = 0;
		useCount = 0;
	}

	template <typename... param>
	T* Alloc(param... constructorArgs)
	{
		UINT64 ret;
		UINT64 nextTop;
		if (top == 0){
			ret = (UINT64)newMemoryBlock(constructorArgs...);
		}
		else
		{
			/// <summary>
			/// 락프리 pop 구간
			while (true)
			{
				ret = top;
				nextTop = (UINT64)(((memoryBlock<T>*)MAKE_NODEPTR(ret))->next);
				if (InterlockedCompareExchange(&top, nextTop, ret) == ret)
				{
					break;
				}
			}
			/// </summary>

		}
		((memoryBlock<T>*)MAKE_NODEPTR(ret))->next = (memoryBlock<T>*)this;

		InterlockedIncrement(&useSize);

		if(sizeof...(constructorArgs) != 0){
			memoryBlock<T>* retNode = (memoryBlock<T>*)MAKE_NODEPTR(ret);
			retNode = new (retNode) memoryBlock<T>(constructorArgs...);

			return &retNode->data;
		}
		return (T*)&(((memoryBlock<T>*)MAKE_NODEPTR(ret))->data);
	}
	
	
	bool Free(T* pData)
	{
		memoryBlock<T>* returnedNode = (memoryBlock<T>*)((char*)(pData)-sizeof(memoryBlock<T>*));
		char* duplicate;

		if (overFlowChecker(returnedNode, &duplicate))
		{
			//cout << "overflow || not legal pool occurrence || reduplication release memory" << endl;
			//
			//error throw
			// overFlow or invalid
			//
			*(char*)nullptr = NULL;
			return false;
		}
		if (underFlowChecker(returnedNode, &duplicate))
		{
			//cout << "underflow occurrence" << endl;
			//
			//error throw
			// underFlow
			//
			*(char*)nullptr = NULL;
			return false;
		}

		return Free(returnedNode);
	}








	inline bool overFlowChecker(memoryBlock<T>* pBlock, char** ptrDuplicate)
	{

		if ((PTRSIZEINT)(pBlock->next) != (PTRSIZEINT)this)
		{
			*ptrDuplicate = (char*)(pBlock->next);
			return true;
		}

		return false;
	}
	bool overFlowChecker(T* pData, char** ptrDuplicate)
	{
		memoryBlock<T>* targetNode = (memoryBlock<T>*)((char*)(pData)-sizeof(memoryBlock<T>*));
		
		return overFlowChecker(targetNode, ptrDuplicate);
	}

	inline bool underFlowChecker(memoryBlock<T>* pBlock, char** ptrDuplicate)
	{
		if (((PTRSIZEINT)(pBlock->nodeLink) & FLAG) != CHECK)
		{
			*ptrDuplicate = (char*)pBlock->nodeLink;
			return true;
		}

		return false;
	}
	bool underFlowChecker(T* pData, char** ptrDuplicate)
	{
		memoryBlock<T>* targetNode = (memoryBlock<T>*)((char*)(pData)-sizeof(memoryBlock<T>*));

		return underFlowChecker(targetNode, ptrDuplicate);
	}


	size_t GetCapacityCount()
	{
		return maxSize;
	}
	size_t GetUseCount()
	{
		return useSize;
	}

	void printPool()
	{
		memoryBlock<T>* curPtr = linkPtr;
		int count = 1;
		while (curPtr != nullptr) {
			memoryBlock<T>* temp = curPtr;
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
			else
			{
				printf("[NOT USING] %4d : %p\t-> (%p)\n", count++, &(temp->data), temp->next);
				printf("\t aligne as %d (ptr address % 64)\n", (PTRSIZEINT)(&(temp->data)) % 64);
			}

			curPtr = (memoryBlock<T>*)((PTRSIZEINT)temp->nodeLink & (~FLAG));

		}
	}
private:
	size_t maxSize;
	size_t useSize;
	
	UINT64 top;
	UINT64 useCount;

	memoryBlock<T>* linkPtr;

	HANDLE heap;



	bool Free(memoryBlock<T>* returnedNode)
	{
		char* duplicate;

		if (overFlowChecker(returnedNode, &duplicate))
		{
			//cout << "overflow || not legal pool occurrence || reduplication release memory" << endl;
			//
			//error throw
			// overFlow or invalid
			//
			*(char*)nullptr = NULL;
			return false;
		}
		if (underFlowChecker(returnedNode, &duplicate))
		{
			//cout << "underflow occurrence" << endl;
			//
			//error throw
			// underFlow
			//
			*(char*)nullptr = NULL;
			return false;
		}


		/// <summary>
		/// 락프리 push 구간
		UINT64 returnKey = MAKE_KEY(returnedNode, InterlockedIncrement(&useCount));
		UINT64 tempTop;
		while (true)
		{
			tempTop = top;
			returnedNode->next = (memoryBlock<T>*)tempTop;
			if (InterlockedCompareExchange(&top, returnKey, tempTop) == tempTop)
				break;
		}
		InterlockedDecrement(&useSize);
		/// </summary>

		return true;
	}

	// 새로운 메모리 블록 할당 및 nodeLink 연결
	// 이후 next에 top을 연결할지 (= 할당 후 next = this 하여 메모리풀에 push(by pool::Free) 할지)
	// 혹은 next에 this를 연결할지 (= 할당 후 바로 메모리풀 외부로 반환할지)
	// 는 newMemoryBlock 함수를 사용하는 곳에서 결정 후 동작
	template <typename... param>
	memoryBlock<T>* newMemoryBlock(param... args)
	{
		char* allocPtr = (char*)HeapAlloc(heap, 0, sizeof(memoryBlock<T>));// new memoryBlock<T>();
		memoryBlock<T>* newNode = new (allocPtr) memoryBlock<T>(args...);

		//newNode->next = top;
		//newNode->freePtr = (alignMemoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK),

		/// <summary>
		/// 락프리 push 구간
		memoryBlock<T>* temp;
		while(true)
		{
			temp = linkPtr;
			newNode->nodeLink = (memoryBlock<T>*)((PTRSIZEINT)temp | CHECK);
			if (InterlockedCompareExchangePointer((void**)&linkPtr, newNode, temp) == temp) {
				break;
			}
		}
		InterlockedIncrement(&maxSize);
		/// </summary>

		return newNode;
	}

	// target MemoryBlock에 대한 연결구조를 바꾸어 주지는 않음 (호출자 측에서 연결이 끊기지 않게 전, 후처리 필요)
	bool deleteMemoryBlock(memoryBlock<T>* target)
	{
		InterlockedDecrement(&maxSize);
		target->~memoryBlock();
		HeapFree(heap, 0, target);

		return true;
	}

	friend class MemoryPool;
};




//
// 독립적인 heap을 사용함
// 데이터를 캐시라인 경계에 맞춤
// 오버, 언더플로우 체크함
// 마지막에 모두 회수함
// 스레드 세이프티함
//
template <typename T>
class AlignObjectPool : BaseObjectPool
{

public:
	AlignObjectPool()
		:maxSize(0), useSize(0), top(0), linkPtr(nullptr), useCount(0), heap(HeapCreate(0, 0, 0)) {

	}
	template <typename... param>
	AlignObjectPool(int _size, param... constructorArgs)
		: maxSize(0), useSize(_size), top(0), linkPtr(nullptr), useCount(0), heap(HeapCreate(0, 0, 0))
	{
		for (int i = 0; i < _size; i++) {
			alignMemoryBlock<T>* newNode = newMemoryBlock(constructorArgs...);
			newNode->next = (alignMemoryBlock<T>*)this;
			Free(newNode);
		}
	}

	virtual	~AlignObjectPool() {
		clear();
	}

	template <typename... param>
	void init(int _size, param... constructorArgs) {
		maxSize = 0;
		useSize = _size;
		useCount = 0;
		top = 0;
		linkPtr = nullptr;

		for (int i = 0; i < _size; i++) {
			alignMemoryBlock<T>* newNode = newMemoryBlock(constructorArgs...);
			newNode->next = (alignMemoryBlock<T>*)this;
			Free(newNode);
		}
	}

	void clear() {
		while (linkPtr != nullptr) {
			alignMemoryBlock<T>* temp = linkPtr;
			char* temp2;
			if (underFlowChecker(temp, &temp2))
			{
				//underFlow occurs
				char* npt = nullptr;
				*npt = 0;
			}

			linkPtr = (alignMemoryBlock<T>*)((PTRSIZEINT)temp->nodeLink & (~FLAG));
			deleteMemoryBlock(temp);
		}
		top = 0;
		useSize = 0;
		useCount = 0;
	}

	template <typename... param>
	T* Alloc(param... constructorArgs)
	{
		UINT64 ret;
		UINT64 nextTop;
		if (top == 0){
			ret = (UINT64)newMemoryBlock(constructorArgs...);
		}
		else
		{
			/// <summary>
			/// 락프리 pop 구간
			while (true)
			{
				ret = top;
				nextTop = (UINT64)(((alignMemoryBlock<T>*)MAKE_NODEPTR(ret))->next);
				if (InterlockedCompareExchange(&top, nextTop, ret) == ret)
				{
					break;
				}
			}
			/// </summary>

		}
		((alignMemoryBlock<T>*)MAKE_NODEPTR(ret))->next = (alignMemoryBlock<T>*)this;

		InterlockedIncrement(&useSize);
		return (T*)&(((alignMemoryBlock<T>*)MAKE_NODEPTR(ret))->data);
	}
	bool Free(T* pData)
	{
		alignMemoryBlock<T>* returnedNode = (alignMemoryBlock<T>*)((char*)(pData)-sizeof(alignMemoryBlock<T>*));
		char* duplicate;

		if (overFlowChecker(returnedNode, &duplicate))
		{
			//cout << "overflow || not legal pool occurrence || reduplication release memory" << endl;
			//
			//error throw
			// overFlow or invalid
			//
			*(char*)nullptr = NULL;
			return false;
		}
		if (underFlowChecker(returnedNode, &duplicate))
		{
			//cout << "underflow occurrence" << endl;
			//
			//error throw
			// underFlow
			//
			*(char*)nullptr = NULL;
			return false;
		}

		return Free(returnedNode);
	}








	inline bool overFlowChecker(alignMemoryBlock<T>* pBlock, char** ptrDuplicate)
	{

		if ((PTRSIZEINT)(pBlock->next) != (PTRSIZEINT)this)
		{
			*ptrDuplicate = (char*)(pBlock->next);
			return true;
		}

		return false;
	}
	bool overFlowChecker(T* pData, char** ptrDuplicate)
	{
		alignMemoryBlock<T>* targetNode = (alignMemoryBlock<T>*)((char*)(pData)-sizeof(alignMemoryBlock<T>*));

		return overFlowChecker(targetNode, ptrDuplicate);
	}

	inline bool underFlowChecker(alignMemoryBlock<T>* pBlock, char** ptrDuplicate)
	{
		if (((PTRSIZEINT)(pBlock->nodeLink) & FLAG) != CHECK)
		{
			*ptrDuplicate = (char*)pBlock->nodeLink;
			return true;
		}

		return false;
	}
	bool underFlowChecker(T* pData, char** ptrDuplicate)
	{
		alignMemoryBlock<T>* targetNode = (alignMemoryBlock<T>*)((char*)(pData)-sizeof(alignMemoryBlock<T>*));

		return underFlowChecker(targetNode, ptrDuplicate);
	}


	size_t GetCapacityCount()
	{
		return maxSize;
	}
	size_t GetUseCount()
	{
		return useSize;
	}

	void printPool()
	{
		alignMemoryBlock<T>* curPtr = linkPtr;
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
			else
			{
				printf("[NOT USING] %4d : %p\t-> (%p)\n", count++, &(temp->data), temp->next);
				printf("\t aligne as %d (ptr address % 64)\n", (PTRSIZEINT)(&(temp->data)) % 64);
			}

			curPtr = (alignMemoryBlock<T>*)((PTRSIZEINT)temp->nodeLink & (~FLAG));

		}
	}
private:
	size_t maxSize;
	size_t useSize;

	UINT64 top;
	UINT64 useCount;

	alignMemoryBlock<T>* linkPtr;

	HANDLE heap;



	bool Free(alignMemoryBlock<T>* returnedNode)
	{
		char* duplicate;

		if (overFlowChecker(returnedNode, &duplicate))
		{
			//cout << "overflow || not legal pool occurrence || reduplication release memory" << endl;
			//
			//error throw
			// overFlow or invalid
			//
			*(char*)nullptr = NULL;
			return false;
		}
		if (underFlowChecker(returnedNode, &duplicate))
		{
			//cout << "underflow occurrence" << endl;
			//
			//error throw
			// underFlow
			//
			*(char*)nullptr = NULL;
			return false;
		}


		/// <summary>
		/// 락프리 push 구간
		UINT64 returnKey = MAKE_KEY(returnedNode, InterlockedIncrement(&useCount));
		UINT64 tempTop;
		while (true)
		{
			tempTop = top;
			returnedNode->next = (alignMemoryBlock<T>*)tempTop;
			if (InterlockedCompareExchange(&top, returnKey, tempTop) == tempTop)
				break;
		}
		InterlockedDecrement(&useSize);
		/// </summary>

		return true;
	}

	// 새로운 메모리 블록 할당 및 nodeLink 연결
	// 이후 next에 top을 연결할지 (= 할당 후 next = this 하여 메모리풀에 push(by pool::Free) 할지)
	// 혹은 next에 this를 연결할지 (= 할당 후 바로 메모리풀 외부로 반환할지)
	// 는 newMemoryBlock 함수를 사용하는 곳에서 결정 후 동작
	template <typename... param>
	alignMemoryBlock<T>* newMemoryBlock(param... args)
	{
		int cashSize = 64;
		char* allocPtr = (char*)HeapAlloc(heap, 0, cashSize + sizeof(alignMemoryBlock<T>));// new alignMemoryBlock<T>();
		char* constructPtr = allocPtr + (((cashSize - sizeof(alignMemoryBlock<T>*)) - ((PTRSIZEINT)allocPtr % cashSize)) + cashSize) % cashSize;
		alignMemoryBlock<T>* newNode = new (constructPtr) alignMemoryBlock<T>(allocPtr, args...);

		//newNode->next = top;
		//newNode->freePtr = (alignMemoryBlock<T>*)((PTRSIZEINT)FreePtr | CHECK),

		/// <summary>
		/// 락프리 push 구간
		alignMemoryBlock<T>* temp;
		while (true)
		{
			temp = linkPtr;
			newNode->nodeLink = (alignMemoryBlock<T>*)((PTRSIZEINT)temp | CHECK);
			if (InterlockedCompareExchangePointer((void**)&linkPtr, newNode, temp) == temp) {
				break;
			}
		}
		InterlockedIncrement(&maxSize);
		/// </summary>

		return newNode;
	}

	// target alignMemoryBlock에 대한 연결구조를 바꾸어 주지는 않음 (호출자 측에서 연결이 끊기지 않게 전, 후처리 필요)
	bool deleteMemoryBlock(alignMemoryBlock<T>* target)
	{
		void* constructPtr = target->freePtr;
		
		InterlockedDecrement(&maxSize);
		target->~alignMemoryBlock();
		HeapFree(heap, 0, constructPtr);

		return true;
	}

	friend class MemoryPool;
};


















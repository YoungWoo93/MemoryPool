#pragma once

#define		NULLKEY_CHECK(node, key)		((0xFFFF000000000000 & node) == key)	
#define		MAKE_KEY(nodePtr, seqNo)		((UINT64)seqNo << 48) | (UINT64)nodePtr
#define		MAKE_NODEPTR(key)				(0x0000FFFFFFFFFFFF & (UINT64)key)

#include "BaseObjectPool.h"

#include <iostream>

using namespace std;

//
// �����͸� ĳ�ö��� ��迡 ������ ����
// ����, ����÷ο� üũ��
// �������� ��� ȸ����
// ������ ������Ƽ��
// ûũ���� �Ҵ���
// TLS�� subPool �ν��Ͻ��� ������ �־��ְ� �̿���
//

template <typename T>
class ObjectPool : BaseObjectPool
{

public:
	ObjectPool()
		:maxBlockCount(0), useBlockCount(0), topBlock(0), blockSize(100), linkPtr(nullptr), useCount(0)) {

	}

	ObjectPool(int chunkBlockSize, int initNodeSize)
		:maxBlockCount((initNodeSize + blockSize - 1) / blockSize), useBlockCount(0), topBlock(0), blockSize(chunkBlockSize), linkPtr(nullptr), useCount(0))
	{
		for (int i = 0; i < maxBlockCount; i++) {
			chunkBlock<T>* newChunk = new chunkBlock<T>();
			newChunk->nextChunkBlock = (chunkBlock<T>*)this;
			FreeChunk(newNode);
		}
	}

	virtual	~ObjectPool() {
		clear();
	}


	void init(int _size) {
		maxSize = 0;
		useSize = _size;
		useCount = 0;
		top = 0;
		linkPtr = nullptr;

		for (int i = 0; i < _size; i++) {
			chunkBlock<T>* newNode = newChunkBlock();
			newNode->next = (chunkBlock<T>*)this;
			FreeChunk(newNode);
		}
	}

	void clear() {
		while (linkPtr != nullptr) {
			chunkBlock<T>* temp = linkPtr;
			char* temp2;
			if (underFlowChecker(temp, &temp2))
			{
				//underFlow occurs
				char* npt = nullptr;
				*npt = 0;
			}

			linkPtr = (chunkBlock<T>*)((PTRSIZEINT)temp->nodeLink & (~FLAG));
			deletechunkBlock(temp);
		}
		top = 0;
		useSize = 0;
		useCount = 0;
	}


	T* Alloc()
	{
		UINT64 ret;
		UINT64 nextTop;

		if (top == 0) {
			ret = (UINT64)newChunkBlock();
		}
		else
		{
			/// <summary>
			/// ������ pop ����
			while (true)
			{
				ret = top;
				nextTop = (UINT64)(((chunkBlock<T>*)MAKE_NODEPTR(ret))->next);
				if (InterlockedCompareExchange(&top, nextTop, ret) == ret)
					break;
			}
			/// </summary>

		}

		chunkBlock<T>* retNode = (chunkBlock<T>*)MAKE_NODEPTR(ret);
		retNode->next = (chunkBlock<T>*)this;


		InterlockedIncrement(&useSize);

		return &retNode->data;
	}




	template <typename... param>
	T* New(param... constructorArgs)
	{
		UINT64 ret;
		UINT64 nextTop;
		if (top == 0) {
			ret = (UINT64)newChunkBlock();
		}
		else
		{
			/// <summary>
			/// ������ pop ����
			while (true)
			{
				ret = top;
				nextTop = (UINT64)(((chunkBlock<T>*)MAKE_NODEPTR(ret))->next);
				if (InterlockedCompareExchange(&top, nextTop, ret) == ret)
				{
					break;
				}
			}
			/// </summary>

		}
		((chunkBlock<T>*)MAKE_NODEPTR(ret))->next = (chunkBlock<T>*)this;

		InterlockedIncrement(&useSize);

		chunkBlock<T>* retNode = (chunkBlock<T>*)MAKE_NODEPTR(ret);
		retNode = new (retNode) chunkBlock<T>(constructorArgs...);

		return &retNode->data;

	}


	bool FreeChunk(T* pData)
	{
		chunkBlock<T>* returnedNode = (chunkBlock<T>*)((char*)(pData)-sizeof(chunkBlock<T>*));
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

		return FreeChunk(returnedNode);
	}

	bool Delete(T* pData)
	{
		chunkBlock<T>* returnedNode = (chunkBlock<T>*)((char*)(pData)-sizeof(chunkBlock<T>*));
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

		&(returnedNode->data)->~T();

		return FreeChunk(returnedNode);
	}






	inline bool overFlowChecker(chunkBlock<T>* pBlock, char** ptrDuplicate)
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
		chunkBlock<T>* targetNode = (chunkBlock<T>*)((char*)(pData)-sizeof(chunkBlock<T>*));

		return overFlowChecker(targetNode, ptrDuplicate);
	}

	inline bool underFlowChecker(chunkBlock<T>* pBlock, char** ptrDuplicate)
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
		chunkBlock<T>* targetNode = (chunkBlock<T>*)((char*)(pData)-sizeof(chunkBlock<T>*));

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
		chunkBlock<T>* curPtr = linkPtr;
		int count = 1;
		while (curPtr != nullptr) {
			chunkBlock<T>* temp = curPtr;
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

			curPtr = (chunkBlock<T>*)((PTRSIZEINT)temp->nodeLink & (~FLAG));

		}
	}
private:
	size_t maxBlockCount;
	size_t useBlockCount;

	UINT64 topBlock;
	UINT64 blockSize;

	chunkBlock<T>* linkPtr;
	int useCount;


	bool FreeChunk(chunkBlock<T>* returnedNode)
	{
		/// <summary>
		/// ������ push ����
		UINT64 returnKey = MAKE_KEY(returnedNode, InterlockedIncrement(&useCount));
		UINT64 tempTop;
		while (true)
		{
			tempTop = top;
			returnedNode->next = (chunkBlock<T>*)tempTop;
			if (InterlockedCompareExchange(&top, returnKey, tempTop) == tempTop)
				break;
		}
		InterlockedDecrement(&useSize);
		/// </summary>

		return true;
	}

	// ���ο� �޸� ��� �Ҵ� �� nodeLink ����
	// ���� next�� top�� �������� (= �Ҵ� �� next = this �Ͽ� �޸�Ǯ�� push(by pool::FreeChunk) ����)
	// Ȥ�� next�� this�� �������� (= �Ҵ� �� �ٷ� �޸�Ǯ �ܺη� ��ȯ����)
	// �� newChunkBlock �Լ��� ����ϴ� ������ ���� �� ����

	chunkBlock<T>* newChunkBlock()
	{
		chunkBlock<T>* newNode = (chunkBlock<T>*)HeapAlloc(heap, 0, sizeof(chunkBlock<T>));// new chunkBlock<T>();

		//newNode->next = top;
		//newNode->freePtr = (alignchunkBlock<T>*)((PTRSIZEINT)FreeChunkPtr | CHECK),

		/// <summary>
		/// ������ push ����
		chunkBlock<T>* temp;
		while (true)
		{
			temp = linkPtr;
			newNode->nodeLink = (chunkBlock<T>*)((PTRSIZEINT)temp | CHECK);
			if (InterlockedCompareExchangePointer((void**)&linkPtr, newNode, temp) == temp) {
				break;
			}
		}
		InterlockedIncrement(&maxSize);
		/// </summary>

		return newNode;
	}

	// target chunkBlock�� ���� ���ᱸ���� �ٲپ� ������ ���� (ȣ���� ������ ������ ������ �ʰ� ��, ��ó�� �ʿ�)
	bool deletechunkBlock(chunkBlock<T>* target)
	{
		InterlockedDecrement(&maxSize);
		HeapFreeChunk(heap, 0, target);

		return true;
	}

	friend class MemoryPool;
};






















#pragma once

#define		NULLKEY_CHECK(node, key)		((0xFFFF000000000000 & node) == key)	
#define		MAKE_KEY(nodePtr, seqNo)		((UINT64)seqNo << 48) | (UINT64)nodePtr
#define		MAKE_NODEPTR(key)				(0x0000FFFFFFFFFFFF & (UINT64)key)

#define		_V_3_

#include "BaseObjectPool.h"

#include <unordered_map>
#include <iostream>
#include <Windows.h>
using namespace std;

//
// 데이터를 캐시라인 경계에 맞추지 않음
// 오버, 언더플로우 체크함
// 마지막에 모두 회수함
// 스레드 세이프티함
// 청크단위 할당함
// TLS에 subPool 인스턴스를 생성해 넣어주고 이용함
//

class TLSPoolBase;
template <typename T>
class TLSPool;

extern __declspec(thread) std::unordered_map<void*, std::shared_ptr<TLSPoolBase>> TLSPoolMap;

template <typename T>
class chunkBlockStack
{
public:
	chunkBlockStack(int _size) :chunkBlockStackTop(nullptr), chunkBlockStackSize(0), chunkBlockStackUniqeCounter(0), chunkBlockSize(_size) {
	}

	void push(chunkBlock<T>* emptyBlock) {
		//n = emptyBlock
		//compareKey = tempTop;
		//pushKey = newTop;
		//topKey = chunkBlockStackTop;
		chunkBlock<T>* compareKey = 0;
		chunkBlock<T>* pushKey = (chunkBlock<T>*)(MAKE_KEY(emptyBlock, InterlockedIncrement(&chunkBlockStackUniqeCounter)));
		for (;;)
		{
			compareKey = chunkBlockStackTop;
			emptyBlock->nextChunkBlock = compareKey;

			if (InterlockedCompareExchangePointer((PVOID*) &chunkBlockStackTop, pushKey, compareKey) == compareKey)
				break;
		}

		InterlockedIncrement(&chunkBlockStackSize);
	}

	chunkBlock<T>* pop() {
		chunkBlock<T>* ret;
		
		chunkBlock<T>* compareKey;
		chunkBlock<T>* nextTopKey;
		//compareKey = tempTop;
		//pushKey = newTop;
		//topKey = chunkBlockStackTop;
		//nextTopKey = nextTop
		// popNode	= ret;
		if ((int)InterlockedDecrement(&chunkBlockStackSize) < 0) {
			InterlockedIncrement(&chunkBlockStackSize);
			ret = new chunkBlock<T>();
			ret->maxSize = chunkBlockSize;
		}
		else
		{
			for (;;)
			{
				compareKey = chunkBlockStackTop;
				ret = (chunkBlock<T>*)MAKE_NODEPTR(compareKey);
				nextTopKey = ret->nextChunkBlock;

				if (InterlockedCompareExchangePointer((PVOID*) &chunkBlockStackTop, nextTopKey, compareKey) == compareKey)
					break;
			}
		}

		return ret;
	}

	chunkBlock<T>* chunkBlockStackTop;	// 인터락
	unsigned int chunkBlockStackSize;			// 인터락
	unsigned int chunkBlockStackUniqeCounter;	// 인터락
	unsigned int chunkBlockSize;
};

template <typename T>
class ObjectPool : public BaseObjectPool
{
public:
	ObjectPool()
		:maxBlockCount(0), useBlockCount(0), topBlock(0), blockSize(100), linkPtr(nullptr), useCount(0), s(100)
		{

	}

	ObjectPool(int initNodeSize, int chunkBlockSize = 100)
		:maxBlockCount((initNodeSize + chunkBlockSize - 1) / chunkBlockSize), useBlockCount(maxBlockCount), topBlock(0), blockSize(chunkBlockSize), linkPtr(nullptr), useCount(0), s(chunkBlockSize)
	{
		for (int i = 0; i < maxBlockCount; i++) {
			chunkBlock<T>* newChunk = new chunkBlock<T>(chunkBlockSize);
			newChunk->nextChunkBlock = (chunkBlock<T>*)this;
			FreeChunk(newChunk);
		}
	}

	virtual	~ObjectPool() {
	}

	int getCurrentThreadUseCount()
	{
		unordered_map<void*, shared_ptr<TLSPoolBase>>::iterator it = TLSPoolMap.find(this);
		if (it == TLSPoolMap.end()) {
			TLSPoolMap[this] = make_shared<TLSPool<T>>(this);
			it = TLSPoolMap.find(this);
		}

		return it->second.get()->getUseSize();
	}
	int getCurrentThreadMaxCount()
	{
		unordered_map<void*, shared_ptr<TLSPoolBase>>::iterator it = TLSPoolMap.find(this);
		if (it == TLSPoolMap.end()) {
			TLSPoolMap[this] = make_shared<TLSPool<T>>(this);
			it = TLSPoolMap.find(this);
		}

		return it->second.get()->getMaxSize();
	}

	T* Alloc()
	{
		unordered_map<void*, shared_ptr<TLSPoolBase>>::iterator it = TLSPoolMap.find(this);
		if (it == TLSPoolMap.end()) {
			TLSPoolMap[this] = make_shared<TLSPool<T>>(this);
			it = TLSPoolMap.find(this);
		}
		
		return (T*)(it->second.get()->subPop());
	}

	void Free(T* _value)
	{
		unordered_map<void*, shared_ptr<TLSPoolBase>>::iterator it = TLSPoolMap.find(this);
		if (it == TLSPoolMap.end()) {
			TLSPoolMap[this] = make_shared<TLSPool<T>>(this);
			it = TLSPoolMap.find(this);
		}

		it->second.get()->subPush(_value);
	}

	template <typename... param>
	T* New(param... constructorArgs)
	{
		unordered_map<void*, shared_ptr<TLSPoolBase>>::iterator it = TLSPoolMap.find(this);
		if (it == TLSPoolMap.end()) {
			TLSPoolMap[this] = make_shared<TLSPool<T>>(this);
			it = TLSPoolMap.find(this);
		}

		T* ret = (T*)(it->second.get()->subPop());
		ret = new (ret) T(constructorArgs...);

		return ret;
	}

	void Delete(T* _value)
	{
		unordered_map<void*, shared_ptr<TLSPoolBase>>::iterator it = TLSPoolMap.find(this);
		if (it == TLSPoolMap.end()) {
			TLSPoolMap[this] = make_shared<TLSPool<T>>(this);
			it = TLSPoolMap.find(this);
		}

		_value->~T();
		it->second.get()->subPush(_value);
	}

	bool FreeChunk(chunkBlock<T>* returnedBlock)
	{
		/// <summary>
		/// 락프리 push 구간
		returnedBlock->lastUseTick = GetTickCount64();
		UINT64 returnKey = MAKE_KEY(returnedBlock, InterlockedIncrement(&useCount));
		chunkBlock<T>* tempTop;

		while (true)
		{
			tempTop = topBlock;
			returnedBlock->nextChunkBlock = (chunkBlock<T>*)tempTop;
			if (InterlockedCompareExchangePointer((PVOID*) &topBlock, (chunkBlock<T>*)returnKey, tempTop) == tempTop)
				break;
		}
		InterlockedDecrement(&useBlockCount);
		/// </summary>

		return true;
	}

	bool FreeChunk(memoryBlock<T>* _head, memoryBlock<T>* _tail, int size)
	{
		if (size == blockSize)
		{
			chunkBlock<T>* chunk = s.pop();
			chunk->curSize = size;
			chunk->headNodePtr = _head;
			chunk->tailNodePtr = _tail;

			FreeChunk(chunk);
		}
		else
		{
			AcquireSRWLockExclusive(&recycleLock);
			recycleChunk->tailNodePtr->next = _head;
			recycleChunk->tailNodePtr = _tail;
			recycleChunk->curSize += size;

			if (recycleChunk->curSize >= blockSize)
			{
				chunkBlock<T>* chunk = s.pop();
				int offset = recycleChunk->curSize - blockSize;

				if (offset > 0)
				{
					chunk->headNodePtr = recycleChunk->headNodePtr;
					memoryBlock<T>* cur = recycleChunk->headNodePtr;

					for (int i = 1; i < offset; i++)
						cur = cur->next;
					
					recycleChunk->headNodePtr = cur->next;
					recycleChunk->curSize -= offset;
					chunk->curSize = offset;
					chunk->tailNodePtr = cur;


				}
				
				FreeChunk(recycleChunk);
				recycleChunk = chunk;
			}
			ReleaseSRWLockExclusive(&recycleLock);
		}
		return true;
	}

	void AllocChunk(memoryBlock<T>** _head, memoryBlock<T>** _tail)
	{
		chunkBlock<T>* popBlock;
		if (topBlock == nullptr)
		{
			popBlock = newChunkBlock();
		}
		else
		{
			while (true)
			{
				popBlock = topBlock;
				chunkBlock<T>* nextTop = ((chunkBlock<T>*)MAKE_NODEPTR(popBlock))->nextChunkBlock;

				if (InterlockedCompareExchangePointer((PVOID*)&topBlock, nextTop, popBlock) == popBlock)
					break;
			}
		}
		
		popBlock = ((chunkBlock<T>*)MAKE_NODEPTR(popBlock));
		popBlock->curSize = 0;
		*_head = popBlock->headNodePtr;
		*_tail = popBlock->tailNodePtr;

		InterlockedIncrement(&useBlockCount);
		s.push(popBlock);
	}


	chunkBlock<T>* newChunkBlock()
	{
		chunkBlock<T>* newChunk = s.pop();
		newChunk->reAlloc();

		InterlockedIncrement(&maxBlockCount);

		return newChunk;
	}

	bool deleteChunkBlock(chunkBlock<T>* target)
	{
		InterlockedDecrement(&maxBlockCount);
		delete target;

		return true;
	}

	int GetUseCount()
	{
		return useBlockCount;
	}
	int GetCapacityCount()
	{
		return maxBlockCount;
	}
	size_t maxBlockCount;
	size_t useBlockCount;

	chunkBlock<T>* topBlock;
	UINT64 blockSize;

	chunkBlock<T>* linkPtr;
	chunkBlock<T>* recycleChunk;
	SRWLOCK recycleLock;

	UINT64 useCount;

	chunkBlockStack<T> s;
	//friend class MemoryPool;
};



class TLSPoolBase {
public:
	TLSPoolBase() {};
	virtual ~TLSPoolBase() {};
	virtual void subPush(void* value) = 0;
	virtual void* subPop() = 0;
	virtual int getUseSize() = 0;
	virtual int getMaxSize() = 0;
};

template <typename T>
class TLSPool :public TLSPoolBase {
public:
	TLSPool(ObjectPool<T>* _mainPool) {
		mainPoolPtr = _mainPool;
		chunkBlockSize = _mainPool->blockSize;
		releaseThreashold = chunkBlockSize * 2 + 1;
		nextTailPtr = nullptr;


		_mainPool->AllocChunk(&headPtr, &tailPtr);
		curNodeSize = chunkBlockSize;
	}
	~TLSPool() {
		while (curNodeSize > chunkBlockSize)
		{
			mainPoolPtr->FreeChunk(nextTailPtr->next, tailPtr, chunkBlockSize);
			tailPtr = nextTailPtr;
			curNodeSize -= chunkBlockSize;
		}

		mainPoolPtr->FreeChunk(headPtr, tailPtr, curNodeSize);
	}
	void subPush(void* value)
	{
		memoryBlock<T>* pushNode = (memoryBlock<T>*)((char*)value - 8);
		// 오버플로 언더플로 체크
		//
		//

		if (curNodeSize == chunkBlockSize)
			nextTailPtr = pushNode;
		
		pushNode->next = headPtr;
		headPtr = pushNode;
		++curNodeSize;

		if (curNodeSize == releaseThreashold)
		{
			mainPoolPtr->FreeChunk(nextTailPtr->next, tailPtr, chunkBlockSize);
			tailPtr = nextTailPtr;
			nextTailPtr = pushNode;
			curNodeSize -= chunkBlockSize;
		}
	}

	void* subPop()
	{
		memoryBlock<T>* popNode = headPtr;
		headPtr = headPtr->next;
		--curNodeSize;
		if (curNodeSize < chunkBlockSize)
		{
			nextTailPtr = tailPtr;
			mainPoolPtr->AllocChunk(&nextTailPtr->next, &tailPtr);
			
			curNodeSize += chunkBlockSize;
		}

		return &popNode->data;
	}

	int getUseSize()
	{
		return curNodeSize;
	}

	int getMaxSize() 
	{
		return releaseThreashold;
	}
private:
	ObjectPool<T>* mainPoolPtr;
	int curNodeSize;
	int chunkBlockSize;
	int releaseThreashold;
	memoryBlock<T>* tailPtr;
	memoryBlock<T>* nextTailPtr;
	memoryBlock<T>* headPtr;
};

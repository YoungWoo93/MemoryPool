#pragma once


#include <Windows.h>
#include <heapapi.h>

#if _WIN64 || __x86_64__ || __ppc64__
#define FLAG				0xFFFF000000000000
#define CHECK				0xCDCD000000000000
#define	PTRSIZEINT			__int64 

#else
#define FLAG				0x80000000
#define CHECK				0x80000000
#define	PTRSIZEINT			__int32
#endif

//template <typename T> class ObjectPool;


class BaseObjectPool {
public:
	BaseObjectPool() {
	}

	~BaseObjectPool() {
	}

	//virtual void* Alloc() = 0;
	//virtual bool Free(void* v) = 0;
};

#pragma pack(push, 1)
template <typename T>
struct  memoryBlock
{
	memoryBlock() {
	}
	//template <typename... param>
	//memoryBlock(memoryBlock* _next, param... args)
	//	: nodeLink(nullptr), data(args ...), next(_next) {
	//}
	~memoryBlock() {
	}
	// 본인의 pool에서 생성된 노드를 일렬로 가지고 있는 연결리스트, 새로 생성된 노드는 pool 내의 linkPtr 값을 nodeLink로 저장하고, 새로 생성된 노드의 주소를 pool::linkPtr에 저장한다.
	memoryBlock* nodeLink;
	T data;
	// 외부에 나가있는 노드는 본인의 pool 주소를, 내부에 있는 노드는 자신이 할당 된 뒤의 top이 될 노드의 주소를 가지고있음
	memoryBlock* next; 
};
#pragma pack(pop)

template <typename T>
struct chunkBlock
{
	chunkBlock()
		: headNodePtr(nullptr), tailNodePtr(nullptr), nextChunkBlock(nullptr), lastUseTick(GetTickCount64()), maxSize(0), curSize(0) {
	}

	chunkBlock(int chunkSize)
		: headNodePtr(nullptr), tailNodePtr(nullptr), nextChunkBlock(nullptr), lastUseTick(GetTickCount64()), maxSize(chunkSize), curSize(chunkSize){
		
		headNodePtr = new memoryBlock<T>();
		tailNodePtr = headNodePtr;
		tailNodePtr->next = nullptr;
		
		while (--chunkSize > 0) 
		{
			memoryBlock<T>* temp = new memoryBlock<T>();
			temp->next = headNodePtr;
			headNodePtr = temp;
		}
	}
	~chunkBlock() {
		while (curSize-- > 0)
		{
			memoryBlock<T>* temp = headNodePtr;
			headNodePtr = headNodePtr->next;
			delete temp;
		}
	}

	void reAlloc() {
		int offset = maxSize - curSize;

		if (curSize == 0){
			headNodePtr = new memoryBlock<T>();
			tailNodePtr = headNodePtr;
			offset -= 1;
			curSize += 1;
		}

		while(offset-- > 0)
		{
			memoryBlock<T>* temp = new memoryBlock<T>();
			temp->next = headNodePtr;
			headNodePtr = temp;

			++curSize;
		}
	}
	memoryBlock<T>*			headNodePtr;
	memoryBlock<T>*			tailNodePtr;
	chunkBlock*				nextChunkBlock;
	unsigned long long int	lastUseTick;
	int						maxSize;
	int						curSize;
};

#pragma pack(push, 1)
template <typename T>
struct alignMemoryBlock
{
	alignMemoryBlock(void* _freePtr)
		: freePtr(_freePtr), nodeLink(nullptr), next(nullptr) {
	}

	template <typename... param>
	alignMemoryBlock(void* _freePtr, param... args)
		: freePtr(_freePtr), nodeLink(nullptr), data(args ...), next(nullptr) {
	}
	//template <typename... param>
	//alignMemoryBlock(void* _freePtr, alignMemoryBlock* _next, param... args)
	//	: freePtr(_freePtr), data(args ...), next(nullptr) {
	//}

	~alignMemoryBlock() {
	}

	// 본인의 pool에서 생성된 노드를 일렬로 가지고 있는 연결리스트, 새로 생성된 노드는 pool 내의 linkPtr 값을 nodeLink로 저장하고, 새로 생성된 노드의 주소를 pool::linkPtr에 저장한다.
	alignMemoryBlock* nodeLink;
	T data;
	// 외부에 나가있는 노드는 본인의 pool 주소를, 내부에 있는 노드는 자신이 할당 된 뒤의 top이 될 노드의 주소를 가지고있음
	alignMemoryBlock* next;
	// align 버전은 T data의 시작 주소를 캐시라인에 맞추기 때문에 실제 메모리 블록이 할당된 주소가 T data의 주소 - sizeof(alignMemoryBlock*) 가 아니다. 어느정도의 패딩이 존재하는지 알 수 없기 때문에 실제 메모리를 free 할때 사용할 freePtr을 별도로 둔다.
	void* freePtr;
};
#pragma pack(pop)
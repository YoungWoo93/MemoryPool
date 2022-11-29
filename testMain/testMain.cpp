#ifdef _DEBUG
#pragma comment(lib, "MemoryPoolD")
#pragma comment(lib, "ProfilerD")
#pragma comment(lib, "SerializerD")

#else
#pragma comment(lib, "MemoryPool")
#pragma comment(lib, "Profiler")
#pragma comment(lib, "Serializer")

#endif


#include "MemoryPool_notaling.h"

#include "../MemoryPool/MemoryPool.h"
#include "Profiler/Profiler/Profiler.h"
#include "Serializer/Serializer/Serializer.h"

#include <iostream>
#include <vector>
#define _USE_PROFILE_

using namespace std;
#define testSize	1000

#include <cmath>

struct sample {
	char s[64];
};
MemoryPool memorypool;
ObjectPool<sample> mp(testSize);
ObjectPool<sample> mp_control(testSize);

vector<sample*> arr(testSize);
vector<sample*> arr1(testSize);
vector<sample*> arr2(testSize);
void basic_test();
void recycle_test();
void fastRecycle_test();
void notRegal_test();
void overFlow_test();
void perfomence_test_alloc();
void perfomence_test_use();
void hjTest();

void genericMemoryPoolAllocTest();
void genericMemoryPooloverflowTest();
void genericMemoryPoolunderflowTest();

void serializerHeapPageTest();

void main()
{
	//genericMemoryPoolAllocTest();

	//basic_test();
	//recycle_test();
	//notRegal_test();
	//overFlow_test();
	fastRecycle_test();

	//perfomence_test_alloc();
	//perfomence_test_use();

	//serializerHeapPageTest();
}

void genericMemoryPoolAllocTest()
{
	for (int i = 1; i < 10000; i++)
	{
		auto p = memorypool.alloc(i);
		memorypool.free(p, i);
	}
	return;
}

void genericMemoryPooloverflowTest()
{
	for (int i = 2; i < 10000; i*=2)
	{
		auto p = memorypool.alloc(i);
		*((char*)p + i * 2) = 'a';
		memorypool.free(p, i);
	}
	return;
}
void genericMemoryPoolunderflowTest()
{
	for (int i = 2; i < 10000; i *= 2)
	{
		auto p = memorypool.alloc(i);
		*((char*)p - 1) = 'a';
		memorypool.free(p, i);
	}
	return;
}


void basic_test()
{
	int count = 100;
	ObjectPool<sample> test(0);

	for (int i = 0; i < count; i++)
	{
		sample* t = test.Alloc();
		_itoa_s(i, t->s, 10);

		cout << t->s << endl;
		test.free(t);
	}


	cout << "==========" << endl;
}

void recycle_test()
{
	ObjectPool<int> test(3);

	int* t1 = test.Alloc();
	*t1 = 1;
	cout << *t1 << "\t" << t1 << endl;

	/*/
	int* t2 = test.Alloc();
	*t2 = 2;
	cout << *t2 << "\t" << t2 << endl;


	int* t3 = test.Alloc();
	*t3 = 3;
	cout << *t3 << "\t" << t3 << endl;


	test.Free(t2);
	/*/
	test.Free(t1);

	t1 = test.Alloc();
	*t1 = 1;
	cout << *t1 << "\t" << t1 << endl;
	test.Free(t1);

	t1 = test.Alloc();
	*t1 = 1;
	cout << *t1 << "\t" << t1 << endl;
	test.Free(t1);

	t1 = test.Alloc();
	*t1 = 1;
	cout << *t1 << "\t" << t1 << endl;
	test.Free(t1);

	int* t5 = test.Alloc();
	*t5 = 1;
	cout << *t5 << "\t" << t5 << endl;

	int* t4 = test.Alloc();
	*t4 = 2;
	cout << *t4 << "\t" << t4 << endl;

	test.clear();
	test.init(10);

	int* t6 = test.Alloc();
	*t6 = 1;
	cout << *t6 << "\t" << t6 << endl;


	int* t7 = test.Alloc();
	*t7 = 2;
	cout << *t7 << "\t" << t7 << endl;


	int* t8 = test.Alloc();
	*t8 = 3;
	cout << *t8 << "\t" << t8 << endl;


}

#include <stack>
void fastRecycle_test() {
	ObjectPool<int> pool(10);
	std::stack<int*> s;
	while (true) {
		printf("%d\t/\t%d\n", pool.GetUseCount(), pool.GetCapacityCount());

		int a = rand() % 10;

		for (int i = 0; i < a; i++) {
			s.push(pool.Alloc());
			*s.top() = i;
		}

		if (pool.GetUseCount() <= 1)
			continue;

		int b = rand() % (pool.GetUseCount() / 2);
		for (int i = 0; i < b; i++) {
			pool.Free(s.top());
			s.pop();
		}
	}
}

struct testSTRUCT {
	char* test[2];
};
void notRegal_test()
{
	ObjectPool<testSTRUCT> test(3);
	ObjectPool<testSTRUCT> test2(3);

	testSTRUCT* a1 = test.Alloc();
	testSTRUCT* a2 = test.Alloc();
	testSTRUCT* a3 = test2.Alloc();


	test.Free(a3);
}

void overFlow_test()
{
	ObjectPool<testSTRUCT> test(3);

	testSTRUCT* a1 = test.Alloc();
	a1->test[0] = (char*)ULLONG_MAX;
	a1->test[1] = (char*)ULLONG_MAX;
	a1->test[2] = (char*)ULLONG_MAX;
	a1->test[3] = (char*)ULLONG_MAX;

	test.Free(a1);
}


#include <windows.h>

void perfomence_test_use()
{
	
	for (int i = 0; i < 100; i++)
	{
		/*/
		cout << i << " / " << 100 << endl;

		for (int a = 1; a <= 64; a *= 2)
		{
			ObjectPool<sample> mpt(testSize, a);

			{
				//scopeProfiler s("Alloc memory pool aling " + to_string(a));
				for (int j = 0; j < testSize; j++)
				{
					arr1[j] = mpt.alignAlloc(a);
				}
			}

			{
				scopeProfiler s("use memory pool aling " + to_string(a));

				for (int c = 0; c < 100; c++)
				{
					for (int j = 0; j < testSize; j++)
					{
						memset(arr1[j]->s, 1, sizeof(arr1[j]->s));
					}
					for (int j = 0; j < testSize; j++)
					{
						memset(arr1[j]->s, 0, sizeof(arr1[j]->s));
					}
				}
			}
			
			{
				//scopeProfiler s("free memory pool " + to_string(a));
				for (int j = 0; j < testSize; j++)
				{
					mpt.free(arr1[j]);
				}
			}
		}
		/*/

		{
			scopeProfiler s("Alloc memory pool control");
			for (int j = 0; j < testSize; j++)
			{
				arr2[j] = mp_control.Alloc();
			}
			//*((char*)arr2[1] - 1) = 'a';
			//*((char*)arr2[1] + sizeof(arr2[1]->s) + 1) = 'a';
			//return;
		}

		{
			scopeProfiler s("use memory pool control");
			for (int c = 0; c < 100; c++)
			{
				for (int j = 0; j < testSize; j++)
				{
					memset(arr2[j]->s, 1, sizeof(arr2[j]->s));
				}
				for (int j = 0; j < testSize; j++)
				{
					memset(arr2[j]->s, 0, sizeof(arr2[j]->s));
				}
			}
		}
		
		{
			for (int j = 0; j < testSize; j++)
			{
				mp_control.Free(arr2[j]);
			}
		}
		
	}
	cout << "end: " << endl;
}


void perfomence_test_alloc()
{
	cout << "nt: " << endl;
	for (int i = 0; i < 1000; i++)
	{
		int c = 0;
		scopeProfiler s("do notting");

		for (int j = 0; j < testSize; j++)
			c = 0;
		for (int j = 0; j < testSize; j++)
			c = 0;
	}

	cout << "mp1: " << endl;
	for (int i = 0; i < 1000; i++)
	{
		scopeProfiler s("use memory pool 1");

		for (int j = 0; j < testSize; j++)
			arr[j] = mp.Alloc();

		for (int j = 0; j < testSize; j++)
			mp.Free(arr[j]);

	}

	cout << "nd: " << endl;
	for (int i = 0; i < 1000; i++)
	{
		scopeProfiler s("use stl Alloc");

		for (int j = 0; j < testSize; j++)
			arr[j] = new sample();

		for (int j = 0; j < testSize; j++)
			delete arr[j];
	}
}


#include <map>

void serializerHeapPageTest()
{
	std::map<UINT64, UINT64> m;
	HANDLE h = HeapCreate(0, 0, 0);
	ObjectPool<serializer> so(testSize);
	serializer* arr[testSize];
	for (int i = 0; i < testSize; i++) {
		arr[i] = so.Alloc();

		UINT64 index = ((unsigned long long int)(arr[i]->getBufferPtr())) / (4 * 1024);
		if (m.find(index) == m.end())
			m[index] = 1;
		else
			m[index]++;
	}

	for (auto it : m) {
		printf("%ld : %ld\n", it.first, it.second);
	}
}
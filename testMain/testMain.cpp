#include "MemoryPool_notaling.h"

#include "../MemoryPool/MemoryPool.h"
#pragma comment(lib, "MemoryPool")

#include "Profiler/Profiler/Profiler.h"
#pragma comment(lib, "Profiler")

#include <iostream>
#include <vector>
#define _USE_PROFILE_

using namespace std;
#define testSize	100000

#include <cmath>

struct sample {
	char s[64];
};

MemoryPool<sample> mp(testSize, (int)pow(2, ceil(log2(sizeof(sample)))));
MemoryPool_notaling<sample> mp_control(testSize);

vector<sample*> arr(testSize);
vector<sample*> arr1(testSize);
vector<sample*> arr2(testSize);
void basic_test();
void recycle_test();
void notRegal_test();
void overFlow_test();
void perfomence_test_alloc();
void perfomence_test_use();
void hjTest();


void main()
{
	//basic_test();
	//recycle_test();
	//notRegal_test();
	//overFlow_test();

	//perfomence_test_alloc();
	perfomence_test_use();

}

void basic_test()
{
	int count = 100;
	MemoryPool<sample> test(0, 64);

	for (int i = 0; i < count; i++)
	{
		sample* t = test.alloc();
		_itoa_s(i, t->s, 10);

		cout << t->s << endl;
		test.Free(t);
	}


	cout << "==========" << endl;
}

void recycle_test()
{
	MemoryPool<int> test(3);

	int* t1 = test.alloc();
	*t1 = 1;
	cout << *t1 << "\t" << t1 << endl;


	int* t2 = test.alloc();
	*t2 = 2;
	cout << *t2 << "\t" << t2 << endl;


	int* t3 = test.alloc();
	*t3 = 3;
	cout << *t3 << "\t" << t3 << endl;


	test.Free(t2);
	test.Free(t1);

	int* t5 = test.alloc();
	*t5 = 1;
	cout << *t5 << "\t" << t5 << endl;

	int* t4 = test.alloc();
	*t4 = 2;
	cout << *t4 << "\t" << t4 << endl;






}



struct testSTRUCT {
	char* test[2];
};
void notRegal_test()
{
	MemoryPool<testSTRUCT> test(3);
	MemoryPool<testSTRUCT> test2(3);

	testSTRUCT* a1 = test.alloc();
	testSTRUCT* a2 = test.alloc();
	testSTRUCT* a3 = test2.alloc();


	test.Free(a3);
}

void overFlow_test()
{
	MemoryPool<testSTRUCT> test(3);

	testSTRUCT* a1 = test.alloc();
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
			MemoryPool<sample> mpt(testSize, a);

			{
				//scopeProfiler s("alloc memory pool aling " + to_string(a));
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
					mpt.Free(arr1[j]);
				}
			}
		}
		/*/

		{
			scopeProfiler s("alloc memory pool control");
			for (int j = 0; j < testSize / 2; j++)
			{
				arr2[j] = mp_control.alloc();
			}
			return;
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
				mp.Free(arr2[j]);
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
			arr[j] = mp.alloc();

		for (int j = 0; j < testSize; j++)
			mp.Free(arr[j]);

	}

	cout << "nd: " << endl;
	for (int i = 0; i < 1000; i++)
	{
		scopeProfiler s("use stl alloc");

		for (int j = 0; j < testSize; j++)
			arr[j] = new sample();

		for (int j = 0; j < testSize; j++)
			delete arr[j];
	}
}
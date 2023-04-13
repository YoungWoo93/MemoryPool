#include "MemoryPool.h"
#include <unordered_map>
//#include "ObjectPool.v.3.0.h"

#ifdef	_V_3_
__declspec(thread) std::unordered_map<void*, std::shared_ptr<TLSPoolBase>> TLSPoolMap;
#endif

#include <vector>
#include <iostream>

using namespace std;

typedef struct _line {
	unsigned int tag;
	unsigned int data[16];
	unsigned int valid : 1;
	unsigned int dirty : 1;
} CACHE_LINE;

/* 주소 bit 연산을 위한 배열 선언 - TAG[1] : L1 캐시 연산용, TAG[2] : L2 캐시 연산용 */
static unsigned int TAG[3][2] = { {},{ 0xfffffff0, 0xffffffe0 },{ 0xfffffff0, 0xffffffe0 } }, INDEX[3][2] = { {},{ 0, 0 },{ 0, 0 } }, OFFSET[3][2] = { {},{ 0x0000000f, 0x0000003f },{ 0x0000000f, 0x0000003f } };
/* 변수 선언 */
static unsigned int L1_associativity, blockSize, L1_cacheSize, L1_sets, inclusive, bitIdx, offsetBits, func, addr, L1_indexBits;
static unsigned int L2_associativity = 8, L2_cacheSize = 16384, L2_indexBits, L2_sets = L2_cacheSize / L2_associativity;
static double totalCount = 0, totalICount = 0, totalDCount = 0, L1IHitCount = 0, L1DHitCount = 0, L2HitCount = 0;
static int writeBackCount = 0;

/* L1, L2 캐시 선언 */
static CACHE_LINE **L1_I_CACHE;
static CACHE_LINE **L1_D_CACHE;
static CACHE_LINE **L2_CACHE;

/* LRU를 위한 vector 선언 */
static std::vector<int> LRU[3][8]; // 0 : L1 - I / 1 : L1 - D / 2 : L2

static int addCache(int cache, unsigned int func, unsigned int addr, unsigned int index, unsigned int tag, unsigned int setNum, int lruUpdate);

/* writeBackCount getter 함수 */
static int getwriteBackCount() {
	return writeBackCount;
}

/* 2^N에서 N을 찾아주는 함수 - index bit 개수를 알기 위함 */
static unsigned int findN(unsigned num) {
	unsigned int result = 0, temp = 1;

	while (true) {
		if (temp == num) return result;
		temp *= 2;
		result++;
	}
}

/* LRU를 업데이트 하는 함수 */
static void updateLRU(int cache, unsigned int setNum, unsigned int setIdx) {
	/* LRU에서 해당하는 setIdx가 위치하는 곳을 찾아서 삭제함 */
	for (int i = 0; i<LRU[cache][setNum].size(); i++) {
		if (LRU[cache][setNum][i] == setIdx) {
			LRU[cache][setNum].erase(LRU[cache][setNum].begin() + i);
			break;
		}
	}
	/* 맨 뒤에 추가함 - 가장 최근에 reference됨을 의미 */
	LRU[cache][setNum].push_back(setIdx);
}

/* 이미 존재하는 캐시를 evict하는 함수 */
static void evictFromCache(int cache, int func, unsigned int addr, unsigned int index, unsigned int tag, unsigned int setNum, unsigned int evictIdx) {
	
	if (cache == 0) { // L1 I-cache일 경우
		/* Evict할 L2 캐시의 tag와 set 번호 계산 */
		int L2_evictIdx = -1;
		unsigned int L2_cacheTag = (addr & TAG[2][bitIdx] | INDEX[2][bitIdx]) >> offsetBits, L2_setNum = L2_cacheTag % L2_associativity;

		/* Inclusive일 경우에는 L2 캐시로 이동 - 어차피 이미 있으니까 set index 찾기만 함 */
		if (inclusive == 0) {
			for (int i = 0; i < L2_sets; i++) {
				if (L2_CACHE[L2_setNum][i].valid == 1 && L2_CACHE[L2_setNum][i].tag == L2_cacheTag) {
					L2_evictIdx = i;
					break;
				}
			}
		}
		else { /* Exclusive일 경우에는 L2 캐시를 추가해주고 LRU도 업데이트해줌 */
			L2_evictIdx = addCache(2, func, addr, -1, L2_cacheTag, L2_setNum, 0);
			updateLRU(2, L2_setNum, L2_evictIdx);
		}

		/* L1 I-cache의 캐시 라인 초기화 */
		for (int i = 0; i < blockSize / 4; i++) {
			L1_I_CACHE[setNum][evictIdx].data[i] = 0;
		}
		L1_I_CACHE[setNum][evictIdx].dirty = 0;
		L1_I_CACHE[setNum][evictIdx].tag = 0;
		L1_I_CACHE[setNum][evictIdx].valid = 0;
	}
	else if (cache == 1) { // L1 D-cache일 경우
		/* Evict할 L2 캐시의 tag와 set 번호 계산 */
		int L2_evictIdx = -1;
		unsigned int L2_cacheTag = (addr & TAG[2][bitIdx] | INDEX[2][bitIdx]) >> offsetBits, L2_setNum = L2_cacheTag % L2_associativity;

		/* Inclusive일 경우에는 L2 캐시로 이동 - 어차피 이미 있으니까 set index 찾기만 함 */
		if (inclusive == 0) {
			for (int i = 0; i < L2_sets; i++) {
				if (L2_CACHE[L2_setNum][i].valid == 1 && L2_CACHE[L2_setNum][i].tag == L2_cacheTag) {
					L2_evictIdx = i;
					break;
				}
			}
		}
		else { /* Exclusive일 경우에는 L2 캐시를 추가해주고 LRU도 업데이트해줌 */
			L2_evictIdx = addCache(2, func, addr, -1, L2_cacheTag, L2_setNum, 1);
			updateLRU(2, L2_setNum, L2_evictIdx);
		}

		/* Dirty bit이 1이면 L2 캐시도 dirty bit 1로 초기화 해줌 */
		if (L1_D_CACHE[setNum][evictIdx].dirty == 1) L2_CACHE[L2_setNum][L2_evictIdx].dirty = 1;

		/* L1 D-cache의 캐시 라인 초기화 */
		for (int i = 0; i < blockSize / 4; i++) {
			L1_D_CACHE[setNum][evictIdx].data[i] = 0;
		}
		L1_D_CACHE[setNum][evictIdx].dirty = 0;
		L1_D_CACHE[setNum][evictIdx].tag = 0;
		L1_D_CACHE[setNum][evictIdx].valid = 0;
	}
	else if (cache == 2) { // L2 캐시일 경우
		/* Inclusive일 경우에 L1 캐시도 함께 삭제 */
		if (inclusive == 0) {
			/* 함께 evict할 L1 캐시의 tag와 set 번호 계산 */
			unsigned int L1_cacheIdx = (addr & INDEX[1][bitIdx]) >> offsetBits, L1_cacheTag = (addr & TAG[1][bitIdx]) >> (offsetBits + L1_indexBits), L1_setNum = L1_cacheIdx % L1_associativity;

			/* Associativity에 따라 분기 처리 */
			if (L1_associativity > 1) {
				L1_cacheTag = (addr & (TAG[1][bitIdx] | INDEX[1][bitIdx])) >> offsetBits;
				L1_setNum = L1_cacheTag % L1_associativity;
				L1_cacheIdx = -1;

				/* L1 캐시 set index 찾음 */
				for (int i = 0; i < L1_sets; i++) {
					if (L1_D_CACHE[L1_setNum][i].valid == 1 && L1_D_CACHE[L1_setNum][i].tag == L1_cacheTag) {
						L1_cacheIdx = i;
						break;
					}
				}
			}
			else {
				/* L1 캐시에 없는 경우 */
				if (L1_D_CACHE[0][L1_cacheIdx].valid != 1 || L1_D_CACHE[0][L1_cacheIdx].tag != L1_cacheTag) {
					L1_cacheIdx = -1;
				}
			}

			/* L1 캐시 초기화 */
			if (L1_cacheIdx > -1) {
				for (int i = 0; i < blockSize / 4; i++) {
					L1_D_CACHE[0][L1_cacheIdx].data[i] = 0;
				}
				L1_D_CACHE[0][L1_cacheIdx].valid = 0;
				L1_D_CACHE[0][L1_cacheIdx].tag = 0;
				L1_D_CACHE[0][L1_cacheIdx].dirty = 0;
			}
		}

		/* L2 캐시의 dirty bit이 1이면 write back 해야하므로 write bakc count 증가 */
		if (L2_CACHE[setNum][evictIdx].dirty == 1) writeBackCount++;

		/* L2 cache의 캐시 라인 초기화 */
		for (int i = 0; i < blockSize / 4; i++) {
			L2_CACHE[setNum][evictIdx].data[i] = 0;
		}
		L2_CACHE[setNum][evictIdx].dirty = 0;
		L2_CACHE[setNum][evictIdx].tag = 0;
		L2_CACHE[setNum][evictIdx].valid = 0;
	}
}

/* 새로운 cache를 추가하는 함수 */
static int addCache(int cache, unsigned int func, unsigned int addr, unsigned int index, unsigned int tag, unsigned int setNum, int lruUpdate) {
	
	bool isFull = true;
	int addIdx = -1;

	/* 새롭게 추가할 data - offset을 제외한 address */
	unsigned int baseAddr = addr & (INDEX[cache][bitIdx] | TAG[cache][bitIdx]);

	if (cache == 0 || cache == 1) { // L1 캐시일 경우
		if (L1_associativity > 1) {
			if (func == 0 || func == 1) { // 읽기 또는 쓰기일 경우
				/* Set가 꽉 차있는지 확인함 */
				for (int i = 0; i<L1_sets; i++) {
					if (L1_D_CACHE[setNum][i].valid == 0) {
						isFull = false;
						addIdx = i;
						break;
					}
				}

				if (isFull) { // Set가 꽉 차있을 경우 LRU 정책으로 evcict함
					addIdx = LRU[1][setNum][0];
					evictFromCache(cache, func, addr, index, tag, setNum, addIdx);
				}

				L1_D_CACHE[setNum][addIdx].tag = tag;
				L1_D_CACHE[setNum][addIdx].valid = 1;
				L1_D_CACHE[setNum][addIdx].dirty = 0;

				for (int i = 0; i < blockSize / 4; i++) {
					L1_D_CACHE[setNum][addIdx].data[i] = baseAddr + (i * 4);
				}
			}
			else if (func == 2) { // 명령어일 경우
				for (int i = 0; i<L1_sets; i++) {
					if (L1_I_CACHE[setNum][i].valid == 0) {
						isFull = false;
						addIdx = i;
						break;
					}
				}

				if (isFull) { // set가 꽉 차있을 경우 LRU 정책으로 evict함
					addIdx = LRU[0][setNum][0];

					evictFromCache(cache, func, addr, index, tag, setNum, addIdx);
				}

				L1_I_CACHE[setNum][addIdx].tag = tag;
				L1_I_CACHE[setNum][addIdx].valid = 1;
				L1_I_CACHE[setNum][addIdx].dirty = 0;

				for (int i = 0; i < blockSize / 4; i++) {
					L1_I_CACHE[setNum][addIdx].data[i] = baseAddr + (i * 4);
				}
			}
		}
		else {
			addIdx = index;

			if (func == 0 || func == 1) { // 읽기 또는 쓰기일 경우
				/* 이미 캐시가 존재하는 경우 evict함 */
				if (L1_D_CACHE[0][addIdx].valid == 1 && L1_D_CACHE[0][addIdx].tag != tag) evictFromCache(cache, func, addr, index, tag, setNum, addIdx);

				L1_D_CACHE[0][addIdx].valid = 1;
				L1_D_CACHE[0][addIdx].tag = tag;
				L1_D_CACHE[0][addIdx].dirty = 0;

				for (int i = 0; i < blockSize / 4; i++) {
					L1_D_CACHE[0][addIdx].data[i] = baseAddr + (i * 4);
				}
			}
			else if (func == 2) { // 명령어일 경우
				/* 이미 캐시가 존재하는 경우 evict함 */
				if (L1_I_CACHE[0][addIdx].valid == 1 && L1_I_CACHE[0][addIdx].tag != tag) evictFromCache(cache, func, addr, index, tag, setNum, addIdx);

				L1_I_CACHE[0][addIdx].valid = 1;
				L1_I_CACHE[0][addIdx].tag = tag;
				L1_I_CACHE[0][addIdx].dirty = 0;

				for (int i = 0; i < blockSize / 4; i++) {
					L1_I_CACHE[0][addIdx].data[i] = baseAddr + (i * 4);
				}
			}
		}
	}
	else if (cache == 2) {
		/* Set가 꽉 차있는지 확인함 */
		for (int i = 0; i<L2_sets; i++) {
			if (L2_CACHE[setNum][i].valid == 0) {
				isFull = false;
				addIdx = i;
				break;
			}
		}

		if (isFull) { // set가 꽉 차있을 경우 LRU 정책으로 evict함
			addIdx = LRU[2][setNum][0];
			evictFromCache(cache, func, addr, index, tag, setNum, addIdx);
		}

		L2_CACHE[setNum][addIdx].tag = tag;
		L2_CACHE[setNum][addIdx].valid = 1;
		L2_CACHE[setNum][addIdx].dirty = 0;

		for (int i = 0; i < blockSize / 4; i++) {
			L2_CACHE[setNum][addIdx].data[i] = baseAddr + (i * 4);
		}
	}

	if(lruUpdate == 1) updateLRU(cache, setNum, addIdx);

	return addIdx;
}

/* Write 처리하는 함수 */
static void writeCache(int cache, int func, unsigned int addr, unsigned int index, unsigned int tag, unsigned int setNum, unsigned int writeIdx, int lruUpdate) {

	if (cache == 1) { // L1 캐시일 경우
		if (writeIdx == -1) {
			writeIdx = addCache(cache, func, addr, index, tag, setNum, 1);
		}
		L1_D_CACHE[setNum][writeIdx].dirty = 1;
	}
	else if (cache == 2) { // L2 캐시일 경우
		if (writeIdx == -1) {
			writeIdx = addCache(cache, func, addr, index, tag, setNum, 1);
		}
		L2_CACHE[setNum][writeIdx].dirty = 1;
	}

	if(lruUpdate == 1) updateLRU(cache, setNum, writeIdx);
}
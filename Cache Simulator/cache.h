#include <vector>
#include <iostream>

using namespace std;

typedef struct _line {
	unsigned int tag;
	unsigned int data[16];
	unsigned int valid : 1;
	unsigned int dirty : 1;
} CACHE_LINE;

/* �ּ� bit ������ ���� �迭 ���� - TAG[1] : L1 ĳ�� �����, TAG[2] : L2 ĳ�� ����� */
static unsigned int TAG[3][2] = { {},{ 0xfffffff0, 0xffffffe0 },{ 0xfffffff0, 0xffffffe0 } }, INDEX[3][2] = { {},{ 0, 0 },{ 0, 0 } }, OFFSET[3][2] = { {},{ 0x0000000f, 0x0000003f },{ 0x0000000f, 0x0000003f } };
/* ���� ���� */
static unsigned int L1_associativity, blockSize, L1_cacheSize, L1_sets, inclusive, bitIdx, offsetBits, func, addr, L1_indexBits;
static unsigned int L2_associativity = 8, L2_cacheSize = 16384, L2_indexBits, L2_sets = L2_cacheSize / L2_associativity;
static double totalCount = 0, totalICount = 0, totalDCount = 0, L1IHitCount = 0, L1DHitCount = 0, L2HitCount = 0;
static int writeBackCount = 0;

/* L1, L2 ĳ�� ���� */
static CACHE_LINE **L1_I_CACHE;
static CACHE_LINE **L1_D_CACHE;
static CACHE_LINE **L2_CACHE;

/* LRU�� ���� vector ���� */
static std::vector<int> LRU[3][8]; // 0 : L1 - I / 1 : L1 - D / 2 : L2

static int addCache(int cache, unsigned int func, unsigned int addr, unsigned int index, unsigned int tag, unsigned int setNum, int lruUpdate);

/* writeBackCount getter �Լ� */
static int getwriteBackCount() {
	return writeBackCount;
}

/* 2^N���� N�� ã���ִ� �Լ� - index bit ������ �˱� ���� */
static unsigned int findN(unsigned num) {
	unsigned int result = 0, temp = 1;

	while (true) {
		if (temp == num) return result;
		temp *= 2;
		result++;
	}
}

/* LRU�� ������Ʈ �ϴ� �Լ� */
static void updateLRU(int cache, unsigned int setNum, unsigned int setIdx) {
	/* LRU���� �ش��ϴ� setIdx�� ��ġ�ϴ� ���� ã�Ƽ� ������ */
	for (int i = 0; i<LRU[cache][setNum].size(); i++) {
		if (LRU[cache][setNum][i] == setIdx) {
			LRU[cache][setNum].erase(LRU[cache][setNum].begin() + i);
			break;
		}
	}
	/* �� �ڿ� �߰��� - ���� �ֱٿ� reference���� �ǹ� */
	LRU[cache][setNum].push_back(setIdx);
}

/* �̹� �����ϴ� ĳ�ø� evict�ϴ� �Լ� */
static void evictFromCache(int cache, int func, unsigned int addr, unsigned int index, unsigned int tag, unsigned int setNum, unsigned int evictIdx) {
	
	if (cache == 0) { // L1 I-cache�� ���
		/* Evict�� L2 ĳ���� tag�� set ��ȣ ��� */
		int L2_evictIdx = -1;
		unsigned int L2_cacheTag = (addr & TAG[2][bitIdx] | INDEX[2][bitIdx]) >> offsetBits, L2_setNum = L2_cacheTag % L2_associativity;

		/* Inclusive�� ��쿡�� L2 ĳ�÷� �̵� - ������ �̹� �����ϱ� set index ã�⸸ �� */
		if (inclusive == 0) {
			for (int i = 0; i < L2_sets; i++) {
				if (L2_CACHE[L2_setNum][i].valid == 1 && L2_CACHE[L2_setNum][i].tag == L2_cacheTag) {
					L2_evictIdx = i;
					break;
				}
			}
		}
		else { /* Exclusive�� ��쿡�� L2 ĳ�ø� �߰����ְ� LRU�� ������Ʈ���� */
			L2_evictIdx = addCache(2, func, addr, -1, L2_cacheTag, L2_setNum, 0);
			updateLRU(2, L2_setNum, L2_evictIdx);
		}

		/* L1 I-cache�� ĳ�� ���� �ʱ�ȭ */
		for (int i = 0; i < blockSize / 4; i++) {
			L1_I_CACHE[setNum][evictIdx].data[i] = 0;
		}
		L1_I_CACHE[setNum][evictIdx].dirty = 0;
		L1_I_CACHE[setNum][evictIdx].tag = 0;
		L1_I_CACHE[setNum][evictIdx].valid = 0;
	}
	else if (cache == 1) { // L1 D-cache�� ���
		/* Evict�� L2 ĳ���� tag�� set ��ȣ ��� */
		int L2_evictIdx = -1;
		unsigned int L2_cacheTag = (addr & TAG[2][bitIdx] | INDEX[2][bitIdx]) >> offsetBits, L2_setNum = L2_cacheTag % L2_associativity;

		/* Inclusive�� ��쿡�� L2 ĳ�÷� �̵� - ������ �̹� �����ϱ� set index ã�⸸ �� */
		if (inclusive == 0) {
			for (int i = 0; i < L2_sets; i++) {
				if (L2_CACHE[L2_setNum][i].valid == 1 && L2_CACHE[L2_setNum][i].tag == L2_cacheTag) {
					L2_evictIdx = i;
					break;
				}
			}
		}
		else { /* Exclusive�� ��쿡�� L2 ĳ�ø� �߰����ְ� LRU�� ������Ʈ���� */
			L2_evictIdx = addCache(2, func, addr, -1, L2_cacheTag, L2_setNum, 1);
			updateLRU(2, L2_setNum, L2_evictIdx);
		}

		/* Dirty bit�� 1�̸� L2 ĳ�õ� dirty bit 1�� �ʱ�ȭ ���� */
		if (L1_D_CACHE[setNum][evictIdx].dirty == 1) L2_CACHE[L2_setNum][L2_evictIdx].dirty = 1;

		/* L1 D-cache�� ĳ�� ���� �ʱ�ȭ */
		for (int i = 0; i < blockSize / 4; i++) {
			L1_D_CACHE[setNum][evictIdx].data[i] = 0;
		}
		L1_D_CACHE[setNum][evictIdx].dirty = 0;
		L1_D_CACHE[setNum][evictIdx].tag = 0;
		L1_D_CACHE[setNum][evictIdx].valid = 0;
	}
	else if (cache == 2) { // L2 ĳ���� ���
		/* Inclusive�� ��쿡 L1 ĳ�õ� �Բ� ���� */
		if (inclusive == 0) {
			/* �Բ� evict�� L1 ĳ���� tag�� set ��ȣ ��� */
			unsigned int L1_cacheIdx = (addr & INDEX[1][bitIdx]) >> offsetBits, L1_cacheTag = (addr & TAG[1][bitIdx]) >> (offsetBits + L1_indexBits), L1_setNum = L1_cacheIdx % L1_associativity;

			/* Associativity�� ���� �б� ó�� */
			if (L1_associativity > 1) {
				L1_cacheTag = (addr & (TAG[1][bitIdx] | INDEX[1][bitIdx])) >> offsetBits;
				L1_setNum = L1_cacheTag % L1_associativity;
				L1_cacheIdx = -1;

				/* L1 ĳ�� set index ã�� */
				for (int i = 0; i < L1_sets; i++) {
					if (L1_D_CACHE[L1_setNum][i].valid == 1 && L1_D_CACHE[L1_setNum][i].tag == L1_cacheTag) {
						L1_cacheIdx = i;
						break;
					}
				}
			}
			else {
				/* L1 ĳ�ÿ� ���� ��� */
				if (L1_D_CACHE[0][L1_cacheIdx].valid != 1 || L1_D_CACHE[0][L1_cacheIdx].tag != L1_cacheTag) {
					L1_cacheIdx = -1;
				}
			}

			/* L1 ĳ�� �ʱ�ȭ */
			if (L1_cacheIdx > -1) {
				for (int i = 0; i < blockSize / 4; i++) {
					L1_D_CACHE[0][L1_cacheIdx].data[i] = 0;
				}
				L1_D_CACHE[0][L1_cacheIdx].valid = 0;
				L1_D_CACHE[0][L1_cacheIdx].tag = 0;
				L1_D_CACHE[0][L1_cacheIdx].dirty = 0;
			}
		}

		/* L2 ĳ���� dirty bit�� 1�̸� write back �ؾ��ϹǷ� write bakc count ���� */
		if (L2_CACHE[setNum][evictIdx].dirty == 1) writeBackCount++;

		/* L2 cache�� ĳ�� ���� �ʱ�ȭ */
		for (int i = 0; i < blockSize / 4; i++) {
			L2_CACHE[setNum][evictIdx].data[i] = 0;
		}
		L2_CACHE[setNum][evictIdx].dirty = 0;
		L2_CACHE[setNum][evictIdx].tag = 0;
		L2_CACHE[setNum][evictIdx].valid = 0;
	}
}

/* ���ο� cache�� �߰��ϴ� �Լ� */
static int addCache(int cache, unsigned int func, unsigned int addr, unsigned int index, unsigned int tag, unsigned int setNum, int lruUpdate) {
	
	bool isFull = true;
	int addIdx = -1;

	/* ���Ӱ� �߰��� data - offset�� ������ address */
	unsigned int baseAddr = addr & (INDEX[cache][bitIdx] | TAG[cache][bitIdx]);

	if (cache == 0 || cache == 1) { // L1 ĳ���� ���
		if (L1_associativity > 1) {
			if (func == 0 || func == 1) { // �б� �Ǵ� ������ ���
				/* Set�� �� ���ִ��� Ȯ���� */
				for (int i = 0; i<L1_sets; i++) {
					if (L1_D_CACHE[setNum][i].valid == 0) {
						isFull = false;
						addIdx = i;
						break;
					}
				}

				if (isFull) { // Set�� �� ������ ��� LRU ��å���� evcict��
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
			else if (func == 2) { // ��ɾ��� ���
				for (int i = 0; i<L1_sets; i++) {
					if (L1_I_CACHE[setNum][i].valid == 0) {
						isFull = false;
						addIdx = i;
						break;
					}
				}

				if (isFull) { // set�� �� ������ ��� LRU ��å���� evict��
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

			if (func == 0 || func == 1) { // �б� �Ǵ� ������ ���
				/* �̹� ĳ�ð� �����ϴ� ��� evict�� */
				if (L1_D_CACHE[0][addIdx].valid == 1 && L1_D_CACHE[0][addIdx].tag != tag) evictFromCache(cache, func, addr, index, tag, setNum, addIdx);

				L1_D_CACHE[0][addIdx].valid = 1;
				L1_D_CACHE[0][addIdx].tag = tag;
				L1_D_CACHE[0][addIdx].dirty = 0;

				for (int i = 0; i < blockSize / 4; i++) {
					L1_D_CACHE[0][addIdx].data[i] = baseAddr + (i * 4);
				}
			}
			else if (func == 2) { // ��ɾ��� ���
				/* �̹� ĳ�ð� �����ϴ� ��� evict�� */
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
		/* Set�� �� ���ִ��� Ȯ���� */
		for (int i = 0; i<L2_sets; i++) {
			if (L2_CACHE[setNum][i].valid == 0) {
				isFull = false;
				addIdx = i;
				break;
			}
		}

		if (isFull) { // set�� �� ������ ��� LRU ��å���� evict��
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

/* Write ó���ϴ� �Լ� */
static void writeCache(int cache, int func, unsigned int addr, unsigned int index, unsigned int tag, unsigned int setNum, unsigned int writeIdx, int lruUpdate) {

	if (cache == 1) { // L1 ĳ���� ���
		if (writeIdx == -1) {
			writeIdx = addCache(cache, func, addr, index, tag, setNum, 1);
		}
		L1_D_CACHE[setNum][writeIdx].dirty = 1;
	}
	else if (cache == 2) { // L2 ĳ���� ���
		if (writeIdx == -1) {
			writeIdx = addCache(cache, func, addr, index, tag, setNum, 1);
		}
		L2_CACHE[setNum][writeIdx].dirty = 1;
	}

	if(lruUpdate == 1) updateLRU(cache, setNum, writeIdx);
}
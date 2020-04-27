#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "cache.h"

using namespace std;

FILE *fp;

int main(int argc, char **argv) {

	/* 변수 초기화 - 입력값 = Associativity(1/2/4/8), Block Size(16/64), Cache Size(1024/2048/4096/8192/16384), Inclusive/Exclusive(0/1) */
	L1_associativity = atoi(argv[1]); blockSize = atoi(argv[2]); L1_cacheSize = atoi(argv[3]); inclusive = atoi(argv[4]);
	L1_sets = L1_cacheSize / blockSize / L1_associativity;
	L2_sets = L2_sets / blockSize;

	/* Index bit 개수 구함 */
	L1_indexBits = findN(L1_sets);
	L2_indexBits = findN(L2_sets);

	/* Block size에 따라 offset bit 개수 결정 */
	if (blockSize == 16)
	{
		bitIdx = 0;
		offsetBits = 4;
	}
	else if (blockSize == 64)
	{
		bitIdx = 1;
		offsetBits = 6;
	}

	/* Address로 부터 tag와 index를 구하기 위한 배열 초기화 */
	INDEX[1][bitIdx] = (L1_sets - 1) << offsetBits;
	TAG[1][bitIdx] = TAG[1][bitIdx] ^ INDEX[1][bitIdx];

	INDEX[2][bitIdx] = (L2_sets - 1) << offsetBits;
	TAG[2][bitIdx] = TAG[2][bitIdx] ^ INDEX[2][bitIdx];

	/* L1, L2 캐시 초기화 */
	L1_I_CACHE = (CACHE_LINE**)malloc(sizeof(CACHE_LINE*) * L1_associativity);
	L1_D_CACHE = (CACHE_LINE**)malloc(sizeof(CACHE_LINE*) * L1_associativity);
	L2_CACHE = (CACHE_LINE**)malloc(sizeof(CACHE_LINE*) * L2_associativity);

	for (int i = 0; i < L1_associativity; i++) {
		L1_I_CACHE[i] = (CACHE_LINE*)malloc(sizeof(CACHE_LINE) * L1_sets);
		L1_D_CACHE[i] = (CACHE_LINE*)malloc(sizeof(CACHE_LINE) * L1_sets);
		for (int j = 0; j < L1_sets; j++) {
			L1_I_CACHE[i][j].valid = 0; L1_I_CACHE[i][j].dirty = 0;
			L1_D_CACHE[i][j].valid = 0; L1_D_CACHE[i][j].dirty = 0;
		}
	}

	for (int i = 0; i < L2_associativity; i++) {
		L2_CACHE[i] = (CACHE_LINE*)malloc(sizeof(CACHE_LINE) * L2_sets);
		for (int j = 0; j < L2_sets; j++) {
			L2_CACHE[i][j].valid = 0; L2_CACHE[i][j].dirty = 0;
		}
	}

	/* 파일 열기 */
	fp = fopen(argv[5], "r");

	while (fscanf(fp, "%d %x", &func, &addr) != EOF) {

		/* L1 캐시와 L2 캐시의 index, tag, set 번호를 구함 */
		unsigned int L1_cacheIdx = (addr & INDEX[1][bitIdx]) >> offsetBits, L1_cacheTag = (addr & TAG[1][bitIdx]) >> (offsetBits + L1_indexBits), L1_setNum = L1_cacheIdx % L1_associativity, L1_setIdx = -1;
		if (L1_associativity > 1) {
			L1_cacheTag = (addr & (TAG[1][bitIdx] | INDEX[1][bitIdx])) >> offsetBits;
			L1_setNum = L1_cacheTag % L1_associativity;
			L1_cacheIdx = -1;
		}
		unsigned int L2_cacheTag = (addr & (TAG[2][bitIdx] | INDEX[2][bitIdx])) >> offsetBits, L2_setNum = L2_cacheTag % L2_associativity, L2_setIdx = -1, L2_cacheIdx = -1;
		bool isL1Miss = true, isL2Miss = true;

		/* L1 캐시 hit 여부를 판단함 */
		if (func == 2) { // 명령어일 경우
			if (L1_associativity == 1) { // Direct map일 경우
				if (L1_I_CACHE[0][L1_cacheIdx].valid == 1 && L1_I_CACHE[0][L1_cacheIdx].tag == L1_cacheTag) { // Cache hit
					isL1Miss = false;
					L1_setIdx = L1_cacheIdx;
				}
			}
			else { // Associative map일 경우
				for (int i = 0; i<L1_sets; i++) {
					if (L1_I_CACHE[L1_setNum][i].valid == 1 && L1_I_CACHE[L1_setNum][i].tag == L1_cacheTag) { // Cache hit
						isL1Miss = false;
						L1_setIdx = i;
						break;
					}
				}
			}
		}
		else { // 읽기 또는 쓰기일 경우
			if (L1_associativity == 1) { // Direct map일 경우
				if (L1_D_CACHE[0][L1_cacheIdx].valid == 1 && L1_D_CACHE[0][L1_cacheIdx].tag == L1_cacheTag) { // Cache hit
					isL1Miss = false;
					L1_setIdx = L1_cacheIdx;
				}
			}
			else { // Associative map일 경우
				for (int i = 0; i<L1_sets; i++) {
					if (L1_D_CACHE[L1_setNum][i].valid == 1 && L1_D_CACHE[L1_setNum][i].tag == L1_cacheTag) { // Cache hit
						isL1Miss = false;
						L1_setIdx = i;
						break;
					}
				}
			}
		}

		/* L2 캐시 hit 여부를 판단함 */
		for (int i = 0; i<L2_sets; i++) {
			if (L2_CACHE[L2_setNum][i].valid == 1 && L2_CACHE[L2_setNum][i].tag == L2_cacheTag) { // Cache hit
				isL2Miss = false;
				L2_setIdx = i;
				break;
			}
		}

		/* 명령어 hit count 처리 */
		if (func == 2) {
			totalICount++;
			if (!isL1Miss) L1IHitCount++;
		}

		/* 읽기 또는 쓰기 hit count 처리 */
		if (func < 2) {
			totalDCount++;
			if (!isL1Miss) L1DHitCount++;
		}

		/* L2 cache hit count 처리 */
		totalCount++;
		if (!isL2Miss) L2HitCount++;

		if (inclusive == 0) { // Inclusive일 경우

			/* 여기서 부터는 과제 내용에 적힌 policy대로 처리 */
			if (func == 0) { // Data read
				if (!isL1Miss) {
					updateLRU(1, L1_setNum, L1_setIdx);
				}
				else if (isL1Miss && !isL2Miss) {
					addCache(1, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, 1);
					updateLRU(2, L2_setNum, L2_setIdx);
				}
				else if (isL1Miss && isL2Miss) {
					addCache(1, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, 1);
					addCache(2, func, addr, L2_cacheIdx, L2_cacheTag, L2_setNum, 1);
				}
			}
			else if (func == 1) { // Data write
				if (!isL1Miss) {
					writeCache(1, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, L1_setIdx, 1);
				}
				else if (isL1Miss && !isL2Miss) {
					writeCache(1, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, L1_setIdx, 1);
					updateLRU(2, L2_setNum, L2_setIdx);
				}
				else if (isL1Miss && isL2Miss) {
					writeCache(1, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, L1_setIdx, 1);
					addCache(2, func, addr, L2_cacheIdx, L2_cacheTag, L2_setNum, 1);
				}
			}
			else if (func == 2) { // Instruction fetch
				if (!isL1Miss) {
					updateLRU(0, L1_setNum, L1_setIdx);
				}
				else if (isL1Miss && !isL2Miss) {
					addCache(0, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, 1);
					updateLRU(2, L2_setNum, L2_setIdx);
				}
				else if (isL1Miss && isL2Miss) {
					addCache(0, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, 1);
					addCache(2, func, addr, L2_cacheIdx, L2_cacheTag, L2_setNum, 1);
				}
			}

		}
		else if (inclusive == 1) { // Exclusive일 경우

			/* 여기서 부터는 과제 내용에 적힌 policy대로 처리 */
			if (func == 0) { // Data read
				if (!isL1Miss) {
					updateLRU(1, L1_setNum, L1_setIdx);
				}
				else if (isL1Miss && !isL2Miss) {
					addCache(1, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, 1);
					evictFromCache(2, func, addr, L2_cacheIdx, L2_cacheTag, L2_setNum, L2_setIdx);
				}
				else if (isL1Miss && isL2Miss) {
					addCache(1, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, 1);
				}
			}
			else if (func == 1) { // Data write
				if (!isL1Miss) {
					writeCache(1, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, L1_setIdx, 1);
				}
				else if (isL1Miss && !isL2Miss) {
					writeCache(1, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, L1_setIdx, 1);
					evictFromCache(2, func, addr, L2_cacheIdx, L2_cacheTag, L2_setNum, L2_setIdx);
				}
				else if (isL1Miss && isL2Miss) {
					writeCache(1, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, L1_setIdx, 1);
				}
			}
			else if (func == 2) { // Instruction fetch
				if (!isL1Miss) {
					updateLRU(0, L1_setNum, L1_setIdx);
				}
				else if (isL1Miss && !isL2Miss) {
					addCache(0, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, 1);
					evictFromCache(2, func, addr, L2_cacheIdx, L2_cacheTag, L2_setNum, L2_setIdx);
				}
				else if (isL1Miss && isL2Miss) {
					addCache(0, func, addr, L1_cacheIdx, L1_cacheTag, L1_setNum, 1);
				}
			}
		}
	}

	fclose(fp);

	/* 결과 출력 */
	printf("%.3f %.3f %.3f %d\n", 1 - (L1IHitCount / totalICount), 1 - (L1DHitCount / totalDCount), 1 - (L2HitCount / totalCount), getwriteBackCount());

	return 0;
}

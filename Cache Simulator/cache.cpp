#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "cache.h"

using namespace std;

FILE *fp;

int main(int argc, char **argv) {

	/* ���� �ʱ�ȭ - �Է°� = Associativity(1/2/4/8), Block Size(16/64), Cache Size(1024/2048/4096/8192/16384), Inclusive/Exclusive(0/1) */
	L1_associativity = atoi(argv[1]); blockSize = atoi(argv[2]); L1_cacheSize = atoi(argv[3]); inclusive = atoi(argv[4]);
	L1_sets = L1_cacheSize / blockSize / L1_associativity;
	L2_sets = L2_sets / blockSize;

	/* Index bit ���� ���� */
	L1_indexBits = findN(L1_sets);
	L2_indexBits = findN(L2_sets);

	/* Block size�� ���� offset bit ���� ���� */
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

	/* Address�� ���� tag�� index�� ���ϱ� ���� �迭 �ʱ�ȭ */
	INDEX[1][bitIdx] = (L1_sets - 1) << offsetBits;
	TAG[1][bitIdx] = TAG[1][bitIdx] ^ INDEX[1][bitIdx];

	INDEX[2][bitIdx] = (L2_sets - 1) << offsetBits;
	TAG[2][bitIdx] = TAG[2][bitIdx] ^ INDEX[2][bitIdx];

	/* L1, L2 ĳ�� �ʱ�ȭ */
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

	/* ���� ���� */
	fp = fopen(argv[5], "r");

	while (fscanf(fp, "%d %x", &func, &addr) != EOF) {

		/* L1 ĳ�ÿ� L2 ĳ���� index, tag, set ��ȣ�� ���� */
		unsigned int L1_cacheIdx = (addr & INDEX[1][bitIdx]) >> offsetBits, L1_cacheTag = (addr & TAG[1][bitIdx]) >> (offsetBits + L1_indexBits), L1_setNum = L1_cacheIdx % L1_associativity, L1_setIdx = -1;
		if (L1_associativity > 1) {
			L1_cacheTag = (addr & (TAG[1][bitIdx] | INDEX[1][bitIdx])) >> offsetBits;
			L1_setNum = L1_cacheTag % L1_associativity;
			L1_cacheIdx = -1;
		}
		unsigned int L2_cacheTag = (addr & (TAG[2][bitIdx] | INDEX[2][bitIdx])) >> offsetBits, L2_setNum = L2_cacheTag % L2_associativity, L2_setIdx = -1, L2_cacheIdx = -1;
		bool isL1Miss = true, isL2Miss = true;

		/* L1 ĳ�� hit ���θ� �Ǵ��� */
		if (func == 2) { // ��ɾ��� ���
			if (L1_associativity == 1) { // Direct map�� ���
				if (L1_I_CACHE[0][L1_cacheIdx].valid == 1 && L1_I_CACHE[0][L1_cacheIdx].tag == L1_cacheTag) { // Cache hit
					isL1Miss = false;
					L1_setIdx = L1_cacheIdx;
				}
			}
			else { // Associative map�� ���
				for (int i = 0; i<L1_sets; i++) {
					if (L1_I_CACHE[L1_setNum][i].valid == 1 && L1_I_CACHE[L1_setNum][i].tag == L1_cacheTag) { // Cache hit
						isL1Miss = false;
						L1_setIdx = i;
						break;
					}
				}
			}
		}
		else { // �б� �Ǵ� ������ ���
			if (L1_associativity == 1) { // Direct map�� ���
				if (L1_D_CACHE[0][L1_cacheIdx].valid == 1 && L1_D_CACHE[0][L1_cacheIdx].tag == L1_cacheTag) { // Cache hit
					isL1Miss = false;
					L1_setIdx = L1_cacheIdx;
				}
			}
			else { // Associative map�� ���
				for (int i = 0; i<L1_sets; i++) {
					if (L1_D_CACHE[L1_setNum][i].valid == 1 && L1_D_CACHE[L1_setNum][i].tag == L1_cacheTag) { // Cache hit
						isL1Miss = false;
						L1_setIdx = i;
						break;
					}
				}
			}
		}

		/* L2 ĳ�� hit ���θ� �Ǵ��� */
		for (int i = 0; i<L2_sets; i++) {
			if (L2_CACHE[L2_setNum][i].valid == 1 && L2_CACHE[L2_setNum][i].tag == L2_cacheTag) { // Cache hit
				isL2Miss = false;
				L2_setIdx = i;
				break;
			}
		}

		/* ��ɾ� hit count ó�� */
		if (func == 2) {
			totalICount++;
			if (!isL1Miss) L1IHitCount++;
		}

		/* �б� �Ǵ� ���� hit count ó�� */
		if (func < 2) {
			totalDCount++;
			if (!isL1Miss) L1DHitCount++;
		}

		/* L2 cache hit count ó�� */
		totalCount++;
		if (!isL2Miss) L2HitCount++;

		if (inclusive == 0) { // Inclusive�� ���

			/* ���⼭ ���ʹ� ���� ���뿡 ���� policy��� ó�� */
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
		else if (inclusive == 1) { // Exclusive�� ���

			/* ���⼭ ���ʹ� ���� ���뿡 ���� policy��� ó�� */
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

	/* ��� ��� */
	printf("%.3f %.3f %.3f %d\n", 1 - (L1IHitCount / totalICount), 1 - (L1DHitCount / totalDCount), 1 - (L2HitCount / totalCount), getwriteBackCount());

	return 0;
}

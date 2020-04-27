#include <stdio.h>

#define MAX 50000

typedef struct Process {
	int PID;
	int ARRIVAL_TIME;
	int CYCLES;
	int CPU_BT[200];
	int IO_BT[200];
	int TOTAL_BT;
	int WT;
	int TT;
	int WAKEUP;
	int SLEEPED_QUEUE;
	int COMPLETE;
	int COMPLETE_TIME;
} Process;

// ť 5�� ����, 0~2:Ready Queue / 3:Sleep Queue / 4:Terminated(BT Completed process) Queue / 5:Arrival Time�� �Ǳ� ������ ����ϴ� Queue
Process queue[6][MAX];
int front[6] = {0,0,0,0,0,0};
int rear[6] = {-1,-1,-1,-1,-1,-1};
int itemCount[6] = {0,0,0,0,0,0};
int time = 0;
FILE *fp, *fp_out;

// Queue�� Control �ϴ� Function�� ���� 
Process peek(int q) {
   return queue[q][front[q]];
}

int isEmpty(int q) {
   return itemCount[q] == 0;
}

int isFull(int q) {
   return itemCount[q] == MAX;
}

int sizeOfQueue(int q) {
   return itemCount[q];
}  

void insertQueue(int q, Process data) {

   if(!isFull(q)) {
	
      if(rear[q] == MAX-1) {
         rear[q] = -1;            
      }       

      queue[q][++rear[q]] = data;
      itemCount[q]++;
   }
}

Process deleteQueue(int q) {
   Process data = queue[q][front[q]++];
	
   if(front[q] == MAX) {
      front[q] = 0;
   }
	
   itemCount[q]--;
   return data;
}

// Wake-Up �ؾߵǴ� ���μ����� ������ ���� ť�� ������ Function 
void wakeUp(int startT, int endT){
	int sizeOfSleepQueue = sizeOfQueue(3), i, toQueue;
	
	// Sleep Queue�� �ִ� ���μ������� �˻��ؼ� Wake-Up ���Ѿ� �ϴ� ���μ����� ���� Queue�� ���� 
	for(i=0; i<sizeOfSleepQueue; i++){
		Process wakeUp = deleteQueue(3);
		
		if(wakeUp.WAKEUP >= startT && wakeUp.WAKEUP <= endT){
			wakeUp.WAKEUP = -1;
			toQueue = wakeUp.SLEEPED_QUEUE-1;
			if(toQueue < 0) toQueue = 0;
			insertQueue(toQueue, wakeUp);
		}else{
			insertQueue(3, wakeUp);
		}
	}
}

// Queue0~2�� ���μ����� ���� ��츦 ����� Sleep Queue�� �ִ� ���μ����� Wake-up �ϴ� Function 
int wakeUpAll(){
	
	int isOk = 0; 
	
	while(1){
		int sizeOfSleepQueue = sizeOfQueue(3), i, toQueue;
	
		// Wake-Up �ؾߵǴ� ���μ����� ������ ���� ť�� ������
		for(i=0; i<sizeOfSleepQueue; i++){
			Process wakeUp = deleteQueue(3);
			
			if(wakeUp.WAKEUP == time){
				wakeUp.WAKEUP = -1;
				toQueue = wakeUp.SLEEPED_QUEUE-1;
				if(toQueue < 0) toQueue = 0;
				insertQueue(toQueue, wakeUp);
				isOk = 1; 
			}else{
				insertQueue(3, wakeUp);
			}
		}
		
		// ���μ����� �� �� �̻� ����� �� ��ȯ�ϰ� ���� 
		if(isOk){
			if(sizeOfQueue(0) > 0) return 0;
			else if(sizeOfQueue(1) > 0) return 1;
			else if(sizeOfQueue(2) > 0) return 2;
		}
		time++;
	}
}

// ���μ����� Burst Time�� ��� �Ϸ��ߴ��� üũ
int isComplete(Process temp){
	int i, completTestSum = 0;
	 
	// CPU �� IO BT�� ��� �Ϸ�Ǿ����� üũ�ϱ� ���� ���� Burst Time�� ��� ���� 
	for(i=0; i<temp.CYCLES*2-1; i+=2){
		if(i % 2 == 0) completTestSum += temp.CPU_BT[i/2];
		else completTestSum += temp.IO_BT[i/2];
	}
	
	// Burst Time�� ���� 0�̸� Complete ���·� �����ϰ� Complete�� �ð��� ���� 
	if(completTestSum == 0){
		return 1;
	}else{
		return 0;
	}
}

// Queue 0 �����ٸ� Function 
int scheduling0(){
	
	while(!isEmpty(0)){
		Process temp = deleteQueue(0);
		
		int cpuBtIdx = 0, ioBtIdx = 0, i, completTestSum = 0, startTime, endTime;
	
		// �����ִ� CPU BT ã��
		for(i=0; i<temp.CYCLES; i++){
			if(temp.CPU_BT[i] > 0){
				cpuBtIdx = i;
				break;
			}
		}
		
		// �����ִ� IO BT ã��
		for(i=0; i<temp.CYCLES-1; i++){
			if(temp.IO_BT[i] > 0){
				ioBtIdx = i;
				break;
			}
		}
		
		startTime = time;
		
		// Time Quantum 2 ���� CPU BT �Ҹ� 
		if(temp.CPU_BT[cpuBtIdx] < 2){
			time += temp.CPU_BT[cpuBtIdx];
			temp.CPU_BT[cpuBtIdx] = 0;
		}else{
			temp.CPU_BT[cpuBtIdx] -= 2;
			time += 2;
		}
		
		endTime = time;
		
		// ���μ��� BT�� �Ϸ���� ��� ���μ����� �Ϸ� ���·� �ٲٰ� �Ϸ� �ð� ����  
		if(isComplete(temp)){
			temp.COMPLETE = 1;
			temp.COMPLETE_TIME = time;
		}
		
		// ���� Burst�� IO Burst�� ������ ��� : ���μ����� Wake-Up �Ǵ� �ð��� Sleep �ƴ� Queue ��ȣ�� ����
		if(temp.CPU_BT[cpuBtIdx] == 0 && temp.COMPLETE == 0){
			temp.WAKEUP = time + temp.IO_BT[ioBtIdx];
			temp.SLEEPED_QUEUE = 0;
		}
		
		// Burst Time �Ҹ� �� Wake-Up �ƾ���� ���μ��� Wake-Up 
		wakeUp(startTime, endTime);
		
		fprintf(fp_out, "[Q0] Time %05d ~ %05d : P%03d\n", startTime, endTime, temp.PID);
		
		// BT �Ϸ�� Queue4 / Asleep ���·� ��ȯ�� Queue3 / ������ ���� Queue1
		if(temp.COMPLETE == 1){
			insertQueue(4, temp);
			fprintf(fp_out, "P%03d Completed!\n", temp.PID);	
		}else if(temp.COMPLETE == 0){
			if(temp.WAKEUP != -1) insertQueue(3, temp);
			else insertQueue(1, temp);
		}
		
	}
	
	return 1;
}

// Queue 1 �����ٸ� Function
int scheduling1(){
	
	while(!isEmpty(1)){
		Process temp = deleteQueue(1);
		
		int cpuBtIdx = 0, ioBtIdx = 0, i, completTestSum = 0, startTime, endTime;
	
		// �����ִ� CPU BT ã��
		for(i=0; i<temp.CYCLES; i++){
			if(temp.CPU_BT[i] > 0){
				cpuBtIdx = i;
				break;
			}
		}
		
		// �����ִ� IO BT ã��
		for(i=0; i<temp.CYCLES-1; i++){
			if(temp.IO_BT[i] > 0){
				ioBtIdx = i;
				break;
			}
		}
		
		startTime = time;
		
		// Time Quantum 6 ���� CPU BT �Ҹ� 
		if(temp.CPU_BT[cpuBtIdx] < 6){
			time += temp.CPU_BT[cpuBtIdx];
			temp.CPU_BT[cpuBtIdx] = 0;
		}else{
			temp.CPU_BT[cpuBtIdx] -= 6;
			time += 6;
		}
		
		endTime = time;
		
		// ���μ��� BT�� �Ϸ���� ��� ���μ����� �Ϸ� ���·� �ٲٰ� �Ϸ� �ð� ����  
		if(isComplete(temp)){
			temp.COMPLETE = 1;
			temp.COMPLETE_TIME = time;
		}
		
		// ���� Burst�� IO Burst�� ������ ��� : ���μ����� Wake-Up �Ǵ� �ð��� Sleep �ƴ� Queue ��ȣ�� ����
		if(temp.CPU_BT[cpuBtIdx] == 0 && temp.COMPLETE != 1){
			temp.WAKEUP = time + temp.IO_BT[ioBtIdx];
			temp.SLEEPED_QUEUE = 1;
		}
		
		fprintf(fp_out, "[Q1] Time %05d ~ %05d : P%03d\n", startTime, endTime, temp.PID);
		
		// BT �Ϸ�� Queue4 / Asleep ���·� ��ȯ�� Queue3 / ������ ���� Queue2 
		if(temp.COMPLETE == 1){
			insertQueue(4, temp);
			fprintf(fp_out, "P%03d Completed!\n", temp.PID);
		}else if(temp.COMPLETE == 0){
			if(temp.WAKEUP != -1) insertQueue(3, temp);
			else insertQueue(2, temp);
		}
		
		// Burst Time �Ҹ� �� Wake-Up �ƾ���� ���μ��� Wake-Up 
		wakeUp(startTime, endTime);
		
		if(sizeOfQueue(0) > 0){
			return 0;
		}
		
	}
	return 2;
}

// Queue 2 �����ٸ� Function
int scheduling2(){
	
	while(!isEmpty(2)){
		Process temp;
		
		int cpuBtIdx = 0, ioBtIdx = 0, i, j, completTestSum = 0, startTime, endTime, minCpuBt = 999999, sizeOfQueue2 = sizeOfQueue(2), notYet=1;
		
		// SRTN �����ٸ��� ���� ť �ȿ� �ִ� ���μ����� �߿��� �ּ� CPU BT ã�� 
		for(i=0; i<sizeOfQueue2; i++){
			Process tempSRTN = deleteQueue(2);
					
			// �����ִ� CPU BT ã��
			for(j=0; j<tempSRTN.CYCLES; j++){
				if(tempSRTN.CPU_BT[j] > 0){
					cpuBtIdx = j;
					break;
				}
			}
			
			// ���� ���� CPU BT�� ã��
			if(minCpuBt > tempSRTN.CPU_BT[cpuBtIdx]){
				minCpuBt = tempSRTN.CPU_BT[cpuBtIdx];
			}
			
			insertQueue(2, tempSRTN);
		}
		
		// ������ ã�� �ּ� CPU BT�� ��ġ�ϴ� ���μ����� temp�� ����
		for(i=0; i<sizeOfQueue2; i++){
			Process tempSRTN = deleteQueue(2);
			
			// �����ִ� CPU BT ã��
			for(j=0; j<tempSRTN.CYCLES; j++){
				if(tempSRTN.CPU_BT[j] > 0){
					cpuBtIdx = j;
					break;
				}
			}
			
			// ������ ã�� ���� ���� CPU BT�� ��ġ�ϸ� temp�� ���μ��� �����ϰ� loop ���� 
			if(minCpuBt == tempSRTN.CPU_BT[cpuBtIdx] && notYet){
				temp = tempSRTN;
				notYet = 0;
			}else{
				insertQueue(2, tempSRTN);
			}
		}
		
		// �����ִ� IO BT ã��
		for(i=1; i<temp.CYCLES*2-1; i+=2){
			if(temp.IO_BT[i] > 0){
				ioBtIdx = i;
				break;
			}
		}
		
		startTime = time;
		
		// ���� ���� CPU BT �Ҹ� 
		time += temp.CPU_BT[cpuBtIdx];
		temp.CPU_BT[cpuBtIdx] = 0;
		
		endTime = time;
		
		// ���μ��� BT�� �Ϸ���� ��� ���μ����� �Ϸ� ���·� �ٲٰ� �Ϸ� �ð� ����  		
		if(isComplete(temp)){
			temp.COMPLETE = 1;
			temp.COMPLETE_TIME = time;
		}
		
		// ���� Burst�� IO Burst�� ������ ��� : ���μ����� Wake-Up �Ǵ� �ð��� Sleep �ƴ� Queue ��ȣ�� ���� 
		if(temp.CPU_BT[cpuBtIdx] == 0 && temp.COMPLETE == 0){
			temp.WAKEUP = time + temp.IO_BT[ioBtIdx];
			temp.SLEEPED_QUEUE = 2;
		}
		
		fprintf(fp_out, "[Q2] Time %05d ~ %05d : P%03d\n", startTime, endTime, temp.PID);
		
		// BT �Ϸ�� Queue4 / Asleep ���·� ��ȯ�� Queue3 / ������ ���� �״�� Queue2 
		if(temp.COMPLETE == 1){
			insertQueue(4, temp);
			fprintf(fp_out, "P%03d Completed!\n", temp.PID);
		}else if(temp.COMPLETE == 0){
			if(temp.WAKEUP != -1) insertQueue(3, temp);
			else insertQueue(2, temp);
		}
		
		// Burst Time �Ҹ� �� Wake-Up �ƾ���� ���μ��� Wake-Up 
		wakeUp(startTime, endTime);
		
		if(sizeOfQueue(0) > 0){
			return 0;
		}
		if(sizeOfQueue(1) > 0){
			return 1;
		}
	}
	return -1;
}

int main(void){
	
	int numberOfPs, i, j, k, nextQueue = 0, sumTT = 0, sumWT = 0;
	Process process[1000];
	
	fp = fopen("input.txt", "r");
	fp_out = fopen("output.txt", "w");
	
	fscanf(fp, "%d", &numberOfPs);
	
	// input.txt�� ���� ���μ��� ���� �Է¹��� 
	for(i=0; i<numberOfPs; i++){
		fscanf(fp, "%d", &process[i].PID);
		fscanf(fp, "%d", &process[i].ARRIVAL_TIME);
		fscanf(fp, "%d", &process[i].CYCLES);
		
		process[i].TOTAL_BT = 0;
		
		for(j=0; j<process[i].CYCLES*2-1; j++){
			if(j%2 == 0){
				fscanf(fp, "%d", &process[i].CPU_BT[j/2]);
				process[i].TOTAL_BT += process[i].CPU_BT[j/2]; // Total burst time�� ����
			}
			else if(j%2 == 1){
				fscanf(fp, "%d", &process[i].IO_BT[j/2]);
			}
		}
		
		// Wake-Up �ð��� �Ϸ� ���� �ʱ�ȭ 
		process[i].WAKEUP = -1;
		process[i].COMPLETE = 0;
		
		// Arrival Time�� 0 �� ��쿡�� Queue 0���� ���� / ������ ���� Queue 5�� ���� ��� 
		if(process[i].ARRIVAL_TIME == 0) insertQueue(0, process[i]);
		else insertQueue(5, process[i]);
	}
	
	fclose(fp);
	
	// ť�� ��� ����� ������ loop 
	while( numberOfPs != sizeOfQueue(4) ){
		
		int numberOfQueue5 = sizeOfQueue(5);
		
		// Arrival Time üũ�Ͽ� ������ ���μ����� Queue 0�� ���� 
		for(i=0; i<numberOfQueue5; i++){
			Process temp = deleteQueue(5);
			
			if(temp.ARRIVAL_TIME <= time){
				insertQueue(0, temp);
				nextQueue = 0;
			}else{
				insertQueue(5, temp);
			}
		}
		
		// Arrival time�� ������ �ʴ� ��� ó��(������ ���μ����� ��� burst time�� �Ϸ�ǰ�
		// Arrival time�� �ʴ� ���μ������� Queue 5���� ��� �� �� ��)
		if( (sizeOfQueue(4) + numberOfQueue5) == numberOfPs && numberOfQueue5 > 0 ){
			Process temp = deleteQueue(5);
			
			time = temp.ARRIVAL_TIME;
			insertQueue(0, temp);
			nextQueue = 0;
		}
		
		// nextQueue�� ���� �����ٸ��ؾ��ϴ� Queue ��ȣ�� �޾� ����ؼ� �����ٸ�
		if(nextQueue == 0){
			nextQueue = scheduling0();
		}else if(nextQueue == 1){
			nextQueue = scheduling1();
		}else if(nextQueue == 2){
			nextQueue = scheduling2();
		}else{ // ��� ���μ����� Sleep ������ �� 
			nextQueue = wakeUpAll();
		}
	}
	
	// ��� ��� - �� ���μ��� �� TT �� WT / ��� TT �� WT 
	fprintf(fp_out, "\n====================== RESULT =======================\n");
	
	for(i=0; i<numberOfPs; i++){
		Process temp = deleteQueue(4);
		
		temp.TT = temp.COMPLETE_TIME - temp.ARRIVAL_TIME;
		temp.WT = temp.TT - temp.TOTAL_BT;
		
		sumTT += temp.TT;
		sumWT += temp.WT;
		
		fprintf(fp_out, "P%03d : Turnaround-Time = %05d / Waiting-Time = %05d\n", temp.PID, temp.TT, temp.WT);
	}
	
	fprintf(fp_out, "---------------------- Average ----------------------\n");
	
	fprintf(fp_out, "Avg. Turnaround-Time = %05d / Avg. Waiting-Time = %05d\n", sumTT / numberOfPs, sumWT / numberOfPs);
	
	fclose(fp_out);
	
	return 0;
}


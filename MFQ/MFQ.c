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

// 큐 5개 선언, 0~2:Ready Queue / 3:Sleep Queue / 4:Terminated(BT Completed process) Queue / 5:Arrival Time이 되기 전까지 대기하는 Queue
Process queue[6][MAX];
int front[6] = {0,0,0,0,0,0};
int rear[6] = {-1,-1,-1,-1,-1,-1};
int itemCount[6] = {0,0,0,0,0,0};
int time = 0;
FILE *fp, *fp_out;

// Queue를 Control 하는 Function들 정의 
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

// Wake-Up 해야되는 프로세스를 깨워서 상위 큐로 보내는 Function 
void wakeUp(int startT, int endT){
	int sizeOfSleepQueue = sizeOfQueue(3), i, toQueue;
	
	// Sleep Queue에 있는 프로세스들을 검사해서 Wake-Up 시켜야 하는 프로세스를 상위 Queue로 보냄 
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

// Queue0~2에 프로세스가 없을 경우를 대비해 Sleep Queue에 있는 프로세스를 Wake-up 하는 Function 
int wakeUpAll(){
	
	int isOk = 0; 
	
	while(1){
		int sizeOfSleepQueue = sizeOfQueue(3), i, toQueue;
	
		// Wake-Up 해야되는 프로세스를 깨워서 상위 큐로 보내기
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
		
		// 프로세스가 한 개 이상 깨어났을 때 반환하고 종료 
		if(isOk){
			if(sizeOfQueue(0) > 0) return 0;
			else if(sizeOfQueue(1) > 0) return 1;
			else if(sizeOfQueue(2) > 0) return 2;
		}
		time++;
	}
}

// 프로세스가 Burst Time을 모두 완료했는지 체크
int isComplete(Process temp){
	int i, completTestSum = 0;
	 
	// CPU 및 IO BT가 모두 완료되었는지 체크하기 위해 남은 Burst Time을 모두 더함 
	for(i=0; i<temp.CYCLES*2-1; i+=2){
		if(i % 2 == 0) completTestSum += temp.CPU_BT[i/2];
		else completTestSum += temp.IO_BT[i/2];
	}
	
	// Burst Time의 합이 0이면 Complete 상태로 변경하고 Complete된 시간을 저장 
	if(completTestSum == 0){
		return 1;
	}else{
		return 0;
	}
}

// Queue 0 스케줄링 Function 
int scheduling0(){
	
	while(!isEmpty(0)){
		Process temp = deleteQueue(0);
		
		int cpuBtIdx = 0, ioBtIdx = 0, i, completTestSum = 0, startTime, endTime;
	
		// 남아있는 CPU BT 찾음
		for(i=0; i<temp.CYCLES; i++){
			if(temp.CPU_BT[i] > 0){
				cpuBtIdx = i;
				break;
			}
		}
		
		// 남아있는 IO BT 찾음
		for(i=0; i<temp.CYCLES-1; i++){
			if(temp.IO_BT[i] > 0){
				ioBtIdx = i;
				break;
			}
		}
		
		startTime = time;
		
		// Time Quantum 2 동안 CPU BT 소모 
		if(temp.CPU_BT[cpuBtIdx] < 2){
			time += temp.CPU_BT[cpuBtIdx];
			temp.CPU_BT[cpuBtIdx] = 0;
		}else{
			temp.CPU_BT[cpuBtIdx] -= 2;
			time += 2;
		}
		
		endTime = time;
		
		// 프로세스 BT가 완료됐을 경우 프로세스를 완료 상태로 바꾸고 완료 시간 저장  
		if(isComplete(temp)){
			temp.COMPLETE = 1;
			temp.COMPLETE_TIME = time;
		}
		
		// 다음 Burst에 IO Burst가 나오는 경우 : 프로세스가 Wake-Up 되는 시간과 Sleep 됐던 Queue 번호를 저장
		if(temp.CPU_BT[cpuBtIdx] == 0 && temp.COMPLETE == 0){
			temp.WAKEUP = time + temp.IO_BT[ioBtIdx];
			temp.SLEEPED_QUEUE = 0;
		}
		
		// Burst Time 소모 중 Wake-Up 됐어야할 프로세스 Wake-Up 
		wakeUp(startTime, endTime);
		
		fprintf(fp_out, "[Q0] Time %05d ~ %05d : P%03d\n", startTime, endTime, temp.PID);
		
		// BT 완료시 Queue4 / Asleep 상태로 전환시 Queue3 / 나머지 경우는 Queue1
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

// Queue 1 스케줄링 Function
int scheduling1(){
	
	while(!isEmpty(1)){
		Process temp = deleteQueue(1);
		
		int cpuBtIdx = 0, ioBtIdx = 0, i, completTestSum = 0, startTime, endTime;
	
		// 남아있는 CPU BT 찾음
		for(i=0; i<temp.CYCLES; i++){
			if(temp.CPU_BT[i] > 0){
				cpuBtIdx = i;
				break;
			}
		}
		
		// 남아있는 IO BT 찾음
		for(i=0; i<temp.CYCLES-1; i++){
			if(temp.IO_BT[i] > 0){
				ioBtIdx = i;
				break;
			}
		}
		
		startTime = time;
		
		// Time Quantum 6 동안 CPU BT 소모 
		if(temp.CPU_BT[cpuBtIdx] < 6){
			time += temp.CPU_BT[cpuBtIdx];
			temp.CPU_BT[cpuBtIdx] = 0;
		}else{
			temp.CPU_BT[cpuBtIdx] -= 6;
			time += 6;
		}
		
		endTime = time;
		
		// 프로세스 BT가 완료됐을 경우 프로세스를 완료 상태로 바꾸고 완료 시간 저장  
		if(isComplete(temp)){
			temp.COMPLETE = 1;
			temp.COMPLETE_TIME = time;
		}
		
		// 다음 Burst에 IO Burst가 나오는 경우 : 프로세스가 Wake-Up 되는 시간과 Sleep 됐던 Queue 번호를 저장
		if(temp.CPU_BT[cpuBtIdx] == 0 && temp.COMPLETE != 1){
			temp.WAKEUP = time + temp.IO_BT[ioBtIdx];
			temp.SLEEPED_QUEUE = 1;
		}
		
		fprintf(fp_out, "[Q1] Time %05d ~ %05d : P%03d\n", startTime, endTime, temp.PID);
		
		// BT 완료시 Queue4 / Asleep 상태로 전환시 Queue3 / 나머지 경우는 Queue2 
		if(temp.COMPLETE == 1){
			insertQueue(4, temp);
			fprintf(fp_out, "P%03d Completed!\n", temp.PID);
		}else if(temp.COMPLETE == 0){
			if(temp.WAKEUP != -1) insertQueue(3, temp);
			else insertQueue(2, temp);
		}
		
		// Burst Time 소모 중 Wake-Up 됐어야할 프로세스 Wake-Up 
		wakeUp(startTime, endTime);
		
		if(sizeOfQueue(0) > 0){
			return 0;
		}
		
	}
	return 2;
}

// Queue 2 스케줄링 Function
int scheduling2(){
	
	while(!isEmpty(2)){
		Process temp;
		
		int cpuBtIdx = 0, ioBtIdx = 0, i, j, completTestSum = 0, startTime, endTime, minCpuBt = 999999, sizeOfQueue2 = sizeOfQueue(2), notYet=1;
		
		// SRTN 스케줄링을 위해 큐 안에 있는 프로세스들 중에서 최소 CPU BT 찾기 
		for(i=0; i<sizeOfQueue2; i++){
			Process tempSRTN = deleteQueue(2);
					
			// 남아있는 CPU BT 찾음
			for(j=0; j<tempSRTN.CYCLES; j++){
				if(tempSRTN.CPU_BT[j] > 0){
					cpuBtIdx = j;
					break;
				}
			}
			
			// 가장 작은 CPU BT를 찾음
			if(minCpuBt > tempSRTN.CPU_BT[cpuBtIdx]){
				minCpuBt = tempSRTN.CPU_BT[cpuBtIdx];
			}
			
			insertQueue(2, tempSRTN);
		}
		
		// 위에서 찾은 최소 CPU BT와 일치하는 프로세스를 temp에 저장
		for(i=0; i<sizeOfQueue2; i++){
			Process tempSRTN = deleteQueue(2);
			
			// 남아있는 CPU BT 찾음
			for(j=0; j<tempSRTN.CYCLES; j++){
				if(tempSRTN.CPU_BT[j] > 0){
					cpuBtIdx = j;
					break;
				}
			}
			
			// 위에서 찾은 가장 작은 CPU BT와 일치하면 temp에 프로세스 저장하고 loop 종료 
			if(minCpuBt == tempSRTN.CPU_BT[cpuBtIdx] && notYet){
				temp = tempSRTN;
				notYet = 0;
			}else{
				insertQueue(2, tempSRTN);
			}
		}
		
		// 남아있는 IO BT 찾음
		for(i=1; i<temp.CYCLES*2-1; i+=2){
			if(temp.IO_BT[i] > 0){
				ioBtIdx = i;
				break;
			}
		}
		
		startTime = time;
		
		// 가장 작은 CPU BT 소모 
		time += temp.CPU_BT[cpuBtIdx];
		temp.CPU_BT[cpuBtIdx] = 0;
		
		endTime = time;
		
		// 프로세스 BT가 완료됐을 경우 프로세스를 완료 상태로 바꾸고 완료 시간 저장  		
		if(isComplete(temp)){
			temp.COMPLETE = 1;
			temp.COMPLETE_TIME = time;
		}
		
		// 다음 Burst에 IO Burst가 나오는 경우 : 프로세스가 Wake-Up 되는 시간과 Sleep 됐던 Queue 번호를 저장 
		if(temp.CPU_BT[cpuBtIdx] == 0 && temp.COMPLETE == 0){
			temp.WAKEUP = time + temp.IO_BT[ioBtIdx];
			temp.SLEEPED_QUEUE = 2;
		}
		
		fprintf(fp_out, "[Q2] Time %05d ~ %05d : P%03d\n", startTime, endTime, temp.PID);
		
		// BT 완료시 Queue4 / Asleep 상태로 전환시 Queue3 / 나머지 경우는 그대로 Queue2 
		if(temp.COMPLETE == 1){
			insertQueue(4, temp);
			fprintf(fp_out, "P%03d Completed!\n", temp.PID);
		}else if(temp.COMPLETE == 0){
			if(temp.WAKEUP != -1) insertQueue(3, temp);
			else insertQueue(2, temp);
		}
		
		// Burst Time 소모 중 Wake-Up 됐어야할 프로세스 Wake-Up 
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
	
	// input.txt로 부터 프로세스 정보 입력받음 
	for(i=0; i<numberOfPs; i++){
		fscanf(fp, "%d", &process[i].PID);
		fscanf(fp, "%d", &process[i].ARRIVAL_TIME);
		fscanf(fp, "%d", &process[i].CYCLES);
		
		process[i].TOTAL_BT = 0;
		
		for(j=0; j<process[i].CYCLES*2-1; j++){
			if(j%2 == 0){
				fscanf(fp, "%d", &process[i].CPU_BT[j/2]);
				process[i].TOTAL_BT += process[i].CPU_BT[j/2]; // Total burst time을 구함
			}
			else if(j%2 == 1){
				fscanf(fp, "%d", &process[i].IO_BT[j/2]);
			}
		}
		
		// Wake-Up 시간과 완료 여부 초기화 
		process[i].WAKEUP = -1;
		process[i].COMPLETE = 0;
		
		// Arrival Time이 0 인 경우에만 Queue 0으로 삽입 / 나머지 경우는 Queue 5로 가서 대기 
		if(process[i].ARRIVAL_TIME == 0) insertQueue(0, process[i]);
		else insertQueue(5, process[i]);
	}
	
	fclose(fp);
	
	// 큐가 모두 비어질 때까지 loop 
	while( numberOfPs != sizeOfQueue(4) ){
		
		int numberOfQueue5 = sizeOfQueue(5);
		
		// Arrival Time 체크하여 도착한 프로세스는 Queue 0로 보냄 
		for(i=0; i<numberOfQueue5; i++){
			Process temp = deleteQueue(5);
			
			if(temp.ARRIVAL_TIME <= time){
				insertQueue(0, temp);
				nextQueue = 0;
			}else{
				insertQueue(5, temp);
			}
		}
		
		// Arrival time이 지극히 늦는 경우 처리(나머지 프로세스는 모두 burst time이 완료되고
		// Arrival time이 늦는 프로세스들이 Queue 5에서 대기 중 일 때)
		if( (sizeOfQueue(4) + numberOfQueue5) == numberOfPs && numberOfQueue5 > 0 ){
			Process temp = deleteQueue(5);
			
			time = temp.ARRIVAL_TIME;
			insertQueue(0, temp);
			nextQueue = 0;
		}
		
		// nextQueue에 다음 스케줄링해야하는 Queue 번호를 받아 계속해서 스케줄링
		if(nextQueue == 0){
			nextQueue = scheduling0();
		}else if(nextQueue == 1){
			nextQueue = scheduling1();
		}else if(nextQueue == 2){
			nextQueue = scheduling2();
		}else{ // 모든 프로세스가 Sleep 상태일 때 
			nextQueue = wakeUpAll();
		}
	}
	
	// 결과 출력 - 각 프로세스 별 TT 및 WT / 평균 TT 및 WT 
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


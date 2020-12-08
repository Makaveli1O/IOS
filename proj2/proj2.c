/****************************************
 * IOS 2.projekt Faneuil Hall Problem	*
 *	2019/2020 VUT FIT		*
 *Riešienie: 				*
 *		Samuel Líška(xliska20)	*
 ****************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h> //fork + sleep
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
/********************************
 * 	MACROS DEFINITIONS 	*
 ********************************/
#define SHM_NAME "/xliska20"

#define FILE_CLOSE 							\
	do { 								\
		if (fclose(output) == EOF)				\
		{							\
			fprintf(stderr, "%s\n", "ERROR: Closing file");	\
		}							\
	} while (0)

#define CLOSE_SEMS \
	sem_close(noJudge);\
	sem_close(checked);\
	sem_close(confirmed);\
	sem_close(allSigned);\

#define UNLINK_SEMS	\
	sem_unlink("/xliskanoJudge"); \
	sem_unlink("/xliskachecked");\
	sem_unlink("/xliskaconfirmed");\
	sem_unlink("/xliskaallSigned");\
	sem_unlink("/xliskafile");\
	sem_unlink("/xliskaallCertificated");


/********************************
 * 	FUNCTIONS DEFINITIONS 	*
 ********************************/

/* Function: argumentHandler
 * Takes 2 arguments number of arguments(argc) and arguments themselves(argv)
 * Returns 0 if arguments are fine. Otherwise returns number indicating specific error.
 * If succes: 	argv[1] -> PI
 *				argv[2] -> IG
 *				argv[3] -> JG
 *				argv[4] -> IT
 *				argv[5] -> JT
 */
int argumentHandler(int argc, char* argv[]);

int mapMemory();

int unmapMemory();

void destroySemaphores();

void start(char* process, int ID);

void entered(char* process, int ID);

void checkIn(char *process, int ID);

void createImmigrant();

void createJudge();

void getCertificate();

void wantsCertificate();

void leave();
/********************************
  *	      Semaphores         *
  ********************************/
/* 
 * @var noJudge - acts as a turnstile for incoming immigrants and spectators; it also protects entered, which counts the number of immigrants in the room
 * @var checked - counts the number of immigrants who have checked in.
 * it is protected by checked. Confirmed signals that the judge has executed confirm.
 * */
sem_t *noJudge;
sem_t *checked;
sem_t *confirmed;
sem_t *allSigned;
sem_t *filesem;
sem_t *allCertificated;

/********************************
 *	 Shared memory		*
 * *****************************/
int *actions; 
int *judge;
int *immigrant;
int *imm_ID;
int *NE;
int *NC;
int *NB;
int *US_citizens;
int *imm_confirmed;
int *citizens_goal;
int *waiting_certification;
int *confirmation;
int *tmp;


FILE* output;
pid_t pid_IMM, pid_JUD, pid_HELP;
int args[5];
int main(int argc, char* argv[]){
	if(argumentHandler(argc, argv) != 0){
		exit(1);
	}

	UNLINK_SEMS;
	noJudge = sem_open("/xliskanoJudge", O_CREAT | O_EXCL, 0644, 1);//sem_init(noJudge, 1, 1);
	checked = sem_open("/xliskachecked", O_CREAT | O_EXCL, 0644, 1);//sem_init(checked, 1, 1);
	confirmed = sem_open("/xliskaconfirmed", O_CREAT | O_EXCL, 0644, 0);//sem_init(confirmed, 1, 0);
	allSigned = sem_open("/xliskaallSigned", O_CREAT | O_EXCL, 0644, 0);//sem_init(allSigned, 1 ,0);
	filesem = sem_open("/xliskafile", O_CREAT | O_EXCL, 0644, 1);
	allCertificated = sem_open("/xliskaallCertificated", O_CREAT | O_EXCL, 0644, 0);
	if(noJudge == SEM_FAILED || checked == SEM_FAILED || confirmed == SEM_FAILED || checked == SEM_FAILED || filesem == SEM_FAILED){
		fprintf(stderr,"ERROR initializing semaphores: %d\t %s",errno, strerror(errno));
		UNLINK_SEMS;
		exit(1);
	}

	if ((output = fopen("proj2.out", "w+")) == NULL){ 
        fprintf(stderr, "ERROR:File either does not exist, or can't be opened.");
		FILE_CLOSE;
		exit(1);
	}

	int shm = shm_open(SHM_NAME, O_RDWR|O_CREAT, 0644);
	if(shm == -1){
		fprintf(stderr,"ERROR:Shared memory was not created.");
		shm_unlink(SHM_NAME);
		close(shm);
		fclose(output);
		exit(1);
	}
	/*Closing buffers before usage*/
	setbuf(output, NULL);
	setbuf(stderr, NULL);	
	
	//sizeof(int*)*13 because there are 13 variables	
	if(ftruncate(shm, sizeof(int*)*13)){
		shm_unlink(SHM_NAME);
		close(shm);
		fclose(output);
		fprintf(stderr,"ERROR: Ftruncate has failed.");
		exit(1);
	}
	if(mapMemory() != 0){
		shm_unlink(SHM_NAME);
		close(shm);
		fclose(output);
		fprintf(stderr,"ERROR: Mapping memory has failed.");
		exit(1);
	}

	pid_HELP = fork();
	//child proces successfully created
	//GENERATOR
	if(pid_HELP == 0){
		int IMM_sleeping;
		//generator
		for (int i = 0; i < args[0]; ++i){
			if(args[1] != 0){
				srand(time(0)*getpid());
				IMM_sleeping = rand()%(args[1]+1);
				usleep(IMM_sleeping*1000);
			}else{
				IMM_sleeping=0;
			}
			
			pid_IMM = fork();
			if(pid_IMM == 0){
				createImmigrant();
				exit(0); //children dies after immigrant is done working
			}else{
				//parent starts creating another immigrant
			}

		}	
		exit(0);
	//parent process MAIN
	}else if(pid_HELP > 0){
		pid_JUD = fork();
		if(pid_JUD == 0){
			int JUDGE_sleeping;
			while(*US_citizens != args[0]){
				if(args[2] != 0){
					srand(time(0)*getpid());
					JUDGE_sleeping = rand()%(args[2]+1);
					usleep(JUDGE_sleeping*1000);
				}else{
					JUDGE_sleeping=0;
				}
				
				if (*judge == 0){
						createJudge();
						continue;
				}
			}
			exit(0);
		}else{//main process wait for generator to end
			int judge_wait;
			int help_wait;
			int imm_wait;
			waitpid(pid_IMM, &imm_wait, 0);
			waitpid(pid_JUD, &judge_wait, 0); //this messes up order of starting immigrants
			waitpid(pid_HELP,&help_wait,0); //wait for all children to end
			//if ( help_wait == -1)
			//{
			//	fprintf(stderr, "ERROR: Process termination error.\n");
			//}
		}

	}else if(pid_HELP < 0){//fork error
		shm_unlink(SHM_NAME);
		close(shm);
		if(unmapMemory() == 1){
			fprintf(stderr,"ERROR: Unmapping memory has failed!\n");
		}
		FILE_CLOSE;
		UNLINK_SEMS;
		fprintf(stderr,"ERROR: Fork has failed.");
		exit(1);
	}

	/*Free everything at the end of MAIN*/
	fprintf(stderr, "ENDING\n");
	shm_unlink(SHM_NAME);
	UNLINK_SEMS;
	destroySemaphores();
	FILE_CLOSE;
	return 0;
}
/********************************
 * 	FUNCTION BODIES 	*
 ********************************/

int argumentHandler(int argc, char* argv[]){
	char* endptr = NULL;
	if(argc < 6){
		fprintf(stderr,"ERROR: Too less input arguments.");
		return 1;
	}else if(argc > 6){
		fprintf(stderr,"ERROR: Too many input arguments.");
		return 2;
	}else{
		for(int i = 1;i < 6; i++){
			int arg = strtoul(argv[i], &endptr, 10);
			if(!(endptr[0] == '\0')){
				fprintf(stderr,"ERROR: One of input arguments is not an integer.");
				return 3;
			}else if(i == 1){
				if(!(arg >= 1)){
					fprintf(stderr,"ERROR: Number of immigrants must be >= 1.");
					return 4;
				}
			}else if((i == 2) || (i == 3) || (i == 4) || (i == 5) || (i == 6)){
				if(!(arg >= 0 && arg <= 2000)){
					fprintf(stderr,"ERROR: Given time (IG|JG|IT|JT) does not meet requirements.");
					return 4;
				}
			}
		args[i-1] = arg;
		}
	}
	return 0;
}

int mapMemory(){
    actions = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);     
    NE = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);          
    NB = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);          
    NC = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);          
	judge = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	imm_ID = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	US_citizens = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	confirmation = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	waiting_certification = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	tmp = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	imm_confirmed = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	citizens_goal= (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(actions == MAP_FAILED || NE == MAP_FAILED || NB == MAP_FAILED || NC == MAP_FAILED || judge == MAP_FAILED || imm_ID == MAP_FAILED || US_citizens == MAP_FAILED || confirmation == MAP_FAILED || waiting_certification == MAP_FAILED || tmp == MAP_FAILED || imm_confirmed == MAP_FAILED || citizens_goal == MAP_FAILED){
		return 1;
	}
	*actions = 0;           
    *NE = 0;                
    *NB = 0;                
    *NC = 0;
	*imm_ID = 0;
	*US_citizens = 0;
	*citizens_goal = args[0];
	*waiting_certification = args[3];
	*confirmation = args[4];
	*tmp = 0;
	*imm_confirmed = 0;
	return 0;
}

int unmapMemory(){
	if(	munmap(actions, sizeof(int*)) != 0 	||
		munmap(NE, sizeof(int*)) != 0	 	||
		munmap(NB, sizeof(int*)) != 0		||
		munmap(NC, sizeof(int*)) != 0		||
		munmap(judge, sizeof(int*)) != 0	||
		munmap(imm_ID, sizeof(int*)) != 0	||
		munmap(US_citizens, sizeof(int*)) != 0	||
		munmap(confirmation, sizeof(int*)) != 0	||
		munmap(waiting_certification, sizeof(int*)) != 0 ||
		munmap(checked, sizeof(int*)) == 0 ||
		munmap(imm_confirmed, sizeof(int*)) == 0 ||
		munmap(citizens_goal, sizeof(int*)) == 0 ||
		munmap(immigrant, sizeof(int *)) == 0

	)
		return 1;
	else	
		return 0;
}

void destroySemaphores(){
	sem_destroy(noJudge);
	sem_destroy(confirmed);
	sem_destroy(allSigned);
	sem_destroy(checked);
	sem_destroy(filesem);
	sem_destroy(allCertificated);
	return;
}

void start(char* process, int ID){
	*actions = *actions + 1;
	fprintf(output,"%d: %s %d: starts\n",*actions, process, ID);
	return;
}

void enter(char* process, int ID){
	*actions = *actions + 1;
	*NE= *NE + 1; //entered the building
	*NB = *NB + 1;
	fprintf(output,"%d: %s %d: enters: %d: %d: %d\n",*actions, process, ID, *NE, *NC, *NB);
	return;
}

void checkIn(char* process, int ID){
	*actions= *actions + 1;
	*NC = *NC + 1; //checked in
	fprintf(output,"%d: %s %d: checks: %d: %d: %d\n",*actions, process, ID, *NE, *NC, *NB);
	return;
}

void wantCertificate(char* process, int ID){
	*actions = *actions + 1;
	fprintf(output,"%d: %s %d: wants certificate: %d: %d: %d\n",*actions, process, ID, *NE, *NC, *NB);
	return;
}

void gotCertificate(char* process, int ID){
	*actions = *actions + 1;
	*imm_confirmed = *imm_confirmed + 1;
	fprintf(output,"%d: %s %d: got certificate: %d: %d: %d\n",*actions, process, ID, *NE, *NC, *NB);
	return;
}



void confirm_start(){
	*actions = *actions + 1;
	fprintf(output,"%d: JUDGE: starts confirmation: %d: %d: %d\n",*actions, *NE, *NC, *NB);
	return;
}

void confirm_end(){
	*tmp = *NC; //save for later
	*NE = 0;
	*NC = 0;
	*actions = *actions + 1;
	fprintf(output,"%d: JUDGE: ends confirmation: %d: %d: %d\n",*actions, *NE, *NC, *NB);
	return;
}

void leave(char* process, int ID){
	*actions = *actions + 1;
	*imm_confirmed = *imm_confirmed - 1;
	*US_citizens +=1;
	*NB = *NB - 1;
	fprintf(output,"%d: %s %d: leaves: %d: %d: %d\n",*actions, process, ID, *NE, *NC, *NB);
	return;
}

void createImmigrant(){
	*imm_ID = *imm_ID + 1;
	int ID = *imm_ID;
	//multiple immigrants waits here in some cases, however they wont start in desired order
	sem_wait(filesem);
	start("IMM", ID);
	sem_post(filesem);
	sem_wait(noJudge); //wants to enter if no judge is in the building
	

	sem_wait(filesem);
	enter("IMM", ID);
	sem_post(filesem);
	sem_post(noJudge);
	sem_wait(checked);
	sem_wait(filesem);
	checkIn("IMM", ID);
	sem_post(filesem);
	// if judge is in the building, and all entered ale checked in
	if(*judge == 1 && *NE == *NC){
		sem_post(allSigned);
	}else{
		sem_post(checked);//let another one in
	}
	sem_wait(confirmed);

	sem_wait(filesem);
	wantCertificate("IMM", ID);
	sem_post(filesem);
	int time_ = 0;
	int waiting = 0;

	if(*waiting_certification != 0){
		srand(time(0)*getpid());
		time_ = *waiting_certification;
		waiting = rand()%(time_+1);
		fprintf(stderr,"IMM: waits for %d\n",waiting);
		usleep(waiting*1000);
	}
	sem_wait(filesem);
	gotCertificate("IMM", ID);
	sem_post(filesem);

	if(*imm_confirmed == *tmp){
		sem_post(allCertificated);
	}else{
		sem_post(confirmed);//let another one in
	}
	sem_wait(noJudge);
	sem_wait(filesem);
	leave("IMM", ID);
	sem_post(filesem);
	sem_post(noJudge);
}

void createJudge(){
	if(*US_citizens == *citizens_goal){
		sem_wait(filesem);
		*actions = *actions + 1;
		fprintf(output,"%d: JUDGE: finishes\n",*actions);
		sem_post(filesem);
		exit(0);
	}else{
		sem_wait(filesem);
		*actions = *actions + 1;
		fprintf(output,"%d: JUDGE: wants to enter\n",*actions);
		sem_post(filesem);
	}
	
	sem_wait(noJudge);//stops new incomers(after enter)
	sem_wait(checked);// block checkins
	/*JUDGE enters*/
	sem_wait(filesem);
	*actions = *actions + 1;
	fprintf(output,"%d: JUDGE: enters: %d: %d: %d\n",*actions, *NE, *NC, *NB);
	sem_post(filesem);
	*judge=1;
	//judge is in the building
	//fprintf(stderr, "%d %d\n", *NE, *NC);
	if(*NE > *NC){
		sem_wait(filesem);
		*actions = *actions + 1;
		fprintf(output,"%d: JUDGE: wait for imm: %d\n",*actions,*imm_ID);
		sem_post(filesem);
		sem_post(checked); //unblock checkins
		sem_wait(allSigned); //locks all signed in until they fill again
		sem_wait(checked); //block back
	}
	//NE == NC
	sem_wait(filesem);
	confirm_start();
	int time_ = *confirmation;
	int confirming_leaving = 0;
	if(time_ != 0){
			srand(time(0)*getpid());
			confirming_leaving = rand()%(time_+1);
			usleep(confirming_leaving*1000);
	}

	confirm_end();//after this they should shout recieving certificates
	sem_post(filesem);
	//release confirmed so imms can got certificates

	if(*tmp != 0)
		sem_post(confirmed);
	
	//wait till confirmed got certificate
	
	if (*tmp == 0){
		sem_post(allCertificated);
	}	//if not everybody checked got certificate wait for them
	sem_wait(allCertificated);
	sem_wait(filesem);
	time_ = *confirmation;
	confirming_leaving = 0;
	if(time_ != 0){
			srand(time(0)*getpid());
			confirming_leaving = rand()%(time_+1);
			usleep(confirming_leaving*1000);
	}
	*actions = *actions + 1;
	fprintf(output,"%d: JUDGE: leaves: %d: %d: %d\n",*actions, *NE, *NC, *NB);
	sem_post(filesem);
	*judge = 0;
	sem_post(checked);
	sem_post(noJudge);
	if(*US_citizens == *citizens_goal){
		sem_wait(filesem);
		*actions = *actions + 1;
		fprintf(output,"%d: JUDGE: finishes\n",*actions);
		sem_post(filesem);
		exit(0);
	}
}

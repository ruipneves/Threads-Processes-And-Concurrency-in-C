#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_t tid[2];

typedef struct {
int number;
char nickname[50];
char FIFOname[50];
char hand[50][4];
int handsize;
char ultimacarta[4];
int numcards[4];
size_t sizof;
} jogadores;

typedef struct {
jogadores vec[50];
char shmname[50];
int players;
int nplayers;
int turn;
int roundnumber;
int dealer;
char lasttablecards[50][5];
int cplayed;
int totalrounds;
char tablecards[50][5];
int njogaram;
pthread_mutexattr_t attrmutex;
pthread_mutex_t globalmutex;
pthread_condattr_t attrcond;
pthread_cond_t globalcond;
int gameover;
} mesa1;

mesa1 *temp_memory;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t condition_var2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* InputThread(void *arg)
{
	pthread_mutex_lock(&mutex);
    unsigned long i = 0;
    pthread_t id = pthread_self();
	
    if(pthread_equal(id,tid[0]))
    {
	 printf("\n First thread processing\n");
	int poscard;
        int existe=0;
	time_t t=time(NULL);
struct tm mytime=*localtime(&t);
	FILE *fp;
				char carta[3];
				while(existe==0)
				{
					printf("Your hand: \n");
					for(i=0;i<temp_memory->vec[temp_memory->turn].handsize;i++)
					{	
						printf("%s\n",temp_memory->vec[temp_memory->turn].hand[i]);
					}
					printf("Que carta deseja jogar?\n");
					scanf("%s",carta);
					printf("%s\n",carta);
					int p;
					for(p=0;p<temp_memory->vec[temp_memory->turn].handsize;p++)
					{
						printf("entrouciclo\n");
						printf("Carta: %s\n",temp_memory->vec[temp_memory->turn].hand[p]);
						if(strcmp(temp_memory->vec[temp_memory->turn].hand[p],carta)==0)
						{
							printf("existe\n");
							poscard=p;
							existe=1;
						}
					}
				}
				printf("Carta: %s\n",carta);
				strcpy(temp_memory->lasttablecards[temp_memory->cplayed],carta);
				strcpy(temp_memory->vec[temp_memory->turn].ultimacarta,carta);
				temp_memory->cplayed+=1;
				//Delete played card from deck
				int p;
				printf("POSCARD: %d\n",poscard);
				printf("HANDSIZE: %d\n",temp_memory->vec[temp_memory->turn].handsize-1);
				for(p=poscard;p<temp_memory->vec[temp_memory->turn].handsize-1;p++)
				{
						strcpy(temp_memory->vec[temp_memory->turn].hand[p],temp_memory->vec[temp_memory->turn].hand[p+1]);
				}
				temp_memory->vec[temp_memory->turn].handsize-=1;
		char date[50];
	fp = fopen(temp_memory->shmname, "a");
			sprintf(date,"%d-%d-%d %d:%d:%d",mytime.tm_year+1900,mytime.tm_mon+1,mytime.tm_mday,mytime.tm_hour,mytime.tm_min,mytime.tm_sec);
			fprintf(fp,"%25s|Player%d-%17s|play                     |%s\n",date,temp_memory->vec[temp_memory->turn].number,temp_memory->vec[temp_memory->turn].nickname,carta);
			fprintf(fp,"%25s|Player%d-%17s|hand                     |",date,temp_memory->vec[temp_memory->turn].number,temp_memory->vec[temp_memory->turn].nickname);
			int cx=0;
			int countc=1;
			for(i=0;i<temp_memory->vec[temp_memory->turn].handsize;i++)
			{
				if(countc==temp_memory->vec[temp_memory->turn].numcards[cx] && temp_memory->vec[temp_memory->turn].numcards[cx]>1)
				{
					fprintf(fp,"%s",temp_memory->vec[temp_memory->turn].hand[i]);
					fprintf(fp,"/");
					cx++;
					countc=1;
				}
				else
				{
					if(i!=temp_memory->vec[temp_memory->turn].handsize-1)
					{
						fprintf(fp,"%s",temp_memory->vec[temp_memory->turn].hand[i]);
						fprintf(fp,"-");
					}
					else
					{
						fprintf(fp,"%s",temp_memory->vec[temp_memory->turn].hand[i]);
					} 
					countc+=1;
				}
			}
			fprintf(fp,"\n");
			fclose(fp);
    }
	pthread_cond_signal(&condition_var2);
	pthread_mutex_unlock( &mutex);
	printf("\n First thread ended\n");

    return NULL;
}

void* RoundThread(void *arg)
{
	pthread_mutex_lock(&mutex);
    unsigned long i = 0;
    pthread_t id = pthread_self();

    if(pthread_equal(id,tid[1]))
    {
        printf("\n Second thread processing\n");
	if(temp_memory->turn+1==temp_memory->nplayers)
	{
		temp_memory->turn=0;
	}
	else
	{
		temp_memory->turn+=1;
	}
	temp_memory->njogaram+=1;
	if(temp_memory->njogaram==temp_memory->nplayers)
	{
		temp_memory->roundnumber=temp_memory->roundnumber+1;
		temp_memory->cplayed=0;
		temp_memory->njogaram=0;
	}
    }
	pthread_cond_broadcast(&temp_memory->globalcond);
	pthread_mutex_unlock( &mutex );
	printf("\n Second thread ended\n");
	printf("ROUND: %d\n",temp_memory->roundnumber);
	printf("TURN: %d\n",temp_memory->turn);

    return NULL;
}

int main(int argc,char *argv[])
{ 	
	time_t t=time(NULL);
struct tm mytime=*localtime(&t);
	int jog=0;
	int numjogs=0;
	char nome[50];
	char shm[50];
	numjogs=atoi(argv[3]);
	printf("Numero jogs: %d\n",numjogs);

	strcpy(nome,argv[1]);
	strcpy(shm,"/");
	strcat(shm,argv[2]);
	printf("%s\n",shm);
	//SHM VARS
	int shm_descritor;
	void *shm_addr;

	//PARA ABRIR/CRIAR A SHARED MEM.
	shm_descritor = shm_open(shm, O_RDWR|O_CREAT|O_EXCL, 0666);
	printf("%d\n",shm_descritor);
	if(shm_descritor==-1)
	{
		printf("if\n");
		shm_descritor = shm_open(shm, O_RDWR, 0666);
	}
	else
	{	
		printf("else\n");
		//PARA DIMENSIONAR A SHARED MEM.
		ftruncate(shm_descritor,sizeof(mesa1));
	}

	//PARA CARREGAR TUDO O QUE ESTA NA SHARED MEM. PARA O TEU PROCESSO LOCAL
	shm_addr= mmap(NULL,sizeof(mesa1),PROT_READ|PROT_WRITE,MAP_SHARED,shm_descritor,0);
	temp_memory=(mesa1 *) shm_addr;
	
	jogadores player;
	
	player.number=temp_memory->players;
	strcpy(player.nickname,argv[1]);
	strcpy(player.FIFOname,"/tmp/");
	strcat(player.FIFOname,"FIFO");
	strcat(player.FIFOname,nome);
	
	if(player.number==0)
	{
		pthread_mutexattr_init(&temp_memory->attrmutex);
		pthread_mutexattr_setpshared(&temp_memory->attrmutex,PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(&temp_memory->globalmutex,&temp_memory->attrmutex);

		pthread_condattr_init(&temp_memory->attrcond);
		pthread_condattr_setpshared(&temp_memory->attrcond,PTHREAD_PROCESS_SHARED);
		pthread_cond_init(&temp_memory->globalcond,&temp_memory->attrcond);

	}
	int createfifo;
	int f3;
	createfifo = mkfifo(player.FIFOname,0666);
	f3=open(player.FIFOname,O_RDWR);
	printf("createfifo %d\n",createfifo);
	
	temp_memory->nplayers=numjogs;
	temp_memory->vec[temp_memory->players]=player;
	printf("Numero Jogs : %d\n",temp_memory->players);
	temp_memory->players=temp_memory->players+1;

	printf("Numero Jogs Apos : %d\n",temp_memory->players);
	
	if(shm_descritor==3)
	{
		temp_memory->gameover=0;
		temp_memory->totalrounds=(52/numjogs);
		temp_memory->turn=0;
		temp_memory->roundnumber=0;
	}
	int i=0;
	int x=0;
	int count=0;
	for(i=0;i<temp_memory->players;i++)
	{
		printf("%s\n",temp_memory->vec[i].nickname);
		printf("%s\n",temp_memory->vec[i].FIFOname);
	}	
	
	char cards[52][4];
	char numbs[13][3]={"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
	char types[4][2]={"c","d","h","s"};
	if(shm_descritor==3)
	{
		for(i=0;i<4;i++)
		{
			for(x=0;x<13;x++)
			{
				strcpy(cards[count],numbs[x]);
				strcat(cards[count],types[i]);
				count++;
			}
		}
		for(i=0;i<count;i++)
		{
			printf("%s\n",cards[i]);
		}
	}
	if(temp_memory->players<temp_memory->nplayers)
	{
		if(pthread_mutex_trylock(&temp_memory->globalmutex)!=0)
		{
			pthread_mutex_lock(&temp_memory->globalmutex);
		}
		printf("Waiting for all players to connect...\n");
		pthread_cond_wait(&temp_memory->globalcond,&temp_memory->globalmutex);
		pthread_mutex_unlock(&temp_memory->globalmutex);
	}
	else
	{
		pthread_cond_broadcast(&temp_memory->globalcond);
	}

	//decide dealer
	temp_memory->dealer=0;
	
	char buffer[160];
	int f1, f2;
	FILE *fp;
	//if the player is the dealer, shuffles deck and gives cards
	if(player.number==0)
	{	

		//open shm_file to write
		char shmname[50];
		char date[50];
		strcpy(temp_memory->shmname,argv[2]);
		strcat(temp_memory->shmname,".txt");
      		fp = fopen(temp_memory->shmname, "w");
		fprintf(fp,"when                     |who                      |what                     |result\n");
		sprintf(date,"%d-%d-%d %d:%d:%d",mytime.tm_year+1900,mytime.tm_mon+1,mytime.tm_mday,mytime.tm_hour,mytime.tm_min,mytime.tm_sec);
		fprintf(fp,"%25s|Dealer-%18s|deal                     |-\n",date,player.nickname);
		fclose(fp);
		//shuffle deck
		for(i=0;i<count-1;i++)
		{
			srand(time(NULL));
			size_t j = i + rand() / (RAND_MAX / (count-i)+1);
			char c[4];
			strcpy(c,cards[j]);
			strcpy(cards[j],cards[i]);
			strcpy(cards[i],c);
		}
		for(i=0;i<count;i++)
		{
			printf("%s\n",cards[i]);
		}
		int y;
		for(y=0;y<temp_memory->nplayers;y++)
		{
			printf("%s\n",temp_memory->vec[y].FIFOname);
			f1=open(temp_memory->vec[y].FIFOname,O_WRONLY | O_NONBLOCK);
			//Order cards to player
			int h,l,k=0;
			int count2=0;
			char cardstemp[52][4];
			temp_memory->vec[player.number].numcards[0]=0;
			temp_memory->vec[player.number].numcards[1]=0;
			temp_memory->vec[player.number].numcards[2]=0;
			temp_memory->vec[player.number].numcards[3]=0;
			for(i=0;i<4;i++)
			{
				for(h=0;h<13;h++)
				{
					for(x=temp_memory->vec[y].number*(52/numjogs);x<temp_memory->vec[y].number*(52/numjogs)+(52/numjogs);x++)
					{
						k=0;
						int len=0;
						while(cards[x][k]!='\0')
						{
							k++;
							len++;
						}
						if(cards[x][len-1]==types[i][0])
						{
							char ch[len-1];
							int cnt=0;
							while(cnt!=len-1)
							{
								ch[cnt]=cards[x][cnt];
								cnt++;
							}
							ch[len-1]='\0';
							if(strcmp(ch,numbs[h])==0)
							{
								strcpy(cardstemp[count2],cards[x]);
								count2++;
								temp_memory->vec[player.number].numcards[i]+=1;
							}
						}
					}
				}
			}
			for(i=0;i<count2;i++)
			{	
				printf("%s\n",cardstemp[i]);
			}
			write(f1,cardstemp,sizeof(cardstemp));
			temp_memory->vec[y].sizof=sizeof(cardstemp);
			fp = fopen(temp_memory->shmname, "a");
			sprintf(date,"%d-%d-%d %d:%d:%d",mytime.tm_year+1900,mytime.tm_mon+1,mytime.tm_mday,mytime.tm_hour,mytime.tm_min,mytime.tm_sec);
			fprintf(fp,"%25s|Player%d-%17s|receive_cards            |\n",date,y,player.nickname);
			int cx=0;
			int countc=1;
			for(i=0;i<count2;i++)
			{
				if(countc==temp_memory->vec[player.number].numcards[cx] && temp_memory->vec[player.number].numcards[cx]>1)
				{
					fprintf(fp,"%s",cardstemp[i]);
					fprintf(fp,"/");
					cx++;
					countc=1;
				}
				else
				{
					if(i!=count2-1)
					{
						fprintf(fp,"%s",cardstemp[i]);
						fprintf(fp,"-");
					}
					else
					{
						fprintf(fp,"%s",cardstemp[i]);
					} 
					countc+=1;
				}
			}
			fprintf(fp,"\n");
			fclose(fp);
			close(f1);
			char cds[52][4];
		
		}
		//decide who's going to start playing
		srand(time(NULL));
		jog = rand()%temp_memory->nplayers;
		printf("Jogador a jogar: %d\n",jog);
		temp_memory->turn=jog;
		pthread_cond_broadcast(&temp_memory->globalcond);
	}
	int f4,g;
	int njogaram=0;
	char cardsplayer[52][4];
	int poscard;
	while(temp_memory->roundnumber<temp_memory->totalrounds)
	{	
		if(pthread_mutex_trylock(&temp_memory->globalmutex)!=0)
		{
			pthread_mutex_lock(&temp_memory->globalmutex);
		}
			if(player.number==temp_memory->turn)
			{
				printf("aqui\n");
				if(temp_memory->roundnumber!=0)
				{
					printf("entroulast\n");
					printf("Last Round: ");
					int t=temp_memory->nplayers;
					for(g=0;g<t;g++)
					{
						printf("%s",temp_memory->lasttablecards[g]);
						if(g!=t-1)
							printf("-");
					}
					printf("\n");
				}
				if(temp_memory->roundnumber==0)
				{
					printf("entrouread\n");
					temp_memory->vec[temp_memory->turn].handsize=(52/numjogs);
					read(f3,cardsplayer,sizeof(cardsplayer));
					for(i=0;i<(52/numjogs);i++)
					{	
						printf("Hand: %s\n",cardsplayer[i]);
						strcpy(temp_memory->vec[temp_memory->turn].hand[i],cardsplayer[i]);
					}
				}
				printf("HANDSIZE: %d\n",temp_memory->vec[temp_memory->turn].handsize);
				int err;
				err= pthread_create(&(tid[0]),NULL,&InputThread,NULL);
				if(err!=0)
					printf("Can't create thread\n");
				else
					printf("Thread Created Successfully\n");
				pthread_mutex_lock(&mutex2);
				pthread_cond_wait(&condition_var2,&mutex2);
				pthread_mutex_unlock(&mutex2);
				int err2;
				err2= pthread_create(&(tid[1]),NULL,&RoundThread,NULL);
				if(err2!=0)
					printf("Can't create thread\n");
				else
					printf("Thread Created Successfully\n");
			}
			else
			{
				/*do{
					printf("1-Ver Vez\n");
					printf("2-Ver Ultima Jogada\n");
					printf("3-Ver Ultima Ronda\n");
					printf("4-Ver Mao\n");
					int op;
					scanf("%d",&op);
					switch(op)
					{
						case 1: printf("Vez: \n", temp_memory->turn);
							break;
						case 2: printf("Ultima Carta Jogada: %s\n",temp_memory->vec[player.number].ultimacarta);
					}
				}while(player.number!=temp_memory->turn);*/
				printf("Waiting for your turn...");
			}
	pthread_cond_wait(&temp_memory->globalcond,&temp_memory->globalmutex);
	pthread_mutex_unlock(&temp_memory->globalmutex);
	}
	munmap(shm_addr, sizeof(mesa1));
	shm_unlink(shm);
	return 0;
}
  



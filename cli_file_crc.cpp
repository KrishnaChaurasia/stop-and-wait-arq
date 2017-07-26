#include<iostream>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/uio.h>
#include<unistd.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/syscall.h>

using namespace std;
#define PORT 5555

void computecrc();
void replyDropOrNotfn();

int size=40;      //32+8
int sizep=9;
char mesg[40]={' '};
char tempmsg[100]={' '};
char poly[9]={'1','0','0','0','0','0','1','1','1'};
char replyA[3]={'A','C','K'};
char replyN[4]={'N','A','C','K'};
int replyDropOrNot;
int flag;

int main(int argc,char * argv[])
{
  int fd;
  struct sockaddr_in server,client;
  struct hostent *hp;
  int con,rec,snd;
  server.sin_family=AF_INET;
  server.sin_addr.s_addr=INADDR_ANY;
  server.sin_port=htons(PORT);

  fd=socket(AF_INET,SOCK_STREAM,0);
  if(fd<0){
    cout<<"Error creating socket\n";
    return 0;
  }
    cout<<"Socket Created\n";

  hp=gethostbyname(argv[1]);
  bcopy((char*)hp->h_addr,(char*)&server.sin_addr.s_addr,hp->h_length);

  con=connect(fd,(struct sockaddr*)&server,sizeof(struct sockaddr));

  if(con<0){
    cout<<"Error connecting\n";
    return 0;
  }
  
  cout<<"\nConnection has been made\n\n";
  
  while(rec=recv(fd,mesg,sizeof(mesg),0)){
    if(rec<0){
      cout<<"Error receiving\n";
      return 0;
    }
    cout<<"Message received : "<<mesg;
    
    int i=0,j=0;
    while(mesg[i]!='\0'){
      tempmsg[j++]=mesg[i++];
    }
    //cout<<"tempmsg : "<<tempmsg<<endl;
    computecrc();
    replyDropOrNotfn();
    if(replyDropOrNot==0){
      if(flag==1){
	snd=send(fd,replyN,4,0);
	cout<<" NACK " <<endl;
      }else{
	snd=send(fd,replyA,3,0);
	cout<<" ACK " <<endl;
      }
    }
    else{
      if(flag==1){
	cout<<"NACK Dropped\n";
      }else{
	cout<<"ACK Dropped\n";
      } 
    }
  }
  close(fd);
  cout<<"\nSocket closed\n";
  return 0;
}

void computecrc(){
  int j=0; int ctr=0; int l=0;
  int count=0;
  int sz=size;
  while(sz>=sizep){
    ++count;l=0;j=0;ctr=0;
    for(int i=0;i<sizep;i++){
      if(tempmsg[j]==poly[i]){
	if(ctr!=0){
	  ++ctr;
	  tempmsg[l++]='0';       //push '0' in temporary array
	}
      }
      else {
	tempmsg[l++]='1';	  //push '1' in temporary array 
	++ctr;
      }
      j++;
    }
    for(int i=sizep;i<=sz;i++){
      tempmsg[l++]=tempmsg[i];
    }
    sz=l-1;
  }
  
  flag=0;
  for(int i=0;i<sz;i++)
    if(tempmsg[i]=='1'){
      flag++;
      break;
    }
}

void replyDropOrNotfn(){     //to determine whether to drop ACK/NACK or not
   int flag=0;
  flag=rand()%40;
  if(flag%2==0){
    cout<<"Reply(ACK/NACK) Dropped\n";
    replyDropOrNot=1;
  }
  else{
    replyDropOrNot=0;
    cout<<"Reply(ACK/NACK) not dropped\n";
  }
}

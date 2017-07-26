#include<iostream>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>
#include<sys/uio.h>
#include<unistd.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/syscall.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>           //for sleep()
using namespace std;
#define PORT 5555
#define BACKLOG 3

void computeCrc();
void corrupt();
void corruptOrNot();
void frameDropOrNotfn();
int frameDropOrNot;
int size=40;               //32+8
int sizep=9;               //size of CRC polynomial 
char mesg[40]={' '};       //original message to be read from file
char tmesg[40]={' '};      //temporary mesg to store the message for resending if NACK is received
char reply[4]={' '};       //reply received to the sent message(ACK/NACK)
char tempmsg[50]={' '};    //used in CRC computation
char poly[9]={'1','0','0','0','0','0','1','1','1'};  //crc polynomial
int noOfAttempts=0;

int main(){
  srand(time(NULL));
  int fd,fd2,bnd,rcv,s;
  struct sockaddr_in server,client;
  int from,n=1;
  int flag=0;    //to decide whether to resend the frame or not based on ack or nack
  fd=socket(AF_INET,SOCK_STREAM,0);
  if(fd<0){
    cout<<"Error creating socket\n";
    return 0;
  }
  cout<<"Socket successfully created....\n";
  server.sin_family=AF_INET;
  server.sin_port=htons(PORT);
  server.sin_addr.s_addr=INADDR_ANY; 

  bnd=bind(fd,(struct sockaddr*)&server,sizeof(server));
  if(bnd<0){
    cout<<"Error binding socket\n";
    return 0;
  }
  cout<<"Binding successful\n";
  if(listen(fd,BACKLOG)<0){
    cout<<"Error listening\n";
    return 0;
  }
  cout<<"Server is listening....\n";
  socklen_t len=sizeof(client);
  fd2=accept(fd,(struct sockaddr*)&client,&len);
  if(fd2<0){
    cout<<"Error accepting\n";
      return 0;
  }
  cout<<"Accept successful....\n"<<endl;
  
  from=open("input.txt",O_RDONLY);
  if(from<0){
    cout<<"Error opening file\n";
    return 0;
  }
  
  flag=1;                   //set flag=1 to send first frame
  int first=0;
  while(n || flag==0){     //|| flag==0 since last frame read will make n=0 but we must keep sending till we receive ACK for that frame
    noOfAttempts++;
    if(flag==1){           //i.e. ACK is received then read
      n=read(from,mesg,33);
      if(mesg[0] !='0' && mesg[0] !='1') break; //to remove the xtra last msg
      
      for(int i=0;i<size-sizep+1;i++){
	tempmsg[i]=mesg[i];
	tmesg[i]=mesg[i];
      }
      cout<<"Message read : "<<mesg<<endl;
      for(int i=size-sizep+1;i<size;i++)
	tempmsg[i]='0';
      computeCrc();
      frameDropOrNotfn();
      if(frameDropOrNot==0){
	corruptOrNot();
	s=send(fd2,mesg,sizeof(mesg),0);
	cout<<"Message sent : "<<mesg<<endl;
	if(s<0){
	  cout<<"Error sending\n";
	  return 0;
	}
	rcv=recv(fd2,reply,sizeof(reply),0);
	cout<<"Reply : "<<reply<<endl<<endl;
	if(reply[0]=='A' && reply[1]=='C' && reply[2]=='K'){
	  flag=1;
	}
	else {
	  flag=0;
	}
      } else {
	sleep(1);
	flag=0;
 	continue; //have one more flag to send the pkt if it is dropped
      }
    } else {
      for(int i=0;i<size-sizep+1;i++){
	tempmsg[i]=tmesg[i];
	mesg[i]=tmesg[i];
      }
      cout<<"Previous Message read from file : "<<tmesg<<endl;
      for(int i=size-sizep+1;i<size;i++)
	tempmsg[i]='0';
      computeCrc();
      //      corruptOrNot();
      cout<<endl;
      frameDropOrNotfn();
      if(frameDropOrNot==0){
	corruptOrNot();
	s=send(fd2,mesg,sizeof(mesg),0);
	cout<<"Message sent : "<<mesg<<endl;
	if(s<0){
	  cout<<"Error sending\n";
	  return 0;
	}
	rcv=recv(fd2,reply,sizeof(reply),0);
	cout<<"Reply : "<<reply<<endl<<endl;
	if(reply[0]=='A' && reply[1]=='C' && reply[2]=='K'){
	  flag=1;
	}
        else {
	  flag=0;
	}
      } 
      else {
	sleep(5);
	flag=0;
	continue;
      }
    }
    for(int i=0;i<31;i++){
      mesg[i]=' ';
      tempmsg[i]=' ';
    }
    for(int i=0;i<4;i++)
      reply[i]=' ';
  }
  close(fd2);
  close(fd);
  shutdown(fd,0);
  cout<<"\nSocket closed\n";
  cout<<"\nNumber of total attempts made : "<<noOfAttempts<<endl<<endl;
  return 0;
}

void computeCrc(){
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
  //  cout<<"tempmsg : ";
  //for(int i=0;i<sz;i++) cout<<tempmsg[i];cout<<endl;
  if(sz<sizep-1){
    int t=sizep-sz-1;
    int i=size-sizep+1;
    while(t>0)
      {
	mesg[i++]='0';
	t--;
      }
    j=0;
    while(tempmsg[j]!='\0')  mesg[i++]=tempmsg[j++];
  }
  else
    {
      int i=size-sizep+1;
      int j=0;
      while(tempmsg[j]!='\0')  mesg[i++]=tempmsg[j++];
    }
}

void corruptOrNot(){
  int flag=0;
  flag=rand()%40;
  if(flag%2==0){
    cout<<"Message corrupted\n";
    corrupt();
  }
  else
    cout<<"Message not corrupted\n";
}

void corrupt(){
  int check[40]={0};
  int corruptbits=rand()%30;
  cout<<"Number of bits to corrupt :"<<corruptbits<<endl;
  while(corruptbits>0){
    int bitno=rand()%40;
    if(check[bitno]==0){
      if(mesg[bitno]=='1')
	mesg[bitno]='0';
      else
	mesg[bitno]='1';
      check[bitno]=1;
      corruptbits--;
    }
  }
}

void frameDropOrNotfn(){
   int flag=0;
  flag=rand()%40;
  if(flag%2==0){
    cout<<"Frame Dropped\n";
    frameDropOrNot=1;
  }
  else{
    frameDropOrNot=0;
    cout<<"Frame not dropped\n";
  }
}
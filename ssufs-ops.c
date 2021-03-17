#include "ssufs-ops.h"
#include <string.h>

extern struct filehandle_t file_handle_array[MAX_OPEN_FILES];

int ssufs_allocFileHandle() {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if (file_handle_array[i].inode_number == -1) {
			return i;
		}
	}
	return -1;
}

int ssufs_create(char *filename){
	/* 1 */
  int test;
  int inodenum;
  int dbnum;
  if((test = open_namei(filename)) != -1)
  {
	//같은 이름의 파일 존재
	fprintf(stderr,"same name file exist!\n");
	return -1;
  }
  //할당 가능한 inode 존재 확인
  if((inodenum = ssufs_allocInode()) <0)
  {
	fprintf(stderr,"inode not enough\n");
	return -1;
  }
  //할당 가능한 data block 존재 확인
  if((dbnum = ssufs_allocDataBlock())<0)
  {
	fprintf(stderr,"data block not enough\n");
	ssufs_freeInode(inodenum);
	return -1;
  }
  //inode 초기화
  struct inode_t *inodeptr = (struct inode_t *)malloc(sizeof(struct inode_t));
  strcpy(inodeptr->name,filename);
  inodeptr->file_size = BLOCKSIZE;
  for(int i=0; i<4;i++)
  {
	inodeptr->direct_blocks[i]=-1;
  }
  inodeptr->direct_blocks[0]=dbnum;
  inodeptr->status = INODE_IN_USE;
  ssufs_writeInode(inodenum,inodeptr);
  free(inodeptr);
  return inodenum;
}

void ssufs_delete(char *filename){
	/* 2 */
  int inodenum;
  if((inodenum= open_namei(filename)) == -1) return;
  for (int i = 0; i < MAX_OPEN_FILES; i++)
  	{
	  if (file_handle_array[i].inode_number == inodenum)
		  {
			  file_handle_array[i].inode_number=-1;
			  file_handle_array[i].offset = 0;
			  ssufs_freeInode(inodenum);
			  return;
			}
  	}
  }

int ssufs_open(char *filename){
	/* 3 */
  int inodenum;
  if((inodenum= open_namei(filename)) == -1) return -1;

  for(int i = 0; i < MAX_OPEN_FILES; i++) {
         if (file_handle_array[i].inode_number == -1) {
             file_handle_array[i].inode_number = inodenum;
             file_handle_array[i].offset=0;
  //           printf("open success %d:%d\n",i,inodenum);
			 return i;
         }
     }

}

void ssufs_close(int file_handle){
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = 0;
}

int ssufs_read(int file_handle, char *buf, int nbytes){
	/* 4 */
	if (nbytes == 0)
	{
		fprintf(stderr, "o byte read\n");
		return -1;
	}
  char *buffer[4];
  for(int i=0;i<4;i++)
  {
	buffer[i] = malloc(BLOCKSIZE);
  }
  int readed=0;
 
  
  //inode에 파일정보 가져오고, offset 저장
  int offset = file_handle_array[file_handle].offset;
  struct inode_t *inode = (struct inode_t *) malloc(sizeof(struct inode_t));
  ssufs_readInode(file_handle_array[file_handle].inode_number, inode);
  //파일 끝을 넘어가는 경우 에러 리턴
  if((offset + nbytes)>inode->file_size)
  {
	return -1;
  }
  if(offset == 0 && nbytes==256)
  {
	//4블록 모두 읽는 경우
	  ssufs_readDataBlock(inode->direct_blocks[0],buffer[0]);
	  ssufs_readDataBlock(inode->direct_blocks[1],buffer[1]);
	  ssufs_readDataBlock(inode->direct_blocks[2],buffer[2]);
	  ssufs_readDataBlock(inode->direct_blocks[3],buffer[3]);
	  strncpy(buf,buffer[0],64);
	  strncat(buf,buffer[1],64);
	  strncat(buf,buffer[2],64);
	  strncat(buf,buffer[3],64);
	  readed = nbytes;
  }
  else {
  int num = (offset+nbytes)/BLOCKSIZE;
  int ret = (offset+nbytes)%BLOCKSIZE;
  if(offset <64)
  {
	if((num ==0&&ret!=0) || (num==1 && ret==0))
	{//한 블록 안에서 읽을 수 있음
	  ssufs_readDataBlock(inode->direct_blocks[0],buffer[0]);
	  memcpy(buf,buffer[0]+offset,nbytes);
	  readed += nbytes;
	}
	else if((num ==1&&ret!=0)||(num==2&&ret==0))
	{
	  //두블럭 읽은 후
	  ssufs_readDataBlock(inode->direct_blocks[0],buffer[0]);
	  ssufs_readDataBlock(inode->direct_blocks[1],buffer[1]);
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buf,buffer[0]+offset,63-offset);
	  readed += 63-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  strncat(buf,buffer[1],nbytes-readed);
	  readed = readed+ (nbytes-readed);
	}
	else if((num ==2&&ret!=0)||(num==3&&ret==0))
	{
	  //3블럭 읽은 후
	  ssufs_readDataBlock(inode->direct_blocks[0],buffer[0]);
	  ssufs_readDataBlock(inode->direct_blocks[1],buffer[1]);
	  ssufs_readDataBlock(inode->direct_blocks[2],buffer[2]);
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buf,buffer[0]+offset,63-offset);
	  readed += 63-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  strncat(buf,buffer[1],BLOCKSIZE);
	  readed+=64;
	  strncat(buf,buffer[2],nbytes-readed);
	  readed = readed+ (nbytes-readed);
	}
	else if((num ==3&&ret!=0)||(num==4&&ret==0))
	{
	  //4블럭 읽은 후
	  ssufs_readDataBlock(inode->direct_blocks[0],buffer[0]);
	  ssufs_readDataBlock(inode->direct_blocks[1],buffer[1]);
	  ssufs_readDataBlock(inode->direct_blocks[2],buffer[2]);
	  ssufs_readDataBlock(inode->direct_blocks[3],buffer[3]);
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buf,buffer[0]+offset,63-offset);
	  readed += 63-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  strncat(buf,buffer[1],BLOCKSIZE);
	  readed+=64;
	  strncat(buf,buffer[2],BLOCKSIZE);
	  readed+=64;
	  strncat(buf,buffer[3],nbytes-readed);
	  readed = readed+ (nbytes-readed);
	}
  }
  else if(offset >=64 && offset <128)
  {
	if((num ==1&&ret!=0)||(num==2&&ret==0))
	{//한 블록 안에서 읽을 수 있음
	  ssufs_readDataBlock(inode->direct_blocks[1],buffer[1]);
	  memcpy(buf,buffer[1]+offset,nbytes);
	  readed += nbytes;
	}
	else if((num ==2&&ret!=0)||(num==3&&ret==0))
	{
	  //두블럭 읽은 후
	  ssufs_readDataBlock(inode->direct_blocks[1],buffer[1]);
	  ssufs_readDataBlock(inode->direct_blocks[2],buffer[2]);
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buf,buffer[1]+offset,127-offset);
	  readed += 127-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  strncat(buf,buffer[2],nbytes-readed);
	  readed = readed+ (nbytes-readed);
	}
	else if((num ==3&&ret!=0)||(num==4&&ret==0))
	{
	  //3블럭 읽은 후
	  ssufs_readDataBlock(inode->direct_blocks[1],buffer[1]);
	  ssufs_readDataBlock(inode->direct_blocks[2],buffer[2]);
	  ssufs_readDataBlock(inode->direct_blocks[3],buffer[3]);
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buf,buffer[0]+offset,127-offset);
	  readed += 127-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  strncat(buf,buffer[1],BLOCKSIZE);
	  readed+=64;
	  strncat(buf,buffer[2],nbytes-readed);
	  readed = readed+ (nbytes-readed);
	}
  }
  else if(offset >=128 && offset < 192)
  {
	if((num ==2&&ret!=0)||(num==3&&ret==0))
	{//한 블록 안에서 읽을 수 있음
	  ssufs_readDataBlock(inode->direct_blocks[2],buffer[2]);
	  memcpy(buf,buffer[2]+offset,nbytes);
	  readed += nbytes;
	}
	else if((num ==3&&ret!=0)||(num==4&&ret==0))
	{
	  //두블럭 읽은 후
	  ssufs_readDataBlock(inode->direct_blocks[2],buffer[2]);
	  ssufs_readDataBlock(inode->direct_blocks[3],buffer[3]);
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buf,buffer[2]+offset,191-offset);
	  readed += 191-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  strncat(buf,buffer[3],nbytes-readed);
	  readed = readed+ (nbytes-readed);
	}
  }
  else if(offset >=192 && offset <256)
  {
	if((num ==3&&ret!=0)||(num==4&&ret==0))
	{//한 블록 안에서 읽을 수 있음
	  ssufs_readDataBlock(inode->direct_blocks[3],buffer[3]);
	  memcpy(buf,buffer[2]+offset,nbytes);
	  readed += nbytes;
	}
  }}
  for(int i=0;i<4;i++)
  {
	free(buffer[i]);
  }
  
	printf("aupp\n");
  if(readed == nbytes)
  {
	//오프셋 증가
	ssufs_lseek(file_handle,nbytes);
	printf("upp\n");
	return 0;
  }

  return -1;
}

int ssufs_write(int file_handle, char *buf, int nbytes){
	/* 5 */
  if(nbytes ==0)
   {
     fprintf(stderr,"0 byte read\n");
     return -1;
   }
   //inode에 파일정보 가져오고, offset 저장
   int offset = file_handle_array[file_handle].offset;
   struct inode_t *inode = (struct inode_t *) malloc(sizeof(struct inode_t));
   ssufs_readInode(file_handle_array[file_handle].inode_number, inode);
   int dbnum;
   int last_blocknum=0;
   int writed=0;

  
   //요청된 바이트 쓰면 파일크기 제한 초과하는 경우 에러
   if(offset+nbytes >256)
   {
	 fprintf(stderr,"maximum filesize is 256byte\n");
	 return -1;
   }
   //새로운 데이터블록이 필요한 경우:data block새로 할당하여 크기를 늘려줌
   if(offset+nbytes > inode->file_size)
   {
	 //free data block 없음
	 if((dbnum = ssufs_allocDataBlock()) ==-1)
	 {
	   fprintf(stderr,"no free data block\n");
	   return -1;
	 }
	 for(int i=0;i<4;i++)
	 {
	   if(inode->direct_blocks[i] ==-1)
	   {
		 //inode구조체 수정-> 마지막에 inode에 다시 써줘야 함
	
		 inode->direct_blocks[i] = dbnum;
		 inode->file_size += BLOCKSIZE;
		 last_blocknum = i;
		 break;
	   }
	 }
   }
   ssufs_writeInode(file_handle_array[file_handle].inode_number,inode);
  char *buffer[4];
  for(int i=0;i<4;i++)
  {

	buffer[i] = malloc(BLOCKSIZE);
	memset(buffer[i],' ',64);
  }
   //공간 확보 끝났으니 쓰기 작업 시작
   
  int num = (offset+nbytes)/BLOCKSIZE;
  int ret = (offset+nbytes)%BLOCKSIZE;
  if(offset <64)
  {
	if((num ==0&&ret!=0) || (num==1&&ret==0))
	{
	  //오프셋부터 nbytes만큼만 수정
	  memcpy(buffer[0]+offset,buf,nbytes);
	  //수정된 블럭을 다시 write
	  ssufs_writeDataBlock(inode->direct_blocks[0],buffer[0]);
	  writed += nbytes;
	}
	else if((num ==1&&ret!=0)||(num==2&&ret==0))
	{
	  
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memset(buffer[0],' ',64);
	  memcpy(buffer[0]+offset,buf,64-offset);
	  ssufs_writeDataBlock(inode->direct_blocks[0],buffer[0]);
	  writed += 64-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  memset(buffer[0],' ',64);
	  memcpy(buffer[1],buf+offset,nbytes-writed);
	  ssufs_writeDataBlock(inode->direct_blocks[1],buffer[1]);
	  writed = writed+ (nbytes-writed);
	}
	else if((num ==2&&ret!=0)||(num==3&&ret==0))
	{
	  
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buffer[0]+offset,buf,64-offset);
	  ssufs_writeDataBlock(inode->direct_blocks[0],buffer[0]);
	  writed += 64-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  memcpy(buffer[1],buf+offset,BLOCKSIZE);
	  ssufs_writeDataBlock(inode->direct_blocks[1],buffer[1]);
	  writed+=64;
	  //2
	  memcpy(buffer[2],buf+offset+64,nbytes-writed);
	  ssufs_writeDataBlock(inode->direct_blocks[2],buffer[2]);
	  writed = writed+ (nbytes-writed);
	}
	else if((num ==3&&ret!=0)||(num==4&&ret==0))
	{
	  
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buffer[0]+offset,buf,64-offset);
	  ssufs_writeDataBlock(inode->direct_blocks[0],buffer[0]);
	  writed += 64-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  memcpy(buffer[1],buf+offset,64);
	  ssufs_writeDataBlock(inode->direct_blocks[1],buffer[1]);
	  writed+=64;
	  //2
	  memcpy(buffer[2],buf+offset+64,64);
	  ssufs_writeDataBlock(inode->direct_blocks[2],buffer[2]);
	  writed+=64;
	  //3
	  memcpy(buffer[3],buf+offset+64+64,nbytes-writed);
	  ssufs_writeDataBlock(inode->direct_blocks[3],buffer[3]);
	  writed = writed+ (nbytes-writed);
	}
  }
  else if(offset >=64 && offset <128)
  {
	if((num ==1&&ret!=0)||(num==2&&ret==0))
	{
	  //오프셋부터 nbytes만큼만 수정
	  memcpy(buffer[1]+(offset-64),buf,nbytes);
	  //수정된 블럭을 다시 write
	  ssufs_writeDataBlock(inode->direct_blocks[1],buffer[1]);
	  writed += nbytes;
	}
	else if((num ==2&&ret!=0)||(num==3&&ret==0))
	{
	 
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buffer[1]+(offset-64),buf,128-offset);
	  ssufs_writeDataBlock(inode->direct_blocks[1],buffer[1]);
	  writed += 128-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  memcpy(buffer[2],buf+(offset-64),nbytes-writed);
	  ssufs_writeDataBlock(inode->direct_blocks[2],buffer[2]);
	  writed = writed+ (nbytes-writed);
	}
	else if(num ==3||(num==4&&ret==0))
	{
	  
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buffer[1]+(offset-64),buf,128-offset);
	  ssufs_writeDataBlock(inode->direct_blocks[1],buffer[1]);
	  writed += 128-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  memcpy(buffer[2],buf+(offset-64),BLOCKSIZE);
	  ssufs_writeDataBlock(inode->direct_blocks[2],buffer[2]);
	  writed+=64;
	  //2
	  memcpy(buffer[3],buf+offset,nbytes-writed);
	  ssufs_writeDataBlock(inode->direct_blocks[3],buffer[3]);
	  writed = nbytes;
	}
  }
  else if(offset >=128 && offset < 192)
  {
	if(num ==2||(num==3&&ret==0))
	{
	  //오프셋부터 nbytes만큼만 수정
	  memcpy(buffer[2]+(offset-128),buf,nbytes);
	  //수정된 블럭을 다시 write
	  ssufs_writeDataBlock(inode->direct_blocks[2],buffer[2]);
	  writed += nbytes;
	}
	else if(num ==3||(num==4&&ret==0))
	{
	  /
	  //0번째 블럭에서 오프셋부터 블록 끝까지의 내용 복사
	  memcpy(buffer[2]+(offset-128),buf,192-offset);
	  ssufs_writeDataBlock(inode->direct_blocks[2],buffer[2]);
	  writed += 192-offset;
	  //1번쨰 블럭에서 나머지 바이트 만큼의 내용복사
	  memcpy(buffer[3],buf+(offset-128),nbytes-writed);
	  ssufs_writeDataBlock(inode->direct_blocks[3],buffer[3]);
	  writed = nbytes;
	}
  }
  else if(offset >=192 && offset <256)
  {
	if((num ==3&&ret!=0)||(num==4&&ret==0))
	{/
	  //오프셋부터 nbytes만큼만 수정
	  memcpy(buffer[3]+(offset-192),buf,nbytes);
	  //수정된 블럭을 다시 write
	  ssufs_writeDataBlock(inode->direct_blocks[3],buffer[3]);
	  writed += nbytes;
	}
  }
  for(int i=0;i<4;i++)
  {
	free(buffer[i]);
  }
  if(writed == nbytes)
  {
	//오프셋 증가
	ssufs_lseek(file_handle,nbytes);
	return 0;
  }

}

int ssufs_lseek(int file_handle, int nseek){
	int offset = file_handle_array[file_handle].offset;

	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);
	
	int fsize = tmp->file_size;
	
	offset += nseek;

	if ((fsize == -1) || (offset < 0) || (offset > fsize)) {
		free(tmp);
		return -1;
	}

	file_handle_array[file_handle].offset = offset;
	free(tmp);

	return 0;
}

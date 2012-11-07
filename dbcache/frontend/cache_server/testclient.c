
#include "db_protocal.h"
#include "dbtype.h"

#include <stdio.h>
#include "SocketWrapper.h"
#include "SysTime.h"
#include "KendyNet.h"
#include "Connector.h"
#include "Connection.h"
#include "common_define.h"
allocator_t wpacket_allocator = NULL;

atomic_32_t wpacket_count = 0;
atomic_32_t rpacket_count = 0; 
atomic_32_t buf_count = 0;  


connector_t con = NULL;

int8_t stage = 0;

void on_process_packet(struct connection *c,rpacket_t r)
{
	int8_t ret = rpacket_read_uint8(r);
	if(ret != 0)
	{
		printf("db op error\n");
		return;
	}
	++stage;
	if(stage == 1)
	{
		//发出get请求
		wpacket_t wpk = wpacket_create(SINGLE_THREAD,NULL,64,0);
		wpacket_write_uint8(wpk,CACHE_GET);//设置
		wpacket_write_string(wpk,"kenny");
		connection_send(c,wpk,NULL);		
	}
	else if(stage == 2)
	{
		uint8_t type = rpacket_read_uint8(r);
		if(type != DB_INT32)
		{
			printf("get error\n");
			return;
		}
		printf("%d\n",rpacket_read_uint32(r));
		//发出del请求
		wpacket_t wpk = wpacket_create(SINGLE_THREAD,NULL,64,0);
		wpacket_write_uint8(wpk,CACHE_DEL);//设置
		wpacket_write_string(wpk,"kenny");
		connection_send(c,wpk,NULL);		
	}
}

void on_connect_callback(HANDLE s,const char *ip,int32_t port,void *ud)
{
	HANDLE *engine = (HANDLE*)ud;
	struct connection *c;
	wpacket_t wpk;
	if(s == -1)
	{
		printf("%d,连接到:%s,%d,失败\n",s,ip,port);
	}
	else
	{
		
		setNonblock(s);
		c = connection_create(s,0,SINGLE_THREAD,on_process_packet,NULL);
		printf("%d,连接到:%s,%d,成功\n",s,ip,port);
		Bind2Engine(*engine,s,RecvFinish,SendFinish);
		wpacket_t wpk = wpacket_create(SINGLE_THREAD,NULL,64,0);
		wpacket_write_uint8(wpk,CACHE_SET);//设置
		wpacket_write_string(wpk,"kenny");
		wpacket_write_uint8(wpk,DB_INT32);//数据类型
		wpacket_write_uint32(wpk,100);
		connection_send(c,wpk,NULL);
		connection_start_recv(c);
	}
}

int32_t main(int32_t argc,char **argv)
{	
	HANDLE engine;
	const char *ip = argv[1];
	uint32_t port = atoi(argv[2]);
	signal(SIGPIPE,SIG_IGN);
	init_system_time(10);
	if(InitNetSystem() != 0)
	{
		printf("Init error\n");
		return 0;
	}	
	int32_t ret;
	int32_t i = 0;
	wpacket_t wpk;

	engine = CreateEngine();
	con =  connector_create();
	for( ; i < 1;++i)
	{
		ret = connector_connect(con,ip,port,on_connect_callback,&engine,1000*20);
		sleepms(1);
	}
	while(1)
	{
		connector_run(con,1);
		EngineRun(engine,1);
	}
	return 0;
}


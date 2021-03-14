//client.c
#include "my_head.h"

void *read_msg_client(void *argc);
//void *send_msg_client(void *argc);
void main_panel(int sfd);
void menu(int socketfd);
int showscreen(void);
void become_vip(int socketfd);
void view_user_msg(int socketfd);
void modify_pwd(int socketfd);
void view_online_client(int socketfd);
void group_chat(int socketfd);
void private_chat(int socketfd);
void shut_mouth(int socketfd);
void cancel_shut_mouth(int socketfd);

char *my_time()
{
	time_t now;
	time(&now);
	return (ctime(&now));
}

client clientsend;//发送给库户端的用户信息
client clientrecv;//客户端返回的用户信息

int main()
{
	int socketfd = 0;
	int ret = 0;

	struct sockaddr_in server_addr;

	//建立socket套接字
	socketfd = socket(AF_INET,SOCK_STREAM,0);
	if(socketfd == -1)
	{
		perror("socket");
		return -1;
	}
	printf("Client side--socket success...\n");

	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(MYPORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//本机地址

	//连接客户端
	ret = connect(socketfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret == -1)
	{
		perror("connect");
		return -1;
	}
	printf("connect success...\n");

	main_panel(socketfd);

	close(socketfd);//关闭socket

	return 0;
}

void main_panel(int socketfd)
{
	int number = 0;
	int flag = 0;

	int writecnt = 0,readcnt = 0;    //返回接收发送个数
	pthread_t th1;			//线程

	while(1)
	{
		printf("\n1.注册用户\n");
		printf("2.用户登录\n");
		printf("3.退出\n");
		printf("请输入选择的功能号：");
		flag = scanf("%d",&number);//scanf返回正确输入个数，不为1时异常
	    	if(getchar() != '\n')
		{
			flag = 0;
		}
		while(flag != 1 || number < 0 || number > 3)
		{
			while(getchar() != '\n');
			printf("输入错误！请重新输入:");
			flag = scanf("%d",&number);
		}

		switch(number)
		{
			case 1:	//注册
			{
				memset(&clientsend,0,sizeof(client));
				clientsend.action = REGISTER;//确定用户行动为注册
				printf("密码要求：\n");
				printf("\t1.密码长度不少于6位\n");
				printf("\t2.密码需包含字母和数字\n");
				printf("用户名：");
				scanf("%s",clientsend.username);
				printf("密码：");
				scanf("%s",clientsend.new_passwd1);
				printf("再次确认密码：");
				scanf("%s",clientsend.new_passwd2);

				writecnt = write(socketfd,&clientsend,sizeof(client));    //发送给server
				if(writecnt == -1)
				{
					perror("client:write");
					return -1;
				}
				else
				{
					printf("正在验证中。。。\n");
					sleep(1);
				}

				memset(&clientrecv,0,sizeof(client));
				readcnt = read(socketfd,&clientrecv,sizeof(client));	//从server接收

				if(clientrecv.flag == REG_SUCCESS)//注册成功
				{
					printf("注册成功！\n");
					sleep(1);
				}
				else if(clientrecv.flag == REG_FAIL_PWD_EASY)
				{//注册成功，密码过于简单提示
					printf("所设密码过于简单!\n");
					sleep(1);
				}
				else if(clientrecv.flag == REG_FAIL_PWD_DIFF)
				{//两次密码不同,注册失败
					printf("两次密码不同，注册失败！\n");
					sleep(1);
				}
				else if(clientrecv.flag == REG_FAIL_ID_UQUA)
				{//用户名不合法，注册失败
					printf("用户名不合法，注册失败！\n");
					sleep(1);
				}
				else if(clientrecv.flag == REG_FAIL_ID_ALE)
				{//用户名已被使用
					printf("该用户名已被占用!\n");
					sleep(1);
				}
				break;
			}
			case 2:	//登陆
			{
				memset(&clientsend,0,sizeof(client));
				clientsend.action = LOGIN;//确定用户行动为登陆
				printf("用户名：");
				scanf("%s",clientsend.username);
				printf("密码：");
				scanf("%s",clientsend.password);
				//向server发送用户名和密码
				write(socketfd,&clientsend,sizeof(client));

				memset(&clientrecv,0,sizeof(client));
				read(socketfd,&clientrecv,sizeof(client));

				if(clientrecv.flag == LOGIN_SUCCESS)
				{
					printf("登录成功！\n");

					//进入聊天室
					if(pthread_create(&th1, NULL, read_msg_client, &socketfd)){
						perror("Create phtread error\n");
						exit(1);
					}

					menu(socketfd);

				}
				else if(clientrecv.flag == LOGIN_FAIL_NOT_EXIST)
				{
					printf("用户不存在，检查用户名是否正确!\n");
					sleep(1);
				}
				else if(clientrecv.flag == LOGIN_FAIL_UNMATCH)
				{
					printf("用户名与密码不匹配，登录失败!\n");
					sleep(1);
				}
				else if(clientrecv.flag == LOGIN_FAIL_ALREADY_LOGIN)
				{//若用户已登录
					printf("该账号已登录！\n");
					sleep(1);
				}
				else if(clientrecv.flag == LOGIN_FAIL_ALREADY_LOGIN)
				{
					printf("用户已经在另一个地方登录，不能重复登录!\n");
				}
				break;
			}
			case 3:	//退出
			{
				clientsend.action = EXIT;//确定用户行动为下线
				printf("Exit Success!\n");
				exit(0);
			}
			default:
				break;
		}
	}
}

//接收消息线程
void *read_msg_client(void *argc)
{
	int socketfd = *((int *)argc);

	while(1)
	{//从server读消息
		read(socketfd,&clientrecv,sizeof(client));

		switch(clientrecv.flag)
		{
			case OBJECT_EXIST:			//私聊对象存在
			{
				object_exist = 1;
				break;
			}
			case PRIVATE_SUCCESS:             //收到私聊
			{
				printf("\n%49s to you | %s",clientrecv.fromname,my_time());
				printf("\t\t\t\t\t%43s\n",clientrecv.msg);
				break;

				//printf("\n%s to you | %s",clientrecv.fromname,my_time());
				//printf("\t %s\n",clientrecv.msg);
				//break;
			}
			//case PRIVATE_FAIL_NONE_EXIST:    //私聊时查无此人
			//{
			//	printf("查无此人!\n\n");
			//	break;
			//}
			case GROUP_SUCCESS:              //收到群聊
			{
				printf("\n%s send to all| %s ",clientrecv.fromname,my_time());
				printf("\t %s\n",clientrecv.msg);
				break;
			}
			case GROUP_FAIL_NO_POWER:                  //群聊被禁言
			{
				printf("您已被禁言，无法发送消息!\n\n");
				break;
			}

			case SHUT_MOUTH_FAIL_NOT_MANAGER:                  //不是管理员，无法禁言其他人
			{
				printf("您不是管理员，无法禁言其他人!\n\n");
				break;
			}
			case SHUT_MOUTH_FAIL_NONE_PERSON:                  //无此用户，无法禁言
			{
				printf("不存在此人，无法禁言!\n\n");
				break;
			}
			case SHUT_MOUTH_SUCCESS:                  //禁言某人成功
			{
				printf("禁言成功!\n\n");
				break;
			}
			case CANCEL_SHUT_MOUTH_FAIL_NOT_MANAGER:                  //不是管理员，无法解禁其他人
			{
				printf("您不是管理员，无法解禁其他人!\n\n");
				break;
			}
			case CANCEL_SHUT_MOUTH_FAIL_NONE_PERSON:                  //无此用户，无法解禁
			{
				printf("不存在此人，无法解禁!\n\n");
				break;
			}
			case CANCEL_SHUT_MOUTH_SUCCESS:                  //解禁某人成功
			{
				printf("解禁成功!\n\n");
				break;
			}
			case VIEW_ONLINE_SUCCESS:        //查看在线用户链表
			{
				printf("\n%d、username:%s\n",clientrecv.seq,clientrecv.username);
				printf("\33[;34mSignature: \33[0m%s\n",clientrecv.sign);
				break;
			}
			case MODIFY_SUCCESS:		//修改密码成功
			{
				printf("\n%s 修改密码成功！\n",clientrecv.username);
				break;
			}
			case MODIFY_FAIL_OLD:		//原密码不正确
			{
				printf("\n%s 原密码不正确,修改密码失败！\n",clientrecv.username);
				break;
			}
			case MODIFY_FAIL_NEW:		//新密码不一致
			{
				printf("\n%s 新密码输入不一致,修改密码失败！\n",clientrecv.username);
				break;
			}
			case VIP_SUCCESS:			//会员开通成功
			{
				printf("恭喜你已成为尊贵的vip用户！\n");
				break;
			}
			case VIP_FAIL:			//会员开通失败
			{
				printf("会员码不正确，开通失败！\n");
				break;
			}
			case LOGIN_HINT:
			{//登陆提示
				printf("\n\033[;31m你的账号在另一个地方被尝试登录，注意账户安全！！！ | %s \033[0m",my_time());
				break;
			}
		}
	}
}

void menu(int socketfd)
{
	int num = 0;

	while(1)
	{
		num = showscreen();

		switch(num)
		{
			case 1:		//私聊
			{
				private_chat(socketfd);
				break;
			}
			case 2:		//群聊
			{
				group_chat(socketfd);
				break;
			}
			case 3:		//查看在线用户
			{
				view_online_client(socketfd);
				break;
			}
			case 4:		//修改密码
			{
				modify_pwd(socketfd);
				break;
			}
			case 5:		//查看用户信息
			{
				view_user_msg(socketfd);
				break;
			}
			case 6:		//成为会员
			{
				become_vip(socketfd);
				break;
			}
			case 7:		//退出
			{

				clientsend.action = EXIT_NOW;
				write(socketfd,&clientsend,sizeof(client));
				main_panel(socketfd);
				break;
				//exit(0);//结束进程
			}
			case 8:		//禁言与解禁
			{
				int num1 = 0;
				printf("********************************\n");
				printf("*请选择一下选项：  *\n");
				printf("*1.禁言        2.解禁       *\n");
				printf("********************************\n");
				scanf("%d",&num1);
				if(num1 == 1 )
				{
					shut_mouth(socketfd);
				}
				else if(num1 == 2)
				{
					cancel_shut_mouth(socketfd);
				}
				break;
			}
		}
	}
}

//屏幕显示内容，并获取用户目标操作
int showscreen()
{
	int num = 0,flag = 0;

	printf("                                \n");
	printf("********************************\n");
	printf("*      Welcome to chatroom     *\n");
	printf("********************************\n");
	printf("                                \n");
	printf("*1.私聊         2.群聊        *\n");
	printf("*3.查看在线用户 4.修改密码     *\n");
	printf("*5.查看用户信息 6.成为会员     *\n");
	printf("*7.退出         8.禁言与解禁（管理员特权）*\n");
	printf("********************************\n");
	printf("\n");

	printf("请输入选择的功能号：");
	flag = scanf("%d",&num);
	if(getchar() != '\n')
	{
		flag = 0;
	}
	while(flag != 1 || num < 0 || num > 8)
	{
		while(getchar() != '\n');
		printf("输入错误！请重新输入:");
		flag = scanf("%d",&num);
	}
	if(num >= 1 && num <= 8)
	{
		return num;
	}
}

/******************私聊程序设计********************************/
void private_chat(int socketfd)
{
	object_exist = 0;//对象不存在为0,默认为0
	char buff[500] = {0};

	clientsend.action = OBJECT_CHECK;

	printf("请输入你要私聊的用户名：\n");
	scanf("%s",buff);//用户名存放在buff数组中
	strcpy(clientsend.toname,buff);//消息接收方
	strcpy(clientsend.fromname,clientsend.username);//消息发送方

	while(strcmp(clientsend.toname,clientsend.username) == 0)
	{//判断私聊对象是否为本身
		printf("私聊对象不能为本身，请重新输入：\n");
		scanf("%s",buff);
		strcpy(clientsend.toname,buff);
	}
	write(socketfd,&clientsend,sizeof(client));//判断私聊对象是否存在
	sleep(1);//1s时间等待返回

	while(1)
	{//判断目标用户是否存在，不存在则继续输入，输入end退出
		if(object_exist==1 || strcmp(buff,"end") == 0)
			break;
		memset(&buff,0,sizeof(char));
		printf("私聊用户不存在，请检查用户名是否正确！\n");
		printf("请重新输入你要私聊的用户名，输入end退出：\n");
		scanf("%s",buff);
		strcpy(clientsend.toname,buff);//消息接收方
		write(socketfd,&clientsend,sizeof(client));//判断私聊对象是否存在
		sleep(1);//1s时间等待返回
	}

	while(1)
	{
		clientsend.action = PRIVATE_CHAT;
		if(strcmp(clientsend.toname,"end") == 0 )
			break;

		printf("请输入发送的内容：\n");                  //我发送的信息内容
		scanf("%s",clientsend.msg);

		if(strcmp(clientsend.msg,"end") == 0 || strcmp(clientsend.toname,"end") == 0 )
		{
			break;
		}
		else
		{
			write(socketfd,&clientsend,sizeof(client));
			//己方特定输出
			printf("\n\033[;31myou send to %s | %s \033[0m",clientsend.toname,my_time());
			printf("\t%s\n",clientsend.msg);
		}
	}
	sleep(1);
}

/*********************群聊程序设计*****************************/
void group_chat(int socketfd)    //群聊
{
	char buff[500] = {0};
	while(1)
	{
		clientsend.action = GROUP_CHAT;
		strcpy(clientsend.fromname,clientsend.username);
		printf("请输入发送的内容：\n");
		scanf("%s",buff);
		strcpy(clientsend.msg,buff);

		if(strcmp(clientsend.msg,"end") == 0 || strcmp(clientsend.toname,"end") == 0)
		{
			break;
		}
		else
		{
			write(socketfd,&clientsend,sizeof(client));

			//己方特定输出
			printf("\n\033[;31myou send to all | %s \033[0m",my_time());
			printf("\t%s\n",buff);
		}
	}
	sleep(1);
}

/*****************查看聊天室用户******************************/
void view_online_client(int socketfd)
{
	clientsend.action = VIEW_ONLINE_USERS;
	write(socketfd,&clientsend,sizeof(client));
	sleep(2);
}

/**********************修改密码***********************************/
void modify_pwd(int socketfd)
{
	//向server发送申请
	char buff[20] = {0};
	char ch = '0';
	clientsend.action = MODIFY_PWD;

	printf("请输入原密码 : ");
	scanf("%s",buff);
	strcpy(clientsend.password,buff);
	memset(&buff,0,sizeof(char));
	printf("请输入新密码 : ");
	scanf("%s",buff);
	strcpy(clientsend.new_passwd1,buff);
	memset(&buff,0,sizeof(char));
	printf("请再次输入新密码 : ");
	scanf("%s",buff);
	strcpy(clientsend.new_passwd2,buff);
	write(socketfd,&clientsend,sizeof(client));

	sleep(1);
}

/************************查看用户信息**************************/
void view_user_msg(int socketfd)
{
	clientsend.action = VIEW_USER_MSG;

	write(socketfd,&clientsend,sizeof(client));

	sleep(3);
}

/************************成为vip*************************/
void become_vip(int socketfd)
{
	printf("心动不如行动，不做大多数！\n");
	printf("即刻联系 vx:*******获取专属会员码！\n");

	char buff[20] = {0};
	printf("请输入会员码：");
	scanf("%s",buff);
	strcpy(clientsend.msg,buff);

	clientsend.action = BECOME_VIP;

	write(socketfd,&clientsend,sizeof(client));
	sleep(2);
}

/************************禁言*************************/
void shut_mouth(int socketfd)
{
	clientsend.action = SHUT_MOUTH;
	strcpy(clientsend.fromname,clientsend.username);//管理员信息
	char buff[500] = {0};
	printf("请输入您要禁言的用户名 : ");
	scanf("%s",buff);
	strcpy(clientsend.toname,buff);//将被禁言的人

	write(socketfd,&clientsend,sizeof(client));

	sleep(1);
}

/************************解禁*************************/
void cancel_shut_mouth(int socketfd)
{
	clientsend.action = CANCEL_SHUT_MOUTH;
	strcpy(clientsend.fromname,clientsend.username);//管理员信息
	char buff[500] = {0};
	printf("请输入您要解禁的用户名 : ");
	scanf("%s",buff);
	strcpy(clientsend.toname,buff);//将被解禁的人

	write(socketfd,&clientsend,sizeof(client));

	sleep(1);
}

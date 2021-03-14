//server.c
#include "my_head.h"

void *read_msg_server(void *argc);//读消息线程函数
//void register_client(int clientfd);//客户端注册函数
int login_client(user *H,client clientrecv,int clientfd);//客户端登陆函数
void add_user(user *H,client clientrecv,int clientfd);
int isidexist(user *H,char *name);
user *CreateList();//建立用户信息表
int get_random_str(char* random_str, const int random_len);
void add_testuser(user *H);
int isidok(char *name);
int is_pwd_easy(char pwd[]);
int is_pwd_correct(user *H,client clientrecv);
int is_user_online(user *H,char *name);//判断用户是否在线

user *my_user_list;//头节点结构体指针

//user userbuff;
client clientrecv;
client clientsend;

char vip_code[] = "lbxdsg";
char *ban_word[] = {"admin","abc","123"};//存放禁用词汇数组

//时间函数
char *my_time()
{
	time_t now;
	time(&now);
	return (ctime(&now));
}

int main()
{
	int socketfd = 0;
	int clientfd = 0;
	int ret = 0;

	struct sockaddr_in server_addr,client_addr;

	socklen_t len = sizeof(struct sockaddr);
	pthread_t th;

	//读缓冲区
	char recvbuff[20] = {0};
	int recvcnt = 0;

	//写缓冲区
	char sendbuff[20] = {0};
	int sendcnt = 0;

	//建表
	my_user_list = CreateList();
	add_testuser(my_user_list);

	/*考虑建立数据库
	include <sqlite3.h>
	存储注册用户信息*/

	//调用socket创建套接字
	socketfd = socket(AF_INET,SOCK_STREAM,0);
	if(socketfd == -1)
	{
		perror("socket");
		return -1;
	}
	printf("Server side -- socket success...\n");

	//填写地址族，端口号，IP地址
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(MYPORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//本机地址

	//绑定socket
	ret = bind(socketfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret == -1)
	{
		perror("bind");
		return -1;
	}
	printf("Binding success...\n");

	//设置监听
	ret = listen(socketfd,MAX_CLIENT);
	if(ret == -1)
	{
		perror("listen");
		return -1;
	}
	printf("Listening success...\n");

	while(1)
	{//死循环
		clientfd = accept(socketfd,(struct sockaddr *)&client_addr,&len);//接收客户端
		if(clientfd == -1)
		{
			perror("accept");
			return -1;
		}
		printf("Accepting success!\n");

		//创建接收消息线程
		if(pthread_create(&th,NULL,read_msg_server,&clientfd) == -1)
		{
			perror("pthread_create");
			return -1;
		}
		pthread_detach(th);
	}

	close(socketfd);

	return 0;
}

void *read_msg_server(void *argc)
{
	struct users *p,*s;					//users结构体指针
	int clientfd = *((int *)argc);
	printf("read_msg_server:clientfd = %d\n",clientfd);  //便于调试

	int i;
	int readcnt = 0;
	int writecnt = 0;

	while(1)
	{
		memset(&clientrecv,0,sizeof(client));
		readcnt = read(clientfd,&clientrecv,sizeof(client));
		if(readcnt == -1)
		{
			perror("read_msg_client:read");
			return 0;
		}

		switch(clientrecv.action)
		{
			case OBJECT_CHECK:			//私聊用户检查
			{
				if(isidexist(my_user_list,clientrecv.toname))
				{//用户存在
					clientrecv.flag = OBJECT_EXIST;
					write(clientfd,&clientrecv,sizeof(client));//返回消息
					break;
				}
				break;
			}
			case PRIVATE_CHAT:			//私聊
			{
				p = my_user_list->next;
				//int temp = 0;
				while(p->next !=NULL)
				{
					p = p->next;
					if(strcmp(p->username,clientrecv.toname) == 0)                                //找到私聊对象
					{
						//temp = 1;
						clientrecv.flag = PRIVATE_SUCCESS;
						printf("%s send follow msg to %s!\n",clientrecv.fromname,clientrecv.toname);
						printf("\t%s\n",clientrecv.msg);
						write(p->clientfd,&clientrecv,sizeof(client));                          //发送给好友
						break;
					}
				}
				//if(temp == 0)
				//{
				//	clientrecv.flag = PRIVATE_FAIL_NONE_EXIST;                                    //查无此人,回复给用户
				//	write(clientfd,&clientrecv,sizeof(client));
				//}
				break;
			}
			case GROUP_CHAT:			//群聊
			{
				p = my_user_list->next;
				while(p->next != NULL)
				{
					p = p->next;
					//先查询发送群聊消息用户是否被禁言
					if(strcmp(p->username,clientrecv.fromname) == 0)
					{
						if(p->power == 1)//用户被禁言
						{
							//memset(&clientrecv,0,sizeof(client));
							clientrecv.flag = GROUP_FAIL_NO_POWER;                            //无发送权限，已被禁言
							writecnt = write(clientfd,&clientrecv,sizeof(client));
							if(writecnt == -1)
							{
								perror("write");
								return 0;
							}
						}
						else if(p->power == 0)
						{//逐个发送消息
							p = my_user_list->next;//跳过管理员节点  当前在管理员节点
							while(p->next != NULL)
							{
								p = p->next;
								if(strcmp(p->username,clientrecv.fromname) != 0) //若信息为当前该用户所发，服务器不会向该用户发信息
								{//非发送用户,发送群聊消息
									clientrecv.flag = GROUP_SUCCESS;
									writecnt = write(p->clientfd,&clientrecv,sizeof(client));          //发送给群
									if(writecnt == -1)
									{
										perror("group:write");
										return 0;
									}
								}
							}
							//服务器端打印群聊消息
							printf("\n%s send to all | %s",clientrecv.fromname,my_time());
							printf("\t%s\n",clientrecv.msg);
						}
						break;
					}
				}
				break;
			}
			case SHUT_MOUTH:                                 //禁言
			{
				p = my_user_list->next;
				int temp = 0;
				if(strcmp(clientrecv.fromname,"admin") != 0)                                //如果是普通用户
				{
					clientrecv.flag = SHUT_MOUTH_FAIL_NOT_MANAGER;                                    //忽略普通用户的禁言请求
					write(clientfd,&clientrecv,sizeof(client));
				}
				else                              //如果是普通用户
				{
					while(p->next !=NULL)
					{
						p = p->next;
						if(strcmp(p->username,clientrecv.toname) == 0)                                //存在目标禁言用户
						{
							temp = 1;
							clientrecv.flag = SHUT_MOUTH_SUCCESS;
							printf("%s have banned %s from talking!\n",clientrecv.fromname,clientrecv.toname);
							p->power=1;
							write(clientfd,&clientrecv,sizeof(client));
							break;
						}
					}
					if(temp == 0)
					{
						clientrecv.flag = SHUT_MOUTH_FAIL_NONE_PERSON;                                    //不存在目标禁言用户
						write(clientfd,&clientrecv,sizeof(client));
					}
				}

				break;
			}
			case CANCEL_SHUT_MOUTH:                                 //解禁
			{
				p = my_user_list->next;
				int temp = 0;
				if(strcmp(clientrecv.fromname,"admin") != 0)                                //如果不是管理员、或者非vip用户
				{
					clientrecv.flag = CANCEL_SHUT_MOUTH_FAIL_NOT_MANAGER;                                    //忽略普通用户的解禁请求
					write(clientfd,&clientrecv,sizeof(client));
				}
				else                              //如果是管理员
				{
					while(p->next !=NULL)
					{
						p = p->next;
						if(strcmp(p->username,clientrecv.toname) == 0)                                //存在目标解禁的用户
						{
							temp = 1;
							clientrecv.flag = CANCEL_SHUT_MOUTH_SUCCESS;
							printf("%s have canceled banning %s from talking!\n",clientrecv.fromname,clientrecv.toname);
							p->power=0;
							write(clientfd,&clientrecv,sizeof(client));
							break;
						}
					}
					if(temp == 0)
					{
						clientrecv.flag = CANCEL_SHUT_MOUTH_FAIL_NONE_PERSON;                          //不存在目标解禁的用户名
						write(clientfd,&clientrecv,sizeof(client));
					}
				}

				break;
			}
			case VIEW_ONLINE_USERS:		//查看在线用户
			{
				i=1;
				p = my_user_list->next;
				while(p->next!=NULL)
				{
					p = p->next;
					if(strcmp(p->username,clientrecv.username) == 0)
					{//先找到目标用户
						s = my_user_list->next;
						while(s->next!=NULL)
						{
							s = s->next;
							if(s->online == 1)
							{//当前用户在线,发送显示
								memset(&clientsend,0,sizeof(client));
								clientsend.flag = VIEW_ONLINE_SUCCESS;
								clientsend.seq = i;
								strcpy(clientsend.username,s->username);
								strcpy(clientsend.sign,s->signature);

								writecnt = write(p->clientfd,&clientsend,sizeof(client));          //发送给目标用户
								if(writecnt == -1)
								{
									perror("view:write");
									return 0;
								}
								i++;
							}
						}
						break;
					}
				}
				break;
			}
			case MODIFY_PWD:			//修改密码
			{
				p = my_user_list->next;
				while(p->next!=NULL)
				{
					p = p->next;
					if(strcmp(p->username,clientrecv.username) == 0)
					{//先定位到要修改密码的用户
						if(strcmp(p->password,clientrecv.password) == 0)
						{//输入旧密码正确
							if(strcmp(clientrecv.new_passwd1,clientrecv.new_passwd2) == 0)
							{//两次输入新密码相同
								//修改成功
								clientsend.flag = MODIFY_SUCCESS;
								memset(&p->password,0,sizeof(char));//清空原密码
								strcpy(p->password,clientrecv.new_passwd1);
								//服务器端进行相应提示
								printf("****************************************\n");
								printf("%s modify password! | %s",clientrecv.username,my_time());
								printf("****************************************\n");
								break;
							}
							else
							{//两次输入新密码不同
								clientsend.flag = MODIFY_FAIL_NEW;
								break;
							}
						}
						else
						{//原密码输入不正确
							clientsend.flag = MODIFY_FAIL_OLD;
							break;
						}
					}
				}
				writecnt = write(p->clientfd,&clientsend,sizeof(client));          //发送给目标用户
				if(writecnt == -1)
				{
					perror("modify:write");
					return 0;
				}
				break;
			}
			case VIEW_USER_MSG:		//查看用户个人信息
				break;
			case BECOME_VIP:			//成为vip
			{
				p = my_user_list->next;
				while(p->next != NULL)
				{//定位开通vip的用户，修改其相关信息
					p=p->next;
					if(strcmp(p->username,clientrecv.username) == 0)
					{//找到该用户
						if(strcmp(clientrecv.msg,vip_code) == 0)
						{//会员码输入正确，开通成功，更新会员码

							//更新会员码
							memset(&vip_code,0,sizeof(char));//清空原会员码
							get_random_str(vip_code, 8);
							printf("new vip code is:\33[40;37m %s \33[0m\n",vip_code);

							clientsend.flag = VIP_SUCCESS;
							p->vip = 1;

							//server端输出相关提示
							printf("********************************\n");
							printf("%s become a vip | %s",p->username,my_time());
							printf("********************************\n");
							break;
						}
						else
						{//会员码不正确，开通失败
							clientsend.flag = VIP_FAIL;
							break;
						}
					}
				}

				writecnt = write(p->clientfd,&clientsend,sizeof(client));          //发送给目标用户
				if(writecnt == -1)
				{
					perror("vip:write");
					return 0;
				}
				break;
			}
			case REGISTER:			//注册
			{
				//注册情况判断
				if(isidok(clientrecv.username))
				{//用户名不合法
					clientsend.flag = REG_FAIL_ID_UQUA;
				}
				else if(isidexist(my_user_list,clientrecv.username))
				{//判断用户名是否已存在
					clientsend.flag = REG_FAIL_ID_ALE;
				}
				else if(strcmp(clientrecv.new_passwd1,clientrecv.new_passwd2) != 0)
				{////两次密码不同,注册失败
					clientsend.flag = REG_FAIL_PWD_DIFF;
				}
				else if(is_pwd_easy(clientrecv.new_passwd1))
				{//注册失败
					clientsend.flag = REG_FAIL_PWD_EASY;
				}
				else
				{//注册成功
					clientsend.flag = REG_SUCCESS;

					//将用户信息存入用户列表
					add_user(my_user_list,clientrecv,clientfd);
				}

				//注册情况返回
				writecnt = write(clientfd,&clientsend,sizeof(client));
				if(writecnt == -1)
				{
					perror("client:write");
					//return -1;
				}
				break;
			}
			case LOGIN:				//登陆
			{
				//登陆情况判断
				int n = login_client(my_user_list,clientrecv,clientfd);
				if(n == LOGIN_SUCCESS)
				{//登陆成功
					clientsend.flag = LOGIN_SUCCESS;
				}
				else if(n == LOGIN_FAIL_UNMATCH)
				{//密码不正确
					clientsend.flag = LOGIN_FAIL_UNMATCH;
				}
				else if(n == LOGIN_FAIL_ALREADY_LOGIN)
				{//账号已登录
					client clienthint;
					clientsend.flag = LOGIN_FAIL_ALREADY_LOGIN;                   //用户已经登录
					clienthint.flag = LOGIN_HINT;   //向该用户发送被重复登录消息

					p = my_user_list->next;
					p = p->next;
					while(p!=NULL)
					{
						if(strcmp(p->username,clientrecv.username) == 0)
						{//找到当前已登录的账户
							writecnt = write(p->clientfd,&clienthint,sizeof(client));   //向该用户发送被重复登录消息
							break;
						}
						p = p->next;
					}

				}
				else
				{
					clientsend.flag = LOGIN_FAIL_NOT_EXIST;
				}

				//登陆情况返回
				writecnt = write(clientfd,&clientsend,sizeof(client));

				if(writecnt == -1)
				{
					perror("client:write");
					//return -1;
				}
				break;
			}
			case EXIT_NOW:
			{
				p = my_user_list->next;
				while(p!=NULL)//逐个比较
				{
					if(strcmp(p->username,clientrecv.username) == 0)
					{//找到当前已登录的账户
						p->online = 0;
						break;
					}
					p = p->next;
				}
				break;
			}
		}
	}
}

/**************判断用户密码是否过于简单**************************/
int is_pwd_easy(char pwd[])
{//简单，返回1，否则返回0
	/*1.密码长度低于6位
	  2.密码不包含数字
	  3.密码不包含字母*/
	int have_num=0,have_char=0;

	//判断1
	int pwd_len,i;
	pwd_len = strlen(pwd);

	//判断2、3
	char c;
	for(i=0;i<pwd_len;i++)
	{
		c = pwd[i];
		if(have_num==0 && c>='0'&& c<='9')
		{//当前位置密码为数字
			have_num = 1;
		}
		if(have_char==0 && ((c>='a' && c<='z') || (c>='A' && c<='Z')))
		{//当前位置密码为字母
			have_char = 1;
		}
		if(have_num == 1 && have_char == 1)
			break;
	}
	if(pwd_len<6 || have_num == 0 || have_char == 0)
		return 1;
	else
		return 0;
}

/**************判断注册用户名是否合法**************************/
int isidok(char *name)
{//包含禁用词汇，返回1,否则返回0
	int i;
	for(i=0;i<=2;i++)
	{
		if(strstr(name,ban_word[i]))
		{//名字包含当前词汇
			return 1;
		}
	}
	return 0;
}

/**************判断注册用户名是否已存在**************************/
int isidexist(user *H,char *name)
{
	H = H->next;//跳过头节点
	user *p;
	p = H;
	while(p!=NULL)
	{
		if(strcmp(p->username,name)==0)
		{
			return 1;
		}
		p = p->next;
	}
	return 0;
}

/*******************将注册用户存入链表**************************/
void add_user(user *H,client clientrecv,int clientfd)
{//H-头节点；clientrecv-注册用户信息
	user *p,*s;
	p = H;
	while(p->next!=NULL)
	{
		p = p->next;//顺移一位

	}
	s = (user*)malloc(sizeof(user));//为新用户节点动态申请空间
	strcpy(s->username,clientrecv.username);//存用户名
	strcpy(s->password,clientrecv.new_passwd1);//存密码
	s->online = 0;//在线
	s->power = 0;//新用户默认未禁言
	s->clientfd = clientfd;//用户socket描述符
	s->next = NULL;
	p->next = s;
}

/*******************账户密码匹配判断**************************/
int is_pwd_correct(user *H,client clientrecv)
{//密码匹配返回0，否则返回1
	user *p;
	p = H->next;
	while(p->next!=NULL)
	{
		p = p->next;//顺移一位
		if(strcmp(clientrecv.username,p->username) == 0)
		{//找到当前用户,用户名匹配
			if(strcmp(clientrecv.password,p->password) == 0)
				return 0;//密码匹配
			else
				return 1;//密码不匹配
		}
	}
}

/*******************账户在线判断**************************/
int is_user_online(user *H,char *name)
{//在线返回1,离线返回0
	user *p;
	p = H->next;
	while(p->next!=NULL)
	{
		p = p->next;//顺移一位
		if(strcmp(name,p->username) == 0)
		{//找到当前用户,用户名匹配
			if(p->online == 1)
				return 1;
			else
				return 0;
		}
	}
}

/*******************用户登陆判断**************************/
int login_client(user *H,client clientrecv,int clientfd)
{
	user *p;
	p = H->next;
	while(p!=NULL)//逐个比较
	{
		if(strcmp(p->username,clientrecv.username) == 0)
		{
			if(strcmp(p->password,clientrecv.password) == 0)
			{//用户名匹配、密码匹配
				if(p->online==0)
				{//用户之前未登录
					p->online=1;
					p->clientfd = clientfd;
					return LOGIN_SUCCESS;
				}
				else
				{//用户之前已经登录
					return LOGIN_FAIL_ALREADY_LOGIN;
				}
			}
			else
			{//用户名匹配、密码不匹配
				return LOGIN_FAIL_UNMATCH;
			}
		}
		p = p->next;
	}
	return LOGIN_FAIL_NOT_EXIST;
}

/*************************生成固定长度的随机字符串******************************/
int get_random_str(char* random_str, const int random_len)
{
    int i, random_num, seed_str_len;
    struct timeval tv;
    unsigned int seed_num;
    char seed_str[] = "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; //随机字符串的随机字符集

    seed_str_len = strlen(seed_str);

    gettimeofday(&tv, NULL);
    seed_num = (unsigned int)(tv.tv_sec + tv.tv_usec); //超了unsigned int的范围也无所谓，我们要的只是不同的种子数字
    srand(seed_num);

    for(i = 0; i < random_len; i++)
    {
        random_num = rand()%seed_str_len;
        random_str[i] = seed_str[random_num];
    }

    return 0;
}

//建立用户信息表,返回链表头节点，添加一个测试用户，方便调试，不用反复注册
user *CreateList()
{
	user *H,*p,*s;
	H = (user*)malloc(sizeof(user));//为头节点动态申请空间
	H->next = NULL;//先建立单头结点的链表
	p = H;
	s = (user*)malloc(sizeof(user));//为特殊用户节点动态申请空间
	strcpy(s->username,"admin");//管理员id:admin
	strcpy(s->password,"123456");//管理员pwd:123456
	s->power = 0;
	s->online = 1;
	s->clientfd = MANAGER;
	s->next = NULL;
	p->next = s;
	return H;
}

//添加测试用户，方便调试，不用反复注册
void add_testuser(user *H)
{
	user *p,*s,*r,*m;
	r = H->next;

	//测试用户James-----123
	p = (user*)malloc(sizeof(user));//为特殊用户节点动态申请空间
	strcpy(p->username,"James");
	strcpy(p->password,"123");
	p->power = 0;//默认未被禁言
	p->online = 0;//默认离线
	p->vip = 0;//默认非会员

	//测试用户Durant-----147
	s = (user*)malloc(sizeof(user));//为特殊用户节点动态申请空间
	strcpy(s->username,"Durant");
	strcpy(s->password,"147");
	s->power = 0;
	s->online = 0;
	s->vip = 0;

	//测试用户Kyrie-----159
	m = (user*)malloc(sizeof(user));//为特殊用户节点动态申请空间
	strcpy(m->username,"Kyrie");
	strcpy(m->password,"159");
	m->power = 0;
	m->online = 0;
	m->vip = 0;


	p->next = s;
	s->next = m;
	m->next = NULL;
	r->next = p;
}

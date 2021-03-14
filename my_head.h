#ifndef _MY_HEAD_H_
#define _MY_HEAD_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
//#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
//inlude <sqlite3.h>

#define BUFFSIZE 50
#define MYPORT 3515
#define MAX_CLIENT 100

//标志符宏定义
//注册
#define REG_SUCCESS       1	//注册成功
#define REG_FAIL_PWD_DIFF 2	//两次密码不同,注册失败
#define REG_FAIL_ID_UQUA  3      //用户名不合法，注册失败
#define REG_FAIL_ID_ALE 100	//用户名已被使用
#define REG_FAIL_PWD_EASY 96	//登陆成功，密码过于简单提示

//登陆
#define LOGIN_SUCCESS	4	//登陆成功
#define LOGIN_FAIL_NOT_EXIST  5  //用户不存在
#define LOGIN_FAIL_UNMATCH 6	//用户名与密码不匹配
#define EXIT 98			//下线
#define LOGIN_FAIL_ALREADY_LOGIN 33	//用户已登录

//接收消息线程返回标志符
//client.falg
#define PRIVATE_SUCCESS 7		//收到私聊成功
#define PRIVATE_FAIL_NONE_EXIST 8//私聊时查无此人
#define GROUP_SUCCESS   9        //群聊成功
#define GROUP_FAIL_NO_POWER 99	//用户被禁言无法发送群聊消息
#define VIEW_ONLINE_SUCCESS 10	//查看在线用户列表成功
#define MODIFY_SUCCESS 11		//修改密码成功
#define MODIFY_FAIL_OLD  12	//原密码不正确，修改密码失败
#define MODIFY_FAIL_NEW  13	//新密码输入不一致,修改密码失败
#define LOGIN_HINT 94		//用户异地登陆提示
#define OBJECT_EXIST 92		//私聊对象存在
#define OBJECT_NOT_EXIST 92	//私聊对象不存在

//client.action行动确定
#define PRIVATE_CHAT 14		//私聊
#define OBJECT_CHECK 93		//私聊用户检查
#define GROUP_CHAT   15		//群聊
#define VIEW_ONLINE_USERS   16	//查看在线用户
#define MODIFY_PWD   17		//修改密码
#define VIEW_USER_MSG 18 		//查看用户个人信息
#define BECOME_VIP	  19		//成为vip
#define REGISTER  20		//注册
#define LOGIN  21			//登陆
#define SHUT_MOUTH 24			//禁言
#define CANCEL_SHUT_MOUTH 25			//解禁言
#define EXIT_NOW 26			//退出

//vip
#define VIP_SUCCESS 22		//开通vip成功
#define VIP_FAIL 23		//开通vip失败

//管理员
#define MANAGER 97			//管理员clientfd

#define SHUT_MOUTH_FAIL_NOT_MANAGER  27	//非管理员，无法禁言
#define SHUT_MOUTH_FAIL_NONE_PERSON  28	//无此用户，无法禁言
#define SHUT_MOUTH_SUCCESS  29	//禁言成功

#define CANCEL_SHUT_MOUTH_FAIL_NOT_MANAGER  30	//非管理员，无法解禁
#define CANCEL_SHUT_MOUTH_FAIL_NONE_PERSON  31	//无此用户，无法解禁
#define CANCEL_SHUT_MOUTH_SUCCESS  32	//解禁成功

//全局变量
int object_exist;

typedef struct users        //用户信息:链表
{	
	char username[20];      //用户名
	char password[20];	    //密码
	char signature[20];     //个性签名	
	int clientfd;	    //
	int power;              //1：禁言，0：未禁言
	int online;             //1：在线，0：离线
	int vip;		    //1:会员，0普通用户		
	struct users *next;
}user;

typedef struct regist
{
	int flag;               //注册登录标志符	
	int vip;                //会员用户          为1时具有会员权限，为0没有
	char username[20];      //用户名
	char password[20];      //密码
	char new_passwd1[20];   //新密码
	char new_passwd2[20];   //新密码验证
	char file[1024];        //文件
	char filename[20];      //文件名
	char sign[50];          //个性签名
//	int friend_num;         //好友数
//	int group_num;          //群组数
	char offline_msg[500];  //离线消息
	int action;		    //通过action表明是什么命令；
	char fromname[20];
	char toname[20];
	char time[20];          //时间
	int online;		    //在线为1，离线为0
	char msg[500];          //消息内容
	int seq;		    //接收相关顺序
}client;


#endif
#ifndef ACCOUNT_H
#define ACCOUNT_H

#define NAME_MAX     32       // 用户名最多 31 字节，另留一位存放结束符
#define PASSWORD_MAX 64       // 密码最多 63 字节，另留一位存放结束符

// 保存一个用户的账号信息
typedef struct
{
    char name[NAME_MAX];
    char password[PASSWORD_MAX];
} User;

// 输入账号信息并完成注册，无参数、无返回值
void doRegister(void);

// 输入账号信息并登录，通过 *user 保存账号；成功返回 1，失败返回 0
int doLogin(User *user);

#endif

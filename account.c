#include <stdlib.h>  // exit
// 账号注册、登录和文件保存，users.txt 每行存一个用户名和密码
#include <errno.h>      /* 区分首次运行无文件与读取错误 */
#include <stdio.h>      /* printf、fopen、fprintf 等输入输出函数 */
#include <string.h>     /* strcmp、strlen 等字符串函数           */
#include <ctype.h>      /* islower、isupper、isdigit 字符判断    */
#include "account.h"    /* 本模块的结构体与函数声明              */

#define USER_MAX     100      /* 一次最多读入内存的账号个数                  */

#define USERS_FILE "users.txt"   /* 账号文件保存在程序运行时的工作目录 */

/* 以下是本文件内部使用的辅助函数，只在本文件可见 */
static int loadUsers(User users[], int maxCount);
static int findUser(const User users[], int count, const char name[]);
static int isNameUsed(const char name[]);

/*
 * 功能：检查密码长度和字符种类
 * 输入：password 是要检查的密码
 * 输出：不修改密码；数字、大小写字母和其他符号分别算一类
 * 返回值：不足 6 位或至多一类返回 0，两类返回 1，三类及以上返回 2
 */
static int checkPassword(char password[])
{
    int length;         /* 密码的字符个数（不含结束符）              */
    int hasLower = 0;   /* 是否含小写字母：1 有，0 无                */
    int hasUpper = 0;   /* 是否含大写字母：1 有，0 无                */
    int hasDigit = 0;   /* 是否含数字：1 有，0 无                    */
    int hasOther = 0;   /* 是否含其它符号：1 有，0 无                */
    int kinds;          /* 密码中实际出现的字符种类数（0～4）        */
    int i;              /* 扫描密码用的下标                          */
    unsigned char ch;   /* 当前字符，用无符号类型可避免 ctype 越界   */

    if (password == NULL)                       /* 没有密码时按不安全处理 */
    {
        return 0;
    }

    length = (int)strlen(password);             /* 取密码长度 */
    if (length < 6)                             /* 长度不足 6 位一律不安全 */
    {
        return 0;
    }

    for (i = 0; i < length; i++)                /* 逐个字符统计所属类别 */
    {
        ch = (unsigned char)password[i];
        if (islower(ch))
        {
            hasLower = 1;
        }
        else if (isupper(ch))
        {
            hasUpper = 1;
        }
        else if (isdigit(ch))
        {
            hasDigit = 1;
        }
        else
        {
            hasOther = 1;                       /* 其它符号也算一类 */
        }
    }

    kinds = hasLower + hasUpper + hasDigit + hasOther;  /* 出现过的类别数 */
    if (kinds <= 1)                             /* 只有一类：不安全 */
    {
        return 0;
    }
    if (kinds == 2)                             /* 两类：中度安全 */
    {
        return 1;
    }
    return 2;                                   /* 三类及以上：安全 */
}

/*
 * 功能：把新账号追加到账号文件
 * 输入：user 是待注册的用户名和密码
 * 输出：成功时将账号写入 users.txt
 * 返回值：成功返回 1；账号为空、重名、已满或文件读写失败返回 0
 */
static int registerUser(User user)
{
    FILE *fp;       /* 指向 users.txt 的文件指针                 */
    int used;       /* 用户名是否已被占用：1 已占用，0 未占用    */

    if (user.name[0] == '\0' || user.password[0] == '\0')   /* 参数为空不作处理 */
    {
        return 0;
    }

    used = isNameUsed(user.name);       /* 先查重，用户名不能重复 */
    if (used != 0)
    {
        return 0;                       /* 1 重名，-1 读取失败，都按失败处理 */
    }

    fp = fopen(USERS_FILE, "a");        /* 追加方式打开，文件不存在时自动创建 */
    if (fp == NULL)
    {
        return 0;                       /* 文件建立失败 */
    }

    /* 每条记录写成“用户名 密码”一行，两个字段之间用空格分隔 */
    if (fprintf(fp, "%s %s\n", user.name, user.password) < 0)
    {
        fclose(fp);
        return 0;                       /* 写入失败 */
    }

    if (fclose(fp) != 0)                /* 关闭失败时，数据可能没有保存完整 */
    {
        return 0;
    }
    return 1;                           /* 注册成功 */
}

/*
 * 功能：检查用户名和密码是否与已保存的账号一致
 * 输入：user 是待验证的账号信息
 * 输出：不修改账号文件
 * 返回值：匹配成功返回 1，否则返回 0
 */
static int loginUser(User user)
{
    User users[USER_MAX];   /* 从文件读入的账号数组            */
    int count;              /* 实际读入的账号个数              */
    int i;                  /* 遍历账号数组用的下标            */

    count = loadUsers(users, USER_MAX);     /* 读入全部账号 */
    for (i = 0; i < count; i++)
    {
        /* 用户名和密码必须同时相同才算登录成功 */
        if (strcmp(users[i].name, user.name) == 0 &&
            strcmp(users[i].password, user.password) == 0)
        {
            return 1;
        }
    }
    return 0;               /* 用户名不存在或密码错误 */
}

/*
 * 功能：从账号文件读取账号
 * 输入：users 是接收账号的数组，maxCount 是最多读取的个数
 * 输出：将读取的账号写入 users
 * 返回值：实际读入的个数；文件不存在返回 0，读取失败返回 -1
 */
static int loadUsers(User users[], int maxCount)
{
    FILE *fp;           /* 指向 users.txt 的文件指针   */
    int count = 0;      /* 已经读入的账号个数          */

    fp = fopen(USERS_FILE, "r");
    if (fp == NULL)     /* 文件还不存在：还没有任何账号 */
    {
        return errno == ENOENT ? 0 : -1;
    }

    /* 每次读两个字段，读到文件末尾或数组装满就停止 */
    while (count < maxCount &&
           fscanf(fp, "%31s %63s", users[count].name, users[count].password) == 2)
    {
        count++;
    }

    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return count;
}

/*
 * 功能：按用户名查找账号
 * 输入：users 是账号数组，count 是账号个数，name 是要查找的用户名
 * 输出：不修改数组
 * 返回值：找到返回数组下标，找不到返回 -1
 */
static int findUser(const User users[], int count, const char name[])
{
    int i;              /* 遍历数组用的下标 */

    for (i = 0; i < count; i++)
    {
        if (strcmp(users[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

/*
 * 功能：检查用户名是否可以注册
 * 输入：name 是要检查的用户名
 * 输出：读取账号文件，不修改文件
 * 返回值：已注册返回 1，可以使用返回 0，读取失败或账号已满返回 -1
 */
static int isNameUsed(const char name[])
{
    User users[USER_MAX];   /* 临时存放读入的账号 */
    int count;              /* 读入的账号个数     */

    count = loadUsers(users, USER_MAX);
    if (count < 0 || count >= USER_MAX) {
        return -1;  /* 文件读取失败或账号数组已满，不再追加 */
    }
    if (findUser(users, count, name) >= 0)
    {
        return 1;
    }
    return 0;
}


/*
 * 功能：读取用户名和密码
 * 输入：user 是接收账号信息的结构体地址，通过键盘输入账号
 * 输出：将输入写入 user->name 和 user->password，读取失败时退出程序
 *       用户名最多 31 字节，密码最多 63 字节，均不能含空格
 * 返回值：无
 */
static void readUser(User *user)
{
    printf("请输入用户名（不含空格，最多 31 字节）：");
    if (scanf("%31s", user->name) != 1) {
        exit(0);
    }
    printf("请输入密码（不含空格，最多 63 字节）：");
    if (scanf("%63s", user->password) != 1) {
        exit(0);
    }
}

/*
 * 功能：完成注册流程并提示结果
 * 输入：无参数，通过键盘输入账号，密码不安全时选择继续或取消
 * 输出：显示密码安全等级和注册结果，成功时写入 users.txt
 * 返回值：无
 */
void doRegister(void)
{
    User user;
    int safety;
    int choice;
    printf("\n---------- 注册新用户 ----------\n");
    readUser(&user);
    safety = checkPassword(user.password);
    if (safety == 0) {
        printf("密码安全等级：不安全（少于 6 位，或只含一类字符）。\n");
        do {
            printf("是否仍要使用该密码注册？1=继续 0=取消：");
            if (scanf("%d", &choice) != 1) {
                exit(0);
            }
        } while (choice < 0 || choice > 1);
        if (choice == 0) {
            printf("已取消注册。\n");
            return;
        }
    } else if (safety == 1) {
        printf("密码安全等级：中度安全（包含两类字符）。\n");
    } else {
        printf("密码安全等级：安全（包含三类及以上字符）。\n");
    }
    if (registerUser(user) == 1) {
        printf("注册成功，请使用该账号登录。\n");
    } else {
        printf("注册失败：用户名已存在、账号已满或文件读写失败。\n");
    }
}

/*
 * 功能：读取账号并完成登录检查
 * 输入：user 是接收账号信息的结构体地址
 * 输出：将输入的账号保存在 *user 中，显示登录结果
 * 返回值：成功返回 1，失败返回 0
 */
int doLogin(User *user)
{
    printf("\n---------- 用户登录 ----------\n");
    readUser(user);
    if (loginUser(*user) == 1) {
        printf("登录成功，欢迎你，%s！\n", user->name);
        return 1;
    }
    printf("登录失败：用户名或密码不正确。\n");
    return 0;
}

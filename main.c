// main.c：账号入口、玩家菜单以及三个模块之间的连接
#include <stdio.h>
#include <stdlib.h>     // srand、exit
#include <time.h>       // time
#include "account.h"
#include "game.h"
#include "rank.h"

#define LOGIN_EXIT     0
#define LOGIN_REGISTER 1
#define LOGIN_SIGNIN   2

// 读取菜单选项和游戏难度

static int readInt(const char prompt[], int min, int max);
static int chooseLevel(void);

// 流程函数：玩家菜单、游戏

static int playerMenu(User user);
static void doPlayGame(User user);

/*
 * 功能：组织注册、登录和玩家菜单的运行流程
 * 输入：无参数，通过键盘选择菜单
 * 输出：显示菜单和操作提示
 * 返回值：0 表示正常结束
 */
int main(void)
{
    User user;
    int running = 1;  // 1 继续运行，0 结束程序

    srand(time(NULL));//设置种子
    printf("\n********** 记忆力大考验 **********\n");
    printf("首次使用请先注册，登录后开始游戏。\n");
    while (running) {
        printf("\n1. 注册新用户\n2. 登录\n0. 退出程序\n");
        switch (readInt("请选择：", 0, 2)) {
            case LOGIN_REGISTER:
                doRegister();
                break;
            case LOGIN_SIGNIN:
                if (doLogin(&user) == 1) {
                    running = playerMenu(user);
                }
                break;
            case LOGIN_EXIT:
                running = 0;
                break;
        }
    }
    printf("感谢使用，再见！\n");
    return 0;
}

/*
 * 功能：读取指定范围内的整数
 * 输入：prompt 是提示文字，min、max 是允许的最小值和最大值
 * 输出：显示提示，超出范围时要求重新输入；未读到整数时退出程序
 * 返回值：输入的有效整数
 */
static int readInt(const char prompt[], int min, int max)
{
    int value;
    while (1) {
        printf("%s", prompt);
        //bug：如果输入非数字，scanf会一直不读取导致缓冲区一直存在字母卡死程序，必须退出或者清空缓冲区
        if (scanf("%d", &value) != 1) {
            printf("输入结束或格式不正确，程序退出。\n");
            exit(0);
        }
        if (value >= min && value <= max) {
            return value;
        }
        printf("请输入 %d 到 %d 之间的整数。\n", min, max);
    }
}

/*
 * 功能：处理登录后的游戏、排行和退出选项
 * 输入：user 是当前登录的用户
 * 输出：显示用户名和玩家菜单，按选择调用游戏或排行榜
 * 返回值：1 表示切换账号，0 表示退出程序
 */
static int playerMenu(User user)
{
    while (1) {
        printf("\n========== 当前用户：%s ==========\n", user.name);
        printf("1. 开始游戏\n2. 查看排行榜\n3. 切换账号\n0. 退出程序\n");
        switch (readInt("请选择：", 0, 3)) {
            case 1:
                doPlayGame(user);
                break;
            case 2:
                doShowRank();
                break;
            case 3:
                printf("已退出当前账号，请重新登录。\n");
                return 1;
            case 0:
                return 0;
        }
    }
}

/*
 * 功能：选择游戏难度
 * 输入：无参数，通过键盘输入难度编号
 * 输出：显示难度菜单
 * 返回值：1、2、3 分别表示初级、中级、高级，0 表示取消
 */
static int chooseLevel(void)
{
    printf("\n---------- 选择难度 ----------\n");
    printf("1. 初级（4x4，8 对图案）\n");
    printf("2. 中级（4x5，10 对图案）\n");
    printf("3. 高级（6x6，12 组，每组 3 张）\n");
    printf("0. 返回玩家菜单\n");
    return readInt("请输入难度编号：", 0, 3);
}

/*
 * 功能：开始一局游戏，通关后保存玩家成绩
 * 输入：user 是当前登录的用户
 * 输出：显示游戏结果、通关用时和成绩保存结果；未通关不保存
 * 返回值：无
 */
static void doPlayGame(User user)
{
    int level = chooseLevel();
    int seconds, result;
    if (level == 0) {
        return;
    }
    if (playGame(level, &seconds) != 1) {
        printf("本局没有完成，本次不计成绩。\n");
        return;
    }
    printf("\n恭喜通关！本局用时 %d 秒。\n", seconds);
    result = saveScore(user.name, level, seconds); //bug：使用了rank里面的saveSource，不能直接像另外的do一样转接进对应模块中
    if (result == 1) {
        printf("成绩已保存到排行榜。\n");
    } else if (result == 0) {
        printf("本次未刷新个人最好成绩或未进入前五名，榜单未变化。\n");
    } else {
        printf("成绩文件读写出错，本次成绩未能保存。\n");
    }
}


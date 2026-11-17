#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>  // Sleep：Windows 睡眠函数
#include "game.h"



// 内部函数声明
// 显示棋盘
void showBoard(int cards[6][6], int state[6][6],
                      int rows, int cols);

// 初始化牌面和状态
void initBoard(int cards[6][6], int state[6][6],
                      int level, int *rows, int *cols);

// 随机打乱牌面
void shuffleBoard(int cards[6][6], int rows, int cols);

// 输入并检查选牌坐标
int chooseCard(int state[6][6], int rows, int cols,
                      int *row, int *col);

// 判断所选牌是否相同
int checkMatch(int cards[6][6], int selectedRows[],
                      int selectedCols[], int count);

// 判断是否全部配对
int isFinished(int state[6][6], int rows, int cols);

// 开局展示牌面，然后盖牌
void previewBoard(int cards[6][6], int state[6][6],
                         int rows, int cols, int seconds);


/*
 * 功能：显示带行号、列号的棋盘
 * 输入：cards 存牌面，state 存牌的状态，rows、cols 是本局行列数
 * 输出：盖住的牌显示 *，其他牌按编号显示字符；不修改数组
 * 返回值：无
 */
void showBoard(int cards[6][6], int state[6][6],
               int rows, int cols)
{
    int i, j;
    /* 牌面编号和字符下标都从 0 开始，显示时直接查表
     * 预留数字、大小写字母和常见符号，共 90 种
     * * 专门表示盖牌，不能作为牌面；空格也不作为牌面
     * 可以修改或追加下面的字符，初始化和配对仍使用数字编号
     */
    const char symbols[] =
        "!@#$%^&()-_=+[]{};:,.<>/?|~`"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "1234567890";

    // 列号放在横线的上面，与下面的牌面分开
    printf("    |");
    for (j = 0; j < cols; j++) {
        printf("%4d", j + 1);
    }
    printf("\n----+");
    for (j = 0; j < cols; j++) {
        printf("----");
    }
    printf("\n");

    // 行号放在竖线左边，牌面放在竖线右边
    for (i = 0; i < rows; i++) {
        printf("%4d|", i + 1);
        for (j = 0; j < cols; j++) {
            if (state[i][j] == 0) {
                printf("%4s", "*");  // 盖住的牌
            } else {
                printf("%4c", symbols[cards[i][j]]);  // 按编号查字符
            }
        }
        printf("\n");
    }
}

/*
 * 功能：按难度生成牌面并初始化状态
 * 输入：level 为 1、2、3；cards、state 接收数据，rows、cols 是接收行列数的地址
 * 输出：写入 cards，将 state 全部设为 0，通过 *rows、*cols 带回行列数
 *       初、中级每种牌两张，高级三张；难度无效时行列数为 0
 * 返回值：无
 */
void initBoard(int cards[6][6], int state[6][6],
               int level, int *rows, int *cols)
{
    int i, j;
    int groupSize = 2;  // 初、中级两张一组，高级改为三张
    int number = 0;  // 已经填入的牌数，从 0 开始

    // 先清空整个数组，避免保留上一局的牌面和状态
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 6; j++) {
            cards[i][j] = 0;
            state[i][j] = 0;
        }
    }

    *rows = 0;
    *cols = 0;
    if (level == 1) {
        *rows = 4;
        *cols = 4;
    } else if (level == 2) {
        *rows = 4;
        *cols = 5;
    } else if (level == 3) {
        *rows = 6;
        *cols = 6;
        groupSize = 3;
    } else {
        return;  // 难度编号无效，不生成棋盘
    }

    // 按每组张数生成编号；两张一组时为 0、0、1、1……
    for (i = 0; i < *rows; i++) {
        for (j = 0; j < *cols; j++) {
            cards[i][j] = number / groupSize;
            number++;
        }
    }
}

/*
 * 功能：随机打乱牌的位置
 * 输入：cards 是牌面数组，rows、cols 是本局行列数
 * 输出：直接交换 cards 中的牌，各种牌的数量不变
 * 返回值：无
 */
void shuffleBoard(int cards[6][6], int rows, int cols)
{
    int i, j, temp;

    /* 把本局棋盘看成一排，共 rows * cols 张牌
     * 从最后一张向前处理，随机选择尚未处理的牌进行交换
     */
    for (i = rows * cols - 1; i > 0; i--) {
        j = rand() % (i + 1);  // 随机位置为 0 到 i，允许选中自己

        /* 一排中的位置除以列数得到行下标，取余得到列下标
         * 例如列数为 4 时，位置 6 对应 cards[1][2]
         */
        temp = cards[i / cols][i % cols];//先保存位置 i 的牌，供下面交换使用
        //交换
        cards[i / cols][i % cols] = cards[j / cols][j % cols];
        cards[j / cols][j % cols] = temp;
    }
}

/*
 * 功能：读取并检查玩家选择的牌
 * 输入：state 是状态数组，rows、cols 是行列数，row、col 是接收坐标的地址
 * 输出：有效坐标通过 *row、*col 带回，换算为从 0 开始的下标
 *       越界或已翻开的牌要重新选择，这里不修改 state
 * 返回值：1 表示选牌成功；输入 0 0、输入结束或格式不对时返回 0
 */
int chooseCard(int state[6][6], int rows, int cols,
               int *row, int *col)
{
    int r, c;  // 玩家输入的行列编号，从 1 开始

    while (1) {
        printf("请输入行号和列号（例如 1 2，输入 0 0 退出）：");
        if (scanf("%d %d", &r, &c) != 2) {
            // 没读到两个整数就结束本次选牌
            printf("输入结束或格式不正确，结束选择。\n");
            return 0;
        }
        if (r == 0 && c == 0) {
            return 0;
        }

        // 必须先检查范围，再访问数组，避免下标越界
        if (r < 1 || r > rows || c < 1 || c > cols) {
            printf("位置越界，请重新选择。\n");
        } else if (state[r - 1][c - 1] == 1) {
            printf("这张牌已经翻开，请选择其他位置。\n");
        } else if (state[r - 1][c - 1] == 2) {
            printf("这张牌已经配对，请选择其他位置。\n");
        } else {
            // 只有有效选择才把数组下标带回，不修改牌的状态
            *row = r - 1;
            *col = c - 1;
            return 1;
        }
    }
}

/*
 * 功能：检查本轮选中的牌是否相同
 * 输入：cards 是牌面数组，selectedRows、selectedCols 存所选牌的行列下标，count 是张数
 * 输出：只比较牌面，不修改数组；调用前要保证至少两张、坐标有效且不重复
 * 返回值：全部相同返回 1，否则返回 0
 */
int checkMatch(int cards[6][6], int selectedRows[],
               int selectedCols[], int count)
{
    int i;
    int first;  // 第一张牌的编号，作为比较对象

    first = cards[selectedRows[0]][selectedCols[0]];

    // 从第二张开始，只要有一张不同，就不是一组相同的牌
    for (i = 1; i < count; i++) {
        if (cards[selectedRows[i]][selectedCols[i]] != first) {
            return 0;
        }
    }
    return 1;  // 所有选中的牌都与第一张相同
}

/*
 * 功能：检查本局是否已经全部配对
 * 输入：state 是状态数组，rows、cols 是本局行列数
 * 输出：只检查本局范围，不修改数组
 * 返回值：所有牌的状态均为 2 返回 1，否则返回 0
 */
int isFinished(int state[6][6], int rows, int cols)
{
    int i, j;

    // 只检查本局使用的区域，不检查整个 6×6 数组
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            if (state[i][j] != 2) {
                return 0;  // 还有牌没有配对，本局未完成
            }
        }
    }
    return 1;  // 全部检查完，每张牌都已经配对
}

/*
 * 功能：开局展示全部牌，倒计时后盖牌
 * 输入：cards 是牌面数组，state 是状态数组，rows、cols 是行列数，seconds 是展示秒数
 * 输出：显示牌面和倒计时，结束后将本局 state 设为 0；不修改 cards
 *       使用 ANSI 清屏，需要终端支持
 * 返回值：无
 */
void previewBoard(int cards[6][6], int state[6][6],
                  int rows, int cols, int seconds)
{
    int i, j;
    int remaining;  // 剩余展示秒数

    // 开局将本局所有牌设为临时翻开，牌面编号不变
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            state[i][j] = 1;
        }
    }

    /* ANSI 转义序列：清除当前屏幕，再将光标移到左上角
     * 需要终端支持 ANSI，不保证清除滚动历史
     */
    printf("\033[2J\033[H");
    printf("请记忆牌面，%d 秒后盖牌。\n", seconds);
    showBoard(cards, state, rows, cols);

    /* 每次显示剩余时间后暂停一秒
     */
    for (remaining = seconds; remaining > 0; remaining--) {
        printf("\r还有 %d 秒盖牌。    ", remaining);
        Sleep(1000);
    }

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            state[i][j] = 0;
        }
    }
    printf("\033[2J\033[H");
    printf("记忆时间结束，所有牌已盖住。\n");
    showBoard(cards, state, rows, cols);
}

/*
 * 功能：运行一整局游戏并统计通关用时
 * 输入：level 为难度 1、2、3，seconds 是接收用时的变量地址
 * 输出：显示游戏过程；通关时把盖牌后计算的用时写入 *seconds，单位为整秒
 *       未通关不修改 *seconds，这里不保存排行榜
 * 返回值：通关返回 1；中途退出、输入结束或难度无效返回 0
 */
int playGame(int level, int *seconds)
{
    int cards[6][6], state[6][6];
    int rows, cols;
    int selectedRows[3], selectedCols[3];  // 本轮所选牌的坐标
    int row, col, count, i;
    int groupSize = 2;
    int showSeconds;
    time_t startTime, endTime;

    // 亮牌时长暂按初级 4 秒、中级 6 秒、高级 8 秒设置
    if (level == 1) {
        showSeconds = 4;
    } else if (level == 2) {
        showSeconds = 6;
    } else if (level == 3) {
        showSeconds = 8;
        groupSize = 3;
    } else {
        printf("难度编号无效。\n");
        return 0;
    }

    initBoard(cards, state, level, &rows, &cols);
    shuffleBoard(cards, rows, cols);
    previewBoard(cards, state, rows, cols, showSeconds);
    startTime = time(NULL);  // 记忆阶段结束后才开始计算通关用时

    while (isFinished(state, rows, cols) == 0) {
        count = 0;  // 每轮重新记录坐标，不需要清空坐标数组
        while (count < groupSize) {
            printf("本轮需要选择 %d 张牌，正在选择第 %d 张。\n",
                   groupSize, count + 1);
            if (chooseCard(state, rows, cols, &row, &col) == 0) {
                printf("本局结束，未产生通关成绩。\n");
                return 0;
            }
            selectedRows[count] = row;
            selectedCols[count] = col;
            state[row][col] = 1;  // 立即标记翻开，阻止重复选择
            count++;

            printf("\033[2J\033[H");
            showBoard(cards, state, rows, cols);
        }

        if (checkMatch(cards, selectedRows, selectedCols, count) == 1) {
            for (i = 0; i < count; i++) {
                state[selectedRows[i]][selectedCols[i]] = 2;
            }
            printf("配对成功！\n");
        } else {
            printf("牌面不同，1 秒后盖回。\n");
            Sleep(1000);  // 只在错配时等待，不限制整局操作时间
            for (i = 0; i < count; i++) {
                state[selectedRows[i]][selectedCols[i]] = 0;
            }
            printf("\033[2J\033[H");
            showBoard(cards, state, rows, cols);
        }
    }

    endTime = time(NULL);
    *seconds = (int)difftime(endTime, startTime);
    printf("全部配对成功，通关用时 %d 秒。\n", *seconds);
    return 1;
}

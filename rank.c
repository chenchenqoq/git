#include <windows.h>
#include <stdlib.h>
/*
 * 各难度分别保存前五名，同一玩家只保留最好成绩
 * ranks.txt 每行保存：难度 用户名 用时
 * 用时相同时，先入榜的排在前面
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "rank.h"

#define RANK_LEVELS 3       /* 级别个数 */
#define RANK_TOP    5       /* 每级保留前几名 */
#define RANK_NAME   32      /* 用户名缓冲区（31 字节 + '\0'） */
#define RANK_FILE   "ranks.txt"

// 排行榜数据只在本文件中使用（static，外部只能通过 saveScore / doShowRank 访问）
typedef struct
{
    char name[RANK_NAME];
    int  seconds;
} Score;

static Score g_rank[RANK_LEVELS][RANK_TOP];                   /* 三个级别，每级 5 个位置 */
static int   g_count[RANK_LEVELS];                            /* 每个级别现在存了几条（0~5） */

static int   g_readError = 0;  /* 文件读取失败时阻止覆盖旧成绩 */
static int   g_loaded = 0;      /* 是否已经从文件读过一次 */

/*
 * 功能：将难度编号换成显示用的名称
 * 输入：level 是难度编号
 * 输出：不修改排行榜
 * 返回值：对应的名称字符串
 */
static const char *levelName(int level)
{
    if (level == 1) return "初级";
    if (level == 2) return "中级";
    if (level == 3) return "高级";
    return "未知";
}

/*
 * 功能：将某一难度的成绩按用时从短到长排序
 * 输入：k 是难度在数组中的下标，从 0 开始
 * 输出：直接调整该难度的成绩数组，用时相同的记录保持原顺序
 * 返回值：无
 */
static void sortLevel(int k)
{
    int i, j;
    Score t;

    for (i = 1; i < g_count[k]; i++) {
        t = g_rank[k][i];
        j = i;
        while (j > 0 && g_rank[k][j - 1].seconds > t.seconds) {  /* 严格大于：同秒不交换，保持稳定 */
            g_rank[k][j] = g_rank[k][j - 1];
            j--;
        }
        g_rank[k][j] = t;
    }
}

/*
 * 功能：读取各难度的成绩并排序
 * 输入：无参数，从 ranks.txt 读取
 * 输出：更新成绩数组、记录个数和读取状态；文件不存在时按空榜处理
 * 返回值：无
 */
static void loadRanks(void)
{
    FILE *fp;
    char line[128];
    char nm[RANK_NAME];
    int  lv, sec, k;

    g_readError = 0;
    /* 先把内存清空 */
    for (k = 0; k < RANK_LEVELS; k++) {
        g_count[k] = 0;
        memset(g_rank[k], 0, sizeof(g_rank[k]));
    }

    fp = fopen(RANK_FILE, "r");                               /* 只读打开 */
    if (fp == NULL) {
        g_readError = (errno != ENOENT);
        g_loaded = 1;           /* 只有文件不存在才按空榜处理 */
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "%d %31s %d", &lv, nm, &sec) != 3)   /* 不是三段就说明这行不合法 */
            continue;
        if (lv < 1 || lv > RANK_LEVELS)                       /* 级别越界，丢弃 */
            continue;
        if (sec < 0)                                         /* 负用时无效，丢弃 */
            continue;

        k = lv - 1;
        if (g_count[k] >= RANK_TOP)     /* 文件里超过 5 条的，多出来的丢弃 */
            continue;

        strncpy(g_rank[k][g_count[k]].name, nm, RANK_NAME - 1);  /* 最多复制 31 字节 */
        g_rank[k][g_count[k]].name[RANK_NAME - 1] = '\0';     /* 复制达到长度限制时，strncpy 不会补结束符，这里手动补上 */
        g_rank[k][g_count[k]].seconds = sec;
        g_count[k]++;
    }

    if (ferror(fp)) {
        g_readError = 1;
    }
    fclose(fp);

    /* 读入后按用时排序（读取时已限制每级最多五条） */
    for (k = 0; k < RANK_LEVELS; k++) {
        sortLevel(k);
    }

    g_loaded = 1;
}

/*
 * 功能：保存当前排行榜
 * 输入：无参数，使用本文件中的成绩数组
 * 输出：将三个难度的成绩写入 ranks.txt
 * 返回值：成功返回 1，失败返回 0
 */
static int writeRanks(void)
{
    FILE *fp;
    int k, i;

    fp = fopen(RANK_FILE, "w");
    if (fp == NULL)
        return 0;

    for (k = 0; k < RANK_LEVELS; k++) {                        /* 一级一级写 */
        for (i = 0; i < g_count[k]; i++) {
            if (fprintf(fp, "%d %s %d\n", k + 1,             /* 级别写 1/2/3，方便人看 */
                        g_rank[k][i].name, g_rank[k][i].seconds) < 0) {
                fclose(fp);
                return 0;
            }
        }
    }

    if (fclose(fp) != 0)        /* 关闭失败也要当作写失败 */
        return 0;

    return 1;
}

/*
 * 功能：提交玩家成绩，保留各难度前五名和同一玩家的最好成绩
 * 输入：name 是玩家名称，level 是难度 1、2、3，seconds 是非负用时秒数
 * 输出：需要更新时修改成绩数组并写入 ranks.txt
 * 返回值：更新并保存成功返回 1，未刷新或未进前五返回 0，参数或文件读写错误返回 -1
 */
int saveScore(const char name[], int level, int seconds)
{
    int k, i, pos, cnt, dup;

    // 检查参数
    if (name == NULL || name[0] == '\0')
        return -1;
    if (level < 1 || level > RANK_LEVELS)
        return -1;
    if (seconds < 0)           /* 允许不足一秒的通关用时为 0，退出由返回值区分 */
        return -1;

    if (!g_loaded)                                            /* 第一次用到才读文件 */
        loadRanks();

    if (g_readError) {
        return -1;
    }
    k = level - 1;              /* 接口是 1/2/3，数组下标才减 1 */

    // 找同名玩家
    dup = -1;                                                 /* -1 表示榜上还没有这个名字 */
    for (i = 0; i < g_count[k]; i++) {
        if (strcmp(g_rank[k][i].name, name) == 0) {
            dup = i;
            break;
        }
    }

    /* 同一玩家只有用时更短才更新成绩 */
    if (dup >= 0 && seconds >= g_rank[k][dup].seconds)
        return 0;

    /* ---- 3. 找插入位置 ----
       第一个"用时比自己大"的位置就是该插入的地方
       用严格大于，所以用时相同的后来者会排在后面            */
    pos = 0;
    while (pos < g_count[k] && g_rank[k][pos].seconds <= seconds)
        pos++;

    /* 榜已满，且比第五名还慢（或一样慢）：进不了榜 */
    if (pos >= RANK_TOP)
        return 0;

    // 若同名玩家已有旧记录，先删掉，避免同一个人占两行
    if (dup >= 0) {
        for (i = dup; i < g_count[k] - 1; i++)                /* 后面的往前盖一格，等于删掉 dup */
            g_rank[k][i] = g_rank[k][i + 1];
        g_count[k]--;                                         /* 删掉一条，条数减一 */
    }

    // 插入新记录：从后往前挪出空位
    cnt = g_count[k];                                         /* 当前条数 */
    if (cnt < RANK_TOP)                                       /* 没满就多一条；满了让最后一名被顶掉 */
        cnt++;

    for (i = cnt - 1; i > pos; i--)                           /* 从末尾往前挪，空出 pos 这一格 */
        g_rank[k][i] = g_rank[k][i - 1];

    strncpy(g_rank[k][pos].name, name, RANK_NAME - 1);         /* 最多复制 31 字节 */
    g_rank[k][pos].name[RANK_NAME - 1] = '\0';                /* 手动补结束符 */
    g_rank[k][pos].seconds = seconds;                         /* 新成绩就位 */
    g_count[k] = cnt;                                         /* 更新条数 */

    // 写回文件
    if (!writeRanks())                                        /* 文件写入失败就报告错误 */
        return -1;

    return 1;
}


/*
 * 功能：检查三个难度中是否有成绩
 * 输入：无参数
 * 输出：首次调用时读取排行榜文件
 * 返回值：有成绩返回 1，全为空返回 0，读取失败返回 -1
 */
static int hasRankData(void)
{
    int k;
    if (!g_loaded) {
        loadRanks();
    }
    if (g_readError) {
        return -1;
    }
    for (k = 0; k < RANK_LEVELS; k++) {
        if (g_count[k] > 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * 功能：显示指定难度的前五名
 * 输入：level 是难度 1、2、3
 * 输出：打印名次、玩家名称和用时；没有成绩或读取失败时显示提示
 * 返回值：无
 */
static void showRank(int level)
{
    int k, i;

    if (level < 1 || level > RANK_LEVELS) {
        printf("难度参数错误（只能是 1、2、3）。\n");
        return;
    }

    if (!g_loaded)                                            /* 第一次用到才读文件 */
        loadRanks();

    if (g_readError) {
        printf("成绩文件读取失败。\n");
        return;
    }
    k = level - 1;

    printf("\n===== %s 排行榜（前五名）=====\n", levelName(level));

    if (g_count[k] == 0) {
        printf("  暂无成绩。\n");
        return;
    }

    printf("  名次   姓名                 用时\n");
    printf("  --------------------------------\n");
    for (i = 0; i < g_count[k]; i++) {
        printf("  %2d    %-16s %4d 秒\n",                      /* %-16s 左对齐；中文按字节补空格，会略偏 */
               i + 1, g_rank[k][i].name, g_rank[k][i].seconds);
    }
}


/*
 * 功能：选择并查看排行榜
 * 输入：无参数，有成绩时通过键盘选择难度
 * 输出：显示榜单或空榜提示后停留 3 秒；选择返回或读取失败时直接返回
 * 返回值：无
 */
void doShowRank(void)
{
    int status = hasRankData();
    int level;
    if (status == -1) {
        printf("成绩文件读取失败，暂时无法查看排行榜。\n");
        return;
    }
    if (status == 0) {
        printf("暂无排行榜成绩，请先完成一局游戏。\n");
    } else {
        printf("\n---------- 选择排行榜 ----------\n");
        printf("1. 初级排行榜\n2. 中级排行榜\n3. 高级排行榜\n0. 返回玩家菜单\n");
        do {
            printf("请选择要查看的排行榜：");
            if (scanf("%d", &level) != 1) {
                exit(0);
            }
        } while (level < 0 || level > 3);
        if (level == 0) {
            return;
        }
        showRank(level);
    }
    Sleep(3000);
}

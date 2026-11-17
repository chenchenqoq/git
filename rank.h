#ifndef RANK_H
#define RANK_H

/*
 * 功能：提交玩家在指定难度的通关用时
 * 输入：name 为玩家名称；level 为难度 1、2、3；seconds 为非负秒数
 * 输出：更新排行榜，保留每个难度前五名及同一玩家的最好成绩，并写入 ranks.txt
 * 返回值：1 表示更新并保存成功；0 表示未刷新成绩或未进入前五；-1 表示参数或文件读写错误
 */
int saveScore(const char name[], int level, int seconds);

// 选择并显示排行榜，显示后停留 3 秒；无参数、无返回值
void doShowRank(void);

#endif

#include "Gobang.h"
#include "AIUtil.h"
#define JUDGE_EDGE(y, x) \
	((y < 0 || y > 18 || x < 0 || x > 18) ? 0 : 1)

/*
@author 应禹尧
*/
AIUtil::AIUtil()
{
}

/*
@author 应禹尧
*/
AIUtil::~AIUtil()
{
}

/*
初始化键值

@author 应禹尧
*/
void AIUtil::init_zobrist() {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 19; j++) {
			for (int k = 0; k < 19; k++) {
				zobrist[i][j][k] = rand14();
			}
		}
	}
}

LL AIUtil::rand14()
{
	double val = rand() / (RAND_MAX + 1.0);
	return (LL)(val * 10e18);
}

/*
初始化哈希值

@author 应禹尧
*/
void AIUtil::init_hashtable()
{
	for (int i = 0; i < HASHSIZE; i++)
		hashtable[i].val = unknow;
}

/*
计算键值

@author 应禹尧
*/
LL AIUtil::cal_zobrist()
{
	LL z = 0;
	int i, j;
	for (i = 0; i < 19; i++) {
		for (j = 0; j < 19; j++) {
			if (state[i][j] == AIChessType::AINOCHESS)
				z ^= zobrist[2][i][j];
			else if (state[i][j] == AIChessType::AIBLACKCHESS)
				z ^= zobrist[0][i][j];
			else if (state[i][j] == AIChessType::AIWHITECHESS)
				z ^= zobrist[1][i][j];
		}
	}
	return z;
}

/*
计算每个空位价值

@author 应禹尧
*/
void AIUtil::copy_and_cal_points()
{
	int i, j;

	/* cpoint[i][j][0] 表示电脑绝杀棋情况 */
	/* cpoint[i][j][1] 表示玩家绝杀棋情况 */
	/* cpoint[i][j][2] 表示电脑得分		  */
	/* cpoint[i][j][3] 表示玩家得分		  */
	memset(cpoint, 0, sizeof(cpoint));

	for (i = 0; i < 19; i++) {
		for (j = 0; j < 19; j++) {
			if (state[i][j] == AIChessType::AINOCHESS) {
				cal_point(i, j);								// 计算每个可落子点的分数
			}
		}
	}
}

int AIUtil::getX()
{
	return comx;
}

int AIUtil::getY()
{
	return comy;
}

/*
alpha_beta剪枝

@para AIType---棋子类型，depth---搜索深度，alpha，beta---极大极小值
@author 应禹尧
*/
int AIUtil::alpha_beta(int AIType, int depth, int alpha, int beta, LL st)
{
	AIStep step[2];									// 落子信息
	int kill[2];									// 绝杀
	int value;										// 价值
	HashType hashf = HASHALPHA;

	step[0].player = AIChessType::AIBLACKCHESS;		// 黑子
	step[1].player = AIChessType::AIWHITECHESS;		// 白子

	int s1 = cal_all_points(&step[0], &kill[0]);	// 黑子总价值
	int s2 = cal_all_points(&step[1], &kill[1]);	// 白子总价值

	if ((value = find_in_hash(depth, alpha, beta, st)) != unknow)
		return value;

	if (depth == 0) {										// 达到搜索深度，返回价值 
		int s = s1 - s2;
		record_hash(depth, s, st, HASHEXACT);				// 记录哈希值
		if (((!AIType) && s >= 0) || (AIType && s < 0))		// 如果玩家占优则返回正值，否则返回负值
			return abs(s);
		if (((!AIType) && s < 0) || (AIType && s >= 0))
			return -abs(s);
	}

	int y, x;

	// 己方可一步成五 || (对方不能一步成五 && 己方可一步成绝杀棋) ||  己方可一步成双活三 && 对方不能一步成绝杀棋
	int self = AIType;
	int opp = (AIType + 1) % 2;
	if (kill[self] == 3 || (kill[opp] < 3 && kill[self] == 2) || (kill[self] == 1 && kill[opp] < 2)) {
		if (depth == DEPTH) {			// 可一步达成
			comy = step[self].y;		// 得到该点信息
			comx = step[self].x;
		}
		alpha = CHENG5;

		return alpha;
	}

	Subpoints sp[19 * 19];
	LL tst;
	int n = set_order(sp);									// 对候选点按高分到低分排序 


	for (int i = 0; i < 10 && i < n; i++) {					// 最多选10 个候选点 						
		tst = st;
		x = sp[i].x;
		y = sp[i].y;
		state[y][x] = AIType;												// 模拟落子
		st ^= zobrist[AIType][y][x];
		change_cpoint(y, x);												// 重新计算四个方向上的得分  
		value = -alpha_beta(AIType ^ 1, depth - 1, -beta, -alpha, st);		// 得到新的价值
		state[y][x] = AIChessType::AINOCHESS;
		st ^= zobrist[AIType][y][x];
		change_cpoint(y, x);												// 还原得分

		if (value > alpha) {				// 更新alpha
			if (depth == DEPTH) {
				comy = y;
				comx = x;
			}
			tst = st;
			hashf = HASHEXACT;
			alpha = value;
		}

		if (alpha >= beta) {
			record_hash(depth, beta, tst, HASHBETA);
			return beta;
		}
	}
	record_hash(depth, alpha, tst, hashf);

	return alpha;
}

/*
计算所有点价值

@para step---落子信息，kill---绝杀信息
@author 应禹尧
*/
int AIUtil::cal_all_points(AIStep *step, int *kill)
{
	int flag;
	int value = 0;

	*kill = 0;
	if (step->player)
		flag = 3;
	else
		flag = 2;

	for (int i = 0; i < 19; i++) {
		for (int j = 0; j < 19; j++) {
			if (state[i][j] == AIChessType::AINOCHESS) {
				value += cpoint[i][j][flag];
				if (cpoint[i][j][flag - 2] >(*kill)) {
					*kill = cpoint[i][j][flag - 2];
					step->y = i;
					step->x = j;
				}
			}
		}
	}

	return value;
}

/*
统计某个点的分数

@para x---横坐标，y---纵坐标
@author 应禹尧
*/
void AIUtil::cal_point(int x, int y)
{
	AIStep tco;
	tco.x = x;
	tco.y = y;

	if (state[x][y] == AIChessType::AINOCHESS) {
		tco.player = 0;
		cpoint[x][y][2] = get_points(&tco, &cpoint[x][y][0]); // 该位置电脑得分 
		tco.player = 1;
		cpoint[x][y][3] = get_points(&tco, &cpoint[x][y][1]); // 该位置玩家得分
	}
}

/*
计算某个点的分数

@para step---落子信息，kill---绝杀情况
@author 应禹尧
*/
int AIUtil::get_points(AIStep *step, int *kill) {
	Points po;
	AIStep tco;
	int value = 0;					// 得分
	int zc4, zl3;					// zc4---总冲四，zl3---总活三

	*kill = 0;
	po.bb5 = po.g5 = po.rc4 = po.l4 = po.l3 = po.rl3 = po.l2 = po.rl2 = po.c4 = po.m3 = po.d4 = po.d3 = po.d2 = 0;
	tco.player = step->player;
	tco.y = step->y;
	tco.x = step->x;

	cal_chess(&po, &tco, 1, 0);		// 竖直方向
	cal_chess(&po, &tco, 0, 1);		// 水平方向
	cal_chess(&po, &tco, 1, 1);		// 主对角线方向
	cal_chess(&po, &tco, -1, 1);	// 副对角线方向

	zc4 = po.c4 + po.rc4;			// 总冲四数量
	zl3 = po.l3 + po.rl3;			// 总活三数量

	if (po.g5 >= 1) {										// 成五 
		*kill = 3;
		value = CHENG5;
		return value;
	}
	else if (po.bb5 >= 1) {
		if (tco.player == AIChessType::AIBLACKCHESS) {		// 黑棋长连禁手
			value = BAN;
			return value;
		}
		else {												// 白棋成五
			*kill = 3;
			value = CHENG5;
			return value;
		}
	}
	if (tco.player == AIChessType::AIBLACKCHESS) {			// 三三禁手、四四禁手
		if (zl3 >= 2 || (po.l4 + zc4 >= 2)) {
			value = BAN;
			return value;
		}
	}
	if (po.l4 >= 1 || zc4 >= 2 || (zc4 && zl3)) {			// 绝杀 
		*kill = 2;
		value = LIVE4;
	}
	else {
		if (zl3 >= 2) {										// 双活三
			*kill = 1;
		}
		value = po.l3*LIVE3 + po.rl3*RLIVE3 + po.l2*LIVE2 + po.rl2*RLIVE2 + po.c4*CHONG4 + po.rc4*RCHONG4 + po.m3*MIAN3 + po.d4*DEAD4 + po.d3*DEAD3 + po.d2*DEAD2;
	}

	return value;
}


/*
判断某一方向棋型

@para m---水平查找，n---竖直查找
@author 应禹尧
*/
void AIUtil::cal_chess(Points *po, AIStep *steps, int m, int n)
{
	/*
	lchess[0] 表示左边相连的连续同类棋数目；
	lempty[0] 表示左边第一个空点数起的连续空点数目
	lchess[1] 表示左边的且与至少隔一个空点的连续同类棋数目
	lempty[1] 表示在 lchess[1] 个同类棋左边的连续空点数目

	rchess，rempty同理
	同理设下为左，上为右
	*/
	int lchess[2], rchess[2];
	int lempty[2], rempty[2];
	int i, j;											// i---x坐标，j---y坐标
	int player = steps->player;							// 棋子类别
	int chessNum = 0;									//连续同色棋子数

	lchess[0] = lchess[1] = lempty[0] = lempty[1] = 0;
	rchess[0] = rchess[1] = rempty[0] = rempty[1] = 0;

	i = steps->x - m;
	j = steps->y - n;
	while (JUDGE_EDGE(i, j) && state[i][j] == player) {
		lchess[0]++;
		i -= m;
		j -= n;
	}
	while (JUDGE_EDGE(i, j) && state[i][j] == AIChessType::AINOCHESS) {
		lempty[0]++;
		i -= m;
		j -= n;
	}
	while (JUDGE_EDGE(i, j) && state[i][j] == player) {
		lchess[1]++;
		i -= m;
		j -= n;
	}
	while (JUDGE_EDGE(i, j) && state[i][j] == AIChessType::AINOCHESS) {
		lempty[1]++;
		i -= m;
		j -= n;
	}

	i = steps->x + m;
	j = steps->y + n;
	while (JUDGE_EDGE(i, j) && state[i][j] == player) {
		rchess[0]++;
		i += m;
		j += n;
	}
	while (JUDGE_EDGE(i, j) && state[i][j] == AIChessType::AINOCHESS) {
		rempty[0]++;
		i += m;
		j += n;
	}
	while (JUDGE_EDGE(i, j) && state[i][j] == player) {
		rchess[1]++;
		i += m;
		j += n;
	}
	while (JUDGE_EDGE(i, j) && state[i][j] == AIChessType::AINOCHESS) {
		rempty[1]++;
		i += m;
		j += n;
	}

	chessNum = lchess[0] + rchess[0] + 1;			// 连续同色棋子数目

	if (chessNum > 5)								// 大于五个同色棋子
		po->bb5++;
	else if (chessNum == 5)							// 五个同色棋子
		po->g5++;
	else if (chessNum == 4) {
		if (lempty[0] >= 1 && rempty[0] >= 1)		// 活四
			po->l4++;
		else if (lempty[0] + rempty[0])				// 冲四
			po->c4++;
		else										// 死四
			po->d4++;
	}
	else if (chessNum == 3) {
		int ok = 0;
		if ((lempty[0] == 1 && lchess[1] >= 1) || (rempty[0] == 1 && rchess[1] >= 1)) {
			po->rc4++;								// 跳冲四
			ok = 1;
		}
		if (!ok && lempty[0] + rempty[0] >= 3 && lempty[0] >= 1 && rempty[0] >= 1) {
			po->l3++;								// 活三
			ok = 1;
		}
		else if (lempty[0] + rempty[0])				// 眠三
			po->m3++;
		else										// 死三
			po->d3++;
	}
	else if (chessNum == 2) {
		int ok = 0;
		if ((lempty[0] == 1 && lchess[1] >= 2) || (rempty[0] == 1 && rchess[1] >= 2)) {
			po->rc4++;								// 跳冲四
			ok = 1;
		}
		if (!ok && ((lempty[0] == 1 && lchess[1] == 1 && rempty[0] >= 1 && lempty[1] >= 1) || (rempty[0] == 1 && rchess[1] == 1 && lempty[0] >= 1 && rempty[1] >= 1))) {
			po->rl3++;								// 跳活三
		}
		else if ((lempty[0] == 1 && lchess[1] == 1 && rempty[0] + lempty[1] >= 1) || (rempty[0] == 1 && rchess[1] == 1 && lempty[0] + rempty[1] >= 1))
			po->m3++;								// 眠三
		if (lempty[0] + rempty[0] >= 4 && lempty[0] >= 1 && rempty[0] >= 1)
			po->l2++;								// 活二
		else if (lempty[0] + rempty[0] == 0)
			po->d2++;								// 死二
	}
	else if (chessNum == 1) {
		int ok = 0;
		if ((lempty[0] == 1 && lchess[1] >= 3) || (rempty[0] == 1 && rchess[1] >= 3)) {
			po->rc4++;								// 跳冲四
			ok = 1;
		}
		if (!ok && ((lempty[0] == 1 && lchess[1] == 2 && rempty[0] >= 1 && lempty[1] >= 1) || (rempty[0] == 1 && rchess[1] == 2 && lempty[0] >= 1 && rempty[1] >= 1))) {
			po->rl3++;								// 跳活三
		}
		else if ((lempty[0] == 1 && lchess[1] == 2 && rempty[0] + lempty[1] >= 1) || (rempty[0] == 1 && rchess[1] == 2 && lempty[0] + rempty[1] >= 1))
			po->m3++;								// 眠三
		if ((lempty[0] == 1 && lchess[1] == 1 && rempty[0] + lempty[1] >= 3 && rempty[0] >= 1 && lempty[1] >= 1)
			|| (rempty[0] == 1 && rchess[1] == 1 && rempty[1] + lempty[0] >= 3 && lempty[0] >= 1 && rempty[1] >= 1)) {
			po->rl2++;								// 跳活二
		}
		if ((lempty[0] == 2 && lchess[1] == 1 && rempty[0] >= 1 && lempty[1] >= 1) || (rempty[0] == 2 && rchess[1] == 1 && lempty[0] >= 1 && rempty[1] >= 1)) {
			po->rl2++;								// 跳活二
		}
	}
}

/*
计算哈希值

@para depth---深度
@author 应禹尧
*/
int AIUtil::find_in_hash(int depth, int alpha, int beta, LL st)
{
	int p = (st&(HASHSIZE - 1));
	int val = hashtable[p].val;

	if (val == unknow)
		return val;
	if (hashtable[p].check == st) {
		if (hashtable[p].depth >= depth) {
			if (hashtable[p].HashType == HASHEXACT) {
				return val;
			}
			if (hashtable[p].HashType == HASHALPHA && val <= alpha) {
				return alpha;
			}
			if (hashtable[p].HashType == HASHBETA && val >= beta) {
				return beta;
			}
		}
	}

	return unknow;
}

/*
记录哈希值

@para depth---深度，
@author 应禹尧
*/
void AIUtil::record_hash(int depth, int val, LL st, HashType AIType)
{
	int p = (st&(HASHSIZE - 1));

	hashtable[p].check = st;
	hashtable[p].val = val;
	hashtable[p].depth = depth;
	hashtable[p].HashType = AIType;
}

/*
比较

@para
@author 应禹尧
*/
int compare(const void* _a, const void* _b)
{
	Subpoints *a = (Subpoints *)_a;
	Subpoints *b = (Subpoints *)_b;

	return ((b->point[0] + b->point[1]) - (a->point[0] + a->point[1]));
}

/*
设置顺序

@para
@author 应禹尧
*/
int AIUtil::set_order(Subpoints *od)
{
	int n = 0;

	for (int i = 0; i < 19; i++) {
		for (int j = 0; j < 19; j++) {
			if (state[i][j] == AIChessType::AINOCHESS) {
				od[n].y = i;
				od[n].x = j;
				od[n].point[0] = cpoint[i][j][2];
				od[n].point[1] = cpoint[i][j][3];
				od[n].kill[0] = cpoint[i][j][0];
				od[n].kill[1] = cpoint[i][j][1];
				n++;
			}
		}
	}

	qsort(od, n, sizeof(Subpoints), compare);

	return n;
}



/*
改变该点4个方向下可落子点的分数

@author 应禹尧
*/
void AIUtil::change_cpoint(int y, int x)
{
	int i, j;

	// 竖直，水平判断
	for (i = 0; i < 19; i++) {
		if (state[y][i] == AIChessType::AINOCHESS) {
			cal_point(y, i);
		}
		if (state[i][x] == AIChessType::AINOCHESS) {
			cal_point(i, x);
		}
	}

	// 对角线判断
	for (i = 0; i < 19; i++) {
		j = i - (y - x);
		if (0 < j && j < 19 && state[i][j] == AIChessType::AINOCHESS) {
			cal_point(i, j);
		}
		j = (y + x) - i;
		if (0 < j && j < 19 && state[i][j] == AIChessType::AINOCHESS) {
			cal_point(i, j);
		}
	}
}

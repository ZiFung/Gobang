#include "AIUtil.h"
#define JUDGE_EDGE(y, x) \
	((y < 0 || y > 18 || x < 0 || x > 18) ? 0 : 1)

/*
	@author 应禹尧
*/
AIUtil::AIUtil(int *board)
{
	this->board = board;
}

/*
	@author 应禹尧
*/
AIUtil::~AIUtil()
{


}

/*
	@author 应禹尧
*/
int AIUtil::getBoard(int x, int y)
{
	return board[x * 19 + y];
}

/*
	初始化键值

	@author 应禹尧
*/
void AIUtil::init_zobrist() {
	int i, j, k;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 19; j++) {
			for (k = 0; k < 19; k++) {
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
	int i;
	//find = 0;
	for (i = 0; i < HASHSIZE; i++)
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
			if (getBoard(i, j) == AIChessType::AINOCHESS)
				z ^= zobrist[2][i][j];
			else if (getBoard(i, j) == AIChessType::AIBLACKCHESS)
				z ^= zobrist[0][i][j];
			else if (getBoard(i, j) == AIChessType::AIWHITECHESS)
				z ^= zobrist[1][i][j];
		}
	}
	return z;
}

int AIUtil::copy_and_cal_points()
{
	int i, j;
	int yes = 0;
	memset(cpoint, 0, sizeof(cpoint));
	for (i = 0; i < 19; i++) {
		for (j = 0; j < 19; j++) {
			if (getBoard(i, j) == AIChessType::AINOCHESS) {
				state[i][j] = AIChessType::AINOCHESS;
			}
			else if (getBoard(i, j) == AIChessType::AIBLACKCHESS) {
				yes++;
				state[i][j] = AIChessType::AIBLACKCHESS;
			}
			else {
				yes++;
				state[i][j] = AIChessType::AIWHITECHESS;
			}
		}
	}
	for (i = 0; i < 19; i++) {
		for (j = 0; j < 19; j++) {
			if (state[i][j] == -1) {
				cal_point(i, j); /* 计算每个可落子点的分数 */
			}
		}
	}
	return yes;
}

/*
	alpha_beta剪枝

	@para type---棋子类型，depth---搜索深度，alpha，beta---极大极小值，st---
	@author 应禹尧
*/
int AIUtil::alpha_beta(int type, int depth, int alpha, int beta, LL st) 
{
	AIStep step[2];
	int kill[2];
	int i;

	step[0].player = 0;
	step[1].player = 1;
	int value;
	HashType hashf = HASHALPHA;
	int s1 = cal_all_points(&step[0], &kill[0]);
	int s2 = cal_all_points(&step[1], &kill[1]);

#if 1
	if ((value = find_in_hash(depth, alpha, beta, st)) != unknow)
		return value;
#endif
	if (depth == 0) { // 达到搜索深度限制，返回估分 
		int s = s1 - s2;
		record_hash(depth, s, st, HASHEXACT);
		if (((!type) && s >= 0) || (type && s < 0)) // 如果 player 占优则返回正值，否则返回负值
			return ABS(s);
		if (((!type) && s < 0) || (type && s >= 0))
			return -ABS(s);
	}

	Subpoints sp[250];
	LL tst;
	int n = set_order(sp, type); // 对候选点按高分到低分排序 
	int y, x;

#if 1
	// 己方可一步成五 || (对方不能一步成五 && 己方可一步成绝杀棋) ||  己方可一步成双活三而对方不能一步成绝杀棋
	int self = type;
	int opp = type ^ 1;
	if (kill[self] == 3 || (kill[opp] < 3 && kill[self] == 2) || (kill[self] == 1 && kill[opp] < 2)) {
		if (depth == DEPTH) {
			comy = step[self].y;
			comx = step[self].x;
		}
		alpha = CHENG5;

		return alpha;
	}
#endif

	for (i = 0; i < 10 && i < n; i++) { // 最多选择 20 个候选点 
		tst = st;
		y = sp[i].y;
		x = sp[i].x;
		state[y][x] = type; // 在 (y, x) 落子
		st ^= zobrist[type][y][x];
		change_cpoint(y, x); // (y, x) 四个方向上的得分受到影响，需要改变  
		value = -alpha_beta(type ^ 1, depth - 1, -beta, -alpha, st);
		state[y][x] = -1;
		st ^= zobrist[type][y][x];
		change_cpoint(y, x);

		if (value > alpha) {
			if (depth == DEPTH) {
				comy = y;
				comx = x;
			}
			tst = st;
			hashf = HASHEXACT;
			alpha = value;
		}

		if (alpha >= beta) { // 千万不能把等号去掉！！！ 
			record_hash(depth, beta, tst, HASHBETA);
			return beta;
		}
	}
	record_hash(depth, alpha, tst, hashf);

	return alpha;
}

/*
	计算所有点价值

	@para step---落子信息，kill---
	@author 应禹尧
*/
int AIUtil::cal_all_points(AIStep *step, int *kill) 
{
	int i, j, flag;
	int value = 0;

	*kill = 0;
	if (step->player)
		flag = 3;
	else
		flag = 2;

	for (i = 0; i < 19; i++) {
		for (j = 0; j < 19; j++) {
			if (getBoard(i, j) == AIChessType::AINOCHESS) {
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
	
	if (getBoard(y, x) == AIChessType::AINOCHESS) {
		tco.player = 0;
		cpoint[y][x][2] = get_points(&tco, &cpoint[y][x][0]); // 攻击力 
		tco.player = 1;
		cpoint[y][x][3] = get_points(&tco, &cpoint[y][x][1]); // 防御力 
	}
}

/*
	计算某个点的分数

	@para step---落子信息，kill---
	@author 应禹尧
*/
int AIUtil::get_points(AIStep *step, int *kill) {
	Points po;
	AIStep tco;
	int player = step->player;
	int y = step->y;
	int x = step->x;
	int value = 0;
	int zc4, zl3;					// nc4---总冲四，nl3---总活三

	*kill = 0;
	po.g5 = po.rc4 = po.l4 = po.l3 = po.rl3 = po.l2 = po.rl2 = po.c4 = po.m3 = po.d4 = po.d3 = po.d2 = 0;
	tco.player = player;
	tco.y = y;
	tco.x = x;

	cal_chess(&po, &tco, 1, 0);
	cal_chess(&po, &tco, 0, 1);
	cal_chess(&po, &tco, 1, 1);
	cal_chess(&po, &tco, -1, 1);

	zc4 = po.c4 + po.rc4;
	zl3 = po.l3 + po.rl3;
	if (po.g5 >= 1) {										// 成五 
		*kill = 3;
		value = CHENG5;
	}
	else if (po.l4 >= 1 || zc4 >= 2 || (zc4 && zl3)) {		// 绝杀 
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

	@para m---水平查找，n---竖直查找，isRestricted---是否禁手
	@author 应禹尧
*/
void AIUtil::cal_chess(Points *po, AIStep *steps, int m, int n)
{
	/*  lchess[0] 表示(y, x) 左边的且与(y, x) 相连的连续同类棋数目；
	lempty[0] 表示(y, x) 左边第一个空点数起的连续空点数目
	lchess[1] 表示(y, x) 左边的且与(y, x) 至少隔一个空点的连续同类棋数目
	lempty[1] 表示在 lchess[1] 个同类棋左边的连续空点数目

	rchess，rempty同理
	同理设下为左，上为右
	*/
	int lchess[2], rchess[2];
	int lempty[2], rempty[2];
	int i, j;											// i---x坐标，j---y坐标
	int player = steps->player;
	int chessNum = 0;

	lchess[0] = lchess[1] = lempty[0] = lempty[1] = 0;
	rchess[0] = rchess[1] = rempty[0] = rempty[1] = 0;

	i = steps->x - m;
	j = steps->y - n;
	while (JUDGE_EDGE(i, j) && getBoard(i, j) == player) {
		lchess[0]++;
		i -= m;
		j -= n;
	}
	while (JUDGE_EDGE(i, j) && getBoard(i, j) == AIChessType::AINOCHESS) {
		lempty[0]++;
		i -= m;
		j -= n;
	}
	while (JUDGE_EDGE(i, j) && getBoard(i, j) == player) {
		lchess[1]++;
		i -= m;
		j -= n;
	}
	while (JUDGE_EDGE(i, j) && getBoard(i, j) == AIChessType::AINOCHESS) {
		lempty[1]++;
		i -= m;
		j -= n;
	}

	i = steps->x + m;
	j = steps->y + n;
	while (JUDGE_EDGE(i, j) && getBoard(i, j) == player) {
		rchess[0]++;
		i += m;
		j += n;
	}
	while (JUDGE_EDGE(i, j) && getBoard(i, j) == AIChessType::AINOCHESS) {
		rempty[0]++;
		i += m;
		j += n;
	}
	while (JUDGE_EDGE(i, j) && getBoard(i, j) == player) {
		rchess[1]++;
		i += m;
		j += n;
	}
	while (JUDGE_EDGE(i, j) && getBoard(i, j) == AIChessType::AINOCHESS) {
		rempty[1]++;
		i += m;
		j += n;
	}

	chessNum = lchess[0] + rchess[0] + 1;

	if (chessNum >= 5)
		po->g5++;
	else if (chessNum == 4) {
		if (lempty[0] >= 1 && rempty[0] >= 1)
			po->l4++; 
		else if (lempty[0] + rempty[0])
			po->c4++;
		else
			po->d4++;
	}
	else if (chessNum == 3) {
		int ok = 0;        // 同一个方向上如果可形成活三和冲四，舍弃活三  
		if ((lempty[0] == 1 && lchess[1] >= 1) || (rempty[0] == 1 && rchess[1] >= 1)) {
			po->rc4++;
			ok = 1;
		}
		if (!ok && lempty[0] + rempty[0] >= 3 && lempty[0] >= 1 && rempty[0] >= 1) {
			po->l3++;
			ok = 1;
		}
		else if (lempty[0] + rempty[0])
			po->m3++;
		else
			po->d3++;
	}
	else if (chessNum == 2) {
		int ok = 0;
		if ((lempty[0] == 1 && lchess[1] >= 2) || (rempty[0] == 1 && rchess[1] >= 2)) {
			po->rc4++;
			ok = 1;
		}
		if (!ok && ((lempty[0] == 1 && lchess[1] == 1 && rempty[0] >= 1 && lempty[1] >= 1) || (rempty[0] == 1 && rchess[1] == 1 && lempty[0] >= 1 && rempty[1] >= 1))) {
			po->rl3++;
		}
		else if ((lempty[0] == 1 && lchess[1] == 1 && rempty[0] + lempty[1] >= 1) || (rempty[0] == 1 && rchess[1] == 1 && lempty[0] + rempty[1] >= 1))
			po->m3++;
		if (lempty[0] + rempty[0] >= 4 && lempty[0] >= 1 && rempty[0] >= 1)
			po->l2++;
		else if (lempty[0] + rempty[0] == 0)
			po->d2++;
	}
	else if (chessNum == 1) {
		int ok = 0;
		if ((lempty[0] == 1 && lchess[1] >= 3) || (rempty[0] == 1 && rchess[1] >= 3)) {
			po->rc4++;
			ok = 1;
		}
		if (!ok && ((lempty[0] == 1 && lchess[1] == 2 && rempty[0] >= 1 && lempty[1] >= 1) || (rempty[0] == 1 && rchess[1] == 2 && lempty[0] >= 1 && rempty[1] >= 1))) {
			po->rl3++;
		}
		else if ((lempty[0] == 1 && lchess[1] == 2 && rempty[0] + lempty[1] >= 1) || (rempty[0] == 1 && rchess[1] == 2 && lempty[0] + rempty[1] >= 1))
			po->m3++;
		if ((lempty[0] == 1 && lchess[1] == 1 && rempty[0] + lempty[1] >= 3 && rempty[0] >= 1 && lempty[1] >= 1)
			|| (rempty[0] == 1 && rchess[1] == 1 && rempty[1] + lempty[0] >= 3 && lempty[0] >= 1 && rempty[1] >= 1)) {
			po->rl2++;
		}
		if ((lempty[0] == 2 && lchess[1] == 1 && rempty[0] >= 1 && lempty[1] >= 1) || (rempty[0] == 2 && rchess[1] == 1 && lempty[0] >= 1 && rempty[1] >= 1)) {
			po->rl2++;
		}
	}
}

/*
	计算哈希值

	@para depth---深度，
	@author 应禹尧
*/
int AIUtil::find_in_hash(int depth, int alpha, int beta, LL st)
{
	int p = (st&(HASHSIZE - 1));
	int val = hashtable[p].val;

	if (val == unknow)
		return val;
	if (hashtable[p].check == st) {
		//find++;
		if (hashtable[p].depth >= depth) {
			if (hashtable[p].type == HASHEXACT) {
				return val;
			}
			if (hashtable[p].type == HASHALPHA && val <= alpha) {
				return alpha;
			}
			if (hashtable[p].type == HASHBETA && val >= beta) {
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
void AIUtil::record_hash(int depth, int val, LL st, HashType type)
{
	int p = (st&(HASHSIZE - 1));

	hashtable[p].check = st;
	hashtable[p].val = val;
	hashtable[p].depth = depth;
	hashtable[p].type = type;
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
int AIUtil::set_order(Subpoints *od, int player)
{
	int i, j;
	int n = 0;
	for (i = 0; i < 19; i++) {
		for (j = 0; j < 19; j++) {
			if (getBoard(i, j) == AIChessType::AINOCHESS) {
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
#if 1
	qsort(od, n, sizeof(Subpoints), compare);
#endif
	return n;
}



/*
	改变点 (y, x) 4 个方向下可落子点的分数

	@para
	@author 应禹尧
*/
void AIUtil::change_cpoint(int y, int x)
{
	int i, j;

	for (i = 0; i < 19; i++) {
		if (getBoard(y, i) == AIChessType::AINOCHESS) {
			cal_point(y, i);
		}
		if (getBoard(i, x) == AIChessType::AINOCHESS) {
			cal_point(i, x);
		}
	}
	for (i = 0; i < 19; i++) {
		j = i - (y - x);
		if (0 < j && j < 19 && getBoard(i, j) == AIChessType::AINOCHESS) {
			cal_point(i, j);
		}
		j = (y + x) - i;
		if (0 < j && j < 19 && getBoard(i, j) == AIChessType::AINOCHESS) {
			cal_point(i, j);
		}
	}
}


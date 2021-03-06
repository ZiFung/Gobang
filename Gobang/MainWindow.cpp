#include "MainWindow.h"
#include "ServerDialog.h"
#include "ServerMsgItem.h"
#include <qmessagebox.h>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	// 初始化标题栏
	this->setWindowTitle(QString::fromLocal8Bit("五子棋"));
	this->setWindowIcon(QIcon(":/MainWindow/image/WHITE_64.ico"));
	this->setFixedSize(this->width(), this->height());

	// 初始化按钮
	setHomePageBtnVisable(true);
	setGamePageBtnVisable(false);

	// 初始化棋盘
	blackChess = QPixmap("./image/BLACK.png");
	whiteChess = QPixmap("./image/WHITE.png");
	HLBlackChess = QPixmap("./image/BLACK(highlight).png");
	HLWhiteChess = QPixmap("./image/WHITE(highlight).png");
	for (int i = 0; i < BOARDLENGTH; i++)
		for (int j = 0; j < BOARDLENGTH; j++)
		{
			chess[i][j].setParent(ui.centralWidget);
			chess[i][j].setGeometry(357 + j * 47, 7 + i * 47, 42, 42);
		}
	ui.btn_chessboard->raise();

	connect(&computer, SIGNAL(showInf(int, int, int)), this, SLOT(showInf(int, int, int)));
	connect(&computer, SIGNAL(showWinnerDialog()), this, SLOT(showWinnerDialog()));

	// 读取音乐和音效
	music.setMedia(QUrl::fromLocalFile("./sound/FlowerDance.mp3"));
	soundEff.setMedia(QUrl::fromLocalFile("./sound/b.mp3"));
	music.play();
	isMusicOn = true;

	gobang = Gobang();
	gobang.readRanking();
	gameType = GameType::NONE;
	winner = ChessType::NOCHESS;

	isclicked_online = false;
}

/*
	清空棋盘

	@author 叶志枫
*/
void MainWindow::clearBoard()
{
	// 初始化高亮棋子保存信息
	Gobang::Step temp; temp.x = -1; temp.y = -1;
	highlightStep(temp, -2);
	// 清理棋盘
	std::deque<Gobang::Step> &steps = gobang.getSteps();
	while (!steps.empty())
	{
		chess[steps.front().x][steps.front().y].setPixmap(QPixmap(""));
		steps.pop_front();
	}
	isGiveUp = false;
}

/*
	打包走棋操作

	@author 叶志枫
*/
void MainWindow::walkAStep(Gobang::Step new_step)
{
	gobang.newStep(new_step);
	showStep(new_step, gobang.getTurn());
	highlightStep(new_step, gobang.getTurn());
	playSoundEffects();
	showInf(gobang.getTurn(), new_step.x, new_step.y);
	switch (isRestricted)
	{
	case QMessageBox::Yes:
		winner = gobang.isOverWithRestricted();
		break;
	case QMessageBox::No:
		winner = gobang.isOverWithoutRestricted();
		break;
	default:
		break;
	}
	showWinnerDialog();
	if (winner != ChessType::NOCHESS)
	{
		gameType = GameType::NONE;
		return;
	}
	gobang.shiftTurn();
}

/*
	显示棋子信息

	@author 王开阳
*/
void MainWindow::showInf(int color, int x, int y)
{
	QString s = ui.text_chessinf->toPlainText();
	switch (color)
	{
	case ChessType::BLACKCHESS:
		s += QString::fromLocal8Bit("黑棋------------( ") + QString::number(x) + " , " + QString::number(y) + " )\n";
		break;
	case ChessType::WHITECHESS:
		s += QString::fromLocal8Bit("白棋------------( ") + QString::number(x) + " , " + QString::number(y) + " )\n";
		break;
	default:
		break;
	}
	ui.text_chessinf->setText(s);
	QTextCursor cursor = ui.text_chessinf->textCursor();
	cursor.movePosition(QTextCursor::End);
	ui.text_chessinf->setTextCursor(cursor);
}

/*
	删除棋子信息

	@author 王开阳
*/
void MainWindow::delInf()
{
	QString s = ui.text_chessinf->toPlainText();
	for (int i = s.length() - 2; i > 0; i--)
		if (s[i] == '\n')
		{
			s = s.left(i + 1);
			break;
		}
	ui.text_chessinf->setText(s);
	QTextCursor cursor = ui.text_chessinf->textCursor();
	cursor.movePosition(QTextCursor::End);
	ui.text_chessinf->setTextCursor(cursor);
}

/*
	显示一步棋

	@author 王开阳
	@para step - 下一步落子
	@para type - 下一步落子类型
*/
void MainWindow::showStep(Gobang::Step step, int type)
{
	if (step.x < 0 || step.x > BOARDLENGTH)
		throw "step.x is out of range";
	if (step.y < 0 || step.y > BOARDLENGTH)
		throw "step.y is out of range";
	if (type != ChessType::BLACKCHESS && type != ChessType::WHITECHESS)
		throw "Invalid type";
	switch (type)
	{
	case ChessType::BLACKCHESS:
		chess[step.x][step.y].setPixmap(blackChess);
		break;
	case ChessType::WHITECHESS:
		chess[step.x][step.y].setPixmap(whiteChess);
		break;
	default:
		chess[step.x][step.y].setPixmap(QPixmap(""));
		break;
	}
}

/*
	高亮棋子

	@author 叶志枫
	@para step - 需要高亮的棋子
	@para type - 需要高亮的棋子类型
*/
void MainWindow::highlightStep(Gobang::Step step, int type)
{
	static int x = -1;
	static int y = -1;
	static int last_type = -1;
	if (-2 == type)
	{
		x = -1; y = -1; last_type = -1;
		return;
	}
	if (-1 != x && -1 != y && -1 != last_type)
	{
		if (ChessType::BLACKCHESS == last_type)
			chess[x][y].setPixmap(blackChess);
		else if (ChessType::WHITECHESS == last_type)
			chess[x][y].setPixmap(whiteChess);
		else
			chess[x][y].setPixmap(QPixmap(""));
	}
	if (step.x < 0 || step.x > BOARDLENGTH)
		throw "step.x is out of range";
	if (step.y < 0 || step.y > BOARDLENGTH)
		throw "step.y is out of range";
	if (type != ChessType::BLACKCHESS && type != ChessType::WHITECHESS)
		throw "Invalid type";
	x = step.x;
	y = step.y;
	last_type = type;
	switch (type)
	{
	case ChessType::BLACKCHESS:
		chess[step.x][step.y].setPixmap(HLBlackChess);
		break;
	case ChessType::WHITECHESS:
		chess[step.x][step.y].setPixmap(HLWhiteChess);
		break;
	default:
		chess[step.x][step.y].setPixmap(QPixmap(""));
		break;
	}
}

/*
	高亮棋子群

	@author 叶志枫
	@para step - 需要高亮的棋子群
*/
void MainWindow::highlightSteps(std::deque<Gobang::Step> steps)
{
	Gobang::Step temp = steps.front();
	steps.pop_front();
	QPixmap *pixmap = temp.x == ChessType::BLACKCHESS ? &HLBlackChess : &HLWhiteChess;
	while (steps.size() > 0)
	{
		Gobang::Step temp = steps.back();
		steps.pop_back();
		chess[temp.x][temp.y].setPixmap(*pixmap);
	}
	pixmap = NULL;
}

/*
	显示排行榜

	@author 王开阳
*/
void MainWindow::showRankings()
{
	std::string names = "", steps = "";
	for (auto it = gobang.getRankings().begin(); it != gobang.getRankings().end(); it++)
	{
		names += it->name + " ---" + "\n";
		steps += "--- " + to_string(it->n) + "\n";
	}
	ui.lbl_names->setText(QString::fromStdString(names));
	ui.lbl_steps->setText(QString::fromStdString(steps));
}

/*
	播放落子音效

	@author 王开阳
*/
void MainWindow::playSoundEffects()
{
	soundEff.stop();
	soundEff.play();
}

/*
	设置主页按钮的可见性

	@author 王开阳
*/
void MainWindow::setHomePageBtnVisable(bool isOn)
{
	ui.btn_pve->setVisible(isOn);
	ui.btn_pvp->setVisible(isOn);
	ui.btn_online->setVisible(isOn);
	ui.btn_load->setVisible(isOn);
}

/*
	设置游戏页按钮的可见性

	@author 王开阳
*/
void MainWindow::setGamePageBtnVisable(bool isOn)
{
	ui.btn_restart->setVisible(isOn);
	ui.btn_prompt->setVisible(isOn);
	ui.btn_retract->setVisible(isOn);
	ui.btn_giveUp->setVisible(isOn);
	ui.btn_save->setVisible(isOn);
	ui.btn_return->setVisible(isOn);
}

/*
	从屏幕获取棋子坐标

	@author 叶志枫
	@return Gobang::Step - 棋子坐标
*/
Gobang::Step MainWindow::getStepFromScreen()
{
	QPoint point = QWidget::mapFromGlobal(cursor().pos());
	Gobang::Step * step = new Gobang::Step;
	if (step == NULL)
	{
		qDebug() << "内存溢出";
		exit(1);
	}
	step->y = (point.x() - 357) / 47;
	step->x = (point.y() - 7) / 47;
	return *step;
}

/*
	显示胜方信息

	@para ChessType - 胜方棋子种类
*/
void MainWindow::showWinnerDialog()
{
	int isSave;
	switch (winner)
	{
	case ChessType::BLACKCHESS:
		if (!isGiveUp)
			highlightSteps(gobang.getWinModel());
		isSave = QMessageBox::information(this, QString::fromLocal8Bit("游戏获胜"), QString::fromLocal8Bit("黑棋获胜！\n是否要保存游戏记录？"), QMessageBox::Yes, QMessageBox::No);
		if (isSave == QMessageBox::Yes)
			gobang.addRanking(getName(), gobang.getSteps().size() / 2 + 1);
		disconnect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
		setHomePageBtnVisable(true);
		setGamePageBtnVisable(false);
		break;
	case ChessType::WHITECHESS:
		if (!isGiveUp)
			highlightSteps(gobang.getWinModel());
		isSave = QMessageBox::information(this, QString::fromLocal8Bit("游戏获胜"), QString::fromLocal8Bit("白棋获胜！\n是否要保存游戏记录？"), QMessageBox::Yes, QMessageBox::No);
		if (isSave == QMessageBox::Yes)
			gobang.addRanking(getName(), gobang.getSteps().size() / 2);
		disconnect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
		setHomePageBtnVisable(true);
		setGamePageBtnVisable(false);
		break;
	case ChessType::NOCHESS:
		if (gobang.getSteps().size() == BOARDLENGTH * BOARDLENGTH)
		{
			QMessageBox::information(this, QString::fromLocal8Bit("游戏平局"), QString::fromLocal8Bit("双方平局！"), QMessageBox::NoButton);
			disconnect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
			setHomePageBtnVisable(true);
			setGamePageBtnVisable(false);
		}
		break;
	default:
		break;
	}
}

/*
	接收玩家姓名

	@author 王开阳
*/
std::string MainWindow::getName()
{
	bool ok = FALSE;
	QString name = QInputDialog::getText(this, QString::fromLocal8Bit("五子棋"), QString::fromLocal8Bit("请输入您的昵称："), QLineEdit::Normal, QString::null, &ok);
	if (ok && !name.isEmpty())
		return name.toStdString();
	return "";
}


/*
	人机对战的选择框

	@author 王开阳
*/
SelectDialog::SelectDialog(QWidget *parent)
	: QDialog(parent)
{
	this->setWindowTitle(QString::fromLocal8Bit("联机对战"));
	this->resize(352, 224);
	// 设置窗口居中
	this->move(parent->geometry().center() - this->rect().center());

	QFont font;
	font.setFamily(QString::fromUtf8("\345\276\256\350\275\257\351\233\205\351\273\221"));
	font.setPointSize(12);

	groupBox = new QGroupBox(this);
	groupBox->setGeometry(QRect(20, 30, 141, 131));
	radioButton = new QRadioButton(groupBox);
	radioButton->setGeometry(QRect(40, 30, 61, 21));
	radioButton->setFont(font);
	radioButton->setChecked(true);
	radioButton_2 = new QRadioButton(groupBox);
	radioButton_2->setGeometry(QRect(40, 60, 61, 21));
	radioButton_2->setFont(font);
	radioButton_3 = new QRadioButton(groupBox);
	radioButton_3->setGeometry(QRect(40, 90, 61, 21));
	radioButton_3->setFont(font);
	groupBox->setTitle(QString::fromLocal8Bit("难度选择"));
	radioButton->setText(QString::fromLocal8Bit("简单"));
	radioButton_2->setText(QString::fromLocal8Bit("中等"));
	radioButton_3->setText(QString::fromLocal8Bit("困难"));

	groupBox_2 = new QGroupBox(this);
	groupBox_2->setGeometry(QRect(190, 30, 141, 131));
	checkBox = new QCheckBox(groupBox_2);
	checkBox->setGeometry(QRect(30, 40, 91, 21));
	checkBox->setFont(font);
	checkBox_2 = new QCheckBox(groupBox_2);
	checkBox_2->setGeometry(QRect(30, 80, 91, 21));
	checkBox_2->setFont(font);
	groupBox_2->setTitle(QString::fromLocal8Bit("游戏选择"));
	checkBox->setText(QString::fromLocal8Bit("先手开始"));
	checkBox_2->setText(QString::fromLocal8Bit("禁手游戏"));

	pushButton = new QPushButton(this);
	pushButton->setGeometry(QRect(40, 180, 91, 31));
	pushButton->setFont(font);
	pushButton_2 = new QPushButton(this);
	pushButton_2->setGeometry(QRect(220, 180, 91, 31));
	pushButton_2->setFont(font);
	pushButton->setText(QString::fromLocal8Bit("确认"));
	pushButton_2->setText(QString::fromLocal8Bit("取消"));

	// 连接信号槽
	connect(pushButton, SIGNAL(clicked()), this, SLOT(okBtnClicked()));
	connect(pushButton_2, SIGNAL(clicked()), this, SLOT(cancelBtnClicked()));
}

void SelectDialog::setMainWindow(MainWindow *mainWindow)
{
	this->mainWindow = mainWindow;
}

/*
棋盘被点击响应事件

@author 王开阳
*/
void SelectDialog::okBtnClicked()
{
	int isFir, isRes;
	if (radioButton->isChecked())
		mainWindow->getGobang().setDifficulty(1);
	else if (radioButton_2->isChecked())
		mainWindow->getGobang().setDifficulty(2);
	else if (radioButton_3->isChecked())
		mainWindow->getGobang().setDifficulty(3);
	isFir = checkBox->isChecked();
	if (checkBox_2->isChecked())
		isRes = QMessageBox::Yes;
	else
		isRes = QMessageBox::No;
	mainWindow->setVariable(isFir, isRes, true);
	this->close();
}

void SelectDialog::cancelBtnClicked()
{
	this->close();
}

void MainWindow::setVariable(int isFir, int isRes, bool ok)
{
	isFirstHand = isFir;
	isRestricted = isRes;
	okClicked = ok;
}

/*
	按钮被点击响应事件

	@author 王开阳
*/
void MainWindow::btnsClicked()
{
	QString btnName = sender()->objectName();

	if (btnName == "btn_ranking")
	{
		ui.lbl_ranking->raise();
		ui.lbl_names->raise();
		ui.lbl_steps->raise();
		ui.btn_close->raise();
		showRankings();
	}
	else if (btnName == "btn_team")
	{
		ui.lbl_team->raise();
		ui.btn_close->raise();
	}
	else if (btnName == "btn_rules")
	{
		ui.lbl_rules->raise();
		ui.btn_close->raise();
	}
	else if (btnName == "btn_music")
		if (isMusicOn)
		{
			ui.btn_music->setStyleSheet("QPushButton{border-image: url(:/MainWindow/image/music(close).png);}"
				"QPushButton:hover{border-image: url(:/MainWindow/image/music.png);}");
			music.pause();
			isMusicOn = false;
		}
		else
		{
			ui.btn_music->setStyleSheet("QPushButton{border-image: url(:/MainWindow/image/music.png);}"
				"QPushButton:hover{border-image: url(:/MainWindow/image/music(close).png);}");
			music.play();
			isMusicOn = true;
		}
	else if (btnName == "btn_close")
	{
		ui.lbl_ranking->lower();
		ui.lbl_team->lower();
		ui.lbl_rules->lower();
		ui.lbl_names->lower();
		ui.lbl_steps->lower();
		ui.btn_close->lower();
	}
	else if (btnName == "btn_exit")
	{
		gobang.writeRanking();
		exit(0);
	}
}

/*
	人机对战按钮被点击响应事件

	@author - 王开阳 叶志枫
*/
void MainWindow::pveBtnClicked()
{
	clearBoard();
	gobang.initBoard();
	okClicked = false;
	SelectDialog dialog = new SelectDialog(this);
	dialog.setMainWindow(this);
	dialog.exec();
	if (!okClicked)
		return;
	computerColor = ChessType::WHITECHESS;
	if (!isFirstHand)
		computerColor = ChessType::BLACKCHESS;
	connect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
	setHomePageBtnVisable(false);
	setGamePageBtnVisable(true);
	gameType = GameType::PVE;
	ui.text_chessinf->setText("");
	computer.Start(this, computerColor);
}

/*
	人人对战响应事件

	@author - 王开阳 叶志枫
*/
void MainWindow::pvpBtnClicked()
{
	clearBoard();
	gobang.initBoard();
	isRestricted = QMessageBox::question(this, QString::fromLocal8Bit("五子棋"), QString::fromLocal8Bit("是否要带禁手开始游戏？"), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
	if (isRestricted == QMessageBox::Cancel)
		return;
	connect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
	setHomePageBtnVisable(false);
	setGamePageBtnVisable(true);
	gameType = GameType::PVP;
	ui.text_chessinf->setText("");
}

/*
	按钮被点击响应事件

	@author - 王开阳 叶志枫
*/
void MainWindow::onlineBtnClicked()
{
	// 创建并显示连接窗口
	ServerDialog serverDialog = new ServerDialog(this);
	serverDialog.setMainWindow(this);
	serverDialog.exec();

	// 用户点击取消或关闭按钮则返回
	if (!serverDialog.isOKClicked())
		return;

	clearBoard();
	gobang.initBoard();
	connect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
	setHomePageBtnVisable(false);
	setGamePageBtnVisable(true);
	gameType = GameType::ONLINE;
	ui.text_chessinf->setText("");
	isRestricted = QMessageBox::Yes;
}

/*
	按钮被点击响应事件

	@author - 王开阳 叶志枫
*/
void MainWindow::loadBtnClicked()
{
	std::string file = selectFile();
	try
	{
		if (file != "")
		{
			clearBoard();
			gobang.initBoard();
			gobang.loadBoard(const_cast<char*>(file.c_str()));
		}
		else
			return;
	}
	catch (const char* msg)
	{
		qDebug() << msg;
	}

	QString s = "";
	int size = gobang.getSteps().size();
	auto iterator = gobang.getSteps().begin();
	for (int i = 0; i < size; i++)
		switch (i % 2)
		{
		case 0:
			chess[iterator->x][iterator->y].setPixmap(blackChess);
			s += QString::fromLocal8Bit("黑棋------------( ") + QString::number(iterator->x) + " , " + QString::number(iterator->y) + " )\n";
			iterator++;
			break;
		case 1:
			chess[iterator->x][iterator->y].setPixmap(whiteChess);
			s += QString::fromLocal8Bit("白棋------------( ") + QString::number(iterator->x) + " , " + QString::number(iterator->y) + " )\n";
			iterator++;
			break;
		default:
			break;
		}
	connect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
	setHomePageBtnVisable(false);
	setGamePageBtnVisable(true);
	isRestricted = QMessageBox::Yes;
	gameType = GameType::PVP;

	ui.text_chessinf->setText(s);
	QTextCursor cursor = ui.text_chessinf->textCursor();
	cursor.movePosition(QTextCursor::End);
	ui.text_chessinf->setTextCursor(cursor);
}

/*
	重新开始按钮被点击响应事件

	@author - 王开阳 叶志枫
*/
void MainWindow::restartBtnClicked()
{
	if (isclicked_online)
	{
		QMessageBox::about(NULL, "Tip", QString::fromLocal8Bit("请勿重复点击"));
		return;
	}
	if (gameType == GameType::ONLINE)
	{
		isclicked_online = true;
		s->msg_send(0, 0, OperationType::RESTART);
		return;
	}
	// 清理棋盘
	clearBoard();
	gobang.initBoard();
	connect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
	ui.text_chessinf->setText("");
}

/*
	提示按钮被点击响应事件

	@author - 叶志枫
*/
void MainWindow::promptBtnClicked()
{
	try
	{
		Gobang::Step new_step;
		switch (gameType)
		{
		case GameType::PVE:
			if (gobang.getTurn() == computerColor)
				return;
			new_step = gobang.AIWalk(gobang.getTurn());
			walkAStep(new_step);
			break;
		case GameType::ONLINE:
			if (isclicked_online)
			{
				QMessageBox::about(NULL, "Tip", QString::fromLocal8Bit("请勿重复点击"));
				return;
			}
			isclicked_online = true;
			s->msg_send(0, 0, OperationType::TIPS);
			break;
		default:
			new_step = gobang.AIWalk(gobang.getTurn());
			walkAStep(new_step);
			break;
		}

	}
	catch (const char* msg)
	{
		qDebug() << msg;
	}
}

/*
	按钮被点击响应事件

	@author - 王开阳 叶志枫
*/
void MainWindow::retractBtnClicked()
{
	Gobang::Step step;
	switch (gameType)
	{
	case GameType::PVE:
		if (gobang.getTurn() == computerColor)
			return;
		step = gobang.popLastStep();
		if (step.x != -1 && step.y != -1)
		{
			chess[step.x][step.y].setPixmap(QPixmap(""));
			delInf();
		}
		step = gobang.popLastStep();
		if (step.x != -1 && step.y != -1)
		{
			chess[step.x][step.y].setPixmap(QPixmap(""));
			delInf();
		}
		highlightStep(step, -2);
		break;
	case GameType::ONLINE:
		if (isclicked_online)
		{
			QMessageBox::about(NULL, "Tip", QString::fromLocal8Bit("请勿重复点击"));
			return;
		}
		isclicked_online = true;
		s->msg_send(0, 0, OperationType::CHEAT);
		break;
	default:
		step = gobang.popLastStep();
		if (step.x != -1 && step.y != -1)
		{
			chess[step.x][step.y].setPixmap(QPixmap(""));
			delInf();
			gobang.shiftTurn();
		}
		highlightStep(step, -2);
		break;
	}
}

/*
	按钮被点击响应事件

	@author - 王开阳 叶志枫
*/
void MainWindow::giveUpBtnClicked()
{
	if (isclicked_online)
	{
		QMessageBox::about(NULL, "Tip", QString::fromLocal8Bit("请勿重复点击"));
		return;
	}
	if (gameType == GameType::ONLINE)
	{
		isclicked_online = true;
		if(s->judge)
			winner = ChessType::BLACKCHESS;
		else
			winner = ChessType::WHITECHESS;
		isGiveUp = true;
		showWinnerDialog();
		disconnect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
		s->msg_send(0, 0, OperationType::GIVEUP);
		return;
	}

	winner = (gobang.getTurn() + 1) % 2;
	isGiveUp = true;
	showWinnerDialog();
	disconnect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
}

/*
	按钮被点击响应事件

	@author - 王开阳 叶志枫
*/
void MainWindow::saveBtnClicked()
{
	std::string dir = selectDirectory();
	try
	{
		if (dir != "")
			gobang.saveBoard(const_cast<char*>(dir.c_str()));
	}
	catch (const char* msg)
	{
		qDebug() << msg;
	}
}

/*
	按钮被点击响应事件

	@author - 王开阳 叶志枫
*/
void MainWindow::returnBtnClicked()
{
	if (gameType == GameType::ONLINE) 
	{		
		s->msg_send(0,0,OperationType::BREAK);
		delete s;
	}

	disconnect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
	gameType = GameType::NONE;
	setHomePageBtnVisable(true);
	setGamePageBtnVisable(false);
}

/*
	棋盘被点击响应事件

	@author 王开阳
*/
void MainWindow::boardClicked()
{
	Gobang::Step new_step;
	try
	{
		switch (gameType)
		{
		case GameType::PVE:
			if (gobang.getTurn() == computerColor)
				return;
			new_step = getStepFromScreen();
			walkAStep(new_step);
			break;
		case GameType::PVP:
			new_step = getStepFromScreen();
			walkAStep(new_step);
			break;
		case GameType::ONLINE:
			if (!s->judge && gobang.getTurn() == ChessType::WHITECHESS)
				return;
			if (s->judge && gobang.getTurn() == ChessType::BLACKCHESS)
				return;
			new_step = getStepFromScreen();
			s->msg_send(new_step.x, new_step.y, OperationType::WALK);
			walkAStep(new_step);
			if (winner != ChessType::NOCHESS)
				delete s;
			break;
		default:
			break;
		}
	}
	catch (const char* msg)
	{
		qDebug() << msg;
	}
}

/*
	窗口关闭事件

	@author - 叶志枫
	@para QCloseEvent - 关闭事件
*/
void MainWindow::closeEvent(QCloseEvent * event)
{
	if (gameType == GameType::ONLINE)
		s->msg_send(0, 0, OperationType::BREAK);
	
	gobang.writeRanking();
	event->accept();
	//event->ignore();
}

/*
	读取存档文件界面

	@author 王开阳
*/
std::string MainWindow::selectFile()
{
	QFileDialog fd;
	fd.setAcceptMode(QFileDialog::AcceptOpen);	//文件对话框为读取类型
	fd.setViewMode(QFileDialog::Detail);		//详细
	fd.setFileMode(QFileDialog::ExistingFile);	//存在的单个文件名
	fd.setWindowTitle(QString::fromLocal8Bit("选择要读取的文件"));
	fd.setDefaultSuffix("txt");
	QStringList filters;
	filters << QString::fromLocal8Bit("文本文件 (*.txt)");
	fd.setNameFilters(filters);					//文件过滤
	if (!fd.exec())
		return "";
	QByteArray ba = fd.selectedFiles()[0].toLocal8Bit();
	return ba.data();
}

/*
	保存存档文件界面

	@author 王开阳
*/
std::string MainWindow::selectDirectory()
{
	QFileDialog fd;
	fd.setAcceptMode(QFileDialog::AcceptSave);	//文件对话框为保存类型
	fd.setViewMode(QFileDialog::Detail);		//详细
	fd.setFileMode(QFileDialog::Directory);		//存在的文件夹
	fd.setWindowTitle(QString::fromLocal8Bit("选择要保存的目录"));
	fd.setDefaultSuffix("txt");
	QStringList filters;
	filters << QString::fromLocal8Bit("文本文件 (*.txt)");
	fd.setNameFilters(filters);					//文件过滤
	if (!fd.exec())
		return "";
	QByteArray ba = fd.selectedFiles()[0].toLocal8Bit();
	return ba.data();
}

/*
	处理联机另一端操作信息

	@author 王锴贞
	@para operation 操作类型
	@para x，y 位置坐标
*/
void MainWindow::handleRecv_mes(int operation, int x, int y)
{
	int isOKClicked = -1;
	Gobang::Step new_step;

	switch (operation)
	{
	case OperationType::WALK:
		try
		{
			if (!s->judge)
			{
				if (gobang.getTurn() == ChessType::BLACKCHESS)
				{
					s->msg_send(0, 0, OperationType::ERR);
					return;
				}
			}
			else
			{
				if (gobang.getTurn() == ChessType::WHITECHESS)
				{
					s->msg_send(0, 0, OperationType::ERR);
					return;
				}
			}
			Gobang::Step new_step;
			new_step.x = x;
			new_step.y = y;
			walkAStep(new_step);
		}
		catch (char *msg)
		{
			QMessageBox::warning(NULL, "ERROR", msg);
		}
		break;
	case OperationType::TIPS:
		if (!s->judge)
		{
			if (gobang.getTurn() == ChessType::BLACKCHESS)
			{
				s->msg_send(0, 0, OperationType::ERR);
				return;
			}
		}
		else
		{
			if (gobang.getTurn() == ChessType::WHITECHESS)
			{
				s->msg_send(0, 0, OperationType::ERR);
				return;
			}
		}
		isOKClicked = QMessageBox::question(this, QString::fromLocal8Bit("提示请求"), QString::fromLocal8Bit("是否同意对方提示"), QMessageBox::Yes, QMessageBox::No);
		switch (isOKClicked)
		{
		case QMessageBox::Yes:
			new_step = gobang.AIWalk(gobang.getTurn());
			walkAStep(new_step);
			s->msg_send(1, 1, OperationType::AGREE);
			break;
		case QMessageBox::No:
			s->msg_send(1, 0, OperationType::DISAGREE);
			break;
		default:
			s->msg_send(0, 0, OperationType::ERR);
			break;
		}
		break;
	case OperationType::RESTART:
		isOKClicked = QMessageBox::question(this, QString::fromLocal8Bit("重新开始请求"), QString::fromLocal8Bit("是否同意重新开始游戏"), QMessageBox::Yes, QMessageBox::No);
		switch (isOKClicked)
		{
		case QMessageBox::Yes:
			clearBoard();
			gobang.initBoard();
			connect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
			ui.text_chessinf->setText("");

			s->msg_send(2, 1, OperationType::AGREE);
			break;
		case QMessageBox::No:
			s->msg_send(2, 0, OperationType::DISAGREE);
			break;
		default:
			s->msg_send(0, 0, OperationType::ERR);
			break;
		}
		break;
	case OperationType::CHEAT:
		isOKClicked = QMessageBox::question(this, QString::fromLocal8Bit("悔棋请求"), QString::fromLocal8Bit("是否同意悔棋"), QMessageBox::Yes, QMessageBox::No);
		switch (isOKClicked)
		{
		case QMessageBox::Yes:
			if (!s->judge)
			{
				if (gobang.getTurn() == ChessType::BLACKCHESS)
				{
					new_step = gobang.popLastStep();
					if (new_step.x != -1 && new_step.y != -1)
					{
						chess[new_step.x][new_step.y].setPixmap(QPixmap(""));
						delInf();
						gobang.shiftTurn();
					}
				}
				else
				{
					for (int c = 1; c < 3; c++) {
						new_step = gobang.popLastStep();
						if (new_step.x != -1 && new_step.y != -1)
						{
							chess[new_step.x][new_step.y].setPixmap(QPixmap(""));
							delInf();
							gobang.shiftTurn();
						}
					}
				}
			}
			else
			{
				if (gobang.getTurn() == ChessType::WHITECHESS)
				{
					new_step = gobang.popLastStep();
					if (new_step.x != -1 && new_step.y != -1)
					{
						chess[new_step.x][new_step.y].setPixmap(QPixmap(""));
						delInf();
						gobang.shiftTurn();
					}
				}
				else
				{
					for (int c = 1; c < 3; c++) {
						new_step = gobang.popLastStep();
						if (new_step.x != -1 && new_step.y != -1)
						{
							chess[new_step.x][new_step.y].setPixmap(QPixmap(""));
							delInf();
							gobang.shiftTurn();
						}
					}
				}
			}
			highlightStep(new_step, -2);
			s->msg_send(3, 1, OperationType::AGREE);
			break;
		case QMessageBox::No:
			s->msg_send(3, 0, OperationType::DISAGREE);
			break;
		default:
			s->msg_send(0, 0, OperationType::ERR);
			break;
		}
		break;
	case OperationType::GIVEUP:
		if (s->judge)
			winner = ChessType::WHITECHESS;
		else
			winner = ChessType::BLACKCHESS;
		isGiveUp = true;
		showWinnerDialog();
		disconnect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
		break;
	case OperationType::ERR:
		isclicked_online = false;
		QMessageBox::warning(NULL, "ERROR", QString::fromLocal8Bit("非法的操作"));
		break;
	case OperationType::AGREE:
		switch (x)
		{
		case 1:
			new_step = gobang.AIWalk(gobang.getTurn());
			walkAStep(new_step);
			break;
		case 2:
			clearBoard();
			gobang.initBoard();
			connect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
			ui.text_chessinf->setText("");
			break;
		case 3:
			if (s->judge)
			{
				if (gobang.getTurn() == ChessType::BLACKCHESS)
				{
					new_step = gobang.popLastStep();
					if (new_step.x != -1 && new_step.y != -1)
					{
						chess[new_step.x][new_step.y].setPixmap(QPixmap(""));
						delInf();
						gobang.shiftTurn();
					}
				}
				else
				{
					for (int c = 1; c < 3; c++) {
						new_step = gobang.popLastStep();
						if (new_step.x != -1 && new_step.y != -1)
						{
							chess[new_step.x][new_step.y].setPixmap(QPixmap(""));
							delInf();
							gobang.shiftTurn();
						}
					}
				}
			}
			else
			{
				if (gobang.getTurn() == ChessType::WHITECHESS)
				{
					new_step = gobang.popLastStep();
					if (new_step.x != -1 && new_step.y != -1)
					{
						chess[new_step.x][new_step.y].setPixmap(QPixmap(""));
						delInf();
						gobang.shiftTurn();
					}
				}
				else
				{
					for (int c = 1; c < 3; c++) {
						new_step = gobang.popLastStep();
						if (new_step.x != -1 && new_step.y != -1)
						{
							chess[new_step.x][new_step.y].setPixmap(QPixmap(""));
							delInf();
							gobang.shiftTurn();
						}
					}
				}
			}
			QMessageBox::about(NULL, "Message", QString::fromLocal8Bit("请继续走棋"));
			highlightStep(new_step, -2);
			break;
		default:
			break;
		}
		isclicked_online = false;
		break;
	case OperationType::DISAGREE:
		switch (x)
		{
		case 1:
			QMessageBox::about(NULL, "Message", QString::fromLocal8Bit("对方不同意提示，对局继续"));
			break;
		case 2:
			QMessageBox::about(NULL, "Message", QString::fromLocal8Bit("对方不同意重新开始，对局继续"));
			break;
		case 3:
			QMessageBox::about(NULL, "Message", QString::fromLocal8Bit("对方不同意悔棋，对局继续"));
			break;
		default:
			break;
		}
		isclicked_online = false;
		break;
	case OperationType::BREAK:
		QMessageBox::about(NULL, "Tip", QString::fromLocal8Bit("断开连接......."));

		disconnect(ui.btn_chessboard, SIGNAL(pressed()), this, SLOT(boardClicked()));
		gameType = GameType::NONE;
		setHomePageBtnVisable(true);
		setGamePageBtnVisable(false);
		delete s;
		
		break;
	default:
		break;
	}
}

#include <thread>
#include <chrono>

AIThread::AIThread()
{
}

AIThread::~AIThread()
{
}

/*
	开启AI走棋线程

	@author 叶志枫
	@para color - 电脑执棋颜色
*/
bool AIThread::Start(MainWindow *mainapp, int color)
{
	this->mainapp = mainapp;
	this->color = color;
	std::thread th(&AIThread::Main, this);
	th.detach();
	return true;
}

/*
	AI走棋线程

	@author 叶志枫
*/
void AIThread::Main()
{
	while (GameType::PVE == mainapp->gameType)
	{
		if (mainapp->getGobang().getTurn() != color)
		{
			this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		Gobang::Step new_step = mainapp->getGobang().AIWalk(color);
		mainapp->getGobang().newStep(new_step);
		mainapp->showStep(new_step, mainapp->getGobang().getTurn());
		mainapp->highlightStep(new_step, mainapp->getGobang().getTurn());
		mainapp->playSoundEffects();
		emit showInf(mainapp->getGobang().getTurn(), new_step.x, new_step.y);
		switch (mainapp->getIsRestricted())
		{
		case QMessageBox::Yes:
			mainapp->winner = mainapp->getGobang().isOverWithRestricted();
			break;
		case QMessageBox::No:
			mainapp->winner = mainapp->getGobang().isOverWithoutRestricted();
			break;
		default:
			break;
		}
		emit showWinnerDialog();
		if (mainapp->winner != ChessType::NOCHESS)
		{
			mainapp->gameType = GameType::NONE;
			return;
		}
		mainapp->getGobang().shiftTurn();
	}
}
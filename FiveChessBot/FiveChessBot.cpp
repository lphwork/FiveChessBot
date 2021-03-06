//FiveChessBot.cpp: 定义应用程序的入口点。
//

#include "stdafx.h"
#include "FiveChessBot.h"
#include "ChessEngine.h"

// 全局变量: 
enum BotStatus {NO_GAME_WINDOW, NOT_STARTED, PLAY_BLACK, PLAY_WHITE};
int gameWindowWidth = 0;                            //qq五子棋窗口宽度
int gameWindowHeight = 0;                           //qq五子棋窗口高度
int gameBoard[15][15];                              //棋盘，没有棋子为0，有棋子为1
double startXRate = 0.215 + 0.005;                  //qq五子棋棋盘X轴开始位置（百分比表示）
double startYRate = 0.145 + 0.005;                  //qq五子棋棋盘Y轴开始位置（百分比表示）
double stepRate = (0.701 - 0.215) / 14 - 0.0002;    //棋盘中每一个格子的长度（百分比表示）
HWND gamehWnd = NULL;                               //qq五子棋窗口句柄
string statusStr;                                   //状态字符串
const string statusStrPrefix = "当前状态：";        //状态字符串的前缀
BotStatus botStatus;                                //保存当前状态


// 此代码模块中包含的函数的前向声明: 
INT_PTR CALLBACK HandleMessages(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void Init();
void ClickOnBoard(int x, int y);
void OnTimer();
void UpdateStatusString();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{

    //显示提示窗口
    string message = "1. 使用本程序前务必把QQ五子棋窗口最大化，否则此程序不能正常运行。\n"
        "2. 点击QQ五子棋上的开始按钮后，若为自己先走，则点击“电脑执黑”，若对方先走，则点击“电脑执白”。正常情况下此程序会自动帮你下棋。\n"
        "3. 本程序为彩蛋程序。\n"
        "4. 程序还未完善，偶尔弹出“未知错误”属于正常现象。";
    MessageBoxA(NULL, message.c_str(), "使用说明", NULL);

    //显示主窗口
    DialogBox(NULL, MAKEINTRESOURCE(IDD_MAIN), NULL, HandleMessages);

    return 0;
}

bool CanStart(HWND hDlg, bool isBlack) {
    int count[2] = { 0, 0 };
    unsigned int i, j;
    HDC hDC = GetDC(gamehWnd);

    for (i = 0; i < 15; i++) {
        for (j = 0; j < 15; j++) {
            int realX = startXRate * gameWindowWidth + stepRate * gameWindowWidth * i;
            int realY = startYRate * gameWindowHeight + stepRate * gameWindowWidth * j;
            COLORREF color = GetPixel(hDC, realX, realY);
            int r = GetRValue(color);
            int g = GetGValue(color);
            int b = GetBValue(color);

            //如果是棋子 
            if (abs(r - g) < 10 && abs(g - b) < 10) {
                //如果是白棋
                if (r > 150) {
                    count[0]++;
                }
                //黑棋
                else {
                    count[1]++;
                }
            }
        }
    }

    if (isBlack) {
        if (count[0] == 0 && count[1] == 0) {
            return true;
        }
    }
    else {
        if (count[0] == 0 && count[1] <= 1) {
            return true;
        }
    }

    return false;
}

void DisableButton(HWND hDlg) {
    EnableWindow(GetDlgItem(hDlg, IDC_START_BLACK), false);
    EnableWindow(GetDlgItem(hDlg, IDC_START_WHITE), false);
}

void EnableButton(HWND hDlg) {
    EnableWindow(GetDlgItem(hDlg, IDC_START_BLACK), true);
    EnableWindow(GetDlgItem(hDlg, IDC_START_WHITE), true);
}

bool IsGameWindowExist() {
    gamehWnd = NULL;
    char classname[32];

    while (true) {
        gamehWnd = FindWindowEx(NULL, gamehWnd, NULL, L"五子棋");
        if (gamehWnd) {
            memset(classname, 0, sizeof(classname));
            GetClassNameA(gamehWnd, classname, 32);
            if(strncmp(classname, "Afx:", 4) == 0) {
                return true;
            }
        }
        else {
            return false;
        }
    }
    return false;
}

void UpdateStatusString() {
    switch (botStatus) {
    case NOT_STARTED:
        statusStr = "已探测到QQ五子棋窗口";
        break;
    case NO_GAME_WINDOW:
        statusStr = "未探测到QQ五子棋窗口";
        break;
    case PLAY_BLACK:
        statusStr = "正在执黑";
        break;
    case PLAY_WHITE:
        statusStr = "正在执白";
        break;
    }
}

void CheckWin(HWND hDlg) {
    int win = ChessEngine::isSomeOneWin();
    if (win != -1) {
        botStatus = NOT_STARTED;
        UpdateStatusString();
        SetDlgItemTextA(hDlg, IDC_STATUS, (statusStrPrefix + statusStr).c_str());
        EnableButton(hDlg);

        string message = win == 0 ? "对方赢了" : "电脑赢了";
        MessageBoxA(hDlg, message.c_str(), "提示", NULL);
    }
}

void OnTimer(HWND hDlg) {
    //检查窗口在不在
    if (!IsGameWindowExist()) {
        botStatus = NO_GAME_WINDOW;
        UpdateStatusString();
        SetDlgItemTextA(hDlg, IDC_STATUS, (statusStrPrefix + statusStr).c_str());
        DisableButton(hDlg);
        return;
    }
    else {
        if (botStatus == NO_GAME_WINDOW) {
            botStatus = NOT_STARTED;

            //获取窗口宽高
            RECT windowRect;
            GetWindowRect(gamehWnd, &windowRect);
            gameWindowWidth = windowRect.right - windowRect.left;
            gameWindowHeight = windowRect.bottom - windowRect.top;

            EnableButton(hDlg);
        }
    }

    UpdateStatusString();
    SetDlgItemTextA(hDlg, IDC_STATUS, (statusStrPrefix + statusStr).c_str());

    if (botStatus == NOT_STARTED)
        return;

    //获取qq游戏的HDC
    HDC hDC = GetDC(gamehWnd);

    //扫描游戏棋盘
    unsigned int i, j;
    for (i = 0; i < 15; i++) {
        for (j = 0; j < 15; j++) {
            int realX = startXRate * gameWindowWidth + stepRate * gameWindowWidth * i;
            int realY = startYRate * gameWindowHeight + stepRate * gameWindowWidth * j;
            COLORREF color = GetPixel(hDC, realX, realY);
            int r = GetRValue(color);
            int g = GetGValue(color);
            int b = GetBValue(color);

            //如果是棋子 
            if (abs(r - g) < 10 && abs(g - b) < 10) {
                //如果原来的棋盘为空 
                if (gameBoard[i][j] == 0) {
                    //如果是白棋 且电脑执黑 
                    if (r > 150 && botStatus == PLAY_BLACK) {
                        gameBoard[i][j] = 1;

                        ChessEngine::nextStep(i, j);
                        ChessEngine::Position result = ChessEngine::getLastPosition();

                        //在棋盘上放置棋子
                        gameBoard[result.x][result.y] = 1;
                        ClickOnBoard(result.x, result.y);

                        CheckWin(hDlg);

                    }
                    //如果是黑棋且电脑执白 
                    else if (r <= 150 && botStatus == PLAY_WHITE) {

                        gameBoard[i][j] = 1;

                        ChessEngine::nextStep(i, j);
                        ChessEngine::Position result = ChessEngine::getLastPosition();

                        gameBoard[result.x][result.y] = 1;
                        ClickOnBoard(result.x, result.y);

                        CheckWin(hDlg);
                    }
                    //错误
                    else {

                        botStatus = NOT_STARTED;
                        UpdateStatusString();
                        SetDlgItemTextA(hDlg, IDC_STATUS, (statusStrPrefix + statusStr).c_str());
                        EnableButton(hDlg);
                        MessageBox(hDlg, L"未知错误", L"提示", MB_OK | MB_ICONINFORMATION);
                        return;
                    }
                }

            }
        }
    }
}

//在棋盘第x列和y行上点击
void ClickOnBoard(int x, int y) {
    if (gamehWnd) {
        int realX = (int)(startXRate * gameWindowWidth + stepRate * gameWindowWidth * x);
        int realY = (int)(startYRate * gameWindowHeight + stepRate * gameWindowWidth * y);
        SendMessage(gamehWnd, WM_MOUSEMOVE, VK_LBUTTON, MAKELPARAM(realX, realY));
        SendMessage(gamehWnd, WM_LBUTTONDOWN, VK_LBUTTON, MAKELPARAM(realX, realY));
    }
}

void Init(HWND hDlg) {
    //初始化棋盘
    memset(gameBoard, 0, sizeof(gameBoard));

    //初始化状态
    if (IsGameWindowExist()) {
        botStatus = NOT_STARTED;

        //获取窗口宽高
        RECT windowRect;
        GetWindowRect(gamehWnd, &windowRect);
        gameWindowWidth = windowRect.right - windowRect.left;
        gameWindowHeight = windowRect.bottom - windowRect.top;
    }
    else {
        botStatus = NO_GAME_WINDOW;
        DisableButton(hDlg);
    }

    //更新状态字符串
    UpdateStatusString();
    SetDlgItemTextA(hDlg, IDC_STATUS, (statusStrPrefix + statusStr).c_str());

    //设置timer
    SetTimer(hDlg, 1, 500, NULL);

    //初始化五子棋引擎
    ChessEngine::beforeStart();
    ChessEngine::setLevel(8);
}

// 消息处理程序。
INT_PTR CALLBACK HandleMessages(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        Init(hDlg);
        return (INT_PTR)TRUE;

    case WM_TIMER:
        OnTimer(hDlg);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hDlg, NULL);
            return (INT_PTR)TRUE;
        case IDC_START_BLACK:
            if (botStatus == NOT_STARTED) {
                if (!CanStart(hDlg, true)) {
                    MessageBoxA(hDlg, "请重新开始游戏再按下按钮", "提示", NULL);
                    return (INT_PTR)TRUE;
                }

                botStatus = PLAY_BLACK;
                DisableButton(hDlg);

                ChessEngine::reset(1);

                //初始化棋盘
                memset(gameBoard, 0, sizeof(gameBoard));
                
                //第一步下到(7, 7)的位置
                gameBoard[7][7] = 1;
                ClickOnBoard(7, 7);
            }
            break;
        case IDC_START_WHITE:
            if (botStatus == NOT_STARTED) {
                if (!CanStart(hDlg, false)) {
                    MessageBoxA(hDlg, "请重新开始游戏再按下按钮", "提示", NULL);
                    return (INT_PTR)TRUE;
                }

                botStatus = PLAY_WHITE;
                DisableButton(hDlg);

                ChessEngine::reset(0);

                //初始化棋盘
                memset(gameBoard, 0, sizeof(gameBoard));
            }
            break;
        }
    }
    return (INT_PTR)FALSE;
}

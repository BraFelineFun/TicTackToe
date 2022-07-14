/*
 * Dependencies:
 *  gdi32
 *  (kernel32)
 *  user32
 *  (comctl32)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <ctime>
#include <fstream>
#include <thread>
#include <iostream>

#define WM_KEYUP        0x0101
#define KEY_SHIFTED     0x8000
#define KEY_TOGGLED     0x0001
#define Enter           0x0D
#define Q_but           0x51
#define C_but           0x43
#define SPACEBAR        0x20
#define BUT_1           0x31
#define BUT_2           0x32
#define BUT_3           0x33
#define BUT_4           0x34
#define BUT_5           0x35
#define BUT_6           0x36
 /// Обозначим для наглядности содержимого в массиве:
#define Nothing         0 //Ничего
#define Ellips          1 //Эллипс
#define Cross           2 //Крестик
using namespace std;



const TCHAR szWinClass[] = _T("Win32SampleApp");
const TCHAR szWinName[] = _T("TicTacToe");
HANDLE handle;
HANDLE thrHandle;
HWND hwnd;               /* This is the handle for our window */
HBRUSH hBrush, hBrush2, ElipseBr;           /* Current brush */
HPEN hPen;               /*Pen*/
HDC hdc;
RECT rect;
LPRECT lprect = new tagRECT;
PAINTSTRUCT ps;
LPCWSTR Name = _T("Proj");  //Имя отображения
LPVOID view;                //Указатель на окно просмотра
UINT ReDraw, EndGame_Message;

int* turn;
bool ismyturn, isMyCircle;

int* linear; //Массив для работы с памятью
bool thr_sleep = 0;

/////////////////////////////////////
int N = 1; //кол-во полей NxN, по умолчанию = 1
std::string config = "Config.txt";
/////////////////////////////////////

void ReadN()
{
    std::ifstream fin(config);
    if (!fin.is_open())
    {
        _tprintf(TEXT("Could not open file (%d).\n"),
            GetLastError());
        return;
    }
    else
    {
        fin >> N;
    }
    fin.close();
    return;
}
void ChangeBackgr() //Градиент заднего фона
{
    int g = 1;
    bool gotGreen = false;
    while (true)
    {
        GetClientRect(hwnd, lprect);

        hBrush2 = CreateSolidBrush(RGB(50, g, 100));

        hBrush = (HBRUSH)SetClassLongPtr(hwnd, -10, (LONG_PTR)hBrush2); //Подменяем предыдущую кисть
        InvalidateRect(hwnd, lprect, 1);
        this_thread::sleep_for(chrono::milliseconds(10));


        if (g == 255)
            gotGreen = 1;
        else if (g == 0)
            gotGreen = 0;

        if (gotGreen)
            g--;
        else
            g++;
    }
    DeleteObject(hBrush2);
}

void Trash()
{
    int x = 0;
    while (1)
    {
        if (x == 10000000)
            x = 0;
        x++;
    }
}


/* Runs Notepad */
void RunNotepad(void)
{
    STARTUPINFO sInfo;
    PROCESS_INFORMATION pInfo;

    ZeroMemory(&sInfo, sizeof(STARTUPINFO));

    puts("Starting Notepad...");
    CreateProcess(_T("C:\\Windows\\Notepad.exe"),
        NULL, NULL, NULL, FALSE, 0, NULL, NULL, &sInfo, &pInfo);
}



bool GameEnd()
//Проверка на окончание игры
{
    int value = Nothing;
    bool win = 0;
    for (int i = 0; i < N; i++) //Проверка на наличие выигрыша по горизонтали
    {
        value = linear[i * N];
        for (int j = 0; j < N; j++)
        {
            if (linear[i * N + j] != value || linear[i * N + j] == Nothing)
            {
                win = 0; break;
            }
            value = linear[i * N + j];
            win = 1;
        }
        if (win && value != Nothing)
            return 1;
    }

    win = 0;
    for (int i = 0; i < N; i++) //По вертикали
    {
        value = linear[i];
        for (int j = 0; j < N; j++)
        {
            if (linear[i + j * N] != value || linear[i + j * N] == Nothing)
            {
                win = 0; break;
            }
            value = linear[i + j * N];
            win = 1;
        }
        if (win && value != Nothing)
            return 1;
    }

    win = 0;
    value = linear[0];
    for (int i = 0; i < N; i++)//По главной диагонали
    {
        if (linear[i * N + i] != value || linear[i * N + i] == Nothing)
        {
            win = 0; break;
        }
        win = 1;
    }
    if (win && value != Nothing)
        return 1;

    win = 0;
    value = linear[N - 1];
    for (int i = 1; i <= N; i++)//По побочной диагонали
    {
        if (linear[i * N - i] != value || linear[i * N - i] == Nothing)
        {
            win = 0; break;
        }
        win = 1;
    }
    if (win && value != Nothing)
        return 1;

    return 0;
}


LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == ReDraw)
    {
        ismyturn = !ismyturn;
        GetClientRect(hwnd, lprect);
        InvalidateRect(hwnd, lprect, 1);
        return 0;
    }
    if (message == EndGame_Message)
    {
        GetClientRect(hwnd, lprect);
        PostQuitMessage(0);
    }
    switch (message)                  /* handle the messages */
    {


    case WM_KEYDOWN:
    {
        /*if (wParam == Enter)             //Enter
        {
            GetClientRect(hwnd, lprect);

            //Рандомная кисть для заднего фона

            hBrush = (HBRUSH)SetClassLongPtr(hwnd, -10, (LONG_PTR)hBrush2); //Подменяем предыдущую кисть
            InvalidateRect(hwnd, lprect, 1);
            DeleteObject(hBrush);
        }*/
        if (wParam == VK_ESCAPE)        //Esc
        {
            PostQuitMessage(0);         //Завершаем работу
            return 0;
        }
        if (wParam == SPACEBAR)         //SPACE
        {
            if (thr_sleep)
            {
                ResumeThread(thrHandle);
                thr_sleep = 0;

            }
            else
            {
                SuspendThread(thrHandle);
                thr_sleep = 1;
            }
            return 0;
        }
        /////Установка приоритета процесса
        if (wParam == BUT_1)
        {
            SetThreadPriority(thrHandle, THREAD_PRIORITY_IDLE);
            cout << GetThreadPriority(thrHandle) << endl;
            return 0;
        }
        if (wParam == BUT_2)
        {
            SetThreadPriority(thrHandle, THREAD_PRIORITY_LOWEST);
            cout << GetThreadPriority(thrHandle) << endl;
            return 0;
        }
        if (wParam == BUT_3)
        {
            SetThreadPriority(thrHandle, THREAD_PRIORITY_BELOW_NORMAL);
            cout << GetThreadPriority(thrHandle) << endl;
            return 0;
        }
        if (wParam == BUT_4)
        {
            SetThreadPriority(thrHandle, THREAD_PRIORITY_NORMAL);
            cout << GetThreadPriority(thrHandle) << endl;
            return 0;
        }
        if (wParam == BUT_5)
        {
            SetThreadPriority(thrHandle, THREAD_PRIORITY_ABOVE_NORMAL);
            cout << GetThreadPriority(thrHandle) << endl;
            return 0;
        }
        if (wParam == BUT_6)
        {
            SetThreadPriority(thrHandle, THREAD_PRIORITY_HIGHEST);
            cout << GetThreadPriority(thrHandle) << endl;
            return 0;
        }
        /////


        if (wParam == Q_but)             // ctrl + q
        {
            if (GetKeyState(VK_CONTROL) < 0)
                PostQuitMessage(0);     //Завершаем работу
            return 0;
        }


        if (wParam == C_but)             //shift + c
        {
            if (GetKeyState(VK_SHIFT) < 0)
                RunNotepad();           //Запускаем блокнот
            return 0;
        }

        return 0;
    }
    case WM_LBUTTONDOWN:
        //Нажатие ЛКМ рисует круг
    {
        GetClientRect(hwnd, lprect);

        int X = GET_X_LPARAM(lParam), Y = GET_Y_LPARAM(lParam); //Координаты щелчка мышью в пикселях

        int a = (lprect->right - lprect->left) / N; //Шаг (в пикселях), с которым меняется клетка
        int b = (lprect->bottom - lprect->top) / N;

        if (ismyturn)
        {
            turn[1]++; //во второй ячейке разделяемой памяти turn содержится количество ходов

            if (isMyCircle)
                linear[X / a + Y / b * N] = Ellips;
            else
                linear[X / a + Y / b * N] = Cross;

            if (turn[1] >= N * 2 - 1)
            {
                SendMessage(HWND_BROADCAST, ReDraw, 0, 0);

                if (GameEnd()) //Проверка, заканчивается ли игра после данного хода
                {
                    int msg;
                    if (isMyCircle)
                        msg = MessageBox(hwnd, (LPCWSTR)L"CIRCLES WON", (LPCWSTR)L"Game is over", 0);
                    else
                        msg = MessageBox(hwnd, (LPCWSTR)L"CROSSES WON", (LPCWSTR)L"Game is over", 0);

                    if (msg == IDOK || msg == IDCANCEL || msg == IDABORT)
                        PostMessage(HWND_BROADCAST, EndGame_Message, wParam, 0);
                }
                else if (turn[1] == N * N) //Если было сделано максимальное количество ходов, но победитель не выявлен, ничья
                {
                    int msg = MessageBox(hwnd, (LPCWSTR)L"IT'S DRAW", (LPCWSTR)L"Game is over", 0);
                    if (msg == IDOK || msg == IDCANCEL || msg == IDABORT)
                        PostMessage(HWND_BROADCAST, EndGame_Message, Nothing, 0);
                }
            }
            else
                SendMessage(HWND_BROADCAST, ReDraw, 0, 0);
        }
        else
        {
            cout << "Plese, Wait for your turn" << endl;
        }

        InvalidateRect(hwnd, lprect, 1); //Если круга не было, то просим перерисовать окно
        return 0;
    }

    case WM_SIZE:
    {
        GetClientRect(hwnd, lprect);
        InvalidateRect(hwnd, lprect, 1);
        return 0;
    }

    case WM_PAINT:
    {
        GetClientRect(hwnd, lprect);
        hdc = BeginPaint(hwnd, &ps);
        int a = (lprect->right - lprect->left) / N;
        int b = (lprect->bottom - lprect->top) / N;

        //////
        //Рисуем круги
        //////
        int R; //Радиус будущего круга

        if (a >= b) //Задаем радиус круга
            R = b / 2;
        else R = a / 2;

        for (int i = 0; i < N * N; i++)
            if (linear[i] == Ellips) //Если найден эллипс, рисуем
            {
                int xl = a * (i % N);
                int xr = xl + a;
                int yt = b * (i - (i % N)) / N;
                int yb = yt + b;

                int Cx = xl + (a / 2), Cy = yt + (b / 2); //Центр нужной клетки


                SelectObject(hdc, hPen);
                SelectObject(hdc, ElipseBr);
                Ellipse(hdc, Cx - R, Cy - R, Cx + R, Cy + R);
            }

        ///////
        //Рисуем крестики
        ///////
        for (int i = 0; i < N * N; i++)
            if (linear[i] == Cross)
            {
                int xl = a * (i % N);
                int xr = xl + a;
                int yt = b * (i - (i % N)) / N;
                int yb = yt + b;

                SelectObject(hdc, hPen);
                MoveToEx(hdc, xl + (xr - xl) * 0.1, yt + (yb - yt) * 0.1, 0);
                LineTo(hdc, xr - (xr - xl) * 0.1, yb - (yb - yt) * 0.1);

                MoveToEx(hdc, xr - (xr - xl) * 0.1, yt + (yb - yt) * 0.1, 0);
                LineTo(hdc, xl + (xr - xl) * 0.1, yb - (yb - yt) * 0.1);
            }

        ///////
        //рисуем линии
        ///////
        SelectObject(hdc, hPen);

        for (int i = 1; i < N; i++)
        {
            MoveToEx(hdc, lprect->left + a * i, lprect->top, 0);
            LineTo(hdc, lprect->left + a * i, lprect->bottom);

            MoveToEx(hdc, lprect->left, lprect->top + b * i, 0);
            LineTo(hdc, lprect->right, lprect->top + b * i);

        }


        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);       /* send a WM_QUIT to the message queue */
        return 0;
    }

    /* for messages that we don't deal with */
    return DefWindowProc(hwnd, message, wParam, lParam);
}

int main(int argc, char** argv)
{
    ReadN();

    HANDLE Queue;
    Queue = CreateFileMapping(INVALID_HANDLE_VALUE, //Используется файл подкачки
        NULL, //Значение защиты — по умолчанию
        PAGE_READWRITE, //Открыт доступ для чтения/записи
        0, //Максимальный размер объекта
        sizeof(int),
        _T("Pro2j")
    );
    turn = (int*)MapViewOfFile(Queue, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (turn[0] == NULL) {
        turn[0]++;
        ismyturn = true;
        isMyCircle = true;
    }
    else
    {
        if (turn[0] == 1)
        {
            ismyturn = false;
            turn[0]++;
            isMyCircle = false;
        }
        else {
            cout << ("Only two players are avaliable") << endl;
            PostQuitMessage(0);
        }
    }

    srand(time(0)); //Для эффективного рандома

    BOOL bMessageOk;
    MSG message;            /* Here message to the application are saved */
    WNDCLASS wincl = { 0 };         /* Data structure for the windowclass */

    /* Harcode show command num when use non-winapi entrypoint */
    int nCmdShow = SW_SHOW;
    /* Get handle */
    HINSTANCE hThisInstance = GetModuleHandle(NULL);

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szWinClass;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by Windows */

    /* Use custom brush to paint the background of the window */
    hBrush = CreateSolidBrush(RGB(0, 0, 255));// - задний фон
    wincl.hbrBackground = hBrush;

    ElipseBr = CreateSolidBrush(RGB(255, 10, 255)); // Эллипс

    hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0)); // - кисть для рисования

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClass(&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindow(
        szWinClass,          /* Classname */
        szWinName,       /* Title Text */
        WS_OVERLAPPEDWINDOW, /* default window */
        CW_USEDEFAULT,       /* Windows decides the position */
        CW_USEDEFAULT,       /* where the window ends up on the screen */
        320,                 /* The programs width */
        320,                 /* and height in pixels */
        HWND_DESKTOP,        /* The window is a child-window to desktop */
        NULL,                /* No menu */
        hThisInstance,       /* Program Instance handler */
        NULL                 /* No Window Creation data */
    );

    /* Make the window visible on the screen */
    ShowWindow(hwnd, nCmdShow);

    ////////
    handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, 0x40, 0, 1024, Name);

    if (handle == NULL)
    {
        _tprintf(TEXT("Could not create file mapping object (%d).\n"),
            GetLastError());
        return 1;
    }

    linear = (int*)MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (linear == NULL)
    {
        _tprintf(TEXT("Could not map view of file (%d).\n"),
            GetLastError());
        return 1;
    }


    ReDraw = RegisterWindowMessage(L"FirstMessageID");
    EndGame_Message = RegisterWindowMessage(L"EndGame");
    /////////

    thrHandle = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE)ChangeBackgr,
        0, 0, NULL); //Создаем новый поток

    HANDLE thrTrash = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Trash, 0, 0, NULL);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while ((bMessageOk = GetMessage(&message, NULL, 0, 0)) != 0)
    {
        /* Yep, fuck logic: BOOL mb not only 1 or 0.
         * See msdn at https://msdn.microsoft.com/en-us/library/windows/desktop/ms644936(v=vs.85).aspx
         */
        if (bMessageOk == -1)
        {
            puts("Suddenly, GetMessage failed! You can call GetLastError() to see what happend");
            break;
        }

        /* Translate virtual-key message into character message */
        TranslateMessage(&message);
        /* Send message to WindowProcedure */
        DispatchMessage(&message);
    }

    /* Cleanup stuff */
    DestroyWindow(hwnd);
    UnregisterClass(szWinClass, hThisInstance);
    DeleteObject(hBrush);
    DeleteObject(hPen);
    DeleteObject(ElipseBr);
    DeleteObject(hBrush2);

    UnmapViewOfFile(turn);
    CloseHandle(Queue);

    UnmapViewOfFile(view);
    CloseHandle(handle);

    return 0;
}
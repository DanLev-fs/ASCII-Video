#define _CRT_SECURE_NO_WARNINGS

#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <windows.h>
#include <math.h>
#include <chrono>
#include <thread>

using namespace std;
using namespace cv;

bool CtrlC = false;
int clamp(int value, int minim, int maxim) { return max(minim, min(value, maxim)); }

void change_console_size(int size) { //Function to change console's font size
    CONSOLE_FONT_INFOEX cfi; //https://stackoverflow.com/questions/35382432/how-to-change-the-console-font-size
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;                   // Width of each character in the font
    cfi.dwFontSize.Y = size;                  // Height
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy(cfi.FaceName, L"Consolas"); // Choose your font
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
}

void maximize_window() { //Function to maximize window https://stackoverflow.com/questions/5300170/maximize-console-windows-in-windows-7/5300345#5300345
    HWND hWnd;
    SetConsoleTitle(L"ASCII Video");
    hWnd = FindWindow(NULL, L"ASCII Video");
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD NewSBSize = GetLargestConsoleWindowSize(hOut);
    SMALL_RECT DisplayArea = { 0, 0, 0, 0 };

    SetConsoleScreenBufferSize(hOut, NewSBSize);

    DisplayArea.Right = NewSBSize.X - 1;
    DisplayArea.Bottom = NewSBSize.Y - 1;

    SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);

    ShowWindow(hWnd, SW_MAXIMIZE);
}

void remove_scrollbar()
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(handle, &info);
    COORD new_size =
    {
        info.srWindow.Right - info.srWindow.Left + 1,
        info.srWindow.Bottom - info.srWindow.Top + 1
    };
    SetConsoleScreenBufferSize(handle, new_size);
}

BOOL WINAPI CtrlHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT)
        CtrlC = true;

    return TRUE;
}

int main(int argc, char* argv[])
{
    int nScreenWidth;			// Console Screen Size X (columns)
    int nScreenHeight;			// Console Screen Size Y (rows)
    int brigth = -100;
    float contrast = 1;

    if (argc < 2)
    {
        cout << "run " << argv[0] << " \"path to video.mp4\"" << endl;
        return 1;
    }

    remove("sound.wav");
    string cmd = "ffmpeg.exe -i \"";
    cmd += argv[1];
    cmd += "\" -ac 2 -f wav sound.wav";
    system(cmd.c_str());

    remove_scrollbar();
    change_console_size(6);
    remove_scrollbar();
    maximize_window();
    remove_scrollbar();

    cout << "Press ENTER";
    cin.get();
    remove_scrollbar();

    // Get Screen Size
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int w_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int w_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    nScreenWidth = (csbi.srWindow.Right - csbi.srWindow.Left + 1);
    nScreenHeight = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);

    // Create Screen Buffer
    char* screen = new char[nScreenWidth * nScreenHeight];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    utils::logging::setLogLevel(utils::logging::LogLevel::LOG_LEVEL_SILENT);
    VideoCapture cap(argv[1]);

    char gradient[] = " .'`^\",:;Il!i<>~+_-?[]{}1()|\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
    int gradientSize = size(gradient) - 2;

    chrono::microseconds time_between_frames = chrono::microseconds(chrono::seconds(1)) / 30;
    chrono::steady_clock::time_point target_tp = chrono::steady_clock::now();

    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        printf("\nERROR: Could not set control handler");
        return 1;
    }

    PlaySoundA("sound.wav", NULL, SND_ASYNC);

    while (true) {
        if ((GetAsyncKeyState((unsigned short)'Q') & 0x8000) || CtrlC) break;
        if (GetAsyncKeyState((unsigned short)'U') & 0x8000) brigth = -100;
        if (GetAsyncKeyState((unsigned short)'J') & 0x8000) contrast = 1;
        if ((GetAsyncKeyState((unsigned short)'O') & 0x8000) && brigth <= 100) brigth += 5;
        if ((GetAsyncKeyState((unsigned short)'I') & 0x8000) && brigth >= -100) brigth -= 5;
        if ((GetAsyncKeyState((unsigned short)'L') & 0x8000) && contrast <= 4) contrast += 0.05;
        if ((GetAsyncKeyState((unsigned short)'K') & 0x8000) && contrast >= 0.25) contrast -= 0.05;
        if (contrast < 0.25) contrast = 0.25;
        if (contrast > 4) contrast = 4;

        int minPixVal = 0;
        Mat image, dispImg;
        double minVal, maxVal;
        cap.read(image);
        dispImg = image;

        if (image.empty())
            break;

        resize(image, image, Size(nScreenWidth, nScreenHeight), 0, 0, INTER_CUBIC);
        image.convertTo(image, -1, contrast, brigth);
        cvtColor(image, image, COLOR_BGR2GRAY);
        //imshow("firstframe", dispImg); waitKey(1);
        minMaxIdx(image, &minVal, &maxVal);

        for (int x = 0; x < image.rows; x++)
        {
            for (int y = 0; y < image.cols; y++)
            {
                int color;
                int val = (int)image.at<uchar>(x, y);
                if (val > minVal)
                    color = clamp(val, 0, gradientSize);
                else
                    color = 0;
                char pixel = gradient[color];
                screen[x * nScreenWidth + y] = pixel;
            }
        }
        
        // Display Frame
        screen[nScreenWidth * nScreenHeight - 1] = '\0';
        WriteConsoleOutputCharacterA(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);

        target_tp += time_between_frames;
        this_thread::sleep_until(target_tp);
    }
    cap.release();
    change_console_size(15);
    remove_scrollbar(); 
    return 0;
}
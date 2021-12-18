/******************************************************************************\
* Copyright (C) 2012-2014 Leap Motion, Inc. All rights reserved.               *
* Leap Motion proprietary and confidential. Not for distribution.              *
* Use subject to the terms of the Leap Motion SDK Agreement available at       *
* https://developer.leapmotion.com/sdk_agreement, or another agreement         *
* between Leap Motion and you, your company or other organization.             *
\******************************************************************************/

#include <iostream>
#include <cstring>
#include <windows.h>
#include <chrono>
#include <conio.h>
#include <WINUSER.H>
#include <sstream>
#include "analyze.h"

#define SHOW

using namespace std::chrono;

int main(int argc, char** argv) {
    
    HANDLE hOut;
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    system("chcp 437");
    system("cls");
 
    SampleListener listener;
    Controller controller;
    controller.addListener(listener);// イベントを受け取るリスナーを登録する
    if (argc > 1 && strcmp(argv[1], "--bg") == 0)
    {
        controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);// 起動時の引数に"--bg"が設定されていた場合はバックグラウンドモード(アプリケーションがアクティブでない場合にもデータを取得する)で動作させる
        controller.setPolicy(Leap::Controller::POLICY_ALLOW_PAUSE_RESUME);
    }
    
    control &con = listener.con;
    con.gearFactor = 2;
    Fourier fft(128);

    int cnt = 0;
    int beat_cnt_temp = 0;
    char c = 0;

    stringstream ss;
    string str;
    string name;
    unsigned char arytemp[129] = { 0 }, ary[129] = { 0 }, * ay = ary, * by = arytemp, * p = nullptr;
    bool keepE = true, keepU = false, keepD = false, keepL = false, keepR = false, keepS = false;

    // Leap Motionのコントローラーおよびイベントを受け取るリスナー（のサブクラス）を作成する
    

    while (1)
    {
        cout << "Midi File name: ";
        std::getline(cin, name);
        con.readMIDI(name);
        //con.printMusic();
        system("cls");

        auto t = high_resolution_clock::now();
        auto temp = t;
        //con.start(duration_cast<microseconds>(t.time_since_epoch()).count());
        con.playState = 1;
        while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000))
        {
            t = high_resolution_clock::now();
            con.refresh(duration_cast<microseconds>(t.time_since_epoch()).count(), listener.fft);
            
            if (duration_cast<microseconds>(t - temp).count() > 7.5 * 1e6 / con.curBpm)
            {
                //printf("\x1b[1m");
                if (cnt++ % 8 == 0)
                {
                    //con.onBeat(duration_cast<microseconds>(t.time_since_epoch()).count(), 300, 20, fft);
                    //con.refresh(duration_cast<microseconds>(t.time_since_epoch()).count(), fft);
                }
#ifdef SHOW
                con.getOnPlayList(ay);
                if ((by[128] = (unsigned char)con.beatRecieved) == 1) con.beatRecieved = false;
                if (con.curBeatPos != beat_cnt_temp)
                {
                    c = '_';//196 '-'223
                    beat_cnt_temp = con.curBeatPos;
                }
                else
                {
                    c = ' ';
                }

                ss << beat_cnt_temp;
                ss >> str;
                cout << '\r' << '\b';

                for (unsigned i = 0; i < 125; i++)
                {
                    if (by[i] == 0)
                    {
                        if (i < str.size() && c == '_')
                        {
                            SetConsoleTextAttribute(hOut, 15);
                            cout << str[i];
                        }
                        else
                        {
                            SetConsoleTextAttribute(hOut, 8);
                            cout << c;
                        }
                    }
                    else
                    {
                        // '_' shows current state at the bottom of each line (starting of the next)
                        // thus current refreshed line are representing previous statement
                        //which stored in arytemp[]
                        if (by[i] == 16)
                        {
                            SetConsoleTextAttribute(hOut, 15);
                            cout << '\261';
                        }
                        else
                        {
                            SetConsoleTextAttribute(hOut, 16 - by[i]);
                            cout << '\333';
                        }

                    }
                }
                if (by[128])
                {
                    SetConsoleTextAttribute(hOut, 11);
                    cout << "___";
                    con.beatRecieved = false;
                }
                else
                {
                    for (int i = 125; i < 128; i++)
                    {
                        if (by[i] != 0)
                        {
                            if (by[i] == 16)
                            {
                                SetConsoleTextAttribute(hOut, 15);
                                cout << '\261';
                            }
                            else
                            {
                                SetConsoleTextAttribute(hOut, 16 - by[i]);
                                cout << '\333';
                            }
                        }
                        else
                        {
                            cout << ' ';
                        }
                    }
                }
                SetConsoleTextAttribute(hOut, 15);
                con.printParameter();
                //printf("\x1b[0m");
                p = by;
                by = ay;
                ay = p;

                ss.clear();
                temp = t;
#endif
            }
            
            if (GetAsyncKeyState(VK_UP) & 0x8000)
            {
                if (!keepU)
                {
                    keepU = true;
                    con.curBpm += 10;
                }
            }
            else
            {
                keepU = false;
            }

            if (GetAsyncKeyState(VK_DOWN) & 0x8000)
            {
                if (!keepD)
                {
                    keepD = true;
                    con.curBpm -= 10;
                }
            }
            else
            {
                keepD = false;
            }

            if (GetAsyncKeyState(VK_LEFT) & 0x8000)
            {
                if (!keepL)
                {
                    keepL = true;
                    con.timeOffset -= 1;
                }
            }
            else
            {
                keepL = false;
            }

            if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
            {
                if (!keepR)
                {
                    keepR = true;
                    con.timeOffset += 1;
                }
            }
            else
            {
                keepR = false;
            }


            if (GetAsyncKeyState(VK_RETURN) & 0x8000)
            {
                if (!keepE)
                {
                    keepE = true;
                    //con.onBeat(duration_cast<microseconds>(t.time_since_epoch()).count(), 300, 20, fft);
                }
            }
            else
            {
                keepE = false;
            }

            if (GetAsyncKeyState(VK_SPACE) & 0x8000)
            {
                if (!keepS)
                {
                    if (con.playState == 3)
                        con.resume(duration_cast<microseconds>(t.time_since_epoch()).count());
                    else if (con.playState == 2)
                        con.pause(duration_cast<microseconds>(t.time_since_epoch()).count());
                    keepS = true;
                }
            }
            else
            {
                keepS = false;
            }
        }
        while (_getch() != VK_ESCAPE);
        con.resetAll();
        memset(arytemp, 0, 128 * sizeof(unsigned char));
        memset(ary, 0, 128 * sizeof(unsigned char));
        system("cls");
    }




    // アプリケーションの終了時にはリスナーを削除する
    controller.removeListener(listener);

    return 0;
}

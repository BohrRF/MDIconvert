#include <iostream>
#include "playControl.h"
#include <WINUSER.H>
#include <conio.h>
#include <chrono>
#include <sstream>
using namespace std;
using namespace std::chrono;

int main()
{
//    for(unsigned char i = 0; i != 255; i+=1)
//    {
//        cout << (int)i << '\333' << i << "\n\n";
//    }
//    getch();

    HANDLE hOut;
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    system("chcp 437");

    control con;
    con.gearFactor = 1;
    con.curVelocityFactor = 1;
    con.curSpanFactor = 1;
    Fourier fft(128);

    int cnt = 0;
    int beat_cnt_temp = 0;
    char c = 0;

    stringstream ss;
    string str;
    string name;
    unsigned char arytemp[129] = {0}, ary[129] = {0}, *ay = ary, *by = arytemp, *p = nullptr;
    bool keepE = true, keepU = false, keepD = false, keepL = false, keepR = false, keepS = false;

    while(1)
    {
        system("cls");
        cout << "Midi File name: ";
        std::getline(cin, name);
        con.readMIDI(name);
        //con.printMusic();
        system("cls");

        auto t = high_resolution_clock::now();
        auto temp = t;
        con.setplayState(duration_cast<microseconds>(t.time_since_epoch()).count(), 2);
        while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000))
        {
            t = high_resolution_clock::now();
            con.refresh(duration_cast<microseconds>(t.time_since_epoch()).count(), fft);

            if(duration_cast<microseconds>(t-temp).count() > 7.5*1e6/con.curBpm && con.playState == 2)
            {
                //printf("\x1b[1m");
                if(cnt++ % 8 == 0)
                {
//                     con.onBeat(duration_cast<microseconds>(t.time_since_epoch()).count(), 300, 20, fft);
//                     con.refresh(duration_cast<microseconds>(t.time_since_epoch()).count(), fft);
                }

                con.readOnPlayList(ay);
                if((ay[128] = (unsigned char)con.beatRecieved) == 0) con.beatRecieved = false;
                if(con.curBeatPos != beat_cnt_temp)
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

                for(unsigned i = 0; i < 125; i++)
                {
                    if(by[i] == 0)
                    {
                        if(i < str.size() && c=='_')
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
                        if(by[i] == 16)
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
                if(by[128])
                {
                    SetConsoleTextAttribute(hOut, 11);
                    cout << "___";
                    con.beatRecieved = false;
                }
                else
                {
                    for(int i = 125; i < 128; i++)
                    {
                        if(by[i] != 0)
                        {
                            if(by[i] == 16)
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
                con.outParameter();
                //printf("\x1b[0m");
                p = by;
                by = ay;
                ay = p;

                ss.clear();
                temp = t;
            }


    //
            if(GetAsyncKeyState(VK_UP) & 0x8000)
            {
                if(!keepU)
                {
                    keepU = true;
                    con.curBpm += 10;
                }
            }
            else
            {
                keepU = false;
            }
            if(GetAsyncKeyState(VK_DOWN) & 0x8000)
            {
                if(!keepD)
                {
                    keepD = true;
                    con.curBpm -= 10;
                }
            }
            else
            {
                keepD = false;
            }

            if(GetAsyncKeyState(VK_LEFT) & 0x8000)
            {
                if(!keepL)
                {
                    keepL = true;
                    con.timeOffset -= 1;
                }
            }
            else
            {
                keepL = false;
            }

            if(GetAsyncKeyState(VK_RIGHT) & 0x8000)
            {
                if(!keepR)
                {
                    keepR = true;
                    con.timeOffset += 1;
                }
            }
            else
            {
                keepR = false;
            }


            if(GetAsyncKeyState(VK_RETURN) & 0x8000)
            {
                if(!keepE)
                {
                    keepE = true;
                    con.onBeat(duration_cast<microseconds>(t.time_since_epoch()).count(), 300, 20, fft);
                }
            }
            else
            {
                keepE = false;
            }

            if(GetAsyncKeyState(VK_SPACE) & 0x8000)
            {
                if(!keepS)
                {
                    if(con.playState == 3)
                        con.resume(duration_cast<microseconds>(t.time_since_epoch()).count());
                    else if(con.playState == 2)
                        con.pause(duration_cast<microseconds>(t.time_since_epoch()).count());
                    keepS = true;
                }
            }
            else
            {
                keepS = false;
            }
        }
        while(getch() != VK_ESCAPE);
        con.resetAll();
        memset(arytemp, 0, 128*sizeof(unsigned char));
        memset(ary, 0, 128*sizeof(unsigned char));
    }
    return 0;
}

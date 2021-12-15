#include "playControl.h"
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <conio.h>

using std::cout;
using std::endl;

unsigned int control::timetrans(const unsigned int &den)
{
    return music.TPQN * 4.0 / den;
}

void control::write_beat(beatSection &beat, const std::vector<int> &notes)
{
    note note_temp;
    tickNode notes_temp;
    unsigned int tick_count = 0;

    beat.setTimeSigniture(music.TPQN, 4, 4);
    for(auto it = notes.begin(); it != notes.end(); it++)
    {
        note_temp.channel = 0;
        note_temp.number = *it++;
        note_temp.tickSpan = timetrans(*it);
        note_temp.velocity = 72;

        notes_temp.notes.emplace_back(note_temp);
        notes_temp.tickOffset = tick_count;
        tick_count += note_temp.tickSpan;
        beat.tickSet.emplace_back(notes_temp);

        notes_temp.notes.clear();
    }
}

void control::initial_music()
{
    beatSection beat_temp;
    write_beat(beat_temp, {60, 16, 64, 16, 67, 16, 72, 16});
    for(int i = 0; i < 16; i++)
        music.beatSet.emplace_back(beat_temp);
    beat_ptr = music.beatSet.begin();
    node_ptr = (*beat_ptr).tickSet.begin();
    playState = false;
}

void control::printMusic() const
{
    int CM[16] = {-1}, CL[16] = {-1}, PN[16] = {-1};
    int beat_num = 1;
    for(const auto &beat : music.beatSet)
    {
        cout << "Beat: " << beat_num++ << endl;
        for(const auto &tick : beat.tickSet)
        {
            for(const auto &a : tick.channelBank)
            {
                CM[a.first] = a.second.first;
                CL[a.first] = a.second.second;
            }
            for(const auto &a : tick.channelSound)
            {
                PN[a.first] = a.second;
            }
            cout << "\tTick Offset: "<< tick.tickOffset << endl;
            for(const auto &e : tick.specialEvents)
            {
                cout << "\t\tsystem exclusive";
                for(const auto &a : e) printf("%x ", a);
                cout << '\n';
            }
            for(const auto &note : tick.notes)
            {
                stringstream ss;
                string str;
                cout << "\t\t"
                     << "c " << note.channel << ' '
                     << "n " << note.number << ' '
                     << "t " << note.tickSpan << ' '
                     << "v " << note.velocity << ' ';

                if(CM[note.channel] == -1) str = "default";
                else {ss << CM[note.channel]; ss >> str;}
                cout << "CC0 " << str << ' ';
                ss.clear(); str.clear();

                if(CL[note.channel] == -1) str = "default";
                else {ss << CM[note.channel]; ss >> str;}
                cout << "CC32 " << str << ' ';
                ss.clear(); str.clear();

                if(PN[note.channel] == -1) str = "default";
                else {ss << PN[note.channel]; ss >> str;}
                cout << "PN " << str << '\n' << endl;
                ss.clear(); str.clear();
            }

        }
    }
}

void control::setplayState(const int64_t &curTimeStamp, const int& mode)
{
    if (mode == 2)
    {
        lastBeatTimeStamp = curTimeStamp + timeOffset * 1000;
        nextBeatTimeStamp = lastBeatTimeStamp + 1e6 * (60.0 / curBpm);
        curNodeTimeStamp = lastBeatTimeStamp + (node_ptr != beat_ptr->tickSet.end() ? node_ptr->tickOffset * 1e6 * (60.0 / curBpm) / beat_ptr->tickBeatLength : 0);
        curBeatPos = beat_ptr - music.beatSet.begin() + 1;
    }
    playState = mode;
}

void control::start(const int64_t &curTimeStamp)
{
    beat_ptr = music.beatSet.begin();
    node_ptr = beat_ptr->tickSet.begin();
    setplayState(curTimeStamp, 2);
}

void control::resume(const int64_t &curTimeStamp)
{
    int64_t pauseTime =  curTimeStamp - pauseTimeStamp;
    lastBeatTimeStamp += pauseTime;
    nextBeatTimeStamp += pauseTime;
    curNodeTimeStamp += pauseTime;
    playState = 2;
}

void control::restart(const int64_t &curTimeStamp)
{
    midi.stopMusic();
    onPlayList.clear();
    beat_ptr = music.beatSet.begin();
    node_ptr = beat_ptr->tickSet.begin();
    setplayState(curTimeStamp, 2);
}

void control::resetBeatPos()
{
    midi.stopMusic();
    onPlayList.clear();
    node_ptr = beat_ptr->tickSet.begin();
    curBeatPos = beat_ptr - music.beatSet.begin() + 1;
    playState = 0;
}


void control::resetAll()
{
    midi.reset();
    music.beatSet.clear();
    onPlayList.clear();
    bpmList.clear();
    resetBeatPos();
}

void control::pause(const int64_t &curTimeStamp)
{
    pauseTimeStamp = curTimeStamp;
    midi.stopMusic();
    onPlayList.clear();
    playState = 3;
}

//void control::applyProgramChange() const
//{
//    for(int i = 0; i < 16; i++)
//    {
//        if(music.channelSound[i] >= 0)
//        {
//            if(music.channelBank[i].first >= 0 && music.channelBank[i].second >= 0)
//                playSound.programChange(i, music.channelBank[i]);
//            playSound.programChange(i, music.channelSound[i]);
//        }
//
//    }
//}

void control::getOnPlayList(unsigned char ary[]) const
{
    memset(ary, 0, 128 * sizeof(char));
    for(const auto &a : onPlayList)
    {
        ary[a.onNote.number] = a.onNote.channel + 1;
    }
}

void control::printParameter() const
{
    printf("\nBeat# %5d / %-5lld err:%+4.0fms /%4.0fms    BPM: %3.1f= %4.1f%+5.1f    offset: %+3.0fms",
           curBeatPos, music.beatSet.size() + 1, 1000 * dif , 60*1000 * gearFactor / handBpm, curBpm, handBpm, speedBias, timeOffset);
}

const double STDRANGE = 300.0;
const double STDACCEL = 20;

void control::onBeat(const int64_t& TimeStamp, const double& hand_amp, const double& hand_accel ,const Fourier& fft)
{
    if (hand_amp < LOW_HAND_AMP) return;

    beatRecieved = true;

    bpmList.push(TimeStamp);
    handBpm = bpmList.calAverage(beat_ptr->timeSigniture_num) * gearFactor;
    if(handBpm < 30) return;

    curVelocityFactor = hand_amp/STDRANGE;//TODO change it into factor form
    curAccel = hand_accel;
    curSpanFactor = STDACCEL / curAccel;//TODO scale this
//system("cls");

    /*
    fft.FFT();
    std::vector<std::pair<double, double>> maxlist;

    fft.find_max_freq(maxlist); // 0 Hz amplitude was eliminated from this list


    //cout << "cur time: " << curTimeStamp << " period: " << 8LL * 1000000 / maxlist[0] << '\n';

    for (int i = 0; i < 5; i++)
    {
        printf("%f, %f\n", 60 * maxlist[i].first, maxlist[i].second);
    }
    //curBpm = 60 * maxlist[0].first;
    */

    if (playState == 3)
    {
        setplayState(TimeStamp, 2);
    }

    if (autoplayMode)
    {
        int64_t curTimeStamp = TimeStamp + timeOffset * 1000;
        dif = ((curTimeStamp - lastBeatTimeStamp) < (nextBeatTimeStamp - curTimeStamp)) ? (curTimeStamp - lastBeatTimeStamp) : (curTimeStamp - nextBeatTimeStamp);
        dif /= 1e6;
//        printf("Beat# %-5d ", curBeatPos);
//        printf("err:%+4.0fms /%4.0fms    ", dif * 1000, 60 * 1000 * gearFactor / handBpm);

        if(!speedFix)
        {
            if (abs(dif) <= 0.3 * 60 * gearFactor / handBpm)
            {
    //            double Tcur = gearFactor * 60 / curBpm;
    //            double Test = gearFactor * 60 / handBpm;
    //            speedBias = curBpm * ((Tcur - dif) / Test - 1);
    //            curNodeTimeStamp = TimeStamp + (curNodeTimeStamp - TimeStamp) * (curBpm / (curBpm + speedBias));
    //            nextBeatTimeStamp = TimeStamp + (nextBeatTimeStamp - TimeStamp) * (curBpm / (curBpm + speedBias));
    //            curBpm = curBpm + speedBias;

                    speedBias = -handBpm * handBpm * dif / (60 * gearFactor);

                    curNodeTimeStamp = TimeStamp + (curNodeTimeStamp - TimeStamp) * (curBpm / (handBpm + speedBias));
                    nextBeatTimeStamp = TimeStamp + (nextBeatTimeStamp - TimeStamp) * (curBpm / (handBpm + speedBias));

                    curBpm = handBpm + speedBias;

    //            speedBias = -curBpm * curBpm * dif / (60 * gearFactor);
    //            curNodeTimeStamp = TimeStamp + (curNodeTimeStamp - TimeStamp) * (curBpm / (curBpm + speedBias));
    //            nextBeatTimeStamp = TimeStamp + (nextBeatTimeStamp - TimeStamp) * (curBpm / (curBpm + speedBias));
    //            curBpm = curBpm + speedBias;


    //            printf("BPM: %3.1f= %4.1f%+5.1f\n", curBpm, handBpm, speedBias);
            }
            else
            {
                playState = 3;
            }
        }
    }
    else
    {
//        printf("Beat# %-5d ", curBeatPos);
//        printf("BPM: %3.1f\n", curBpm);
        curBpm = handBpm;
//        // there is some same operation in freash but this meant to excecute no matter node_ptr reached the end of this beat or not
//        if (++beat_ptr == music.beatSet.end())
//        {
//            beat_ptr = music.beatSet.begin();
//            if (!musicLoop) playState = 0;
//        }
//        node_ptr = beat_ptr->tickSet.begin();
//
//        // in case the first note in this beat has offset
//        double usPerTick = 1e6 * (60.0 / curBpm) / beat_ptr->tickBeatLength;
//        curNodeTimeStamp = TimeStamp + (node_ptr != beat_ptr->tickSet.end() ? node_ptr->tickOffset * usPerTick : 0);
//        lastBeatTimeStamp = TimeStamp;
    }

//    outTimePara();
    /*
    cout << "curAccel   " << curAccel << endl;
    cout << "curhandamp " << hand_amp << endl;
    */

}

int control::refresh(const int64_t& TimeStamp, const Fourier& fft)
{

    //cout << curTimeStamp << " " << curBpm << endl;

    /*isBeatEnter => waiting for next Beat, set to true every onbeat()
    * curTimeStamp > lastBeatTimeStamp + 1e6 * (60.0 / curBpm) => if next beat should already came
    * hand_amp < 40 => hand_amp is too small to recongnized as a beat is comming
    */

    /*
        0 -> not change
        1 -> next node
        2 -> next beat
    */

    int if_next = 0;
    if (playState != 2) return if_next;
    //bool isListChange = false;

    ///TODO: stop the replay, go to (pause processing sequence) OR (a tempo and start right after next hand beat)
    if(TimeStamp >= nextBeatTimeStamp)
    {
        lastBeatTimeStamp = nextBeatTimeStamp;
        nextBeatTimeStamp = lastBeatTimeStamp + 1e6 * (60.0 / curBpm);
        //cout << lastBeatTimeStamp << ' ' << nextBeatTimeStamp << endl;
        if(node_ptr == beat_ptr->tickSet.begin())
            curBeatPos = beat_ptr - music.beatSet.begin() + 1;
    }

    if (TimeStamp >= curNodeTimeStamp)
    {
        if_next = 1;
        double usPerTick = 1e6 * (60.0 / curBpm) / beat_ptr->tickBeatLength;
        unsigned int offSetTemp = 0;
        //unsigned int beatLengthTemp = 0;

        if(node_ptr == beat_ptr->tickSet.begin())
        {
            //std::cout << "current Beat Position: " << std::setw(5) << curBeatPos;
            if(node_ptr != beat_ptr->tickSet.end())
            {
                //std::cout << "\ttickOffsets: ";
            }
        }

        if (node_ptr != beat_ptr->tickSet.end())
        {
            //std::cout << "\ntickOffset:" << node_ptr->tickOffset;
            if_next = 2;
            for(const auto &a : node_ptr->sysMessage)
            {
                midi.sysMsg(a);
            }
            for(const auto &a : node_ptr->channelSound)
            {
                auto res = std::find_if(node_ptr->channelBank.begin(), node_ptr->channelBank.end(), [&](const std::pair<unsigned, std::pair<unsigned, unsigned>> &b){return b.first == a.first;});
                if(res != node_ptr->channelBank.end())
                {
                    midi.programChange(a.first, res->second);
                }
                midi.programChange(a.first, a.second);
            }
            for (auto& n : node_ptr->notes)//play
            {
                //cout << "c " << n.channel << ' ' << "n " << n.number << ' ' << "t " << n.tickSpan << ' ' << "v " << n.velocity << '\n' << endl;
                //cout << n.velocity * curVelocityFactor << endl;
                auto res = std::find_if(onPlayList.begin(), onPlayList.end(), [&](const onPlayNote &a){return a.onNote == n;});
                if(res != onPlayList.end())
                {
                    midi.noteOff(res->onNote.number, res->onNote.channel);
                    onPlayList.erase(res);
                }
                midi.noteOn(n.number, n.velocity * curVelocityFactor, n.channel);
                onPlayNote noteTemp(n, curNodeTimeStamp + n.tickSpan * usPerTick * curSpanFactor);
                //cout << "time stamp" << curTimeStamp << " span" << n.tickSpan * nsPerTick * curSpanFactor << endl;

                onPlayList.emplace_back(noteTemp);
                //isListChange = true;
            }
            for(const auto &e : node_ptr->specialEvents)
            {
                midi.specialEvent(e);
            }
            offSetTemp = node_ptr->tickOffset;
            ++node_ptr;
            // node_ptr has already been moved to the next during the calculation of the condition
            if(node_ptr != beat_ptr->tickSet.end())
            {
                curNodeTimeStamp += (node_ptr->tickOffset - offSetTemp) * usPerTick;
            }
        }
//        else
        if(node_ptr == beat_ptr->tickSet.end()) // the end of this beat?
        {
            unsigned int beatLengthTemp = beat_ptr->tickBeatLength;
            if (++beat_ptr == music.beatSet.end()) // end of the whole music?
            {
                beat_ptr = music.beatSet.begin();
                curBeatPos = 0;

                if (!musicLoop) playState = 0;
            }
            node_ptr = beat_ptr->tickSet.begin();


            // first node offset in new beat + previous beat length - last node offset in previous beat
            nextBeatTimeStamp = curNodeTimeStamp + (beatLengthTemp - offSetTemp) * usPerTick;
            curNodeTimeStamp = nextBeatTimeStamp + (node_ptr != beat_ptr->tickSet.end() ? node_ptr->tickOffset : 0) * usPerTick;

//            nextBeatTimeStamp = curNodeTimeStamp + (beatLengthTemp - offSetTemp) * usPerTick;
//            offSetTemp = (node_ptr != beat_ptr->tickSet.end() ? node_ptr->tickOffset : 0);
//            curNodeTimeStamp = nextBeatTimeStamp + offSetTemp * usPerTick;

            //curNodeTimeStamp += ((node_ptr != beat_ptr->tickSet.end() ? node_ptr->tickOffset : 0) + beatLengthTemp) * usPerTick;


//            if(((node_ptr != beat_ptr->tickSet.end() ? node_ptr->tickOffset : 0) + beatLengthTemp - offSetTemp) * usPerTick > 1000000)
//            {
//                cout << "tickoff" << node_ptr->tickOffset<< endl;
//                cout << "beatLen" << beatLengthTemp << endl;
//                cout << "offtemp" << offSetTemp << endl;
//                cout << "usPertick" << usPerTick << endl;
//                cout << ((node_ptr != beat_ptr->tickSet.end() ? node_ptr->tickOffset : 0) + beatLengthTemp - offSetTemp) * usPerTick << endl;
//            }


            //curNodeTimeStamp = lastBeatTimeStamp + (node_ptr != beat_ptr->tickSet.end() ? node_ptr->tickOffset * usPerTick : 0);
            if(!autoplayMode)
                playState = 3;
        }
    }



    std::list<onPlayNote>::iterator it = onPlayList.begin();
    while (it != onPlayList.end())
    {
        if (it->endTimeStamp <= TimeStamp)
        {
            //stop music
            midi.noteOff(it->onNote.number, it->onNote.channel);
            it = onPlayList.erase(it);

            //isListChange = true;
        }
        else
            it++;
    }

    /*
    if (isListChange)
    {
        system("cls");
        cout << "Current Bpm " << curBpm << endl;
        for (const auto& a : onPlayList)
        {
            cout << "c " << a.onNote.channel << ' ' << "n " << a.onNote.number << ' ' << "t " << a.onNote.tickSpan << ' ' << "v " << a.onNote.velocity << " end " << a.endTimeStamp << '\n' << endl;
        }
    }
    */
    return if_next;
}



typedef struct {
    // トラックチャンクのデータを格納する構造体
    char type[4]; // チャンクタイプを示す文字列を格納。「MTrk」が入るはず。[4byte]
    int size;     // トラックチャンクデータのサイズ [4byte]
    char *data;   // トラックデータ（イベントの羅列）へのポインタ
} TrackChunk;

short mergeChar7bit(char x, char y){
    // charの下7bitずつを結合してshort型で出力
    // 【引数】：結合対象となる2つのchar型変数x, y
    short s;
    s = (unsigned char)x; // 上位バイトを先に入れておく
    s <<= 7;              // 7bit左シフト
    s = (s | (unsigned char)(y & 0x7f));   // 下位バイトの下7bitを合成。。。
    return s;
}

int convertEndian(void *input, int s){
    // エンディアン変換をおこなう関数
    // stdlib.hをインクルードしてください。
    // 【引数】: void *input...エンディアン変換対象へのポインタ
    // 【引数】: size_t    s...変換対象のバイト数

    int i;   // カウンタ
    char *temp;   // 変換時に用いる一時的配列

    if((temp = (char *)calloc(s, sizeof(char))) == NULL){
        perror("Error: Cannot get memory for temp.");
        return 0;   // 領域確保できず（失敗）
    }

    for(i = 0; i < s; i++){   // inputデータをtempに一時保管
        temp[i] = ((char *)input)[i];
    }

    for(i = s; i > 0; i--){   // tempデータを逆方向にしてinputへ代入
        ((char *)input)[i - 1] = temp[s - i];
    }

    free(temp);   // 確保した領域を解放

    return 0;   // 正常終了
}



int control::readMIDI(std::string FILENAME)
{
    int i, j, k, cnt;   // カウンタ
    FILE *fp;         // ファイルポインタ生成

    // ヘッダチャンク情報
    char  header_chunk_type[4]; // チャンクタイプを示す文字列を格納。「MThd」が入るはず。[4byte]
    int   header_chunk_size;    // ヘッダチャンクデータのサイズ [4byte]
    short smf_format;     // SMFのフォーマットタイプ（0か1か2） [2byte]
    short tracks;         // トラックチャンク総数 [2byte]
    short division;       // 四分音符あたりの分解能（ここではデルタタイム） [2byte]

    // トラックチャンク情報
    TrackChunk *track_chunks;   // トラックチャンク情報を格納する配列のためのポインタ
    unsigned char c;   // イベント解析の際に使用する一時保存用変数
    unsigned char status = 0;   // ステータスバイト用の一時変数
    unsigned int delta;   // デルタタイム


    // エンディアン判定
    int endian = 1; //   エンディアン判定にいろいろ使用（0:BigEndian, 1:LittleEndian）
    if(*(char *)&endian){   // リトルエンディアンなら...
        endian = 1;   // Little Endian
    } else {   // ビッグエンディアンなら...
        endian = 0;   // Big Endian
    }
    if(FILENAME[0] == '\"' || FILENAME[0] == '\'')
    {
        FILENAME = FILENAME.substr(1, FILENAME.size() - 2);
    }
    if(FILENAME[1] != ':')
    {
        FILENAME = "./" + FILENAME;
    }
    else if(FILENAME.substr(FILENAME.size()-4, 4) != ".mid")
    {
        FILENAME += ".mid";
    }
    // MIDIファイルを開く
    if((fp = fopen(FILENAME.c_str(), "rb")) == NULL){   // バイナリ読み取りモードでファイルを開く
        perror("Error: Cannot open the file.");   // 失敗したらエラーを吐く
        return 1;
    }

    // ヘッダチャンク取得
    fread(header_chunk_type, 1, 4, fp);   // チャンクタイプ
    fread(&header_chunk_size, 4, 1, fp);  // チャンクデータサイズ
    fread(&smf_format, 2, 1, fp);   // SMFフォーマットタイプ
    fread(&tracks, 2, 1, fp);       // トラックチャンク総数
    fread(&division, 2, 1, fp);     // 分解能（デルタタイム）

    // 必要ならエンディアン変換
    if(endian){   // リトルエンディアンなら要変換
        // エンディアン変換処理
        convertEndian(&header_chunk_size, sizeof(header_chunk_size));
        convertEndian(&smf_format, sizeof(smf_format));
        convertEndian(&tracks, sizeof(tracks));
        convertEndian(&division, sizeof(division));
    }

    music.TPQN = division;

    // 読み取ったヘッダチャンク情報を出力
    printf("# Header ========================\n");
    printf("header_chunk_type : %c%c%c%c\n", header_chunk_type[0], header_chunk_type[1], header_chunk_type[2], header_chunk_type[3]);
    printf("header_chunk_size : %d\n", header_chunk_size);
    printf("smf_format : %hd\n", smf_format);
    printf("tracks     : %hd\n", tracks);
    printf("division   : %hd\n", division);

    // トラックチャンク取得
    if((track_chunks = (TrackChunk *)calloc(tracks, sizeof(TrackChunk))) == NULL){   // トラック数に応じて領域確保
        perror("Error: Cannot get memory for track_chunks.");
        return 0;   // 領域確保できず（失敗）
    }

    for(i = 0; i < tracks; i++)// トラック数だけ繰返し
    {
        fread(track_chunks[i].type, 1, 4, fp);   // チャンクタイプ MTrk
        fread(&track_chunks[i].size, 4, 1, fp);   // チャンクデータサイズ
        if(endian)// リトルエンディアンなら要変換
        {
            convertEndian(&track_chunks[i].size, sizeof(track_chunks[i].size));
        }
        if((track_chunks[i].data = (char *)calloc(track_chunks[i].size, sizeof(char))) == NULL)// データサイズに応じて領域確保
        {
            perror("Error: Cannot get memory for track_chunks[i].data .");
            return 1;   // 領域確保できず（失敗）
        }
        fread(track_chunks[i].data, track_chunks[i].size, sizeof(char), fp);   // データ（イベントの羅列）
    }

    note note_temp;
    tickNode notes_temp;
    tickNode specials_temp;
    beatSection beat_temp;
    unsigned curRhythm = 4;
    unsigned delta_count = 0;
    unsigned curOffset = 0;
    unsigned curNum = 4;
    auto beat_it = music.beatSet.begin();
    bool onDumperHold = false;
    int note_count[16][128] = {0};
    int dumperStop_count[16][128] = {0};
    int dumperRemain_count[16][128] = {0};
    std::vector<std::pair<unsigned, unsigned>> sound_temp;
    std::vector<std::pair<unsigned, std::pair<unsigned, unsigned>>> bank_temp;
    std::vector<unsigned> special_temp;

    for(i = 0; i < tracks; i++)// トラック数だけ繰返し
    {
        if(music.beatSet.empty())
            music.beatSet.push_back(beat_temp);
        beat_it = music.beatSet.begin();
        // when beat_it moves, do this
        curNum = beat_it->timeSigniture_num;
        curRhythm = beat_it->timeSigniture_den;

        delta_count = 0;
        curOffset = 0;

        for(int i = 0; i < 16; i++)
        {
            for(int j = 0; j < 128; j++)
            if(note_count[i][j] != 0)
            {
                std::cerr << "Channel:" << i << "Note Number: " << j << endl;
                std::cerr << "All notes have to be stopped at the end of the track." << std::endl;
                exit(1);
            }
        }

        printf("# Track[%02d] =====================\n", i+1);
        printf("track_chunks[%d].type : %c%c%c%c\n", i, track_chunks[i].type[0], track_chunks[i].type[1], track_chunks[i].type[2], track_chunks[i].type[3]);
        printf("track_chunks[%d].size : %d\n", i, track_chunks[i].size);
        printf("track_chunks[%d].data\n", i);
        for(j=0; j<track_chunks[i].size; j++)// データ分だけ繰返し
        {
            delta = 0;   // 初期化
            while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                delta = delta | (c & 0x7F);   // 合成
                delta <<= 7;   // 7bit左シフト
            }
            delta = delta | c;   // 合成
            delta_count += delta;

//            std::cout << "beat# " << delta_count / TPQN + 1 << std::endl;

            int gap = delta + curOffset;
//            cout << gap << endl;
//            cout << curRhythm << ' ' << TPQN << ' ' << (int)(4.0 / curRhythm * TPQN)<< endl;
            while(gap >= (int)(4.0 / curRhythm * music.TPQN)) // 60/480.0 > 0 , 60/480=0, in this case have to =0 make (i) don't loop
            {
                //printf("beat# %lld\n", beat_it - music.beatSet.begin() + 1);
                //if(beat_it - music.beatSet.begin() + 1 == 16) getchar();
                if(++beat_it == music.beatSet.end())
                {
                    music.beatSet.push_back(beat_temp);
                    beat_it = music.beatSet.end() - 1;
                }
                // when beat_it moves, do this
                gap -= (int)(4.0 / curRhythm * music.TPQN);
                curNum = beat_it->timeSigniture_num;
                curRhythm = beat_it->timeSigniture_den;
            }
            curOffset = gap;

            //printf("%7d:: ", curOffset);   // デルタタイム出力


//          if(curOffset > 10000)
//            {
//                cout << gap << endl;
//                getch();
//            }


            // ランニングステータスルールに対する処理
            if((track_chunks[i].data[j] & 0x80) == 0x80){
                // ランニングステータスルールが適用されない場合は、ステータスバイト用変数を更新。
                status = (unsigned char)track_chunks[i].data[j];   // ステータスバイトを保持しておく
            } else {
                //printf("\b@");   // ランニングステータスルール適用のしるし
                j--;   // データバイトの直前のバイト（デルタタイムかな？）を指すようにしておく。
                       // 次の処理でj++するはずなので、そうすればデータバイトにアクセスできる。
            }
            //printf("0x%02x", status);

            // データ判別
            if((status & 0xf0) == 0x80 || ((status & 0xf0) == 0x90 && (unsigned char)track_chunks[i].data[j + 2] == 0))
            {
                // ノート・オフ【ボイスメッセージ】
                //printf("Note Off [%02dch] : ", (status & 0x0f));
                j++;
                //printf("Note%d", (unsigned char)track_chunks[i].data[j]);
                //printf("[0x%02x] ", (unsigned char)track_chunks[i].data[j]);
                j++;
                //printf("Velocity=%d", (unsigned char)track_chunks[i].data[j]);
                unsigned char channel = status & 0x0f;
                unsigned char note = (unsigned char)track_chunks[i].data[j - 1];
                if(onDumperHold)
                {
                    dumperStop_count[channel][note]++;
                    if(dumperRemain_count[channel][note] > 0)
                        dumperRemain_count[channel][note]--;
                }
                else
                {
                    bool flag = false;
                    int n_count = 0;
                    for(auto itr = beat_it; itr != music.beatSet.begin() - 1; itr--)
                    {
                        for(auto it = itr->tickSet.rbegin(); it != itr->tickSet.rend(); it++)
                        {
                            for(auto &a : it->notes)
                            {
                                if(a.number == note && a.channel == (status & 0x0f) && !a.isOffed && ++n_count == note_count[channel][a.number])
                                {
                                    //std::cout << "\nn:" << a.number << "   t: " << delta_count - a.tickSpan << " = " << delta_count << "  -   " << a.tickSpan << std::endl;
                                    a.tickSpan = delta_count - a.tickSpan;
                                    note_count[channel][a.number]--;
                                    a.isOffed = true;
                                    flag = true;
                                    break;
                                }
                            }
                            if(flag) break;
                        }
                        if(flag) break;
                    }
                    if(flag == false)
                    {
                        std::cerr << '\n' << "note number: " << note << " note count: "<< note_count[note] << endl;
                        std::cerr << "Can't find note to turn off." << endl;
                        getch();
                    }
                }
            }
            else if((status & 0xf0) == 0x90)
            {
                // ノート・オン【ボイスメッセージ】
                //printf("Note On  [%02dch] : ", (status & 0x0f));
                j++;
                //printf("Note%d", (unsigned char)track_chunks[i].data[j]);
                //printf("[0x%02x] ", (unsigned char)track_chunks[i].data[j]);
                j++;
                //printf("Velocity=%d", (unsigned char)track_chunks[i].data[j]);
                note_temp.channel = (status & 0x0f);
                note_temp.number = (unsigned char)track_chunks[i].data[j - 1];
                note_temp.velocity = (unsigned char)track_chunks[i].data[j];
                note_temp.tickSpan = delta_count;
                note_count[note_temp.channel][note_temp.number]++;

                if(onDumperHold)
                {
                    dumperRemain_count[note_temp.channel][note_temp.number]++;
                }

                //std::cout << "curOff: " << curOffset << std::endl;
                auto it = beat_it->tickSet.begin();
                while(it != beat_it->tickSet.end())
                {
                    if(it->tickOffset == curOffset)
                    {
                        it->notes.push_back(note_temp);
                        break;
                    }
                    else if(it->tickOffset > curOffset) // if add || it+1 == beat_it->tickSet.end() in condition will increase operating time
                    {
                        notes_temp.notes.clear();
                        notes_temp.notes.push_back(note_temp);
                        notes_temp.tickOffset = curOffset;
                        it = beat_it->tickSet.insert(it, notes_temp);
                        break;
                    }
                    it++;
                }
                if(it == beat_it->tickSet.end())
                {
                    notes_temp.notes.clear();
                    notes_temp.notes.push_back(note_temp);
                    notes_temp.tickOffset = curOffset;
                    it = beat_it->tickSet.insert(it, notes_temp);
                }

                for(const auto &a : bank_temp)
                    it->channelBank.push_back(a);
                bank_temp.clear();

                for(const auto &a : sound_temp)
                    it->channelSound.push_back(a);
                sound_temp.clear();
            }
            else if((status & 0xf0) == 0xa0) // ポリフォニック・キー・プレッシャー【ボイスメッセージ】
            {
                //printf("Polyphonic Key Pressure [%02dch] : ", (status & 0x0f));
                j++;
                //printf("Note%d", (unsigned char)track_chunks[i].data[j]);
                //printf("[0x%02x] ", (unsigned char)track_chunks[i].data[j]);
                j++;
                //printf("Pressure=%d", (unsigned char)track_chunks[i].data[j]);
            }
            else if((status & 0xf0) == 0xb0) // コントロールチェンジ【ボイスメッセージ】、【モードメッセージ】
            {
                //printf("Control Change [%02dch] : ", (status & 0x0f));
                j++;
                c = (unsigned char)track_chunks[i].data[j++];
                if(c <= 63) // 連続可変タイプのエフェクトに関するコントロール情報（MSBもLSBも）
                {
                    // （ホントは「0<=c && c<=63」と書きたいけど、warningが出るので「c<=63」にする）
                    //printf("VariableEffect(");
                    switch(c){
                        case 0:    // 上位バイト[MSB]
                            //printf("BankSelect[%s]", (c==0)?"MSB":"LSB");
                            bank_temp.push_back(std::make_pair(status & 0x0f, std::make_pair((unsigned char)track_chunks[i].data[j], 0)));
                            break;
                        case 32:   // 下位バイト[LSB]
                            //printf("BankSelect[%s]", (c==0)?"MSB":"LSB");   // バンク・セレクト
                            //cout << delta_count << ' ' << (status & 0x0f) << "bank" << endl;
                            bank_temp.back().second.second = (unsigned char)track_chunks[i].data[j];
                            break;
                        case 1:
                        case 33:
                            //printf("ModulationDepth[%s]", (c==1)?"MSB":"LSB");   // モジュレーション・デプス
                            break;
                        case 2:
                        case 34:
                            //printf("BreathType[%s]", (c==2)?"MSB":"LSB");   // ブレス・タイプ
                            break;
                        case 4:
                        case 36:
                            //printf("FootType[%s]", (c==4)?"MSB":"LSB");   // フット・タイプ
                            break;
                        case 5:
                        case 37:
                            //printf("PortamentoTime[%s]", (c==5)?"MSB":"LSB");   // ポルタメント・タイム
                            break;
                        case 6:
                        case 38:
                            //printf("DataEntry[%s]", (c==6)?"MSB":"LSB");   // データ・エントリー
                            break;
                        case 7:
                        case 39:
                            //printf("MainVolume[%s]", (c==7)?"MSB":"LSB");   // メイン・ボリューム
                            break;
                        case 8:
                        case 40:
                            //printf("BalanceControl[%s]", (c==8)?"MSB":"LSB");   // バランス・コントロール
                            break;
                        case 10:
                        case 42:
                            //printf("Panpot[%s]", (c==10)?"MSB":"LSB");   // パンポット
                            break;
                        case 11:
                        case 43:
                            //printf("Expression[%s]", (c==11)?"MSB":"LSB");   // エクスプレッション
                            break;
                        case 16:
                        case 17:
                        case 18:
                        case 19:
                        case 48:
                        case 49:
                        case 50:
                        case 51:
                            //printf("SomethingElse_No.%d[%s]", c, (c==16)?"MSB":"LSB");   // 汎用操作子
                            break;
                        default:
                            ;//printf("##UndefinedType_No.%d[%s]", (c<32)?c:c-32, (c<32)?"MSB":"LSB");   // よくわからないコントロール
                    }
//                    if(c != 0 && c != 32)
//                    {
//                        auto it = beat_it->tickSet.begin();
//                        special_temp.clear();
//                        special_temp.push_back(0xb0 + (status & 0x0f));
//                        special_temp.push_back(c);
//                        special_temp.push_back((unsigned char)track_chunks[i].data[j]);
//
//                        while(it != beat_it->tickSet.end())
//                        {
//                            if(it->tickOffset == curOffset)
//                            {
//                                it->specialEvents.push_back(special_temp);
//                                break;
//                            }
//                            else if(it->tickOffset > curOffset) // if add || it+1 == beat_it->tickSet.end() in condition will increase operating time
//                            {
//                                specials_temp.specialEvents.clear();
//                                specials_temp.specialEvents.push_back(special_temp);
//                                specials_temp.tickOffset = curOffset;
//                                it = beat_it->tickSet.insert(it, specials_temp);
//                                break;
//                            }
//                            it++;
//                        }
//                        if(it == beat_it->tickSet.end())
//                        {
//                            specials_temp.specialEvents.clear();
//                            specials_temp.specialEvents.push_back(special_temp);
//                            specials_temp.tickOffset = curOffset;
//                            it = beat_it->tickSet.insert(it, specials_temp);
//                        }
//                    }

                    //printf(")=%d", (unsigned char)track_chunks[i].data[j]);

                }
                else if(64 <= c && c <= 95)// 連続可変でないタイプのエフェクトに関するコントロール情報
                {
                    //printf("InvariableEffect(");
                    switch(c){
                        case 64:
                            //printf("Hold1(Damper)");   // ホールド１（ダンパー）
                            if((unsigned char)track_chunks[i].data[j] > 64)
                            {
                                onDumperHold = true;
                            }
                            else
                            {
                                onDumperHold = false;
                                bool flag = false;
                                int n_count[16][128] = {0};
                                for(auto itr = beat_it; itr != music.beatSet.begin() - 1; itr--)
                                {
                                    for(auto it = itr->tickSet.rbegin(); it != itr->tickSet.rend(); it++)
                                    {
                                        for(auto &a : it->notes)
                                        {
                                            if(n_count[a.channel][a.number] < (dumperStop_count[a.channel][a.number] + dumperRemain_count[a.channel][a.number]) &&
                                               a.channel == (status & 0x0f) &&
                                               !a.isOffed &&
                                               ++n_count[a.channel][a.number] > dumperRemain_count[a.channel][a.number])
                                            {
                                                //std::cout << "\nn:" << a.number << "   t: " << delta_count - a.tickSpan << " = " << delta_count << "  -   " << a.tickSpan << std::endl;
                                                a.tickSpan = delta_count - a.tickSpan;
                                                a.isOffed = true;
                                                note_count[a.channel][a.number]--;
                                            }
                                        }
                                        flag = true;
                                        for(int i = 0; i < 16; i++)
                                        {
                                            for(int j = 0; j < 128; j++)
                                            {
                                                 if((dumperStop_count[i][j] + dumperRemain_count[i][j]) != n_count[i][j]) flag = false;
                                            }
                                        }
                                        if(flag) break;
                                    }
                                    if(flag) break;
                                }
                                flag = true;
                                for(int i = 0; i < 16; i++)
                                {
                                    for(int j = 0; j < 128; j++)
                                    {
                                         if((dumperStop_count[i][j] + dumperRemain_count[i][j]) != n_count[i][j])
                                         {
                                             std::cerr << '\n' << i << "\tdumperStop: " << dumperStop_count[i] << "\tn_count: " << n_count[i] << std::endl;
                                             flag = false;
                                         }
    //                                     if(dumperStop_count[i] > 0) std::cout << "\nstop" << i << '\t' << dumperStop_count[i] << '\t' << note_count[i] << std::endl;
    //                                     if(dumperRemain_count[i] > 0) std::cout << "\nremain" << i << '\t' << dumperRemain_count[i] << '\t' << note_count[i] << std::endl;
                                         dumperRemain_count[i][j] = 0;
                                         dumperStop_count[i][j] = 0;
                                    }
                                }
                                if(!flag)
                                {
                                    std::cerr << "Dumper did not turn off completely." << std::endl;
                                    getchar();
                                }
                            }

                            break;
                        case 65:
                            //printf("Portamento");   // ポルタメント
                            break;
                        case 66:
                            //printf("Sostenuto");   // ソステヌート
                            break;
                        case 67:
                            //printf("SoftPedal");   // ソフト・ペダル
                            break;
                        case 69:
                            //printf("Hold2(Freeze)");   // ホールド２（フリーズ）
                            break;
                        case 71:
                            //printf("HarmonicIntensity");   // ハーモニック・インテンシティ
                            break;
                        case 72:
                            //printf("ReleaseTime");   // リリース・タイム
                            break;
                        case 73:
                            //printf("AttackTime");   // アタック・タイム
                            break;
                        case 74:
                            //printf("Brightness");   // ブライトネス
                            break;
                        case 75:
                            //printf("Decay Time");   // エンベロープのディケイタイム
                            break;
                        case 76:
                            //printf("Vibrato Rate");   // ビブラートの周期
                            break;
                        case 77:
                            //printf("Vibrato Depth");   // ビブラートの深さ
                            break;
                        case 78:
                            //printf("Vibrato Decay");   // ビブラートのディケイタイム
                            break;
                        case 80:
                        case 81:
                        case 82:
                        case 83:
                            //printf("SomethingElse_No.%d", c);   // 汎用操作子
                            break;
                        case 91:
                            //printf("ExternalEffect");   // 外部エフェクト
                            break;
                        case 92:
                            //printf("Tremolo");   // トレモロ
                            break;
                        case 93:
                            //printf("Chorus");   // コーラス
                            break;
                        case 94:
                            //printf("Celeste");   // セレステ
                            break;
                        case 95:
                            //printf("Phaser");   // フェイザー
                            break;
                        default:
                            ;//printf("##UndefinedType_No.%d", c);   // よくわからないコントロール
                    }
                    //printf(")=%d", (unsigned char)track_chunks[i].data[j]);


//                    if(c != 64)
//                    {
//                        auto it = beat_it->tickSet.begin();
//                        special_temp.clear();
//                        special_temp.push_back(0xb0 + (status & 0x0f));
//                        special_temp.push_back(c);
//                        special_temp.push_back((unsigned char)track_chunks[i].data[j]);
//
//                        while(it != beat_it->tickSet.end())
//                        {
//                            if(it->tickOffset == curOffset)
//                            {
//                                it->specialEvents.push_back(special_temp);
//                                break;
//                            }
//                            else if(it->tickOffset > curOffset) // if add || it+1 == beat_it->tickSet.end() in condition will increase operating time
//                            {
//                                specials_temp.specialEvents.clear();
//                                specials_temp.specialEvents.push_back(special_temp);
//                                specials_temp.tickOffset = curOffset;
//                                it = beat_it->tickSet.insert(it, specials_temp);
//                                break;
//                            }
//                            it++;
//                        }
//                        if(it == beat_it->tickSet.end())
//                        {
//                            specials_temp.specialEvents.clear();
//                            specials_temp.specialEvents.push_back(special_temp);
//                            specials_temp.tickOffset = curOffset;
//                            it = beat_it->tickSet.insert(it, specials_temp);
//                        }
//                    }
                }
                else if(96<=c && c<=119)// 特殊な情報
                {

                    //printf("SpecialPurpose(");
                    switch(c){
                        case 96:
                            //printf("DataIncrement");   // データ・インクリメント
                            break;
                        case 97:
                            //printf("DataDecrement");   // デクリメント
                            break;
                        case 98:
                            //printf("NRPN[LSB]");   // NRPNのLSB
                            break;
                        case 99:
                            //printf("NRPN[MSB]");   // NRPNのMSB
                            break;
                        case 100:
                            //printf("RPN[LSB]");   // RPNのLSB
                            break;
                        case 101:
                            //printf("RPN[MSB]");   // RPNのMSB
                            break;
                        default:
                            ;//printf("##UndefinedType_No.%d", c);   // よくわからないコントロール
                    }
                    //printf(")=%d", (unsigned char)track_chunks[i].data[j]);


                    auto it = beat_it->tickSet.begin();
                    special_temp.clear();
                    special_temp.push_back(0xb0 + (status & 0x0f));
                    special_temp.push_back(c);
                    special_temp.push_back((unsigned char)track_chunks[i].data[j]);

                    while(it != beat_it->tickSet.end())
                    {
                        if(it->tickOffset == curOffset)
                        {
                            it->specialEvents.push_back(special_temp);
                            break;
                        }
                        else if(it->tickOffset > curOffset) // if add || it+1 == beat_it->tickSet.end() in condition will increase operating time
                        {
                            specials_temp.specialEvents.clear();
                            specials_temp.specialEvents.push_back(special_temp);
                            specials_temp.tickOffset = curOffset;
                            it = beat_it->tickSet.insert(it, specials_temp);
                            break;
                        }
                        it++;
                    }
                    if(it == beat_it->tickSet.end())
                    {
                        specials_temp.specialEvents.clear();
                        specials_temp.specialEvents.push_back(special_temp);
                        specials_temp.tickOffset = curOffset;
                        it = beat_it->tickSet.insert(it, specials_temp);
                    }
                }
                else if(120 <= c && c <= 127)// モード・メッセージ
                {
                    //printf("ModeMessage(");
                    switch(c){
                        case 120:
                            //printf("AllSoundOff");   // オール・サウンド・オフ
                            break;
                        case 121:
                            //printf("ResetAllController");   // リセット・オール・コントローラー
                            break;
                        case 122:
                            //printf("LocalControl");   // ローカル・コントロール
                            break;
                        case 123:
                            //printf("AllNoteOff");   // オール・ノート・オフ
                            break;
                        case 124:
                            //printf("OmniOn");   // オムニ・オン
                            break;
                        case 125:
                            //printf("OmniOff");   // オムニ・オフ
                            break;
                        case 126:
                            //printf("MonoModeOn");   // モノモード・オン（ポリモード・オフ）
                            break;
                        case 127:
                            //printf("PolyModeOn");   // ポリモード・オン（モノモード・オフ）
                            break;
                        default:
                            ;//printf("##UndefinedType_No.%d", c);   // よくわからないコントロール
                    }
                    //printf(")=%d", (unsigned char)track_chunks[i].data[j]);

//                    auto it = beat_it->tickSet.begin();
//                    special_temp.clear();
//                    special_temp.push_back(0xb0 + (status & 0x0f));
//                    special_temp.push_back(c);
//                    special_temp.push_back((unsigned char)track_chunks[i].data[j]);
//
//                    while(it != beat_it->tickSet.end())
//                    {
//                        if(it->tickOffset == curOffset)
//                        {
//                            it->specialEvents.push_back(special_temp);
//                            break;
//                        }
//                        else if(it->tickOffset > curOffset) // if add || it+1 == beat_it->tickSet.end() in condition will increase operating time
//                        {
//                            specials_temp.specialEvents.clear();
//                            specials_temp.specialEvents.push_back(special_temp);
//                            specials_temp.tickOffset = curOffset;
//                            it = beat_it->tickSet.insert(it, specials_temp);
//                            break;
//                        }
//                        it++;
//                    }
//                    if(it == beat_it->tickSet.end())
//                    {
//                        specials_temp.specialEvents.clear();
//                        specials_temp.specialEvents.push_back(special_temp);
//                        specials_temp.tickOffset = curOffset;
//                        it = beat_it->tickSet.insert(it, specials_temp);
//                    }
                }
                //printf("");
            }
            else if((status & 0xf0) == 0xc0)// プログラム・チェンジ【ボイスメッセージ】
            {

                //printf("Program Change [%02dch] : ", (status & 0x0f));
                j++;
                //printf("ToneNo=%d", (unsigned char)track_chunks[i].data[j]);
                //cout << delta_count << ' ' << (status & 0x0f) << "tone" << endl;
                sound_temp.push_back(std::make_pair(status & 0x0f, (unsigned char)track_chunks[i].data[j]));

            }
            else if((status & 0xf0) == 0xd0)// チャンネル・プレッシャー【ボイスメッセージ】
            {

                //printf("Channel Pressure [%02dch] : ", (status & 0x0f));
                j++;
                //printf("Pressure=%d", (unsigned char)track_chunks[i].data[j]);
//                special_temp.clear();
//                special_temp.push_back(0xd0 + (status & 0x0f));
//                special_temp.push_back((unsigned char)track_chunks[i].data[j]);
//
//                auto it = beat_it->tickSet.begin();
//                while(it != beat_it->tickSet.end())
//                {
//                    if(it->tickOffset == curOffset)
//                    {
//                        it->specialEvents.push_back(special_temp);
//                        break;
//                    }
//                    else if(it->tickOffset > curOffset) // if add || it+1 == beat_it->tickSet.end() in condition will increase operating time
//                    {
//                        specials_temp.specialEvents.clear();
//                        specials_temp.specialEvents.push_back(special_temp);
//                        specials_temp.tickOffset = curOffset;
//                        it = beat_it->tickSet.insert(it, specials_temp);
//                        break;
//                    }
//                    it++;
//                }
//                if(it == beat_it->tickSet.end())
//                {
//                    specials_temp.specialEvents.clear();
//                    specials_temp.specialEvents.push_back(special_temp);
//                    specials_temp.tickOffset = curOffset;
//                    it = beat_it->tickSet.insert(it, specials_temp);
//                }

            } else if((status & 0xf0) == 0xe0){
                // ピッチ・ベンド・チェンジ【ボイスメッセージ】
                //printf("Pitch Bend Change [%02dch] : ", (status & 0x0f));
                j++;
                //printf("Bend=%hd", mergeChar7bit(track_chunks[i].data[j+1], track_chunks[i].data[j]) - 8192);
                //printf(" (LSB:%d", (unsigned char)track_chunks[i].data[j]);
                j++;
                //printf(", MSB:%d)", (unsigned char)track_chunks[i].data[j]);

                special_temp.clear();
                special_temp.push_back(0xe0 + (status & 0x0f));
                special_temp.push_back((unsigned char)track_chunks[i].data[j - 1]);
                special_temp.push_back((unsigned char)track_chunks[i].data[j]);

                auto it = beat_it->tickSet.begin();
                while(it != beat_it->tickSet.end())
                {
                    if(it->tickOffset == curOffset)
                    {
                        it->specialEvents.push_back(special_temp);
                        break;
                    }
                    else if(it->tickOffset > curOffset) // if add || it+1 == beat_it->tickSet.end() in condition will increase operating time
                    {
                        specials_temp.specialEvents.clear();
                        specials_temp.specialEvents.push_back(special_temp);
                        specials_temp.tickOffset = curOffset;
                        it = beat_it->tickSet.insert(it, specials_temp);
                        break;
                    }
                    it++;
                }
                if(it == beat_it->tickSet.end())
                {
                    specials_temp.specialEvents.clear();
                    specials_temp.specialEvents.push_back(special_temp);
                    specials_temp.tickOffset = curOffset;
                    it = beat_it->tickSet.insert(it, specials_temp);
                }

            }
            else if((status & 0xf0) == 0xf0)// 【システム・メッセージ】
            {
                auto it = beat_it->tickSet.begin();
                std::vector<BYTE> data;
                switch(status & 0x0f)// エクスクルーシブ・メッセージ
                {
                    case 0x00:
                        //printf("F0 Exclusive Message : ");
                        j++;
                        // SysExのデータ長を取得
                        cnt = 0;   // 初期化
                        while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){   // フラグビットが1の間はループ
                            cnt = cnt | c;   // 合成
                            cnt <<= 7;   // 7bit左シフト
                        }
                        cnt = cnt | c;   // 合成
                        //printf(" Length=%u", (unsigned int)cnt);   // SysExのデータ長を取得
                        data.push_back(0xf0);
                        for(k=1; k<=cnt; k++){   // 長さの分だけデータ取得
                            //printf("[%02x]", (unsigned char)track_chunks[i].data[j]);
                            data.push_back((unsigned char)track_chunks[i].data[j]);
                            j++;
                        }
                        //playSound.sysMsg(data);

                        while(it != beat_it->tickSet.end())
                        {
                            if(it->tickOffset == curOffset)
                            {
                                it->sysMessage.push_back(data);
                                break;
                            }
                            else if(it->tickOffset > curOffset) // if add || it+1 == beat_it->tickSet.end() in condition will increase operating time
                            {
                                specials_temp.specialEvents.clear();
                                specials_temp.sysMessage.push_back(data);
                                specials_temp.tickOffset = curOffset;
                                it = beat_it->tickSet.insert(it, specials_temp);
                                specials_temp.sysMessage.clear();
                                break;
                            }
                            it++;
                        }
                        if(it == beat_it->tickSet.end())
                        {
                            specials_temp.specialEvents.clear();
                            specials_temp.sysMessage.push_back(data);
                            specials_temp.tickOffset = curOffset;
                            it = beat_it->tickSet.insert(it, specials_temp);
                            specials_temp.sysMessage.clear();
                        }
                        j--;   // 行き過ぎた分をデクリメント
                        break;

                    case 0x01:   // MIDIタイムコード
                        //printf("MIDI Time Code : ");
                        j++;
                        //printf("(frame/sec/min/hour)=%d", (unsigned char)track_chunks[i].data[j]);
                        break;
                    case 0x02:   // ソング・ポジション・ポインタ
                        //printf("Song Position Pointer : ");
                        j++;
                        //printf("Position=%hd[MIDI beat]", mergeChar7bit(track_chunks[i].data[j+1], track_chunks[i].data[j]));
                        //printf(" (LSB:%d", (unsigned char)track_chunks[i].data[j]);
                        j++;
                        //printf(", MSB:%d)", (unsigned char)track_chunks[i].data[j]);
                        break;
                    case 0x03:   // ソング・セレクト
                        //printf("Song Select : ");
                        j++;
                        //printf("SelectNo=%d", (unsigned char)track_chunks[i].data[j]);
                        break;
                    case 0x06:   // チューン・リクエスト
                        //printf("Tune Request");
                        break;
                    case 0x07:   // エンド・オブ・エクスクルーシブでもあるけども...
                                 // F7ステータスの場合のエクスクルーシブ・メッセージ
                        ////printf("@End of Exclusive");
                        //printf("F7 Exclusive Message : ");
                        j++;

                        // SysExのデータ長を取得
                        cnt = 0;   // 初期化
                        while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){   // フラグビットが1の間はループ
                            cnt = cnt | c;   // 合成
                            cnt <<= 7;   // 7bit左シフト
                        }
                        cnt = cnt | c;   // 合成
                        //printf(" Length=%u", (unsigned int)cnt);   // SysExのデータ長を取得

                        for(k=1; k<=cnt; k++){   // 長さの分だけデータ取得
                            //printf("[%02x]", (unsigned char)track_chunks[i].data[j]);
                            j++;
                        }
                        j--;   // 行き過ぎた分をデクリメント

                        break;
                    case 0x08:   // タイミング・クロック
                        //printf("Timing Clock");
                        break;
                    case 0x0A:   // スタート
                        //printf("Start");
                        break;
                    case 0x0B:   // コンティニュー
                        //printf("Continue");
                        break;
                    case 0x0C:   // ストップ
                        //printf("Stop");
                        break;
                    case 0x0E:   // アクティブ・センシング
                        //printf("Active Sensing");
                        break;
                    //case 0x0F:   // システムリセット（何か間違っている気がする。。。） in midi file 0x0f is meta event, in midi device is system reset?
                    //    //printf("System Reset");
                    case 0x0F:   // メタイベント
                        //printf("Meta Ivent : ");
                        j++;
                        switch((unsigned char)track_chunks[i].data[j]){
                            case 0x00:   // シーケンス番号
                                //printf("Sequence Number=");
                                j += 2;   // データ長の分を通り越す
                                cnt = (unsigned char)track_chunks[i].data[j++];
                                cnt <<= 8;   // 8bit左シフト
                                cnt = cnt | (unsigned char)track_chunks[i].data[j];
                                //printf("%d", cnt);
                                break;
                            case 0x01:   // テキスト[可変長]
                                //printf("Text=");
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("%c", track_chunks[i].data[j]>31 && track_chunks[i].data[j]<127 ? track_chunks[i].data[j] : '?');
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント
                                break;
                            case 0x02:   // 著作権表示[可変長]
                                //printf("Copyright=");
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("%c", track_chunks[i].data[j]>31 && track_chunks[i].data[j]<127 ? track_chunks[i].data[j] : '?');
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント
                                break;
                            case 0x03:   // シーケンス名（曲タイトル）・トラック名[可変長]
                                //printf("Title=");
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("%c", track_chunks[i].data[j]>31 && track_chunks[i].data[j]<127 ? track_chunks[i].data[j] : '?');
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント
                                break;
                            case 0x04:   // 楽器名[可変長]
                                //printf("InstName=");
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("%c", track_chunks[i].data[j]>31 && track_chunks[i].data[j]<127 ? track_chunks[i].data[j] : '?');
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント
                                break;
                            case 0x05:   // 歌詞[可変長]
                                //printf("Words=");
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("%c", track_chunks[i].data[j]>31 && track_chunks[i].data[j]<127 ? track_chunks[i].data[j] : '?');
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント
                                break;
                            case 0x06:   // マーカー[可変長]
                                //printf("Marker=");
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("%c", track_chunks[i].data[j]>31 && track_chunks[i].data[j]<127 ? track_chunks[i].data[j] : '?');
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント
                                break;
                            case 0x07:   // キューポイント[可変長]
                                //printf("CuePoint=");
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("%c", track_chunks[i].data[j]>31 && track_chunks[i].data[j]<127 ? track_chunks[i].data[j] : '?');
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント
                                break;
                            case 0x08:   // プログラム名（音色名）[可変長]
                                //printf("ProgramName=");
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("%c", track_chunks[i].data[j]>31 && track_chunks[i].data[j]<127 ? track_chunks[i].data[j] : '?');
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント
                                break;
                            case 0x09:   // デバイス名（音源名）[可変長]
                                //printf("DeviceName=");
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("%c", track_chunks[i].data[j]>31 && track_chunks[i].data[j]<127 ? track_chunks[i].data[j] : '?');
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント
                                break;
                            case 0x20:   // MIDIチャンネルプリフィックス[1byte]
                                //printf("MidiChannelPrefix=");
                                j += 2;   // データ長の分を通り越す
                                cnt = (unsigned char)track_chunks[i].data[j];
                                //printf("%d", cnt);
                                break;
                            case 0x21:   // ポート指定[1byte]
                                //printf("Port=");
                                j += 2;   // データ長の分を通り越す
                                cnt = (unsigned char)track_chunks[i].data[j];
                                //printf("%d", cnt);
                                break;
                            case 0x2F:   // トラック終端[0byte]
                                //printf("End of Track");
                                j += 1;   // データ長の分を通り越す
                                break;
                            case 0x51:   // テンポ設定[3byte]
                                //printf("TempoSet(usPerQuarterNote)=");
                                j += 2;   // データ長の分を通り越す
                                cnt = (unsigned char)track_chunks[i].data[j++];
                                cnt <<= 8;   // 8bit左シフト
                                cnt = cnt | (unsigned char)track_chunks[i].data[j++];
                                cnt <<= 8;   // 8bit左シフト
                                cnt = cnt | (unsigned char)track_chunks[i].data[j];
                                //printf("%d", cnt);
                                curBpm = (curBpm == 120 ? 60*1e6 / cnt : curBpm);
                                break;
                            case 0x54:   // SMPTEオフセット[5byte]
                                //printf("SMPTE_Offset=");
                                j += 2;   // データ長の分を通り越す
                                cnt = (unsigned char)track_chunks[i].data[j++];
                                switch(cnt & 0xC0){   // フレーム速度
                                    case 0x00:
                                        //printf("24fps");
                                        break;
                                    case 0x01:
                                        //printf("25fps");
                                        break;
                                    case 0x10:
                                        //printf("30fps(DropFrame)");
                                        break;
                                    case 0x11:
                                        //printf("30fps");
                                        break;
                                }
                                //printf(",Hour:%d", (cnt & 0x3F));   // 時間
                                //printf(",Min:%d", (unsigned char)track_chunks[i].data[j]);   // 分
                                j++;
                                //printf(",Sec:%d", (unsigned char)track_chunks[i].data[j]);   // 秒
                                j++;
                                //printf(",Frame:%d", (unsigned char)track_chunks[i].data[j]);   // フレーム
                                j++;
                                //printf(",SubFrame:%d", (unsigned char)track_chunks[i].data[j]);   // サブフレーム
                                break;
                            case 0x58:   // 拍子設定[4byte]
                                //printf("Rhythm=");
                                j += 2;   // データ長の分を通り越す
                                //printf("%d", (unsigned char)track_chunks[i].data[j]);
                                j++;
                                cnt = 1;
                                for(k=0; k<(int)track_chunks[i].data[j]; k++){   // 拍子の分母を算出する
                                    cnt *= 2;   // 2の累乗
                                }
                                //printf("/%d", cnt);

                                curRhythm = cnt;
                                curNum = (unsigned char)track_chunks[i].data[j - 1];
                                beat_temp.setTimeSigniture(music.TPQN, curNum, curRhythm);
                                beat_it->setTimeSigniture(music.TPQN, curNum, curRhythm);

                                j++;
                                //printf(" ClockPerBeat=%d", (unsigned char)track_chunks[i].data[j]);
                                j++;
                                //printf(" 32NotePer4Note=%d", (unsigned char)track_chunks[i].data[j]);
                                break;
                            case 0x59:   // 調設定[2byte]
                                //printf("Key=");
                                j += 2;   // データ長の分を通り越す
                                cnt = (int)track_chunks[i].data[j++];
                                if(c > 0){
                                    //printf("Sharp[%d]", c);
                                } else if(c == 0){
                                    //printf("C");
                                } else {
                                    //printf("Flat[%d]", c);
                                }
                                cnt = (int)track_chunks[i].data[j];
                                if(c == 0){
                                    //printf("_Major");
                                } else {
                                    //printf("_Minor");
                                }

                                break;
                            case 0x7F:   // シーケンサ特定メタイベント
                                //printf("SpecialIvent=");
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("[%02x]", (unsigned char)track_chunks[i].data[j]);
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント
                                break;
                            default :
                                cnt = 0;
                                j += 1;
                                while((c = (unsigned char)track_chunks[i].data[j++]) & 0x80){
                                    cnt = cnt | (c & 0x7F);   // 合成
                                    cnt <<= 7;   // 7bit左シフト
                                }
                                cnt = cnt | (c & 0x7F);   // 合成
                                for(k=1; k<=cnt; k++){
                                    //printf("%c", track_chunks[i].data[j]);
                                    j++;
                                }
                                j--;   // 行き過ぎた分をデクリメント

                        }
                        break;
                    default:
                        ;//printf("# SysEx (Something else...[Status:%02x])", status);   // 見知らぬステータスなら
                }
                //printf("");
            } else {
                //printf("## Something else...[Status:%02x]", status);   // 見知らぬステータスなら
            }
            //printf("\n");
        }

        int beat_num = 1;

        for(const auto &beat : music.beatSet)
        {
            beat_num++;
            for(const auto &tick : beat.tickSet)
            {
                for(const auto &note : tick.notes)
                {
                    if(!note.isOffed)
                    {
                        std::cerr << "Beat: " << beat_num << std::endl;
                        std::cerr << "\tTick Offset: "<< tick.tickOffset << std::endl;
                        std::cerr << "\t\t" << "c " << note.channel << ' '
                                       << "n " << note.number << ' '
                                       << "t " << note.tickSpan << ' '
                                       << "v " << note.velocity << '\n' << std::endl;
                        getchar();
                    }
                }
            }
        }
    }

    // track_chunks,track_chunks[i].dataはcalloc()で領域確保しているので解放し忘れないように！
    for(i = 0; i < tracks; i++)
    {
        free(track_chunks[i].data);
    }
    free(track_chunks);
    getch();
    return 0;
}



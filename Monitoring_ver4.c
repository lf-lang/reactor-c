/*
ファイルの読み書きによるプロセス間通信を行う
inotify系の関数を用いて、書き込みを監視する。
監視対象はファイルで、ファイルの配置場所は
あらかじめfile_path（inotify_fdにしている）として指定しておく。

コマンド、通信用ファイルのパスを先に格納

書き込み内容を読み取った後に内容を消去するように変更

並列実行を考慮した監視を可能にする。以下，並列実行の要素
１．afterを加えることによるreactionの同時実行（同じタグで複数reactionがトリガされる場合に生成）
２．thread生成による並列実行
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <sys/timerfd.h>
#include <sys/select.h>
#include <signal.h>


#define RTI_federate_nodes 2 //RTIを含めたfederate数
#define EVENT_BUF_LEN     (1024 * (EVENT_SIZE + 16))
#define EVENT_SIZE  (sizeof(struct inotify_event))
#define max_cp_num 10


typedef struct check_point_info {
    bool check_do;         //監視実行中フラグ
    bool start_cp;
    bool end_cp;           //プロセスから送信される最後のcheck point番号
    int timer_count;       //各CPのカウント値（deadline配列より抽出）
} cp_info;

typedef struct process_info {
    pid_t pid;             //Pid
    int p_state;           //プロセスの状態
    FILE *fd;              //Pid，プロセス名，CPが書かれるファイルを指すポインタ
    char file_path[256];   //通信用ファイルの配置場所指定
    char command[256];     //コマンドを格納
    char cp_num[256];      //受信したcheck pointの番号
    int deadline[10];      //デッドラインを格納する配列（各reaction単位）
    cp_info* cp_array[max_cp_num];  //各cpに関しての情報を格納
} process_info;

/*
２回目以降のCP受信によりタイムカウントをリセットする関数
*/
void time_count_update(process_info *p_info) {
    int cp_num_value = atoi(p_info -> cp_num);
    
    if(p_info->cp_array[cp_num_value]->end_cp == true) {
        p_info->cp_array[cp_num_value - 1]->check_do = false;
    } else if(p_info->cp_array[cp_num_value]->start_cp == true) {
        p_info->cp_array[cp_num_value]->timer_count = p_info->deadline[cp_num_value];
        p_info->cp_array[cp_num_value]->check_do = true;
    } else {
        p_info->cp_array[cp_num_value]->timer_count = p_info->deadline[cp_num_value];
        p_info->cp_array[cp_num_value]->check_do = true;
        p_info->cp_array[cp_num_value - 1]->check_do = false;
    }    
}

/*
タイマーイベントによるカウントダウン
*/
void count_down(process_info *p_info) {
   //sudoでkillする．system()を用いる
    for(int i =0; i<RTI_federate_nodes; i++) {
        for(int j = 0; j<max_cp_num; j++) {
            if(p_info[i].cp_array[j]->check_do == true) {
                p_info[i].cp_array[j]->timer_count--;
                if(p_info[i].cp_array[j]->timer_count == 0) {
                    char cmd[50];
                    sprintf(cmd, "sudo kill %d", p_info[i].pid);
                    printf("will kill PID: %d\n", p_info[i].pid);

                    int result = system(cmd);
                    if(result == -1) {
                        perror("system kill");
                    } else {
                        printf("QM: kill success\n");
                    }
                }
            }
        }
    }
}

/*
プロセスの情報を先に格納しておく（他に良い方法がありそう）
*/
void p_info_write(process_info *p_info) {
    //p_infoのメンバー変数を初期化
    for (int i = 0; i < RTI_federate_nodes; ++i) {
        // 各 process_info 構造体のメンバーを初期化
        p_info[i].pid = 0;
        p_info[i].p_state = 0;
        p_info[i].fd = NULL;
        memset(p_info[i].file_path, 0, sizeof(p_info[i].file_path));
        memset(p_info[i].command, 0, sizeof(p_info[i].command));
        memset(p_info[i].cp_num, 0, sizeof(p_info[i].cp_num));
        memset(p_info[i].deadline, 0, sizeof(p_info[i].deadline));

        // cp_array の各ポインタを NULL に初期化
        for (int j = 0; j < max_cp_num; ++j) {
            p_info[i].cp_array[j] = NULL;
        }

        // cp_info 構造体のインスタンスを動的に作成して初期化
        for (int j = 0; j < max_cp_num; ++j) {
            p_info[i].cp_array[j] = (cp_info*)malloc(sizeof(cp_info));
            if (p_info[i].cp_array[j] == NULL) {
                perror("Failed to allocate memory for cp_info");
            }
            p_info[i].cp_array[j]->check_do = false;
            p_info[i].cp_array[j]->start_cp = false;
            p_info[i].cp_array[j]->end_cp = false;
            p_info[i].cp_array[j]->timer_count = 0;
        }
    }

    //コマンドを格納
    strcpy(p_info[0].command, "taskset -c 1 RTI -n 1 & echo $! > /home/yoshinoriterazawa/LF/RTI.txt");
    strcpy(p_info[1].command, "taskset -c 0,2 /home/yoshinoriterazawa/LF/fed-gen/filewrite/bin/federate__writer & echo $! > /home/yoshinoriterazawa/LF/federate_writer.txt");

    //通信用ファイルのパスを格納
    strcpy(p_info[0].file_path, "/home/yoshinoriterazawa/LF/RTI.txt");
    strcpy(p_info[1].file_path, "/home/yoshinoriterazawa/LF/federate_writer.txt");

    //実行シーケンスの最初のCPを設定
    p_info[1].cp_array[0]->start_cp = true;
    p_info[1].cp_array[4]->start_cp = true;
    

    //end_cp_num（最後のCP番号）を格納（とりあえずfederateのみ）
    p_info[1].cp_array[3]->end_cp = true;
    p_info[1].cp_array[7]->end_cp = true;

    //デッドラインを格納（とりあえずfederateのみ）
    p_info[1].deadline[0] = 1010;
    p_info[1].deadline[1] = 100;
    p_info[1].deadline[2] = 1010;
    p_info[1].deadline[4] = 1010;
    p_info[1].deadline[5] = 100;
    p_info[1].deadline[6] = 1010;
}

/*
プログラムを実行する
*/
void executeProgram(process_info *p_info, int wd[], int *inotify_fd) {
    //監視のためにinotifyインスタンスを生成
    if((*inotify_fd = inotify_init()) == -1) {
        perror("Error inotify_init");
        exit(EXIT_FAILURE);
    }
    
    for(int i =0; i < RTI_federate_nodes; i++) {
        sleep(1);
        int result = system(p_info[i].command);
        if (result == -1) {
            //デバッグ
            perror("Error executing program");
        } else {
            //デバッグ
            printf("QM: Command %s executed successfully\n", p_info[i].command);
            
            //Pid取得
            p_info[i].fd = fopen(p_info[i].file_path, "r+");
            if(p_info[i].fd == NULL) {
                perror("QM: Faild make file");
            } else {
                fscanf(p_info[i].fd, "%d", &(p_info[i].pid));
                printf("QM: scanned pid %d\n", p_info[i].pid);
                int fd = fileno(p_info[i].fd);
                if(ftruncate(fd, 0) != 0) {
                    perror("Failed to truncate file\n");
                    close(fd);
                }
            }
            fclose(p_info[i].fd);
            printf("QM: file closed\n");
            

            //通信用ファイルを監視対象に設定
            wd[i] = inotify_add_watch(*inotify_fd, p_info[i].file_path, IN_MODIFY);
            if(wd[i] == -1) {
                perror("QM: Error inotify_add_watch");
                exit(EXIT_FAILURE);
            } else {
                printf("QM: 監視対象設定完了\n");
            }
        }
    }
}

/*
CPの書き込みを待ち、読み込む関数
*/
void watch_cp_write(process_info *p_info, int wd[], struct itimerspec timer, int *inotify_fd, int *timer_fd, fd_set *rdfs, int max_fd) {
    char buffer[EVENT_BUF_LEN]; //イベント格納バッファ
    char line[256];
    timerfd_settime(*timer_fd, 0, &timer, NULL);
    //int count = 0;  //デバッグ用
    
    while (1) {
        // ファイル変更イベント，タイマーイベントを待つ
        fd_set tmp_fds = *rdfs;
        int ret = select(max_fd, &tmp_fds, NULL, NULL, NULL);
        if (ret == -1) {
            perror("select");
            break;
        }

        if (FD_ISSET(*timer_fd, &tmp_fds)) {
            // タイマーイベントの処理
            uint64_t expirations;
            if (read(*timer_fd, &expirations, sizeof(expirations)) == -1) {
                perror("read");
            } else {
                count_down(p_info);
            }
        }

        if (FD_ISSET(*inotify_fd, &tmp_fds)) {
            //変更イベントを読み取る。readを使うことで一度に読み取り
            int length = read(*inotify_fd, buffer, EVENT_BUF_LEN);  //lengthはバイト数が入る。readで一度に読み取り
            if (length < 0) {
                perror("QM: read event");
                exit(EXIT_FAILURE);
            }
            //count++;

            //読み込んだ変更イベントを1つずつ処理する
            // event_point : 変更内容を順に取得するために使用．event毎の先頭アドレスを指す
            int event_point = 0;
            while (event_point < length) {
                struct inotify_event *event = (struct inotify_event *) &buffer[event_point];  //キャストすることで、それぞれのイベントの先頭アドレスを指定
                // 変更のみ対応
                if (event->mask & IN_MODIFY) {
                    // 変更されたファイルを探す
                    for(int i = 0; i< RTI_federate_nodes; i++) {
                        if(event->wd == wd[i]) {
                            p_info[i].fd = fopen(p_info[i].file_path, "r+");
                            if (!p_info[i].fd) {
                                perror("QM: Error opening file");
                                continue;
                            }

                            while (flock(fileno(p_info[i].fd), LOCK_EX) == -1) { // 排他ロックを取得
                                perror("QM: Failed to lock file");
                                usleep(1000); // 待機してリトライ
                            }

                            int last_line = 0;
                            while (fgets(line, sizeof(line), p_info[i].fd) != NULL) {
                                last_line = 1;
                                sscanf(line, "cp: %s", p_info[i].cp_num);
                                printf("QM: CP_num %s\n", p_info[i].cp_num);

                                //CP受信による実行時間監視の開始
                                time_count_update(&p_info[i]);
                                //実行デバック
                                int cp_num_value = atoi(p_info[i].cp_num);
                                printf("QM: updated count %d\n", p_info[i].cp_array[cp_num_value]->timer_count);
                            }

                            if (last_line) {
                                int fd = fileno(p_info[i].fd);
                                if (ftruncate(fd, 0) != 0) {
                                    perror("Failed to truncate file\n");
                                }
                                rewind(p_info[i].fd); // ファイルポインタを先頭に戻す
                            }

                            fclose(p_info[i].fd);
                        }
                    }
                    event_point += EVENT_SIZE + event->len;
                }
            }
        }
    }
}

int main() {
    process_info p_info[RTI_federate_nodes]; //RTI, federateの情報を格納する構造体配列の作成
    int wd[RTI_federate_nodes]; //RTI, federateを監視するためのウォッチディスクリプタ配列
    int inotify_fd;  //inotifyインスタンスのファイルディスクリプタ
    fd_set rdfs;  //

    /*カウントタイマー作成*/
    int timer_fd;
    struct itimerspec timer;
    /*カウントタイマーの時間を設定．起動間隔1ms*/
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_nsec = 1000000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_nsec = 1000000;
    timer_fd = timerfd_create(CLOCK_REALTIME, 0);
    if (timer_fd == -1) {
        perror("timerfd_create");
        return EXIT_FAILURE;
    }


    //必要情報を先に記録する
    p_info_write(p_info);

    //RTI, federateの初期起動，コマンドを基に実行
    executeProgram(p_info, wd, &inotify_fd);

    //最大のファイルディスクリプタを設定
    int max_fd = (inotify_fd > timer_fd ? inotify_fd : timer_fd) + 1;
    FD_ZERO(&rdfs);
    FD_SET(inotify_fd, &rdfs);
    FD_SET(timer_fd, &rdfs);

    //それぞれのプロセスからの通信を待つ
    watch_cp_write(p_info, wd, timer, &inotify_fd, &timer_fd, &rdfs, max_fd);

    return 0;
}
/*
ファイルの読み書きによるプロセス間通信を行う
inotify系の関数を用いて、書き込みを監視する。
監視対象はファイルで、ファイルの配置場所は
あらかじめfile_path（inotify_fdにしている）として指定しておく。

コマンド、通信用ファイルのパスを先に格納

書き込み内容を読み取った後に内容を消去するように変更
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
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


typedef struct process_info {
    char p_name[256];      //プロセス名
    pid_t pid;             //Pid
    int p_state;           //プロセスの状態
    FILE *fd;              //Pid，プロセス名，CPが書かれるファイルを指すポインタ
    char file_path[256];   //通信用ファイルの配置場所指定
    char command[256];     //コマンドを格納
    bool check_do;         //監視実行中フラグ
    char cp_num[256];      //受信したcheck pointの番号
    char end_cp_num[256];  //プロセスから送信される最後のcheck point番号
    int timer_count;       //各CPのカウント値（deadline配列より抽出）
    int deadline[10];          //デッドラインを格納する配列（各reaction単位）
} process_info;


/*
初期CP受信によりタイムカウントをスタートする関数
*/
void time_count_start(process_info *p_info) {
    p_info->timer_count = p_info->deadline[0];
    p_info->check_do = true;
}

/*
２回目以降のCP受信によりタイムカウントをリセットする関数
*/
void time_count_decrement(process_info *p_info) {
    int cp_num_value = atoi(p_info -> cp_num);
    
    if(strcmp(p_info->end_cp_num, p_info->cp_num) == 0) {
        p_info->check_do = false;
    } else {
        p_info->timer_count = p_info->deadline[cp_num_value];
    }
}

/*
タイマーイベントによるカウントダウン
*/
void count_down(process_info *p_info) {
    //kill関数を使う方法　管理Appをsudoで実行している場合、子プロセス（federateなど）はsudoでしか停止できない？
    /*
    for(int i =0; i<RTI_federate_nodes; i++) {
        if(p_info[i].check_do == true) {
            p_info[i].timer_count--;
            if(p_info[i].timer_count == 0) {
                if(kill(p_info[i].pid, SIGKILL) == -1) {
                    perror("kill");
                    // エラーハンドリングなどを記述
                } else {
                    //デバッグ
                    printf("QM: do kill\n");
                }
            }
        }
    }
    */
   //sudoでkillする．system()を用いる
    for(int i =0; i<RTI_federate_nodes; i++) {
        //printf("check_do[%d]: %d, timer_count[%d]: %d\n", i, p_info[i].check_do, i, p_info[i].timer_count); // デバッグ用プリント
        if(p_info[i].check_do == true) {
            p_info[i].timer_count--;
            if(p_info[i].timer_count == 0) {
                char cmd[50];
                sprintf(cmd, "sudo kill %d", p_info[i].pid);
                printf("will kill PID: %d, P_name: %s\n", p_info[i].pid, p_info[i].p_name);
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

/*
プロセスの情報を先に格納しておく（他に良い方法がありそう）
*/
void p_info_write(process_info *p_info) {
    //コマンドを格納
    strcpy(p_info[0].command, "taskset -c 0 RTI -n 1 &");
    strcpy(p_info[1].command, "taskset -c 2 /home/yoshinoriterazawa/LF/fed-gen/filewrite/bin/federate__writer &");

    //通信用ファイルのパスを格納
    strcpy(p_info[0].file_path, "/home/yoshinoriterazawa/LF/RTI.txt");
    strcpy(p_info[1].file_path, "/home/yoshinoriterazawa/LF/federate_writer.txt");

    //check_do(カウントダウン実行の有無)をfalseに設定
    p_info[0].check_do = false;
    p_info[1].check_do = false;

    //end_cp_num（最後のCP番号）を格納（とりあえずfederateのみ）
    strcpy(p_info[1].end_cp_num, "2");

    //デッドラインを格納（とりあえずfederateのみ）
    p_info[1].deadline[0] = 1010;
    p_info[1].deadline[1] = 1000;
    //p_info[1].deadline[2] = 1010;
}

/*
Pidとプロセス名を取得し、process_infoに格納する
*/
void wait_pid_and_pname(process_info *p_info, int wd[], int inotify_fd) {
    char buffer[EVENT_BUF_LEN]; //イベント格納バッファ
    int pid_as_int;
    int get_count = 0; // Pid，プロセス名の取得数を記録

    printf("QM: wait_pid_and_pname start\n");
    fflush(stdout);

    //　全てのRTI，federateからPid，プロセス名を取得するまで繰り返す。
    while (get_count < RTI_federate_nodes) {
        //変更イベントを読み取る。readを使うことで一度に読み取り
        int length = read(inotify_fd, buffer, EVENT_BUF_LEN);  //lengthはバイト数が入る。readで一度に読み取り
        if (length < 0) {
            perror("QM: read event");
            exit(EXIT_FAILURE);
        }

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
                        p_info[i].fd = fopen(p_info[i].file_path, "r");
                        if (!p_info[i].fd) {
                            perror("QM: Error opening file");
                            continue;
                        }

                        if (fgets(buffer, sizeof(buffer), p_info[i].fd) != NULL) {
                            // デバッグ表示
                            printf("QM: Data read from file by QM: %s\n", buffer);
                            // Pid，プロセス名が記録されていない場合は，読み込んだPid，プロセス名を記録
                            if(sscanf(buffer, "process_name:%[^,],pid:%d", p_info[i].p_name, &pid_as_int) == 2) {
                                p_info[i].pid = (pid_t)pid_as_int;
                                printf("QM: read PID %d\n", p_info[i].pid);
                                get_count++;
                            } else {
                                perror("QM: failed get pid");
                            }
                        } else {
                            printf("QM: Error reading file %d\n", i);
                        }
                        fclose(p_info[i].fd);
                        printf("QM: プロセス情報受信数 %d\n", get_count);
                    }
                }
                event_point += EVENT_SIZE + event->len;
            }
        }
    }

    //関数終了デバッグ
    //printf("wait_pid_and_pname End\n");
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


            //pidの取得
            //p_info[i].pid = system("$!");
            //デバッグ
            //printf("QM: pid %d\n", p_info[i].pid);

            
            //通信用のファイルを生成
            p_info[i].fd = fopen(p_info[i].file_path, "w");
            if(p_info[i].fd == NULL) {
                perror("QM: Faild make file");
            } else {
                printf("QM: Make file success\n");
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
    //すべてのRTI，federateのPid，プロセス名を取得
    wait_pid_and_pname(p_info, wd, *inotify_fd);
}


/*
CPの書き込みを待ち、読み込む関数
*/
void watch_cp_write(process_info *p_info, int wd[], struct itimerspec timer, int *inotify_fd, int *timer_fd, fd_set *rdfs, int max_fd) {
    char buffer[EVENT_BUF_LEN]; //イベント格納バッファ
    timerfd_settime(*timer_fd, 0, &timer, NULL);
    int count = 0;
    
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
            count++;

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

                            if (fgets(buffer, sizeof(buffer), p_info[i].fd) != NULL) {
                                // デバッグ表示
                                //printf("QM: Data read from file by QM: %s", buffer);
                                //printf("QM: デバッグカウント %d\n", count);
                                
                                //CP情報を取得する
                                sscanf(buffer, "cp: %s", p_info[i].cp_num);
                                printf("QM: CP_num %s\n", p_info[i].cp_num);

                                //CP受信による実行時間監視の開始
                                if(strcmp(p_info[i].cp_num, "0") == 0) {
                                    time_count_start(&p_info[i]);
                                    printf("QM: counter %d\n", p_info[i].timer_count);
                                } else {
                                    printf("QM: counter %d\n", p_info[i].timer_count);
                                    time_count_decrement(&p_info[i]);
                                }

                                int fd =fd = fileno(p_info[i].fd);
                                if(ftruncate(fd, 0) != 0) {
                                    perror("Failed to truncate file\n");
                                    close(fd);
                                }

                            } else {
                                //printf("QM: Error reading file %d\n", i);
                                //printf("QM: デバッグカウント %d\n", count);
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
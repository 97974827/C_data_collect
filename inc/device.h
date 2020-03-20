//
// Created by 82105 on 2020-03-13.
//

/*     Device Header File      */

#ifndef DATA_COLLECT_DEVICE_H
#define DATA_COLLECT_DEVICE_H

int openSerial(char *);
int closeSerial(int);
void setTime();
void getDeviceState(int);
char *rePlaceString(char *);
void noSaveCutString(char *);
void operationCutString(char *, struct tm *);
void selfSaveCutString(char *, struct tm *);
char *getCheckSum(char *);
char *timeToString(struct tm *);


// 저장값 없는 동작상태 저장 구조체 - 카드충전기 변수 추가해야함
typedef struct _no_save_device_state{
    char m_stx[3];
    char m_data_len[4];
    char m_cmd[3];
    char m_type[3];
    char m_addr[3];
    char m_state[2]; // self, garage
    char m_cash[8];
    char m_card[8];
    char m_master[8];
    char m_version[5];
    char m_check_sum[3];
    char m_etx[3];
} NOSAVESTATE;

// 동작 중일때 정보 저장 구조체
typedef struct _operation_device_state{
    char m_stx[3];
    char m_data_len[4];
    char m_cmd[3];
    char m_type[3];
    char m_addr[3];
    char m_state[2]; // self, garage
    char m_hour[3];
    char m_minute[3];
    char m_second[3];
    char m_current_cash[6];
    char m_current_card[6];
    char m_current_master[6];
    char m_use_cash[6];
    char m_use_card[6];
    char m_use_master[6];
    char m_use_time[5];
    char m_card_num[9];
    char m_check_sum[3];
    char m_etx[3];
} OPERATIONSTATE;

// 저장값 있는 동작상태 저장 구조체
typedef struct _save_device_state{
    char m_stx[3];
    char m_data_len[4];
    char m_cmd[3];
    char m_type[3];
    char m_addr[3];
    char m_save_cnt[4];
    char m_year[3];
    char m_month[3];
    char m_day[3];
    char m_start_hour[3];
    char m_start_minute[3];
    char m_start_second[3];
    char m_end_hour[3];
    char m_end_minute[3];
    char m_end_second[3];
    char m_card_num[9];
    char m_remain_money[6];
    char m_use_card[5];
    char m_use_cash[5];
    char m_use_master[5];
    char m_self_time[5];
    char m_form_time[5];
    char m_under_time[5];
    char m_coating_time[5];
    char m_check_sum[3];
    char m_etx[3];
} SAVESTATE;

#endif //DATA_COLLECT_DEVICE_H

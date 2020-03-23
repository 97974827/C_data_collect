//
// Created by 82105 on 2020-03-13.
//
/*     Device Source File      */

extern NOSAVESTATE *No_save_state;
extern OPERATIONSTATE *Operation_state;
extern SAVESTATE *Save_state;


int openSerial(char *serial_port){
    printf("serial 열기\n");
    struct termios oldtio, newtio;
    int fd; // 파일 디스크립터
    fd = open(serial_port, O_RDWR | O_NOCTTY);
    if(fd < 0){
        perror("Serial Device Open Fail ");
        return -1;
    }

    tcgetattr(fd, &oldtio);         // save current serial port settings
    bzero(&newtio, sizeof(newtio)); // clear struct for new port settings

    newtio.c_cflag = BAUD_RATE | CRTSCTS | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR | ICRNL;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;  // 문자 사이의 timer를 disable
    newtio.c_cc[VMIN] = 0;   // 최소 5 문자 받을 때까진 blocking

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);
    return fd;
}

// 프로그램 시작 최초 1회 실행,
void setTime(int serial_port){
    printf("\nsetTime\n");
    char buffer[255] = "\0";
    char *replace_str = 0;
    char year[5] = "\0";
    char month[3] = "\0";
    char day[3] = "\0";
    char hour[3] = "\0";
    char minute[3] = "\0";
    char second[3] = "\0";

    time_t std_time;      // time_t 값에서 표준시간 지역 시간값 구함
    struct tm *time_info; // 시간 정보 구조체
    time(&std_time);      // time() : 현재시간 판별
    time_info = localtime(&std_time); // localtime() : tm 구조체에 시간을 변환함
    sprintf(year, "%04d", time_info->tm_year+1900);
    int j=0;
    for(int i=0; j<strlen(year); i++){
        if(i>=0 && i<2) continue;
        year[j] = year[i];
        j++;
    }
    sprintf(month, "%02d", time_info->tm_mon+1);
    sprintf(day, "%02d", time_info->tm_mday);
    sprintf(hour, "%02d", time_info->tm_hour);
    sprintf(minute, "%02d", time_info->tm_min);
    sprintf(second, "%02d", time_info->tm_sec);

//    printf("year : %s\n", year);
//    printf("month : %s\n", month);
//    printf("day : %s\n", day);
//    printf("hour : %s\n", hour);
//    printf("minute : %s\n", minute);
//    printf("second : %s\n", second);

    strcpy(buffer, "GL029TS");
    strcat(buffer, "01"); // 공급업체
    strcat(buffer, "00"); // 장비분류
    strcat(buffer, "01"); // 장비번호
    strcat(buffer, year);
    strcat(buffer, month);
    strcat(buffer, day);
    strcat(buffer, hour);
    strcat(buffer, minute);
    strcat(buffer, second);

    replace_str = getCheckSum(buffer);
    strcat(buffer, replace_str);
    strcat(buffer, "CH");

    int write_res = write(serial_port, buffer, strlen(buffer));
    printf("write_length : %d\n", write_res);
    if (write_res < 0){
        perror("485 getDeviceState Write Err ");
        return;
    }
    printf("Time TX : %s\n", buffer);
    sleep(1);
    int read_res = read(serial_port, buffer, 255);
    printf("read_length  : %d\n", read_res);
    if (read_res < 0){
        perror("485 getDeviceState Read Err");
        return;
    }
    printf("Time RX : %s\n", buffer);

}

void getDeviceState(int serial_port){
    // 저장값 없을때 : self=garage , 진공/매트/리더 같음
    // 시리얼 송신값 수정 해야할것 : (DB조회) 공급업체, 기기종류, 기기주소
    printf("\ngetDeviceState\n");
    char buffer[255] = "\0"; //  프로토콜 송수신 값
    char *replace_str = 0;

    time_t std_time;      // time_t 값에서 표준시간 지역 시간값 구함
    struct tm *time_info; // 시간 정보 구조체
    time(&std_time);      // time() : 현재시간 판별
    time_info = localtime(&std_time); // localtime() : tm 구조체에 시간을 변환함

    strcpy(buffer, "GL017RD");
    strcat(buffer, "01"); // 공급업체
    strcat(buffer, "00"); // 장비분류
    strcat(buffer, "01"); // 장비번호

    replace_str = getCheckSum(buffer); // Err : 체크섬이 잘릴때가 있음
    strcat(buffer, replace_str);
    strcat(buffer, "CH");

    int write_res = write(serial_port, buffer, strlen(buffer));
    printf("write_length : %d\n", write_res);
    if (write_res < 0){
        perror("485 getDeviceState Write Err ");
        return;
    }
    printf("state TX : %s\n", buffer);
    sleep(1);
    int read_res = read(serial_port, buffer, 255);
    printf("read_length  : %d\n", read_res);
    if (read_res < 0){
        perror("485 getDeviceState Read Err");
        return;
    }
    printf("state RX : %s\n", buffer);
    replace_str = rePlaceString(buffer);
    printf("replace_str : %s\n", replace_str); // 수신 프로토콜 전체 문자 포인터 변수

    // 설정된 장비 종류와 수량에 맞게 조건 설정 -> 상태에 따라 조건 새로 지정 해야함
    // 세차장비 / 충전장비 나눠서 지정 해야함
    char *cmd_search_ptr = 0;       // cmd 검색 문자열 검색을 위한 포인터 변수
    char *etx_search_ptr = 0;       // etx 검색 문자열 검색을 위한 포인터 변수
    char *check_search_ptr = 0;     // checksum 문자열 검색을 위한 포인터 변수

    char cmd_arr_value[3] = "\0";   // 검색 문자열 저장
    char etx_arr_value[3] = "\0";   // 검색 문자열 저장
    char check_arr_value[3] = "\0"; // 체크섬 계산 확인 문자열 저장

    // self / garage 저장값 없을 떄 대기동작 -> DB gl_wash_total 테이블에 데이터 업데이트
    if (strlen(replace_str) == 41) {
        cmd_search_ptr = strstr(replace_str, "SN"); // search_ptr : SN00010000000000002700000000250127CH
        etx_search_ptr = strstr(replace_str, "CH");

        check_search_ptr = getCheckSum(replace_str);
        strncpy(check_arr_value, etx_search_ptr-2, 2);

        strncpy(cmd_arr_value, cmd_search_ptr, 2);  // char_value : SN
        strncpy(etx_arr_value, etx_search_ptr, 2);

        if (cmd_search_ptr!=NULL && etx_search_ptr!=NULL && check_search_ptr!=NULL) {
            if (strcmp(cmd_arr_value, "SN")==0 && strcmp(etx_arr_value, "CH")==0 && strcmp(check_arr_value, check_search_ptr)==0){
                noSaveCutString(replace_str);
            } /*else {
                perror("485 getDeviceState Err ");
                return;
            }*/
        }
        else {
            perror("485 getDeviceState nosave Null Pointer Exception ");
            return;
        }
    }
    // self / garage 동작 중 상태 저장 -> DB에서 정지, 등록카드 검사 -> 정지카드면 현재 카드 사용중지 프로토콜 송신
    if (strlen(replace_str) == 64) {
        cmd_search_ptr = strstr(replace_str, "SR");
        //cmd_search_ptr = strstr(replace_str, "GR");
        etx_search_ptr = strstr(replace_str, "CH");

        check_search_ptr = getCheckSum(replace_str);
        strncpy(check_arr_value, etx_search_ptr-2, 2);

        strncpy(cmd_arr_value, cmd_search_ptr, 2);
        strncpy(etx_arr_value, etx_search_ptr, 2);

        if (cmd_search_ptr!=NULL && etx_search_ptr!=NULL){// && check_search_ptr!=NULL){
            if(strcmp(cmd_arr_value, "SR")==0 && strcmp(etx_arr_value, "CH")==0){// && strcmp(check_arr_value, check_search_ptr)==0) {
                operationCutString(replace_str, time_info);
            } else {
                perror("485 getDeviceState Err ");
                return;
            }
        }
        else {
            perror("485 getDeviceState Operation Null Pointer Exception ");
            return;
        }
    }
    // 셀프 저장값 있을 때 대기 동작
    if (strlen(replace_str) == 77) {
        cmd_search_ptr = strstr(replace_str, "SW");
        etx_search_ptr = strstr(replace_str, "CH");

        strncpy(cmd_arr_value, cmd_search_ptr, 2);
        strncpy(etx_arr_value, etx_search_ptr, 2);

        check_search_ptr = getCheckSum(replace_str);
        strncpy(check_arr_value, etx_search_ptr-2, 2);

        if (cmd_search_ptr!=NULL && etx_search_ptr!=NULL){
//            printf("cmd_arr_value   : %s\n", cmd_arr_value);
//            printf("etx_arr_value   : %s\n", etx_arr_value);
//            printf("check_arr_value : %s\n", check_arr_value);
//            printf("check_search_ptr: %s\n", check_search_ptr);
            int res = strcmp(check_arr_value, check_search_ptr);
            //printf("strcmp compare value : %d\n", res);
            if(strcmp(cmd_arr_value, "SW")==0 && strcmp(etx_arr_value, "CH")==0) {
                selfSaveCutString(replace_str, time_info);
                okSign(serial_port);
            } //else {
//                perror("485 getDeviceState Err ");
//                return;
//            }
        }
        else {
            perror("485 getDeviceState Saving self Null Pointer Exception ");
            return;
        }

    }
}

// self / garage 저장값 없을때
void noSaveCutString(char *str){
    printf("def 셀프 저장값 없음\n");
    char stx[3] = "\0";
    char data_len[4] = "\0";
    char cmd[3] = "\0";
    char div[3] = "\0";
    char addr[3] = "\0";
    char state[2] = "\0";
    char cash[8] = "\0";
    char card[8] = "\0";
    char master[8] = "\0";
    char version[5] = "\0";
    char checksum[3] = "\0";
    char etx[3] = "\0";

    strncpy(stx, str, sizeof(stx)-1);
    str+=sizeof(stx)-1;

    strncpy(data_len, str, sizeof(data_len)-1);
    str+=sizeof(data_len)-1;

    strncpy(cmd, str, sizeof(cmd)-1);
    str+=sizeof(cmd)-1;

    strncpy(div, str, sizeof(div)-1);
    str+=sizeof(div)-1;

    strncpy(addr, str, sizeof(addr)-1);
    str+=sizeof(addr)-1;

    strncpy(state, str, sizeof(state)-1);
    str+=sizeof(state)-1;

    strncpy(cash, str, sizeof(cash)-1);
    str+=sizeof(cash)-1;

    strncpy(card, str, sizeof(card)-1);
    str+=sizeof(card)-1;

    strncpy(master, str, sizeof(master)-1);
    str+=sizeof(master)-1;

    strncpy(version, str, sizeof(version)-1);
    str+=sizeof(version)-1;

    strncpy(checksum, str, sizeof(checksum)-1);
    str+=sizeof(checksum)-1;

    strncpy(etx, str, sizeof(etx)-1);

    No_save_state = (NOSAVESTATE *)malloc(sizeof(NOSAVESTATE));
    strcpy(No_save_state->m_stx, stx); // 값만 복사
    strcpy(No_save_state->m_data_len, data_len);
    strcpy(No_save_state->m_cmd, cmd);
    strcpy(No_save_state->m_type, div);
    strcpy(No_save_state->m_addr, addr);
    strcpy(No_save_state->m_state, state);
    strcpy(No_save_state->m_cash, cash);
    strcpy(No_save_state->m_card, card);
    strcpy(No_save_state->m_master, master);
    strcpy(No_save_state->m_version, version);
    strcpy(No_save_state->m_check_sum, checksum);
    strcpy(No_save_state->m_etx, etx);

    free(No_save_state);
}

// self / garage 동작상태 저장 -> 실시간으로 보여줌
void operationCutString(char *str, struct tm *time_info){
    printf("def 셀프 동작중\n");
    char stx[3] = "\0";
    char data_len[4] = "\0";
    char cmd[3] = "\0";
    char type[3] = "\0";
    char addr[3] = "\0";
    char state[2] = "\0"; // self, garage
    char hour[3] = "\0";
    char minute[3] = "\0";
    char second[3] = "\0";
    char current_cash[6] = "\0";
    char current_card[6] = "\0";
    char current_master[6] = "\0";
    char use_cash[6] = "\0";
    char use_card[6] = "\0";
    char use_master[6] = "\0";
    char use_time[5] = "\0";
    char card_num[9] = "\0";
    char check_sum[3] = "\0";
    char etx[3] = "\0";

    strncpy(stx, str, sizeof(stx)-1);
    str += sizeof(stx)-1;

    strncpy(data_len, str, sizeof(data_len)-1);
    str += sizeof(data_len)-1;

    strncpy(cmd, str, sizeof(cmd)-1);
    str += sizeof(cmd)-1;

    strncpy(type, str, sizeof(type)-1);
    str += sizeof(type)-1;

    strncpy(addr, str, sizeof(addr)-1);
    str += sizeof(addr)-1;

    strncpy(state, str, sizeof(state)-1);
    str += sizeof(state)-1;

    strncpy(hour, str, sizeof(hour)-1);
    printf("start_time : %s시", hour);
    str += sizeof(hour)-1;

    strncpy(minute, str, sizeof(minute)-1);
    printf(" %s분", minute);
    str += sizeof(minute)-1;

    strncpy(second, str, sizeof(second)-1);
    printf(" %s초\n", second);
    str += sizeof(second)-1;

    strncpy(current_cash, str, sizeof(current_cash)-1);
    str += sizeof(current_cash)-1;

    strncpy(current_card, str, sizeof(current_card)-1);
    str += sizeof(current_card)-1;

    strncpy(current_master, str, sizeof(current_master)-1);
    str += sizeof(current_master)-1;

    strncpy(use_cash, str, sizeof(use_cash)-1);
    str += sizeof(use_cash)-1;

    strncpy(use_card, str, sizeof(use_card)-1);
    str += sizeof(use_card)-1;

    strncpy(use_master, str, sizeof(use_master)-1);
    str += sizeof(use_master)-1;

    strncpy(use_time, str, sizeof(use_time)-1);
    str += sizeof(use_time)-1;

    strncpy(card_num, str, sizeof(card_num)-1);
    str += sizeof(card_num)-1;

    strncpy(check_sum, str, sizeof(check_sum)-1);
    str += sizeof(check_sum)-1;

    strncpy(etx, str, sizeof(etx)-1);

    Operation_state = (OPERATIONSTATE *)malloc(sizeof(OPERATIONSTATE));
    strcpy(Operation_state->m_stx, stx); // 값만 복사
    strcpy(Operation_state->m_data_len, data_len);
    strcpy(Operation_state->m_cmd, cmd);
    strcpy(Operation_state->m_type, type);
    strcpy(Operation_state->m_addr, addr);
    strcpy(Operation_state->m_state, state);
    strcpy(Operation_state->m_hour, hour);
    strcpy(Operation_state->m_minute, minute);
    strcpy(Operation_state->m_second, second);
    strcpy(Operation_state->m_current_cash, current_cash);
    strcpy(Operation_state->m_current_card, current_card);
    strcpy(Operation_state->m_current_master, current_master);
    strcpy(Operation_state->m_use_cash, use_cash);
    strcpy(Operation_state->m_use_card, use_card);
    strcpy(Operation_state->m_use_master, use_master);
    strcpy(Operation_state->m_use_time, use_time);
    strcpy(Operation_state->m_card_num, card_num);
    strcpy(Operation_state->m_check_sum, check_sum);
    strcpy(Operation_state->m_etx, etx);


    free(Operation_state);
}

void selfSaveCutString(char *str, struct tm *time_info){
    printf("\ndef 셀프 저장값 있음\n");
    char stx[3]="\0";
    char data_len[4]="\0";
    char cmd[3]="\0";
    char type[3]="\0";
    char addr[3]="\0";
    char save_cnt[4]="\0";
    char year[3]="\0";
    char month[3]="\0";
    char day[3]="\0";
    char start_hour[3]="\0";
    char start_minute[3]="\0";
    char start_second[3]="\0";
    char end_hour[3]="\0";
    char end_minute[3]="\0";
    char end_second[3]="\0";
    char card_num[9]="\0";
    char remain_money[6]="\0";
    char use_card[5]="\0";
    char use_cash[5]="\0";
    char use_master[5]="\0";
    char self_time[5]="\0";
    char form_time[5]="\0";
    char under_time[5]="\0";
    char coating_time[5]="\0";
    char check_sum[3]="\0";
    char etx[3]="\0";

    strncpy(stx, str, sizeof(stx)-1);
    str += sizeof(stx)-1;

    strncpy(data_len, str, sizeof(data_len)-1);
    str += sizeof(data_len)-1;

    strncpy(cmd, str, sizeof(cmd)-1);
    str += sizeof(cmd)-1;

    strncpy(type, str, sizeof(type)-1);
    str += sizeof(type)-1;

    strncpy(addr, str, sizeof(addr)-1);
    str += sizeof(addr)-1;

    strncpy(save_cnt, str, sizeof(save_cnt)-1);
    str += sizeof(save_cnt)-1;

//    sprintf(year, "%02d", time_info->tm_year);
//    printf("year : %s\n", year);
    strncpy(year, str, sizeof(year)-1);
    printf("year : %s\n", year);
    str += sizeof(year)-1;

//    sprintf(month, "%02d", time_info->tm_mon+1);
    strncpy(month, str, sizeof(month)-1);
    printf("month : %s\n", month);
    str += sizeof(month)-1;

//    sprintf(day, "%02d", time_info->tm_mday);
    strncpy(day, str, sizeof(day)-1);
    printf("day : %s\n", day);
    str += sizeof(day)-1;

//    sprintf(start_hour, "%02d", time_info->tm_hour);
    strncpy(start_hour, str, sizeof(start_hour)-1);
    printf("시작시간 : %s시", start_hour);
    str += sizeof(start_hour)-1;

//    sprintf(start_minute, "%02d", time_info->tm_min);
    strncpy(start_minute, str, sizeof(start_minute)-1);
    printf(" %s분", start_minute);
    str += sizeof(start_minute)-1;

//    sprintf(start_second, "%02d", time_info->tm_sec);
    strncpy(start_second, str, sizeof(start_second)-1);
    printf(" %s초\n", start_second);
    str += sizeof(start_second)-1;

    strncpy(end_hour, str, sizeof(end_hour)-1);
    printf("종료시간 : %s시", end_hour);
    str += sizeof(end_hour)-1;

    strncpy(end_minute, str, sizeof(end_minute)-1);
    printf(" %s분", end_minute);
    str += sizeof(end_minute)-1;

    strncpy(end_second, str, sizeof(end_second)-1);
    printf(" %s초\n", end_second);
    str += sizeof(end_second)-1;

    strncpy(card_num, str, sizeof(card_num)-1);
    str += sizeof(card_num)-1;

    strncpy(remain_money, str, sizeof(remain_money)-1);
    str += sizeof(remain_money)-1;

    strncpy(use_card, str, sizeof(use_card)-1);
    str += sizeof(use_card)-1;

    strncpy(use_cash, str, sizeof(use_cash)-1);
    str += sizeof(use_cash)-1;

    strncpy(use_master, str, sizeof(use_master)-1);
    str += sizeof(use_master)-1;

    strncpy(self_time, str, sizeof(self_time)-1);
    str += sizeof(self_time)-1;

    strncpy(form_time, str, sizeof(form_time)-1);
    str += sizeof(form_time)-1;

    strncpy(under_time, str, sizeof(under_time)-1);
    str += sizeof(under_time)-1;

    strncpy(coating_time, str, sizeof(coating_time)-1);
    str += sizeof(coating_time)-1;

    strncpy(check_sum, str, sizeof(check_sum)-1);
    str += sizeof(check_sum)-1;

    strncpy(etx, str, sizeof(etx)-1);


    //printf("start-time : %s\n", timeToString(time_info));


    // 데이터 베이스 저장구간
    Save_state = (SAVESTATE *)malloc(sizeof(SAVESTATE));
    strcpy(Save_state->m_stx, stx);
    strcpy(Save_state->m_data_len, data_len);
    strcpy(Save_state->m_cmd, cmd);
    strcpy(Save_state->m_type, type);
    strcpy(Save_state->m_addr, addr);
    strcpy(Save_state->m_save_cnt, save_cnt);
    strcpy(Save_state->m_year, year);
    strcpy(Save_state->m_month, month);
    strcpy(Save_state->m_day, day);
    strcpy(Save_state->m_start_hour, start_hour);
    strcpy(Save_state->m_start_minute, start_minute);
    strcpy(Save_state->m_start_second, start_second);
    strcpy(Save_state->m_end_hour, end_hour);
    strcpy(Save_state->m_end_minute, end_minute);
    strcpy(Save_state->m_end_second, end_second);
    strcpy(Save_state->m_card_num, card_num);
    strcpy(Save_state->m_remain_money, remain_money);
    strcpy(Save_state->m_use_card, use_card);
    strcpy(Save_state->m_use_cash, use_cash);
    strcpy(Save_state->m_use_master, use_master);
    strcpy(Save_state->m_self_time, self_time);
    strcpy(Save_state->m_form_time, form_time);
    strcpy(Save_state->m_under_time, under_time);
    strcpy(Save_state->m_coating_time, coating_time);
    strcpy(Save_state->m_check_sum, check_sum);
    strcpy(Save_state->m_etx, etx);




    free(Save_state);
}


void okSign(int serial_port){
    printf("\nOK SiGN\n");
    char buf[255] = "\0";

    strcpy(buf, "GL017OK");
    strcat(buf, "01");  // 공급업체
    strcat(buf, "00");  // 장비종류
    strcat(buf, "01");  // 장비주소
    char *ptr = getCheckSum(buf);
    strcat(buf, ptr);
    strcat(buf, "CH");

    printf("len : %d\n", strlen(buf));
    printf("buf : %s\n", buf);
    int write_res = write(serial_port, buf, strlen(buf));
    if(write_res < 0){
        perror("OK SIGN Err ");
        return;
    }

    sleep(1);
}

// checksum calculate
char *getCheckSum(char *str){
    char checksum[3] = "\0"; // 배열에 널문자 자리와 널문자 꼭 포함
    int sum = 0;
    int i = 0;

    while(str[i]!='\0'){
        //printf("str : %c, %d\n", str[i], str[i]);
        sum += str[i];
        //str++;
        i++;
    }
    int mod = sum % 100;
    sprintf(checksum, "%d", mod); // 바꿀문자열, "%기존포맷", 바꿀정수변수

    //printf("getchecksum : %s, %p\n", checksum, checksum);
    char *ptr = checksum;
    //printf("ptr         : %s, %p\n", ptr, ptr);
    return ptr;
}

// time formatting function
char *timeToString(struct tm *time_info){
    static char str[20];

    sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", time_info->tm_year+1900, time_info->tm_mon+1, time_info->tm_mday,
            time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

    return str;
}

char *rePlaceString(char *str){
    int idx = 0;
    while(str[idx]!='\0'){
        if (str[idx]==' '){
            str[idx]='0';
        }
        idx++;
    }
    return str;
}

int closeSerial(int fd){
    printf("serial 닫기\n");
    close(fd);
    return fd;
}
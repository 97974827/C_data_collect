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

void getDeviceState(int serial_port){
    // 저장값 없을때 : self=garage , 진공/매트/리더 같음
    // 시리얼 송신값 수정 해야할것 : (DB조회) 공급업체, 기기종류, 기기주소

    char buffer[255] = "\0"; //  프로토콜 송수신 값
    char *replace_str = 0;

    strcpy(buffer, "GL017RD");
    strcat(buffer, "01"); // 공급업체
    strcat(buffer, "00"); // 장비분류
    strcat(buffer, "01"); // 장비번호

    replace_str = getCheckSum(buffer);
    //printf("replace_str : %s, %p\n", replace_str, replace_str);
    //printf("buffer : %s, %d, %p\n", buffer, strlen(buffer), buffer);
    //strncat(buffer, replace_str, 2);
    strcat(buffer, replace_str);
    strcat(buffer, "CH");
    //printf("buffer : %s, %d, %p\n", buffer, strlen(buffer), buffer);

    int write_res = write(serial_port, buffer, strlen(buffer));
    printf("write_length : %d\n", write_res);
    if (write_res < 0){
        perror("485 getDeviceState Write Err ");
        return;
    }
    printf("TX : %s\n", buffer);
    sleep(1);
    int read_res = read(serial_port, buffer, 255);
    printf("read_length  : %d\n", read_res);
    if (read_res < 0){
        perror("485 getDeviceState Read Err");
        return;
    }
    printf("RX : %s\n", buffer);
    replace_str = rePlaceString(buffer);
    printf("replace_str : %s\n", replace_str); // 수신 프로토콜 전체 문자 포인터 변수

    // 설정된 장비 종류와 수량에 맞게 조건 설정 -> 상태에 따라 조건 새로 지정 해야함
    // 세차장비 / 충전장비 나눠서 지정 해야함
    char *cmd_search_ptr = 0;       // cmd 검색 문자열 검색을 위한 포인터 변수
    char *etx_search_ptr = 0;       // etx 검색 문자열 검색을 위한 포인터 변수
    char cmd_arr_value[3] = "\0";   // 검색 문자열 저장
    char etx_arr_value[3] = "\0";   // 검색 문자열 저장

    // self / garage 저장값 없을 떄 대기동작
    if (strlen(replace_str) == 41) {
        cmd_search_ptr = strstr(replace_str, "SN"); // search_ptr : SN00010000000000002700000000250127CH
        etx_search_ptr = strstr(replace_str, "CH");
        strncpy(cmd_arr_value, cmd_search_ptr, 2);  // char_value : SN
        strncpy(etx_arr_value, etx_search_ptr, 2);

        if (cmd_search_ptr!=NULL && etx_search_ptr!=NULL) {
            if (strcmp(cmd_arr_value, "SN")==0 && strcmp(etx_arr_value, "CH")==0){
                noSaveCutString(replace_str);
            }
        }
        else {
            perror("485 getDeviceState nosave Null Pointer Exception ");
            return;
        }
    }
    // self / garage 동작 중 상태 저장
    if (strlen(replace_str) == 64) {
        cmd_search_ptr = strstr(replace_str, "SR");
        etx_search_ptr = strstr(replace_str, "CH");
        strncpy(cmd_arr_value, cmd_search_ptr, 2);
        strncpy(etx_arr_value, etx_search_ptr, 2);

        if (cmd_search_ptr!=NULL && etx_search_ptr!=NULL){
            if(strcmp(cmd_arr_value, "SR")==0 && strcmp(etx_arr_value, "CH")==0) {
                operationCutString(replace_str);
            }
        }
        else {
            perror("485 getDeviceState Operation Null Pointer Exception ");
            return;
        }
    }
    // 셀프 저장값 있을 때 대기동작
    if (strlen(replace_str) == 77) {
        cmd_search_ptr = strstr(replace_str, "SW");
        etx_search_ptr = strstr(replace_str, "CH");
        strncpy(cmd_arr_value, cmd_search_ptr, 2);
        strncpy(etx_arr_value, etx_search_ptr, 2);

        if (cmd_search_ptr!=NULL && etx_search_ptr!=NULL){
            if(strcmp(cmd_arr_value, "SW")==0 && strcmp(etx_arr_value, "CH")==0) {
                saveCutString(replace_str);
            }
        }
        else {
            perror("485 getDeviceState selfSave Null Pointer Exception ");
            return;
        }

    }
}

void noSaveCutString(char *str){
    printf("def nosavecutstring\n");
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

void operationCutString(char *str){
    printf("def operation cutstring\n");
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
    str += sizeof(hour)-1;

    strncpy(minute, str, sizeof(minute)-1);
    str += sizeof(minute)-1;

    strncpy(second, str, sizeof(second)-1);
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

void saveCutString(char *str){
    printf("def Savecutstring\n");

    int arr[3] = {5,3,7};
    int *iip;
    iip = arr;

    for(int i=0; i<3; i++){
        printf("%d ", *(iip + i));
    }
    printf("\n");
    for(int i=0; i<3; i++){
        printf("%d ", *iip++);
    }
}

char *getCheckSum(char *str){
    int sum = 0;
    int i = 0;
    while(str[i]!='\0'){
        //printf("str : %c, %d\n", str[i], str[i]);
        sum += str[i];
        i++;
    }
    int mod = sum % 100;
    char checksum[3] = "\0"; // 배열에 널문자 자리와 널문자 꼭 포함
    sprintf(checksum, "%d", mod); // 바꿀문자열, "%기존포맷", 바꿀정수변수
    //printf("getchecksum : %s, %p\n", checksum, checksum);
    char *ptr = checksum;
    //printf("ptr         : %s, %p\n", ptr, ptr);
    return ptr;
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
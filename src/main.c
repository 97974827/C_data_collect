#include "../inc/main.h"
#include "../inc/common.h"
#include "../inc/device.h"
#include "../src/device.c"
//#include "/home/data_collect/Public/inc/main.h"
//#include "/home/data_collect/Public/inc/common.h"

NOSAVESTATE *No_save_state;
OPERATIONSTATE *Operation_state;
SAVESTATE *Save_state;

int g_no_save_count = 0;    // 저장값 없는 데이터 갯수
int g_operation_count = 0;  // 동작중 일때 데이터 갯수
int g_save_count = 0;       // 저장값 있는 데이터 갯수

int main(int argc, char **argv) {

    enum Device_type device_type;
    int serial_port = 0;

    printf("\n--- 메인 시작 ---\n");
    //printf("MySQL 클라이언트 버전 : %s\n", mysql_get_client_info());

    serial_port = openSerial(SERIAL_PORT);
    device_type = SELF;
    //printf("DEVICE_TYPE : %d\n", device_type);

    // 포트개방
    if(serial_port >= 0){
        getDeviceState(serial_port);

        serial_port = closeSerial(serial_port);
        if(serial_port < 0 ){
            perror("Serial Device Close Fail ");
            return 1;
        }
    }


    printf("\n--- 메인 끝 ---\n");
    return 0;
}


//
// Created by 82105 on 2020-03-12.
//

/*     Main Header File      */
#ifndef DATA_COLLECT_MAIN_H
#define DATA_COLLECT_MAIN_H

/* header */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>


//#include <my_global.h>
//#include <mysql.h>

/* define */
#define MYSQL_HOST      "localhost"
#define MYSQL_USER      "pi"
#define MYSQL_PWD       "gls08300"
#define MYSQL_DB        "db_datacollect"
#define MYSQL_SET       "utf8mb4"

#define DC_VERSION      "1.0.2"

#define SERIAL_PORT     "/dev/ttyS0"
#define BAUD_RATE       B9600

#define TRUE 1
#define FALSE 0



#endif //DATA_COLLECT_MAIN_H

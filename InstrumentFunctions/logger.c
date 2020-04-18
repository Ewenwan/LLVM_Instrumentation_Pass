/* compile with
   cc -std=c11 -pthread main.c
   
   自己的日志函数，可以插入其他代码中，用来对函数 插桩
   
   */
#include <stdlib.h>
#include <sys/time.h>
#include "zlog.h"

zlog_category_t *variable_values_cat;   // 记录变量 

zlog_category_t *function_calls_cat;    // 记录函数调用

int initialized = 0;

// 初始化函数   用来插入 main函数中
int init() {
    int rc = zlog_init("zlog.conf");  // 初始化 zlog  日志
    if (rc) {
        printf("init failed\n");
        return -1;
    }

    variable_values_cat = zlog_get_category("variable_values_cat");
    if (!variable_values_cat) {
        printf("get cat fail\n");
        zlog_fini();
        return -2;
    }

    function_calls_cat = zlog_get_category("function_calls_cat");
    if (!function_calls_cat) {
        printf("get cat fail\n");
        zlog_fini();
        return -2;
    }
    initialized = 1;
    return 0;
}

// 记录变量 变化
void log_variable_change(const char* variable, int value) {
    initialized || init();  // 先确保已经初始化完成

    zlog_info(variable_values_cat, "%s %d", variable, value); // 变量和值
}

// 记录函数调用  
void log_function_call(const char* function) {
    initialized || init();

    zlog_info(function_calls_cat, "%s", function);
}

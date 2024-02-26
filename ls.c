#include <ctype.h>
#include <dirent.h>
#include <error.h>
#include <getopt.h>
#include <grp.h>
#include <locale.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
-a 列出目录下的所有文件，包括以.开头的隐含文件
-l 列出文件的详细信息（包括文件属性和权限等）
-R
使用递归连同目录中的子目录中的文件显示出来，如果要显示隐藏文件就要添加-a参数(列出所有子目录下的文件）
-t 按修改时间进行排序，先显示最后编辑的文件
-r 对目录反向排序（以目录的每个首字符所对应的ASCII值进行大到小排序）
-i 输出文件的i节点的索引信息
-s 在每个文件名后输出该文件的大小
*/
/*
struct stat name
{
    dev_t     st_dev;           //文件的设备编号
    int_t     st_ino;           //节点
    mode_t    st_mode;          //文件的类型和存取的权限
    nlink_t   st_nlink;         //连到该文件的硬连接数目，刚建立的文件值为1
    uid_t     st_uid;           //用户ID
    gid_t     st_gid;           //组ID
    dev_t     st_rdev;          //(设备类型)若此文件为设备文件，则为其设备编号
    offf_t    st_size;          //文件字节数(文件大小)
    unsigned  long st_bilsize;  //块大小（文件系统的I/O缓存区大小）
    unsigend  long st_blocks;   //块数
    time_t    st_atime;         //最后一次访问时间
    time_t    st_mtime;         //最后一次修改时间
    time_t    st_ctime;         //最后一次改变时间(指属性)
}
*/
/*
typedef struct {
    int     fd;                 // 目录文件描述符
    int     flags;              // 标志位
    int     size;               // 缓冲区大小
    char    *buffer;            // 缓冲区
    char    *start;             // 当前文件指针
    char    *end;               // 结束文件指针
    int     lock;               // 锁标志
    int     lseek_zero;         // 是否重新定位文件指针
    struct  dirent *dirent;     // 目录项结构体
    int     dirent_no;          // 目录项编号
    char    *next;              // 下一个目录项
    int     dirent_pos;         // 目录项位置
} DIR;
*/
/*
struct dirent {
    ino_t          d_ino;       // inode 编号
    off_t          d_off;       // 下一个 dirent 的偏移量
    unsigned short d_reclen;    // 当前记录的长度
    unsigned char  d_type;      // 文件类型；并非所有文件系统类型都支持
    char           d_name[256]; // 文件名
};
*/
/*
struct passwd {
    char    *pw_name;       // 用户名
    char    *pw_passwd;     // 加密后的密码
    uid_t   pw_uid;         // 用户ID
    gid_t   pw_gid;         // 用户组ID
    char    *pw_gecos;      // 用户的真实姓名及其他信息
    char    *pw_dir;        // 用户的主目录
    char    *pw_shell;      // 用户的登录shell
};
*/
/*
对染色部分进行如下解释：
以“printf("\033[01;%dm%s\033[0m\n", BLUE, pathname_file_list);”为例
\033[：这是 ANSI 转义序列的起始部分，表示开始设置文本格式或颜色。
01;%d：这是 ANSI 转义序列中设置文本样式和颜色的部分。
在这里，01表示设置为粗体（bold），
%d 对应BLUE即为34 表示设置文本颜色为蓝色。
m：表示转义序列的结束，即设置结束。
%s：这是 printf 格式字符串中的占位符，用于将字符串 pathname_file_list
插入到该位置。
\033[0m：这是 ANSI 转义序列，用于重置文本格式为默认值。
0表示重置所有样式，m 表示转义序列的结束。
*/
int flag_a = 0, flag_l = 0, flag_R = 0, flag_t = 0, flag_r = 0, flag_i = 0,
    flag_s = 0;
int cnt = 0;
#define BLUE 34
#define GREEN 32
#define WHITE 37

void ls_i(char *pathname_file_list, char *name); // 输出文件的i节点的索引信息
void ls_s(char *pathname_file_list,
          char *name); // 在每个文件名后输出该文件的大小
void ls_t(char **pathname_buffer_list, int len);
// 按修改时间进行排序，先显示最后编辑的文件
void ls_r(char **pathname_buffer_list, int len);
void MODE(int mode, char *str); // 文件权限
char *UID(uid_t uid);           // 获取用户名
char *GID(gid_t gid);           // 获取用户组名
void ls_l(char *pathname_file_list,
          char *name); // 列出文件的详细信息（包括文件属性和权限等）
void ls(char *name);
void judge_file(char *name); // 判断文件类型
void ls_l_file(char *name) {
    struct stat path;
    struct passwd *pw_ptr;
    char pathname[1024];
    sprintf(pathname, "%s", name);
    char mode[11]; // 权限
    lstat(pathname, &path);
    pw_ptr = getpwuid(path.st_uid);
    MODE(path.st_mode, mode);      // 获取权限可视化的字符串
    printf("%5ld", path.st_nlink); // 文件硬链接数目
    // printf(" %5s", UID(path.st_uid)); // 用户名
    // printf(" %5s", GID(path.st_gid)); // 组名
    // 组名为什么这里调用会导致卡顿(符号链接没有uid和gid)
    printf(" %5s", pw_ptr->pw_name); // 用户名
    printf(" %5s", pw_ptr->pw_name); // 组名
    printf(" %7ld", path.st_size);   // 文件大小
    char lasttime[64];               // 时间
    strcpy(lasttime, ctime(&(path.st_mtime)));
    lasttime[strlen(lasttime) - 1] = '\0';
    printf(" %5s ", lasttime);
}

void ls_i_file(char *name) {
    struct stat STA;
    char pathname[1024];
    sprintf(pathname, "%s", name);
    lstat(pathname, &STA); // 通过lstat获取路径pathname的状态储存到STA当中
    printf("%7ld", (long)STA.st_ino);
}

void ls_s_file(char *name) {
    char pathname[1024];
    sprintf(pathname, "%s", name);
    struct stat STA;
    lstat(pathname, &STA); // 通过lstat获取路径pathname的状态储存到STA当中
    printf("%5ld ", (long)STA.st_blocks);
}
void file_list(char *name) {
    if (flag_i) {
        ls_i_file(name);
    }
    if (flag_s) {
        ls_s_file(name);
    }
    if (flag_l) {
        ls_l_file(name);
    }

    printf(" %s", name);
} // 在最开始进行判断时，倘若是文件则直接输出文件名
int main(int argc, char **argv) {
    int opt, i = 1;
    // opt 用于存储 getopt 函数返回的选项字符
    // i 则被初始化为 1,表示开始遍历命令行参数数组 argv
    int flag_name = 0; // 用于区分是否有指定文件
    int j = 0;         // 记录指定文件数量
    char **name;
    while (i < argc) { // 遍历命令行参数
        if (argv[i][0] != '-') {
            j++; // 如果命令行参数不以 '-' 开头，则将 j
                 // 值加一，用于统计指定文件的数量
        }
        i++;
    }

    if (j == 0) {
        name = (char **)malloc(2 * sizeof(char *));
    } else {
        name = (char **)malloc((j + 1) * sizeof(char *));
    }

    if (argc == 1) { // 命令行参数为1时默认为当前目录
        name[0] = ".";
    } else {
        i = 1; // 重置
        j = 0;
        // 遍历命令行参数
        while (i < argc) {
            if (argv[i][0] != '-') {
                name[j] = argv[i]; // 将指定文件添加到目录数组中
                flag_name = 1;
                j++;
            }
            i++;
        }
        if (flag_name == 0) {
            name[0] = ".";
        }
    }
    while ((opt = getopt(argc, argv, "alRtris")) != -1) {
        if (opt == 'a') {
            flag_a = 1;
        } else if (opt == 'l') {
            flag_l = 1;
        } else if (opt == 'R') {
            flag_R = 1;
        } else if (opt == 't') {
            flag_t = 1;
        } else if (opt == 'r') {
            flag_r = 1;
        } else if (opt == 'i') {
            flag_i = 1;
        } else if (opt == 's') {
            flag_s = 1;
        }
    }

    if (j == 0) {
        judge_file(name[0]);
    }

    for (int k = 0; k < j; k++) {
        printf("%s:\n", name[k]);
        judge_file(name[k]);
        printf("\n");
    }
    free(name);
    return 0;
}

void judge_file(char *name) { // 判断文件类型
    struct stat STA;
    if (stat(name, &STA) != 0) {
        perror("stat in judge_file");
        return;
    }
    if (S_ISDIR(STA.st_mode)) { // 目录
        ls(name);
    } else {
        file_list(name); // 文件
    }
}

void printcolor(char *pathname_file_list, const char *name) {
    char path_color[1024];
    sprintf(path_color, "%s/%s", name, pathname_file_list);
    struct stat pr_color;
    if (stat(path_color, &pr_color) == -1) {
        perror(path_color);
        exit(EXIT_FAILURE);
    }
    if (flag_l) {
        switch (pr_color.st_mode & S_IFMT) {
        case S_IFREG: // 常规文件
            if (pr_color.st_mode & S_IXUSR) {
                printf("\033[01;%dm%s\033[0m\n", BLUE,
                       pathname_file_list); // 可执行文件，颜色为蓝色
            } else {
                printf("\033[01;%dm%s\033[0m\n", WHITE,
                       pathname_file_list); // 普通文件，颜色为白色
            }
            break;
        case S_IFDIR: // 目录
            printf("\033[01;%dm%s\033[0m\n", GREEN,
                   pathname_file_list); // 目录，颜色为绿色
            break;
        default:
            printf("\033[01;%dm%s\033[0m\n", WHITE,
                   pathname_file_list); // 默认为白色
            break;
        }
    } else {
        switch (pr_color.st_mode & S_IFMT) {
        case S_IFREG: // 常规文件
            if (pr_color.st_mode & S_IXUSR) {
                printf(" \033[01;%dm%s\033[0m", BLUE,
                       pathname_file_list); // 可执行文件，颜色为蓝色
            } else {
                printf(" \033[01;%dm%s\033[0m", WHITE,
                       pathname_file_list); // 普通文件，颜色为白色
            }
            break;
        case S_IFDIR: // 目录
            printf(" \033[01;%dm%s\033[0m", GREEN,
                   pathname_file_list); // 目录，颜色为绿色
            break;
        default:
            printf(" \033[01;%dm%s\033[0m", WHITE,
                   pathname_file_list); // 默认为白色
            break;
        }
        cnt++;
        if ((cnt % 5) == 0) {
            printf("\n");
            cnt = 0;
        }
    }
}

void ls(char *name) {
    DIR *dir; // 指向目录流的指针，用于在程序中操作目录。
    struct dirent *dir_ptr; // 获取到目录中每个文件或子目录的信息
    char **pathname_buffer_list; // 用于存储获取到的当前工作目录的路径。

    int file_num = 0; // 记录文件个数

    dir = opendir(name); // 打开
    if (dir == NULL) {
        perror("ls dir opendir(name)");
        return;
    }

    while ((dir_ptr = readdir(dir)) != NULL) { // 读取
        char pathname_buffer[1024];
        sprintf(pathname_buffer, "%s/%s", name, dir_ptr->d_name); // 写入

        if (access(pathname_buffer, 4) == -1) { // 判断权限
            continue;
        }
        file_num++;
    }

    rewinddir(dir); // 将目录流挪至目录开始位置

    if (file_num == 0) { // 空文件
        closedir(dir);
        return;
    }

    int pathname_buffer_list_num = 0;
    // 用于遍历pathname_buffer_list二维数组
    pathname_buffer_list = (char **)malloc((file_num + 1) * sizeof(char *));
    if (pathname_buffer_list == NULL) {
        perror("pathname_buffer_list malloc");
        exit(EXIT_FAILURE);
    }

    while ((dir_ptr = readdir(dir)) != NULL) { // 遍历该目录下的每一个文件
        char pathname_buffer[1024];
        sprintf(pathname_buffer, "%s/%s", name, dir_ptr->d_name);
        if (access(pathname_buffer, 4) == -1) {
            continue;
        }
        pathname_buffer_list[pathname_buffer_list_num] =
            (char *)malloc(1024 * sizeof(char));
        if (pathname_buffer_list[pathname_buffer_list_num] == NULL) {
            perror("pathname_buffer_list[pathname_buffer_list_num] malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(pathname_buffer_list[pathname_buffer_list_num], dir_ptr->d_name);
        pathname_buffer_list_num++; // 记录该文件夹下的文件名及个数
    }
    if (flag_t) {
        ls_t(pathname_buffer_list, pathname_buffer_list_num);
    }
    if (flag_r) {
        ls_r(pathname_buffer_list, pathname_buffer_list_num);
    }
    for (int k = 0; k < file_num; k++) {
        if (flag_a == 0 && pathname_buffer_list[k][0] == '.') {
            continue;
        }
        if (flag_i) {
            ls_i(pathname_buffer_list[k], name);
        }
        if (flag_s) {
            ls_s(pathname_buffer_list[k], name);
        }
        if (flag_l) {
            ls_l(pathname_buffer_list[k], name);
        }
        printcolor(pathname_buffer_list[k], name);
    }

    rewinddir(dir); // 在对该目录完成tralis任务后将目录流返回到目录开始

    if (flag_R) {
        while ((dir_ptr = readdir(dir)) != NULL) {
            if (flag_a == 0 && dir_ptr->d_name[0] == '.') {
                continue;
            }
            if (dir_ptr->d_type == DT_DIR && // 是否为目录
                strcmp(dir_ptr->d_name, ".") != 0 &&
                strcmp(dir_ptr->d_name, "..") !=
                    0) { // 是否为当前目录或者上级目录
                char path_R[1024];
                char repath[1024];
                sprintf(path_R, "%s/%s", name, dir_ptr->d_name);
                realpath(
                    path_R,
                    repath); // 使用了 realpath
                             // 函数来获取指定路径的绝对路径，并将结果保存在
                             // repath 字符数组中
                printf("\n%s:\n", repath); // 打印绝对路径
                ls(path_R);
            }
        }
    }
    closedir(dir); // 关闭目录流

    for (int j = 0; j < pathname_buffer_list_num; j++) {
        free(pathname_buffer_list[j]);
    }
    free(pathname_buffer_list); // 释放内存
}

void ls_l(char *pathname_file_list, char *name) {

    struct stat path;
    struct passwd *pw_ptr;
    char pathname[1024];
    sprintf(pathname, "%s/%s", name, pathname_file_list);
    char mode[11]; // 权限
    lstat(pathname, &path);
    pw_ptr = getpwuid(path.st_uid);
    MODE(path.st_mode, mode);      // 获取权限可视化的字符串
    printf("%5ld", path.st_nlink); // 文件硬链接数目
    // printf(" %5s", UID(path.st_uid)); // 用户名
    // printf(" %5s", GID(path.st_gid)); // 组名
    // 组名为什么这里调用会导致卡顿(符号链接没有uid和gid)
    printf(" %5s", pw_ptr->pw_name); // 用户名
    printf(" %5s", pw_ptr->pw_name); // 组名
    printf(" %7ld", path.st_size);   // 文件大小
    char lasttime[64];               // 时间
    strcpy(lasttime, ctime(&(path.st_mtime)));
    lasttime[strlen(lasttime) - 1] = '\0';
    printf(" %5s ", lasttime);
}

void MODE(int mode, char *str) {
    strcpy(str, "----------"); // 无权限状态

    if (S_ISDIR(mode)) // 是否为目录
    {
        str[0] = 'd';
    }

    if (S_ISCHR(mode)) // 是否为字符设置
    {
        str[0] = 'c';
    }

    if (S_ISBLK(mode)) // 是否为块设备
    {
        str[0] = 'b';
    }

    if (S_ISLNK(mode)) { // 是否为符号链接
        str[0] = 'l';
    }

    if (S_ISFIFO(mode)) { // 是否为管道
        str[0] = 'p';
    }

    // 逻辑与
    /*
        S_IRUSR：用户读权限
        S_IWUSR：用户写权限
        S_IRGRP：用户组读权限
        S_IWGRP：用户组写权限
        S_IROTH：其他组读权限
        S_IWOTH：其他组写权限
    */

    if ((mode & S_IRUSR)) {
        str[1] = 'r';
    }
    if ((mode & S_IWUSR)) {
        str[2] = 'w';
    }
    if ((mode & S_IXUSR)) {
        str[3] = 'x';
    }
    if ((mode & S_IRGRP)) {
        str[4] = 'r';
    }
    if ((mode & S_IWGRP)) {
        str[5] = 'w';
    }
    if ((mode & S_IXGRP)) {
        str[6] = 'x';
    }
    if ((mode & S_IROTH)) {
        str[7] = 'r';
    }
    if ((mode & S_IWOTH)) {
        str[8] = 'w';
    }
    if ((mode & S_IXOTH)) {
        str[9] = 'x';
    }

    printf(" %s ", str);
}

char *UID(uid_t uid) {
    struct passwd *pw_ptr;
    static char username[20];
    if ((pw_ptr = getpwuid(uid)) == NULL) {
        sprintf(username, "%u", (unsigned int)uid);
        return username;
    } else {
        return pw_ptr->pw_name;
    }
}

char *GID(gid_t gid) {
    struct group *gr_ptr;
    static char groupname[20];
    if ((gr_ptr = getgrgid(gid)) == NULL) {
        sprintf(groupname, "%u", (unsigned int)gid);
        return groupname;
    } else {
        return gr_ptr->gr_name;
    }
}

// 打印i节点编号 -i
void ls_i(char *pathname_file_list, char *name) {
    struct stat STA;
    char pathname[1024];
    sprintf(pathname, "%s/%s", name, pathname_file_list);
    lstat(pathname, &STA); // 通过lstat获取路径pathname的状态储存到STA当中
    printf("%7ld", (long)STA.st_ino);
}
// 文件大小 -s
void ls_s(char *pathname_file_list, char *name) {
    char pathname[1024];
    sprintf(pathname, "%s/%s", name, pathname_file_list);
    struct stat STA;
    lstat(pathname, &STA); // 通过lstat获取路径pathname的状态储存到STA当中
    printf("%5ld ", (long)STA.st_blocks);
}
// void ls_t(char **pathname_buffer_list, int len) {
//     // 冒泡排序(烂完)
//     for (int i = 0; i < len - 1; i++) {
//         for (int j = 0; j < len - i - 1; j++) {
//             struct stat stat1, stat2;
//             char pathname1[1024], pathname2[1024];
//             // 获取第一个文件的修改时间
//             sprintf(pathname1, "%s/%s", ".", pathname_buffer_list[j]);
//             stat(pathname1, &stat1);
//             // 获取第二个文件的修改时间
//             sprintf(pathname2, "%s/%s", ".", pathname_buffer_list[j + 1]);
//             stat(pathname2, &stat2);
//             // 如果第一个文件的修改时间晚于第二个文件的修改时间，则交换它们
//             if (stat1.st_mtime < stat2.st_mtime) {
//                 char *temp = pathname_buffer_list[j];
//                 pathname_buffer_list[j] = pathname_buffer_list[j + 1];
//                 pathname_buffer_list[j + 1] = temp;
//             }
//         }
//     }
// }
// 比较函数，用于比较两个文件的修改时间
int compare_mtime(const void *a, const void *b) {
    char *file1 = *(char **)a;
    char *file2 = *(char **)b;
    struct stat stat1, stat2;
    char pathname1[1024], pathname2[1024];
    // 获取路径名
    sprintf(pathname1, "%s/%s", ".", file1);
    sprintf(pathname2, "%s/%s", ".", file2);
    // 使用stat获取pathname1和pathname2的具体信息进行比较
    stat(pathname1, &stat1);
    stat(pathname2, &stat2);
    if (stat1.st_mtime < stat2.st_mtime)
        return -1;
    else if (stat1.st_mtime > stat2.st_mtime)
        return 1;
    else
        return 0;
}
void ls_t(char **pathname_buffer_list, int len) {
    // 使用 qsort 函数对文件列表按修改时间进行排序
    qsort(pathname_buffer_list, len, sizeof(char *), compare_mtime);
}

// 比较函数，用于比较两个目录项的名称
int compare_names(const void *a, const void *b) {
    const char *name1 = *(const char **)a;
    const char *name2 = *(const char **)b;
    return strcmp(name2, name1); // 以相反的顺序比较名称
}

void ls_r(char **pathname_buffer_list, int len) {
    // 使用qsort函数对目录项数组进行排序
    qsort(pathname_buffer_list, len, sizeof(char *), compare_names);
}
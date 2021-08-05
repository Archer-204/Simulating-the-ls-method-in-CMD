#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stddef.h>
#include <grp.h>
#include <pwd.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

//typedefined structures
typedef struct{
    char filename[32];
    char pathname[PATH_MAX+1];
}DIR_Ex;
typedef struct{
    char filename[32];
    struct stat s;
}stat_s;

//Signals for command options
static bool s_i_sign = false;
static bool s_l_sign = false;
static bool s_R_sign = false;
static bool s_dir_sign = false;

//Variables for recording length
static int max_length_user = 0;
static int max_length_group = 0;
static int max_length_size = 0;
static int max_length_ino = 0;
static int max_length_link = 0;


// helper functions:

//Get the # digits of a long type variable
int get_digit(long num){
    int n = 1;
    for(; num >= 10; n++){
        num /= 10;
    }
    return n;
}
//Set all length variables empty
void empty(){
    max_length_ino = 0;
    max_length_size = 0;
    max_length_link = 0;
    max_length_user = 0;
    max_length_group = 0;
}
//Get the max length of pathname under a directory
void get_max_length(DIR *dir, struct stat temp_stat){
    struct  dirent *dp;
    // struct stat temp_stat;
    struct passwd *pw;
    struct group *grp;
    empty();

    pw = getpwuid(temp_stat.st_uid);
    grp = getgrgid(temp_stat.st_gid);
    while( (dp = readdir(dir)) != NULL) {
        if(lstat(dp->d_name, &temp_stat) == -1) {
                perror("lstat");
                exit(EXIT_FAILURE);
        }
        if(dp->d_name[0] != '.'){
            max_length_ino = get_digit(temp_stat.st_ino) > max_length_ino ? get_digit(temp_stat.st_ino): max_length_ino;
            max_length_size = get_digit(temp_stat.st_size) > max_length_size ? get_digit(temp_stat.st_size): max_length_size;
            max_length_link = get_digit(temp_stat.st_nlink) > max_length_link ? get_digit(temp_stat.st_nlink): max_length_link;
            if(pw){
                max_length_user = strlen(pw->pw_name) > max_length_user ? strlen(pw->pw_name): max_length_user;
            }
            if(grp){
                max_length_group = strlen(grp->gr_name) > max_length_group ? strlen(grp->gr_name): max_length_group;
            }

            // printf("max_length_ino: %d\n", max_length_ino);
            // printf("max_length_size: %d\n", max_length_size);
            // printf("max_length_link: %d\n", max_length_link);
            // printf("max_length_user: %d\n", max_length_user);
            // printf("max_length_group: %d\n", max_length_group);
        }
        
    }
    // printf("max_length_ino: %d\n", max_length_ino);
    //         printf("max_length_size: %d\n", max_length_size);
    //         printf("max_length_link: %d\n", max_length_link);
    //         printf("max_length_user: %d\n", max_length_user);
    //         printf("max_length_group: %d\n", max_length_group);
    rewinddir(dir);
}

//Sort a stat_s type variable in the order of alphabet
void sort_i(stat_s *stat_, int index)
{
    stat_s *temp = (stat_s*)malloc(sizeof(stat_s));
    for(int i = index -1; i > 0; i--){
        for(int j = 0; j < i; j++){
            if(strcmp(stat_[i].filename,stat_[j].filename)<0){
                memcpy(temp,&stat_[i],sizeof(stat_s));
                memcpy(&stat_[i],&stat_[j],sizeof(stat_s));
                memcpy(&stat_[j],temp,sizeof(stat_s));
            }
        }
    }
    // free(temp);
}

//Sort a DIR_Ex type variable in the order of alphabet
void sort_i_R(DIR_Ex *dir_, int index)
{
    DIR_Ex *temp = (DIR_Ex*)malloc(sizeof(DIR_Ex));
    for(int i = index -1; i > 0; i--){
        for(int j = 0; j < i; j++){
            if(strcmp(dir_[i].pathname,dir_[j].pathname)<0){
                // memset(temp, 0, sizeof(DIR_Ex));
                memcpy(temp,&dir_[i],sizeof(DIR_Ex));
                memcpy(&dir_[i],&dir_[j],sizeof(DIR_Ex));
                memcpy(&dir_[j],temp,sizeof(DIR_Ex));
            }
        }
    }
    free(temp);
}

//Sort a dirent type variable in the order of alphabet
void sort(struct dirent *dir,int index)
{
    struct dirent *temp = (struct dirent *)malloc(sizeof(struct dirent ));
    for(int i=index-1; i>0; i--)
    {
        for(int j=0; j<i; j++)
        {
            if(strcmp(dir[i].d_name,dir[j].d_name)<0)
            {
                memcpy(temp,&dir[i],sizeof(struct dirent ));
                memcpy(&dir[i],&dir[j],sizeof(struct dirent ));
                memcpy(&dir[j],temp,sizeof(struct dirent ));
            }
        }
    }
    free(temp);
}

// https://stackoverflow.com/questions/4761764/how-to-remove-first-three-characters-from-string-with-c

void chop_N(char *str, size_t n)
{
    if(n != 0 && str != 0){
        int len = strlen(str);
        if (n > len)
            return;  // Or: n = len;
        memmove(str, str+n, len - n + 1);
    }
}

//Deal with special signal
int spec_sign(char* name){
    return (strchr(name, ' ') || strchr(name, '!') || strchr(name, '$') ||
            strchr(name, '^') || strchr(name, '&') || strchr(name, '(') ||
            strchr(name, ')'));
}

//Print files in sorted order in the form of -iR
void show_ls_iR(DIR* ret_opendir, char* path){
    struct dirent* ret = NULL;
    struct stat stat_info;
    char nextpath[PATH_MAX+1];
    char* buf =  NULL;
    char* p=NULL;
    struct dirent lst[4096]={0};
    int index = 0;
    int i = 0;
    while(ret = readdir(ret_opendir)) {
        strcpy(nextpath,path);
        char* filename = ret->d_name;
        if(filename[0]!='.') {
            memcpy(&lst[index],ret,sizeof(struct dirent));
            index++;
        }
    }
    sort(lst,index);
    while(i<index){
        ret = &lst[i];
        char* filename = ret->d_name;

        memset(nextpath,0,PATH_MAX+1);
        strcpy(nextpath,path);

        int end = 0;
        while(path[end])
            end++;
        if(path[end-1] != '/')
        strcat(nextpath,"/");
        strcat(nextpath,filename);
        memset(&stat_info,0,sizeof(struct stat));
        int stat_ret = lstat(nextpath, &stat_info);
        if(stat_ret ==-1){
            printf("error %s \n",nextpath);
            i++;
            continue;
        }
    
        if(filename[0]!='.') {
            printf("%li ", stat_info.st_ino);
            printf("%s\n", ret->d_name);
            ssize_t bufsiz = stat_info.st_size + 1;
            if (stat_info.st_size == 0)
                bufsiz = PATH_MAX;
            buf = malloc(bufsiz);
            if (buf == NULL) {
                i++;
                continue;
            }
            if(S_ISLNK(stat_info.st_mode)) {
                ssize_t nbytes = readlink(ret->d_name, buf, bufsiz);
                if (nbytes == -1) {
                    i++;
                    continue;
                }
            }
            free(buf);
        }
        i++;
    }
    rewinddir(ret_opendir);
    puts("");
    puts("");
}

//Print files in sorted order in the form of -R
void show_ls_R(DIR* ret_opendir){
    struct dirent* ret = NULL;
    struct dirent lst[4096]={0};
    int index = 0;
    int i = 0;
    while(ret = readdir(ret_opendir)) {
        char* filename = ret->d_name;
        if(filename[0]!='.' && index < 4096) {
            memcpy(&lst[index],ret,sizeof(struct dirent));
            index++;
        }
    }
    sort(lst,index);
    while(i<index){
        printf("%s\n",lst[i].d_name);
        i++;
    }

    rewinddir(ret_opendir);
    puts("");
    puts("");
}

//Print file in the form of -l
void l_display(struct stat buff)
{
    // Max time display using 32
    char buff_time[32];
    struct passwd* user;
    struct group* grp;
    if(S_ISLNK(buff.st_mode)){
        printf("l");
    }else{
        printf( (S_ISDIR(buff.st_mode)) ? "d" : "-");
    }
    printf( (buff.st_mode & S_IRUSR) ? "r" : "-");
    printf( (buff.st_mode & S_IWUSR) ? "w" : "-");
    printf( (buff.st_mode & S_IXUSR) ? "x" : "-");
    printf( (buff.st_mode & S_IRGRP) ? "r" : "-");
    printf( (buff.st_mode & S_IWGRP) ? "w" : "-");
    printf( (buff.st_mode & S_IXGRP) ? "x" : "-");
    printf( (buff.st_mode & S_IROTH) ? "r" : "-");
    printf( (buff.st_mode & S_IWOTH) ? "w" : "-");
    printf( (buff.st_mode & S_IXOTH) ? "x" : "-");
    user = getpwuid(buff.st_uid);
    grp  = getgrgid(buff.st_gid);
    // printf("%ld ",buff.st_nlink);     // link number
    // printf("%s ",user->pw_name);      // User name
    // printf("%s ",grp->gr_name);       // Group name
    // printf("%6ld ",buff.st_size);
    printf("% *ld ",max_length_link,buff.st_nlink);     // link number
    printf("%*s ",max_length_user,user->pw_name);      // User name
    printf("%*s ",max_length_group,grp->gr_name);       // Group name
    printf("%*ld ",max_length_size,buff.st_size);
    strcpy(buff_time,ctime(&buff.st_mtime));
    chop_N(buff_time,4);
    // Print the date as required
    printf("%.*s",  7, buff_time);
    printf("%.*s ", 4, buff_time + 16);
    printf("%.*s",  5, buff_time + 7);
}

//Print error messages and exit
void errorinfo( char* msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

//Recording all directory names using recursion
void print_i_R_rec( char* pathname, DIR_Ex* dir,int *index){
    char nextpath[PATH_MAX+1]= {0};
    struct stat file_message = {0};
    int end = 0;

    DIR* odir = opendir(pathname);
    if(odir == NULL)
        errorinfo("opendir");
    memset(&dir[*index].pathname,0,PATH_MAX+1);
    strncpy(dir[*index].pathname, pathname,strlen(pathname));
    dir[*index].pathname[strlen(pathname)]='\0';
    ++(*index);

    struct dirent* ret_readdir = NULL;
    while(ret_readdir = readdir(odir))  {
        char* filename = ret_readdir->d_name;
        end = 0;
        while(pathname[end])
            end++;
        memset(nextpath,0,PATH_MAX+1);
        strncpy(nextpath,pathname,strlen(pathname));
        nextpath[strlen(pathname)] = '\0';
        if(pathname[end-1] != '/'){
            strncat(nextpath,"/",strlen("/"));
        }
        strncat(nextpath,filename,strlen(filename));
        memset(&file_message, 0, sizeof(struct stat));
        int ret_stat = lstat(nextpath, &file_message);
        if(ret_stat == -1)
            printf("%s error!", filename);
        else if(S_ISDIR(file_message.st_mode)  && filename[0]!='.') {
            print_i_R_rec(nextpath,dir,index);
        }
    }
    closedir(odir);
}

// -R command
void print_R_Sort( char* pathname){
    DIR_Ex dir[1024] = {0};
    int index = 0;
    int i = 0;
    struct stat s_buf;
    
    stat(pathname,&s_buf);
    if(S_ISREG(s_buf.st_mode)){
        if(spec_sign(pathname)){
            printf(" \'%s\'",pathname);
        }else{
            printf(" %s ",pathname);
        }
    }else{
        print_i_R_rec(pathname,dir,&index);
        sort_i_R(dir,index);

        while(i < index){
            printf("%s:\n", dir[i].pathname);
            DIR* odir = opendir(dir[i].pathname);
            if(odir == NULL){
                i++;
                continue;
            }
            show_ls_R(odir);
            closedir(odir);
            i++;
        }
    }
}

// -iR command
void print_i_R_Sort( char* pathname){
    DIR_Ex dir[1024] = {0};
    int index = 0;
    int i = 0;
    struct stat s_buf;
    stat(pathname,&s_buf);
    
    if(S_ISREG(s_buf.st_mode)){
        printf("%li ",s_buf.st_ino);
        printf("%s\t",pathname);
    }else{
        print_i_R_rec(pathname,dir,&index);
        sort_i_R(dir,index);
        while(i < index){
            printf("%s:\n", dir[i].pathname);
            DIR* odir = opendir(dir[i].pathname);
            if(odir == NULL){
                i++;
                continue;
            }
            show_ls_iR(odir, dir[i].pathname);
            closedir(odir);
            i++;
        }
    }
}

//Print files in alphabet order with -ilR form
void show_ls_ilR(DIR* ret_opendir, char* path){
    struct dirent* ret = NULL;
    struct stat stat_info;
    char nextpath[PATH_MAX+1];
    char* buf = NULL;
    char* p=NULL;
    struct dirent lst[4096]={0};
    int index = 0;
    int i = 0;
    while(ret = readdir(ret_opendir)) {
        strcpy(nextpath,path);
        char* filename = ret->d_name;
        if(filename[0]!='.' && index < 4096) {
            memcpy(&lst[index],ret,sizeof(struct dirent));
            index++;
        }

    }
    sort(lst,index);

    while(i<index){
        ret = &lst[i];
        char* filename = ret->d_name;
        memset(nextpath,0,PATH_MAX+1);
        strcpy(nextpath,path);

        int end = 0;
        while(path[end])
            end++;
        if(path[end-1] != '/')
            strcat(nextpath,"/");
        strcat(nextpath,filename);

        memset(&stat_info,0,sizeof(struct stat));
        int stat_ret = lstat(nextpath, &stat_info);
        if(stat_ret ==-1){
            printf("error %s \n",nextpath);
            i++;
            continue;
        }

        if(filename[0]!='.') {
            printf("%li ", stat_info.st_ino);
            l_display(stat_info);
            if(spec_sign(filename)){
                printf(" \'%s\'",filename);
            }else{
                printf(" %s",filename);
            }
            ssize_t bufsiz = stat_info.st_size + 1;
            if (stat_info.st_size == 0)
                bufsiz = PATH_MAX;
            buf = malloc(bufsiz);
            if (buf == NULL) {
                i++;
                continue;
            }
            if(S_ISLNK(stat_info.st_mode)) {
                ssize_t nbytes = readlink(ret->d_name, buf, bufsiz);
                if (nbytes == -1) {
                    i++;
                    continue;
                }
            }
            free(buf);
            printf("\n");
        }
        i++;
    }
    rewinddir(ret_opendir);
    puts("");
    puts("");
}

// -ilR command
void print_i_l_R_Sort( char* pathname){
    DIR_Ex dir[1024] = {0};
    int index = 0;
    int i = 0;
    struct stat s_buf;
    stat(pathname,&s_buf);
    if(S_ISREG(s_buf.st_mode)){
        printf("%li ",s_buf.st_ino);
        l_display(s_buf);
        if(spec_sign(pathname)){
            printf(" \'%s\'",pathname);
        }else{
            printf(" %s ",pathname);
        }
    }else{
        print_i_R_rec(pathname,dir,&index);
        sort_i_R(dir,index);
        while(i < index){
            printf("%s:\n", dir[i].pathname);
            DIR* odir = opendir(dir[i].pathname);
            if(odir == NULL){
                i++;
                continue;
            }

            show_ls_ilR(odir, dir[i].pathname);
            closedir(odir);
            i++;
        }
    }
}


//Print files in alphabet order with -lR form
void show_ls_lR(DIR* ret_opendir, char* path){
    struct dirent* ret = NULL;
    struct stat stat_info;
    char nextpath[PATH_MAX+1];
    char* buf = NULL;
    char* p=NULL;
    struct dirent lst[4096]={0};
    int index = 0;
    int i = 0;
    while(ret = readdir(ret_opendir)) {
        strcpy(nextpath,path);
        char* filename = ret->d_name;
        if(filename[0]!='.' && index < 4096) {
            memcpy(&lst[index],ret,sizeof(struct dirent));
            index++;
        }
    }

    sort(lst,index);
    while(i<index){
        ret = &lst[i];
        char* filename = ret->d_name;

        memset(nextpath,0,PATH_MAX+1);
        strcpy(nextpath,path);

        int end = 0;
        while(path[end])
            end++;
        if(path[end-1] != '/')
            strcat(nextpath,"/");
        strcat(nextpath,filename);

        memset(&stat_info,0,sizeof(struct stat));
        int stat_ret = lstat(nextpath, &stat_info);
        if(stat_ret ==-1){
            printf("error %s \n",nextpath);
            i++;
            continue;
        }

        if(filename[0]!='.') {
            l_display(stat_info);
            if(spec_sign(filename)){
                printf(" \'%s\'",filename);
            }else{
                printf(" %s ",filename);
            }
            ssize_t bufsiz = stat_info.st_size + 1;
            if (stat_info.st_size == 0)
                bufsiz = PATH_MAX;
            buf = malloc(bufsiz);
            if (buf == NULL) {
                i++;
                continue;
            }
            if(S_ISLNK(stat_info.st_mode)) {
                ssize_t nbytes = readlink(ret->d_name, buf, bufsiz);
                if (nbytes == -1) {
                    i++;
                    continue;
                }
            }
            printf("\n");
            free(buf);
        }
        i++;
    }
    rewinddir(ret_opendir);
    puts("");
    puts("");
}

// -lR command
void print_l_R_Sort( char* pathname){
    DIR_Ex dir[1024] = {0};
    int index = 0;
    int i = 0;
    
    struct stat s_buf;
    stat(pathname,&s_buf);
    if(S_ISREG(s_buf.st_mode)){
        l_display(s_buf);
        if(spec_sign(pathname)){
            printf(" \'%s\'",pathname);
        }else{
            printf(" %s ",pathname);
        }
    }else{
        print_i_R_rec(pathname,dir,&index);
        sort_i_R(dir,index);
        
        while(i < index){
            printf("%s:\n", dir[i].pathname);
            DIR* odir = opendir(dir[i].pathname);
            if(odir == NULL){
                i++;
                continue;
            }
            show_ls_lR(odir, dir[i].pathname);
            closedir(odir);
            i++;
        }
    }
}

//Print files in alphabet order with -i form
void print_i( char* directory)
{
    DIR *dir;
    struct  dirent *dp;
    stat_s stat_temp[255] = {0};
    char* buff = NULL;
    char curr_dir[PATH_MAX];
    char original_dir[PATH_MAX];
    int i = 0;
    int index = 0;

    getcwd(original_dir, sizeof(original_dir));
    chdir(directory);

    struct stat s_buf;
    stat(directory,&s_buf);
    if(S_ISREG(s_buf.st_mode)){
        printf("%li ",s_buf.st_ino);
        printf("%s\t",directory);
    }else{
        if(getcwd(curr_dir, sizeof(curr_dir)) == NULL) {
            perror("change dir fail");
        }
        if((dir = opendir(curr_dir)) == NULL){
            perror("opendir fail");
        }else{
            struct stat stat_info;
            while( (dp = readdir(dir)) != NULL) {
                if(lstat(dp->d_name, &stat_info) == -1) {
                    perror("lstat");
                    exit(EXIT_FAILURE);
                }
                if(dp->d_name[0] != '.'){
                    strcpy(stat_temp[i].filename, dp->d_name);
                    memcpy(&stat_temp[i].s, &stat_info, sizeof(stat_s));
                    i++;
                }
            }
            sort_i(stat_temp,i);
            while(index < i){
                stat_s *tmp = &stat_temp[index];
                printf("%*li ", max_length_ino,tmp->s.st_ino);
                if(spec_sign(tmp->filename)){
                    printf("\'%s\'\n",tmp->filename);
                }else{
                    printf("%s \n",tmp->filename);
                }
                // Handle special path
                int buff_size = tmp->s.st_size + 1;
                if (tmp->s.st_size == 0){
                    buff_size = PATH_MAX;
                }
                buff = malloc(buff_size);
                if(S_ISLNK(tmp->s.st_mode)) {
                    int bytes = readlink(tmp->filename, buff, buff_size);
                    if (bytes == -1) {
                        perror("readlink");
                        exit(EXIT_FAILURE);
                    }
                }
                index++;
                free(buff);
            }
        }
        rewinddir(dir);
        chdir(original_dir);
        closedir(dir);
    }
}

//Print files in alphabet order with -l form
void print_l( char* directory){
    DIR *dir;
    struct  dirent *dp;
    char* buff = NULL;
    char curr_dir[PATH_MAX];
    char original_dir[PATH_MAX];
    stat_s stat_temp[128] = {0};
    int i = 0;
    int index = 0;
    getcwd(original_dir, sizeof(original_dir));
    chdir(directory);
    struct stat s_buf;
    stat(directory,&s_buf);
    if(S_ISREG(s_buf.st_mode)){
        l_display(s_buf);
        if(spec_sign(directory)){
            printf(" \'%s\'",directory);
        }else{
            printf("%s ",directory);
        }
    }else{
        if(getcwd(curr_dir, sizeof(curr_dir)) == NULL) {
            perror("change dir fail");
        }
        if ((dir = opendir(curr_dir)) == NULL){
            perror("opendir fail");
        }
        else {
            struct stat stat_info;
            get_max_length(dir, s_buf);
            while( (dp = readdir(dir)) != NULL) {
                if(lstat(dp->d_name, &stat_info) == -1) {
                    perror("lstat");
                    exit(EXIT_FAILURE);
                }
                // if has l option
                if(dp->d_name[0] != '.'){
                    strcpy(stat_temp[index].filename, dp->d_name);
                    memcpy(&stat_temp[index].s, &stat_info, sizeof(stat_s));
                    index++;
                }
            }
            sort_i(stat_temp,index);
            while(i<index){
                stat_s *temp = &stat_temp[i];
                l_display(temp->s);
                if(spec_sign(temp->filename)){
                    printf(" \'%s\'",temp->filename);
                }else{
                    printf(" %s ",temp->filename);
                }
                int buff_size = temp->s.st_size + 1;
                if (temp->s.st_size == 0)
                    buff_size = PATH_MAX;
                buff = malloc(buff_size);
                if (buff == NULL) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                if(S_ISLNK(temp->s.st_mode)) {
                    int bytes = readlink(temp->filename, buff, buff_size);
                    if (bytes == -1) {
                        perror("readlink");
                        exit(EXIT_FAILURE);
                    }
                    // keep
                    printf("-> %.*s",  bytes, buff);
                }
                free(buff);
                printf("\n");
                i++;
            }
        }
        free(dp);
        chdir(original_dir);
        closedir(dir);
    }
}

//Print files in alphabet order with -il form
void print_i_l( char* directory){
    DIR *dir;
    struct  dirent *dp;
    char* buff = NULL;
    char curr_dir[PATH_MAX];
    char original_dir[PATH_MAX];
    stat_s stat_[128] = {0};
    int i = 0;
    int index = 0;
    getcwd(original_dir, sizeof(original_dir));
    chdir(directory);
    struct stat s_buf;
    stat(directory,&s_buf);
    
    if(S_ISREG(s_buf.st_mode)){
        printf(" %li ",s_buf.st_ino);
        l_display(s_buf);
        if(spec_sign(directory)){
            printf(" \'%s\'",directory);
        }else{
            printf(" %s ",directory);
        }
    }else{

        if(getcwd(curr_dir, sizeof(curr_dir)) == NULL) {
            perror("change dir fail");
        }
        if ((dir = opendir(curr_dir)) == NULL){
            perror("opendir fail");
            exit(EXIT_FAILURE);
        }
        struct stat stat_info;
        get_max_length(dir, stat_info);
        while( (dp = readdir(dir)) != NULL) {
            if(lstat(dp->d_name, &stat_info) == -1) {
                perror("lstat");
                exit(EXIT_FAILURE);
            }
            // if has l option
            if(dp->d_name[0] != '.'){
                strcpy(stat_[index].filename,dp->d_name);
                memcpy(&stat_[index].s, &stat_info, sizeof(stat_s));
                index++;
            }
        }
        sort_i(stat_, index);
        while(i<index){
            stat_s *temp = &stat_[i];
            printf("%li ", temp->s.st_ino);
            l_display(temp->s);
            if(spec_sign(temp->filename)){
                printf(" \'%s\'",temp->filename);
            }else{
                printf(" %s ",temp->filename);
            }
            // Handle special path
            int buff_size = stat_info.st_size + 1;
            if(stat_info.st_size == 0){
                buff_size = PATH_MAX;
            }
            buff = malloc(buff_size);
            if(S_ISLNK(stat_info.st_mode)){
                int bytes = readlink(dp->d_name, buff, buff_size);
                if(bytes == -1){
                    perror("link read fail");
                    exit(EXIT_FAILURE);
                }
            }
            free(buff);
            printf("\n");
            i++;
        }
        free(dp);
        chdir(original_dir);
        closedir(dir);
    }

}

//Check the signals and decide to use with command function
void myPrint( char* directory, bool s_i_sign, bool s_l_sign,bool s_R_sign){
    if(s_i_sign && s_l_sign && s_R_sign){
        print_i_l_R_Sort(directory);
    }else if(s_i_sign && s_l_sign){
        print_i_l(directory);
    }else if(s_i_sign && s_R_sign){
        print_i_R_Sort(directory);
    }else if(s_l_sign && s_R_sign){
        print_l_R_Sort(directory);
    }else if(s_R_sign){
        print_R_Sort(directory);
    }else if(s_l_sign){
        print_l(directory);
    }else if(s_i_sign){
        print_i(directory);
    }else{
        DIR *dir;
        struct dirent *dp;
        char* buff = NULL;
        char curr_dir[PATH_MAX];
        char original_dir[PATH_MAX];
        stat_s stat_temp[128] = {0};
        int i = 0;
        int index = 0;
        getcwd(original_dir, sizeof(original_dir));
        chdir(directory);
        struct stat s_buf;
        stat(directory,&s_buf);
        if(S_ISREG(s_buf.st_mode)){
            printf("%s\t",directory);
        }
        else{
            if(getcwd(curr_dir, sizeof(curr_dir)) == NULL) {
                perror("change dir fail");
            }
            if((dir = opendir(curr_dir)) == NULL){
                perror("opendir fail");
            }else{
                struct stat stat_info;
                while( (dp = readdir(dir)) != NULL) {
                    if(lstat(dp->d_name, &stat_info) == -1) {
                        perror("lstat");
                        exit(EXIT_FAILURE);
                    }
                    if(dp->d_name[0] != '.'){
                        strcpy(stat_temp[index].filename, dp->d_name);
                        memcpy(&stat_temp[index].s, &stat_info, sizeof(stat_s));
                        index++;
                    }
                }
                sort_i(stat_temp,index);
                while(i<index){
                    stat_s *temp = &stat_temp[i];
                    if(spec_sign(temp->filename)){
                        printf("\'%s\'",temp->filename);
                    }else{
                        printf("%s",temp->filename);
                    }
                    // Handle special path
                    int buff_size = stat_info.st_size + 1;
                    if(stat_info.st_size == 0){
                        buff_size = PATH_MAX;
                    }
                    buff = malloc(buff_size);
                    if(S_ISLNK(stat_info.st_mode)){
                        int bytes = readlink(dp->d_name, buff, buff_size);
                        if(bytes == -1){
                            perror("link read fail");
                            exit(EXIT_FAILURE);
                        }
                    }
                    free(buff);
                    printf("\n");
                    i++;
                }
            }
            closedir(dir);
        }
    }
}

void sort_str(char* argv[],int index)
{
    for(int i=index-1; i>1; i--)
    {
        for(int j=1; j<i; j++)
        {
            if(strcmp(argv[i], argv[j])<0)
            {
                char* temp = argv[i];
                argv[i] = argv[j];
                argv[j] = temp;
            }
        }
    }
}
int main(int argc, char* argv[])
{
    int index = 0;
    char* currParameter;

    // Initialize the directory to the current one
    char* directory = ".";
    char** files = malloc(argc-1);
    sort_str(argv, argc);

    // Get the options
    if(argc > 1) {
        for(index = 1; index<argc; index++) {
            currParameter = argv[index];
            // -XX
            if(currParameter[0] == '-') {
                if(strchr(currParameter, 'i') != NULL) {s_i_sign = true;}
                if(strchr(currParameter, 'l') != NULL) {s_l_sign = true;}
                if(strchr(currParameter, 'R') != NULL) {s_R_sign = true;}
            }else {
                s_dir_sign = true;
                directory = argv[index];
                break;
            }
        }
    }
    if((s_i_sign|| s_l_sign || s_R_sign) && !s_dir_sign ){
        myPrint(directory,s_i_sign,s_l_sign,s_R_sign);
    }
    while(index < argc) {
        myPrint(directory,s_i_sign,s_l_sign,s_R_sign);
        printf("\n");
        index++;
        directory = argv[index];
    }
    free(files);
    return 0;
}
#include "lsof.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <errno.h>

#define PROC_PATH "/proc"
#define COMM "/comm"
#define STATUS "/status"
#define CWD_PATH "/cwd"
#define RTD_PATH "/root"
#define TXT_PATH "/exe"
#define MEM_PATH "/maps"

#define FD_CWD "cwd"
#define FD_RTD "rtd"
#define FD_TXT "txt"
#define TYPE_DIR "DIR"
#define TYPE_REG "REG"
#define TYPE_MEM "mem"

using namespace std;

Lsof::Lsof(std::vector<std::string> commandFilter,
           std::vector<std::string> typeFilter,
           std::vector<std::string> filenamesFilter)
{
    m_commandFilter = commandFilter;
    m_typeFilter = typeFilter;
    m_filenamesFilter = filenamesFilter;

    // printf("command %s\n", m_commandFilter[0].c_str());
    // printf("type %s\n", m_typeFilter[0].c_str());
    // printf("filenames %s\n", m_filenamesFilter[0].c_str());
}

Lsof::~Lsof()
{
}

bool Lsof::isNumber(const std::string& str)
{
    for (int i = 0; i < str.length(); i++) {
        if (std::isdigit(str[i]) == 0) return false;
    }
    return true;
}


void Lsof::readFileToVector(string filePath, std::vector<std::string> &datas)
{
    std::ifstream ifs(filePath, std::ios::in);
    if (!ifs.is_open()) {
        cout << "Failed to open file.\n";
    } else {
        std::string s;
        while (std::getline(ifs, s)) {
            // cout << s << "\n";
            datas.push_back(s);
        }
        ifs.close();
    }
    ifs.close();
    return ;
}

std::string Lsof::getCommand(std::string pid)
{   
    std::string path = std::string(PROC_PATH) + "/" + pid + std::string(COMM);
    // printf("path = %s\n", path.c_str());
    std::vector<std::string> datas;
    readFileToVector(path, datas);
    // printf("datas[0] %ld,  %s \n",  datas.size(), datas[0].c_str());
    return datas[0];
}

std::string Lsof::getName(std::string pid)
{   
    std::string path = std::string(PROC_PATH) + "/" + pid + std::string(STATUS);
    string name = "";
    // printf("path = %s\n", path.c_str());
    std::vector<std::string> datas;
    readFileToVector(path, datas);
    // printf("datas[0] %ld,  %s \n",  datas.size(), datas[0].c_str());
    // printf("len  %ld\n", datas[0].length());

    int head = datas[0].find_last_of("\t", datas[0].length());
    int end = datas[0].length();
    // printf("head %d, end %d \n",  head, end);

    name = datas[0].substr(head + 1, end - head);
    return name;
}

int Lsof::getCwd(MSG &msg){
    std::string path = std::string(PROC_PATH) + "/" + msg.pid + std::string(CWD_PATH);

    char buf[1024];
    ssize_t len = readlink(path.c_str(), buf, sizeof(buf));
    if (len != -1)
    {
        buf[len] = '\0';
    }
    else
    {
        // TODO error
    }
    msg.fd = std::string(FD_CWD);
    msg.type = std::string(TYPE_DIR);
    msg.name = std::string(buf, len);
    msg.node = getINode(msg.name);
    
    return 0;
}

int Lsof::getRtd(MSG &msg){
    std::string path = std::string(PROC_PATH) + "/" + msg.pid + std::string(RTD_PATH);

    char buf[1024];
    ssize_t len = readlink(path.c_str(), buf, sizeof(buf));
    if (len != -1)
    {
        buf[len] = '\0';
    }
    else
    {
        // TODO error
    }
    msg.fd = std::string(FD_RTD);
    msg.type = std::string(TYPE_DIR);
    msg.name = std::string(buf, len);
    msg.node = getINode(msg.name);
    
    return 0;
}

int Lsof::getTxt(MSG &msg){
    std::string path = std::string(PROC_PATH) + "/" + msg.pid + std::string(TXT_PATH);

    char buf[1024];
    ssize_t len = readlink(path.c_str(), buf, sizeof(buf));
    if (len != -1)
    {
        buf[len] = '\0';
    }
    else
    {
        // TODO error
    }
    msg.fd = std::string(FD_TXT);
    msg.type = std::string(TYPE_REG);
    msg.name = std::string(buf, len);
    msg.node = getINode(msg.name);
    
    return 0;
}

std::string Lsof::getINode(std::string path)
{
    struct stat buffer;
    int         status;
    status = stat(path.c_str(), &buffer);
    // printf("st_ino %ld\n", buffer.st_ino);
    return std::to_string(buffer.st_ino);
}

void Lsof::getStat(std::string path){
    struct stat buffer;
    int         status;
    status = stat(path.c_str(), &buffer);
    
    printf("st_gid %d\n", buffer.st_gid);
    printf("st_uid %d\n", buffer.st_uid);
    printf("st_size %ld\n", buffer.st_size);
    switch (buffer.st_mode & S_IFMT) {
           case S_IFBLK:  printf("block device\n");            break;
           case S_IFCHR:  printf("character device\n");        break;
           case S_IFDIR:  printf("directory\n");               break;
           case S_IFIFO:  printf("FIFO/pipe\n");               break;
           case S_IFLNK:  printf("symlink\n");                 break;
           case S_IFREG:  printf("regular file\n");            break;
           case S_IFSOCK: printf("socket\n");                  break;
           default:       printf("unknown?\n");                break;
    }
    printf ("COMMAND         PID             USER            FD              TYPE            NODE            NAME\n");
    // printf ("%-9.9s  %6d  %6d  %6d", cmd, pid, pgrp, ppid);
    return;
}

void Lsof::getPidFolder(std::string pid){
    MSG msg;
    msg.pid = pid;
    msg.command = getCommand(pid);
    msg.user = getName(pid);
    int ret = getCwd(msg);
    m_msgs.push_back(msg);
    ret = getRtd(msg);
    m_msgs.push_back(msg);
    ret = getTxt(msg);
    m_msgs.push_back(msg);
    return;
}

int Lsof::run()
{
    printf("run\n");

    DIR *dp;
    struct dirent *dirp;    
    if ((dp = opendir(PROC_PATH)) == NULL)
    {
        printf("path %s read fial\n", PROC_PATH);
        return errno;
    }
    printf ("COMMAND\tPID\tUSER\tFD\tTYPE\tNODE\tNAME\n");
    while ((dirp = readdir(dp)) != NULL)
    {
        if (isNumber(dirp->d_name)) getPidFolder(dirp->d_name);
    }
        
    return 0;
}

int Lsof::show()
{
    for (int i = 0; i < m_msgs.size(); i++)
    {
        MSG msg = m_msgs[i];
        printf ("%s\t%s\t%s\t%s\t%s\t%s\t%s\n", msg.command.c_str(), msg.pid.c_str(), msg.user.c_str(), msg.fd.c_str(), msg.type.c_str(), msg.node.c_str(), msg.name.c_str());
    }
    return 0;
}

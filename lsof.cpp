#include "lsof.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <regex>

#define PROC_PATH "/proc"
#define COMM "/comm"
#define STATUS "/status"
#define CWD_PATH "/cwd"
#define RTD_PATH "/root"
#define TXT_PATH "/exe"
#define MEM_PATH "/maps"
#define FD_PATH "/fd"

#define FD_CWD "cwd"
#define FD_RTD "rtd"
#define FD_TXT "txt"
#define FD_MEM "mem"
#define FD_DEL "DEL"
#define FD_NOFO "NOFD"

#define TYPE_BLK "BLK"
#define TYPE_DIR "DIR"
#define TYPE_REG "REG"
#define TYPE_CHR "CHR"
#define TYPE_FIFO "FIFO"
#define TYPE_SOCK "SOCK"
#define TYPE_LINK "LINK"
#define TYPE_UNKNOWN "unknown"

#define DEL_MSG "(deleted)"
#define MSG_PD " (Permission denied)"
#define ERR -1
using namespace std;

Lsof::Lsof(std::vector<std::string> commandFilter,
           std::vector<std::string> typeFilter,
           std::vector<std::string> filenamesFilter)
{
    m_commandFilter = commandFilter;
    m_typeFilter = typeFilter;
    m_filenamesFilter = filenamesFilter;
}

const std::vector<std::string> split(const std::string& str, const std::string& pattern) {
    std::vector<std::string> result;
    std::string::size_type begin, end;

    end = str.find(pattern);
    begin = 0;

    while (end != std::string::npos) {
        if (end - begin != 0) {
            result.push_back(str.substr(begin, end-begin)); 
        }    
        begin = end + pattern.size();
        end = str.find(pattern, begin);
    }

    if (begin != str.length()) {
        result.push_back(str.substr(begin));
    }
    return result;        
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


int Lsof::readFileToVector(string filePath, std::vector<std::string> &datas)
{
    std::ifstream ifs(filePath, std::ios::in);
    if (!ifs.is_open()) {
        // printf("Failed to open file Path = %s\n", filePath.c_str());
        return ERR;
    } else {
        std::string s;
        while (std::getline(ifs, s)) {
            // cout << s << "\n";
            datas.push_back(s);
        }
        ifs.close();
    }
    ifs.close();
    return 0;
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

int Lsof::getMsg(MSG &msg, std::string fdPath, std::string fd, std::string fdType){
    std::string path = std::string(PROC_PATH) + "/" + msg.pid + std::string(fdPath);
    std::string link;
    int ret = getLink(path, link);
    msg.fd = std::string(fd);
    msg.name = link;
    if (ret == 0)
    {
        msg.type = std::string(fdType);
        msg.node = getINode(msg.name);
    }
    else
    {
        msg.type = std::string(TYPE_UNKNOWN);
        msg.node = "";
    } 
    return 0;
}

int Lsof::getMem(MSG &msg)
{
    std::string path = std::string(PROC_PATH) + "/" + msg.pid + std::string(MEM_PATH);
    std::vector<std::string> datas;
    int ret = readFileToVector(path, datas);
    if (ret == ERR) return ERR;
    int head, end = 0;
    std::string name = "";
    for (int i = 0; i < datas.size(); i++)
    {
        std::vector<std::string> result = split(datas[i], " ");
        std::string rowName = result[result.size() -1];
        std::string rowNode = result[result.size() -2];
        // printf("node %s name %s \n",rowNode.c_str(), rowName.c_str());
        if (rowNode.compare("00:00") != 0 &&rowNode.compare("0") != 0 && rowName.compare(name) != 0)
        {
            name = rowName;
            msg.name = name;
            msg.fd = FD_MEM;
            msg.node = rowNode;
            m_msgs.push_back(msg);
        }
    }
    return 0;
}

int Lsof::getFd(MSG &msg)
{
    std::string path = std::string(PROC_PATH) + "/" + msg.pid + std::string(FD_PATH);
    std::vector<std::string> dirList;
    int ret = getDirList(path, dirList);
    if (ret == ERR)
    {
        msg.fd = FD_NOFO;
        msg.type = "\t";
        msg.name = path + MSG_PD;
        msg.node = "";
        m_msgs.push_back(msg);
    }
    else {
        for (int i = 0; i< dirList.size(); i++)
        {
            // printf("dirList %s\n", dirList[i].c_str());
            std::string link;
            ret = getLink(path + "/" + dirList[i], link);
            msg.fd = dirList[i] + getOpenMode(path + "/" + dirList[i]);
            msg.name = link;
            // msg.type = getType(path + "/" + dirList[i]);
            // msg.node = getINode(path + "/" + dirList[i]);
            if (ret == 0)
            {
                msg.type = getType(path + "/" + dirList[i]);
                msg.node = getINode(path + "/" + dirList[i]);
            }
            else
            {
                msg.type = std::string(TYPE_UNKNOWN);
                msg.node = "";
            } 
            m_msgs.push_back(msg);
        }
    }
    
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

std::string Lsof::getType(std::string path)
{
    struct stat buffer;
    int         status;
    status = stat(path.c_str(), &buffer);
    std::string pathType(TYPE_UNKNOWN);
    switch (buffer.st_mode & S_IFMT) {
           case S_IFBLK:  pathType = TYPE_BLK;            break; // block device
           case S_IFCHR:  pathType = TYPE_CHR;                 break; // character device
           case S_IFDIR:  pathType = TYPE_DIR;                 break; // directory
           case S_IFIFO:  pathType = TYPE_FIFO;                break; // FIFO/pipe
           case S_IFLNK:  pathType = TYPE_LINK;                 break; // symlink
           case S_IFREG:  pathType = TYPE_REG;                 break; // regular file
           case S_IFSOCK: pathType = TYPE_SOCK;                break; // socket
           default:       pathType = TYPE_UNKNOWN;             break; // unknown
    }
    return pathType;
}

std::string Lsof::getOpenMode(std::string path)
{
    struct stat buffer;
    int         status;
    status = lstat(path.c_str(), &buffer);
    std::string rwu = "";
    switch ( buffer.st_mode & (S_IRUSR | S_IWUSR)) {
           case (S_IRUSR | S_IWUSR):  rwu = "u";            break;
           case S_IRUSR:              rwu = "r";            break;
           case S_IWUSR:              rwu = "w";            break;
           default:                   rwu = "";            break;
    }
    return rwu;
}

int Lsof::getLink(std::string path, std::string &link)
{
    char buf[1024];
    ssize_t len = readlink(path.c_str(), buf, sizeof(buf));
    if (len != -1)
    {
        buf[len] = '\0';
        link = std::string(buf, len);
        return 0;
    }
    else
    {
        link = path + MSG_PD;
        return ERR;
    }
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
    int ret = getMsg(msg, CWD_PATH, FD_CWD, TYPE_DIR);
    m_msgs.push_back(msg);
    ret = getMsg(msg, RTD_PATH, FD_RTD, TYPE_DIR);
    m_msgs.push_back(msg);
    ret = getMsg(msg, TXT_PATH, FD_TXT, TYPE_REG);
    m_msgs.push_back(msg);
    ret = getMem(msg);
    ret = getFd(msg);
    return;
}

int Lsof::getDirList(std::string path, std::vector<std::string> &dirlist)
{
    DIR *dp;
    struct dirent *dirp;    
    if ((dp = opendir(path.c_str())) == NULL)
    {
        // printf("path %s read fial\n", path.c_str());
        return ERR;
    }
    while ((dirp = readdir(dp)) != NULL)
    {
        if (isNumber(dirp->d_name)) dirlist.push_back(dirp->d_name);
    }
    return 0;
}

int Lsof::checkDel(MSG &msg)
{
    int head = 0;
    int end = msg.name.find(DEL_MSG);
    // printf("head %d, end %d \n",  head, end);
    if (end != -1) {
        msg.fd = FD_DEL;
        msg.name = msg.name.substr(head, end - head);
    }
    return 0;
}

bool Lsof::regexSearchMatch(MSG msg)
{
    std::regex reg_type("REG");
    std::regex reg_name("usr");
    
    if (m_commandFilter.size() > 0)
    {
        std::regex reg(m_commandFilter[0]);
        if (!std::regex_search(msg.command.c_str(), reg))
        {
            return false;
        }
    }
    
    if (m_typeFilter.size() > 0)
    {
        std::regex reg(m_typeFilter[0]);
        if (!std::regex_search(msg.type.c_str(), reg))
        {
            return false;
        }
    }

    if (m_filenamesFilter.size() > 0)
    {
        std::regex reg(m_filenamesFilter[0]);
        if (!std::regex_search(msg.name.c_str(), reg))
        {
            return false;
        }
    }  
    return true;
}

int Lsof::run()
{
    DIR *dp;
    struct dirent *dirp;    
    if ((dp = opendir(PROC_PATH)) == NULL)
    {
        printf("path %s read fial\n", PROC_PATH);
        return errno;
    }
    while ((dirp = readdir(dp)) != NULL)
    {
        if (isNumber(dirp->d_name)) getPidFolder(dirp->d_name);
    }
        
    return 0;
}

int Lsof::show()
{
    printf ("COMMAND\tPID\tUSER\tFD\tTYPE\tNODE\tNAME\n");
    for (int i = 0; i < m_msgs.size(); i++)
    {
        MSG msg = m_msgs[i];
        int ret = checkDel(msg);
        if (regexSearchMatch(msg))
        {
            printf ("%s\t%s\t%s\t%s\t%s\t%s\t%s\n", msg.command.c_str(), msg.pid.c_str(), msg.user.c_str(), msg.fd.c_str(), msg.type.c_str(), msg.node.c_str(), msg.name.c_str());
        }
    }
    return 0;
}

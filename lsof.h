#ifndef __LSOF_H__
#define __LSOF_H__

#include <string>
#include <vector>

struct MSG {   
    std::string command;
    std::string pid;
    std::string user;
    std::string fd;
    std::string type;
    std::string node;
    std::string name;
};

class Lsof{
    private:
        std::vector<std::string> m_commandFilter, m_typeFilter, m_filenamesFilter;
        void getStat(std::string path);
        bool isNumber(const std::string& str);
        void getPidFolder(std::string pid);
        int getCommand(std::string pid, std::string &command);
        int getUserName(std::string pid, std::string &userName);
        int getINode(std::string path, std::string &inode);
        int getType(std::string path, std::string &pathType);
        std::string getOpenMode(std::string path);
        int getLink(std::string path, std::string &link);
        int getDirList(std::string path, std::vector<std::string> &dirlist);
        int getMsg(MSG &msg, std::string fdPath, std::string fd, std::string fdType);
        int getMem(MSG &msg);
        int getFd(MSG &msg);
        int checkDel(MSG &msg);
        bool regexSearchMatch(MSG msg);
        int readFileToVector(std::string filePath, std::vector<std::string> &datas);
        std::vector<MSG> m_msgs;
    public:
        Lsof(std::vector<std::string> commandFilter, std::vector<std::string> typeFilter, std::vector<std::string> filenamesFilter);
        ~Lsof();
        int run();
        int show();
};

#endif //__LSOF_H__
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
        std::string getCommand(std::string pid);
        std::string getName(std::string pid);
        std::string getINode(std::string path);
        int getCwd(MSG &msg);
        int getRtd(MSG &msg);
        int getTxt(MSG &msg);
        void readFileToVector(std::string filePath, std::vector<std::string> &datas);
        std::vector<MSG> m_msgs;
    public:
        Lsof(std::vector<std::string> commandFilter, std::vector<std::string> typeFilter, std::vector<std::string> filenamesFilter);
        ~Lsof();
        int run();
        int show();
};

#endif //__LSOF_H__
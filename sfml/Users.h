#ifndef H_USERS
#define H_USERS

#include <string>
#include <map>

std::string generateChallenge( void );
std::string calculateChecksum( const std::string& );

std::string solveChallenge( const std::string&, const std::string&, const std::string& );

class UserInfo {
    private:
        std::string username;
        std::string password; // stored in the clear for now

    public:
        UserInfo(const std::string&, const std::string&);

        bool verifyResponse(const std::string&, const std::string&) const;
};

class UserInfoManager {
    private:
        std::map< std::string, UserInfo >
    public:
        UserInfo* login(const std::string&, const std::str
};

#endif

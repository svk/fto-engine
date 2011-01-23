
std::string generateChallenge(void) {
    return "0123456789abcdef";
}

std::string calculateChecksum( const std::string& s ) {
    const int l = s.length();
    int rv = 0;
    std::ostringstream oss;
    for(int i=0;i<l;i++) {
        rv *= 31;
        rv += s[i];
    }
    oss << rv;
    return oss.str();
}

std::string solveChallenge( const std::string& username,
                            const std::string& challenge,
                            const std::string& password ) {
    static const char salt[]="what I did have was a waffle iron which had been given to me that day by an alcoholic";
    return calculateChecksum( username + "/" + password + "/" + challenge + "/" + salt);
}

bool UserInfo::verifyResponse(const std::string& challenge,
                              const std::string& response) {
    return solveChallenge( username, challenge, password ) == response;
}

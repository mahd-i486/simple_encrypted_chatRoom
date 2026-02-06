#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cctype>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

// ---------------- CONFIG ----------------
const std::string API_URL = "https://yourdomain.com/chatapi.php";  //put your endpoint here
const std::string API_TOKEN = "super_duper_seeeeeckkroot_key"; //change this
const std::string SECRET_KEY = "super_duper_seeeeeckkroot_key2"; //change this too

// ---------------- UTF-8 HELPERS ----------------
#ifdef _WIN32
void setupConsoleUTF8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}
#else
void setupConsoleUTF8() {
    setlocale(LC_ALL, "en_US.UTF-8");
}
#endif

// ---------------- HEX ENCODE / DECODE ----------------
std::string toHex(const std::string& input) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char c : input) {
        ss << std::setw(2) << static_cast<int>(c);
    }
    return ss.str();
}

std::string fromHex(const std::string& hexStr) {
    std::string result;
    for (size_t i = 0; i < hexStr.length(); i += 2) {
        std::string byteString = hexStr.substr(i, 2);
        char byte = static_cast<char>(std::stoi(byteString, nullptr, 16));
        result.push_back(byte);
    }
    return result;
}

// ---------------- XOR ENCRYPTION / DECRYPTION ----------------
std::string xorCipher(const std::string& text) {
    std::string result = text;
    for (size_t i = 0; i < text.size(); ++i) {
        result[i] = text[i] ^ SECRET_KEY[i % SECRET_KEY.size()];
    }
    return result;
}

// ---------------- CHECK IF TEXT CONTAINS PERSIAN/ARABIC ----------------
bool containsPersianOrArabic(const std::string& str) {
    for (unsigned char c : str) {
        if (static_cast<unsigned char>(c) >= 0xD8 && static_cast<unsigned char>(c) <= 0xDB) {
            return true;
        }
    }
    return false;
}

// ---------------- REVERSE STRING FOR RTL DISPLAY ----------------
std::string reverseForRTL(const std::string& str) {
    if (!containsPersianOrArabic(str)) {
        return str;
    }
    
    std::string result;
    std::vector<std::string> characters;
    size_t i = 0;
    
    while (i < str.length()) {
        unsigned char c = str[i];
        
        if ((c & 0x80) == 0) {
            characters.push_back(str.substr(i, 1));
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            characters.push_back(str.substr(i, 2));
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            characters.push_back(str.substr(i, 3));
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            characters.push_back(str.substr(i, 4));
            i += 4;
        } else {
            characters.push_back(str.substr(i, 1));
            i += 1;
        }
    }
    
    for (auto it = characters.rbegin(); it != characters.rend(); ++it) {
        result += *it;
    }
    
    return result;
}

// ---------------- CURL EXECUTION ----------------
std::string execCommand(const std::string& cmd) {
    char buffer[128];
    std::string result = "";
    
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    
    if (!pipe) return "";
    
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    
    return result;
}

// ---------------- URL ENCODING ----------------
std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    
    for (char c : value) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || 
            c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) 
                    << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    
    return escaped.str();
}

// ---------------- SEND MESSAGE ----------------
bool sendMessage(const std::string& user, const std::string& msg) {
    std::string encodedUser = urlEncode(user);
    std::string encodedMsg = urlEncode(msg);
    
    std::string cmd = "curl -s -X POST \"" + API_URL + "\" " +
                     "-H \"X-API-TOKEN: " + API_TOKEN + "\" " +
                     "-H \"Content-Type: application/x-www-form-urlencoded\" " +
                     "-d \"user=" + encodedUser + "&content=" + encodedMsg + "\"";
    
    std::string response = execCommand(cmd);
    return response.find("\"success\"") != std::string::npos;
}

// ---------------- PARSE JSON RESPONSE ----------------
std::vector<std::vector<std::string>> parseJsonResponse(const std::string& json) {
    std::vector<std::vector<std::string>> messages;
    size_t pos = 0;
    
    while ((pos = json.find("\"username\":\"", pos)) != std::string::npos) {
        std::vector<std::string> message;
        
        pos += 12;
        size_t endUser = json.find("\"", pos);
        if (endUser == std::string::npos) break;
        message.push_back(json.substr(pos, endUser - pos));
        
        size_t contentPos = json.find("\"content\":\"", endUser);
        if (contentPos == std::string::npos) break;
        contentPos += 11;
        size_t endContent = json.find("\"", contentPos);
        if (endContent == std::string::npos) break;
        message.push_back(json.substr(contentPos, endContent - contentPos));
        
        size_t timePos = json.find("\"created_at\":\"", endContent);
        if (timePos == std::string::npos) break;
        timePos += 14;
        size_t endTime = json.find("\"", timePos);
        if (endTime == std::string::npos) break;
        message.push_back(json.substr(timePos, endTime - timePos));
        
        messages.push_back(message);
        pos = endTime;
    }
    
    // REMOVE THIS LINE to show oldest first (bottom to top)
    // std::reverse(messages.begin(), messages.end());
    
    return messages;
}

// ---------------- DISPLAY MESSAGES ----------------
void displayMessages(const std::vector<std::vector<std::string>>& messages) {
    const std::string RESET = "\033[0m";
    const std::string CYAN = "\033[36m";
    const std::string GRAY = "\033[90m";
    const std::string YELLOW = "\033[33m";
    const std::string BOLD = "\033[1m";
    
    std::cout << "\n" << BOLD << YELLOW
              << "==================== CHAT ROOM ====================\n"
              << RESET;
    
    // Display messages in original order (oldest to newest)
    for (const auto& msg : messages) {
        if (msg.size() < 3) continue;
        
        try {
            std::string decryptedUser = xorCipher(fromHex(msg[0]));
            std::string decryptedContent = xorCipher(fromHex(msg[1]));
            
            decryptedUser.erase(std::remove_if(decryptedUser.begin(), decryptedUser.end(),
                [](char c) { return (c >= 0 && c < 32) || c == 127; }),
                decryptedUser.end());
            
            decryptedContent.erase(std::remove_if(decryptedContent.begin(), decryptedContent.end(),
                [](char c) { return (c >= 0 && c < 32) || c == 127; }),
                decryptedContent.end());
            
            std::string displayUser = reverseForRTL(decryptedUser);
            std::string displayContent = reverseForRTL(decryptedContent);
            
            std::string timeStr = "??:??";
            if (msg[2].length() >= 16) {
                timeStr = msg[2].substr(11, 5);
            }
            
            std::cout << GRAY << "[" << timeStr << "] " << RESET
                      << CYAN << displayUser << RESET
                      << ": " << displayContent << "\n";
                      
        } catch (...) {
            std::cout << GRAY << "[??:??] " << RESET << "(Error displaying message)\n";
        }
    }
    
    std::cout << BOLD << YELLOW
              << "===================================================\n"
              << RESET;
}

// ---------------- FETCH CHAT ----------------
bool fetchChat() {
    std::string cmd = "curl -s -H \"X-API-TOKEN: " + API_TOKEN + "\" \"" + API_URL + "\"";
    std::string response = execCommand(cmd);
    
    if (response.empty() || response.find("error") != std::string::npos) {
        std::cerr << "Error fetching chat.\n";
        return false;
    }
    
    auto messages = parseJsonResponse(response);
    if (messages.empty()) {
        std::cout << "No messages yet. Be the first to send one!\n";
        return true;
    }
    
    displayMessages(messages);
    return true;
}

// ---------------- CLEAR SCREEN ----------------
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// ---------------- MAIN ----------------
int main() {
    setupConsoleUTF8();
    
    std::cout << "=== Encrypted Chat Client ===\n\n";
    
    std::string username;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);
    
    if (username.empty()) {
        username = "Anonymous";
    }
    
    std::string encryptedUsername = toHex(xorCipher(username));
    
    clearScreen();
    std::cout << "Welcome, " << username << "!\n";
    std::cout << "Commands: /r = refresh, /c = clear screen, /exit = quit\n\n";
    
    if (!fetchChat()) {
        std::cout << "Could not connect to server. Check your internet connection.\n";
    }
    
    std::string input;
    while (true) {
        std::cout << "\nMessage (or command): ";
        std::getline(std::cin, input);
        
        if (input == "/exit" || input == "/quit") {
            std::cout << "Goodbye!\n";
            break;
        }
        
        if (input == "/r" || input == "/refresh") {
            fetchChat();
            continue;
        }
        
        if (input == "/c" || input == "/clear") {
            clearScreen();
            std::cout << "Welcome, " << username << "!\n";
            std::cout << "Commands: /r = refresh, /c = clear screen, /exit = quit\n\n";
            fetchChat();
            continue;
        }
        
        if (input.empty()) {
            continue;
        }
        
        std::string encryptedMessage = toHex(xorCipher(input));
        if (sendMessage(encryptedUsername, encryptedMessage)) {
            std::cout << "Message sent!\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            fetchChat();
        } else {
            std::cerr << "Failed to send message.\n";
        }
    }
    
    return 0;
}
#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <fstream>

namespace vk_demo {

class ValidationLogger {
public:
    static ValidationLogger& instance() {
        static ValidationLogger logger;
        return logger;
    }
    
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.push_back(message);
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.clear();
    }
    
    bool hasMessages() const {
        return !m_messages.empty();
    }
    
    bool saveToFile(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_messages.empty()) return false;
        
        std::ofstream file(filepath);
        if (!file.is_open()) return false;
        
        file << "Validation Log (" << m_messages.size() << " messages)\n";
        file << "==========================================\n\n";
        
        for (const auto& msg : m_messages) {
            file << msg << "\n\n";
        }
        
        return true;
    }
    
    const std::vector<std::string>& getMessages() const { return m_messages; }

private:
    ValidationLogger() = default;
    std::vector<std::string> m_messages;
    mutable std::mutex m_mutex;
};

} // namespace vk_demo
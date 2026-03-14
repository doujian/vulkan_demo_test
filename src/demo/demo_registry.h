#pragma once

#include "demo_base.h"
#include <map>
#include <functional>
#include <vector>
#include <string>

namespace vk_demo {

class DemoRegistry {
public:
    using DemoFactory = std::function<std::unique_ptr<DemoBase>()>;
    
    static DemoRegistry& instance() {
        static DemoRegistry registry;
        return registry;
    }
    
    void registerDemo(const std::string& name, const std::string& description, DemoFactory factory) {
        m_factories[name] = factory;
        m_descriptions[name] = description;
    }
    
    std::unique_ptr<DemoBase> createDemo(const std::string& name) {
        auto it = m_factories.find(name);
        if (it != m_factories.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    std::vector<std::string> getDemoNames() const {
        std::vector<std::string> names;
        for (const auto& pair : m_factories) {
            names.push_back(pair.first);
        }
        return names;
    }
    
    std::string getDescription(const std::string& name) const {
        auto it = m_descriptions.find(name);
        if (it != m_descriptions.end()) {
            return it->second;
        }
        return "";
    }
    
    bool hasDemo(const std::string& name) const {
        return m_factories.find(name) != m_factories.end();
    }

private:
    DemoRegistry() = default;
    DemoRegistry(const DemoRegistry&) = delete;
    DemoRegistry& operator=(const DemoRegistry&) = delete;
    
    std::map<std::string, DemoFactory> m_factories;
    std::map<std::string, std::string> m_descriptions;
};

class DemoRegistrar {
public:
    DemoRegistrar(const std::string& name, const std::string& description, DemoRegistry::DemoFactory factory) {
        DemoRegistry::instance().registerDemo(name, description, factory);
    }
    
    static void* getSymbol() { return nullptr; }
};

#define REGISTER_DEMO(Class, Name, Description) \
    extern void* _##Class##_symbol; \
    void* _##Class##_symbol = ::vk_demo::DemoRegistrar::getSymbol(); \
    static ::vk_demo::DemoRegistrar _##Class##_registrar(Name, Description, []() -> std::unique_ptr<::vk_demo::DemoBase> { \
        return std::make_unique<Class>(); \
    });

void forceLinkDemos();

} // namespace vk_demo
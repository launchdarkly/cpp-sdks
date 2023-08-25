#pragma once

/**
 * Utility class to allow user-provided backends to satisfy the ILogBackend
 * interface.
 */
class LogBackendWrapper : public launchdarkly::ILogBackend {
   public:
    explicit LogBackendWrapper(LDLogBackend backend) : backend_(backend) {}
    bool Enabled(launchdarkly::LogLevel level) noexcept override {
        return backend_.Enabled(static_cast<LDLogLevel>(level),
                                backend_.UserData);
    }
    void Write(launchdarkly::LogLevel level,
               std::string message) noexcept override {
        return backend_.Write(static_cast<LDLogLevel>(level), message.c_str(),
                              backend_.UserData);
    }

   private:
    LDLogBackend backend_;
};

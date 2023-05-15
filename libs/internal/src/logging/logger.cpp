#include <iostream>

#include <launchdarkly/logging/logger.hpp>

namespace launchdarkly {
Logger::LogRecord::LogRecord(LogLevel level, bool open)
    : level_(level), open_(open) {}

bool Logger::LogRecord::Open() const {
    return open_;
}

Logger::LogRecordStream::LogRecordStream(Logger const& logger,
                                         Logger::LogRecord rec)
    : rec_(std::move(rec)), ostream_(std::ostream(&rec_)), logger_(logger) {}

Logger::LogRecordStream::~LogRecordStream() {
    if (rec_.open_) {
        ostream_.flush();
        logger_.PushRecord(std::move(rec_));
    }
}

Logger::Logger(std::shared_ptr<ILogBackend> backend)
    : backend_(std::move(backend)) {}

Logger::LogRecord Logger::OpenRecord(LogLevel level) const {
    return {level, backend_->Enabled(level)};
}

bool Logger::Enabled(LogLevel level) const {
    return backend_->Enabled(level);
}

void Logger::PushRecord(LogRecord record) const {
    if (record.Open()) {
        backend_->Write(record.level_, record.str());
    }
}
}  // namespace launchdarkly

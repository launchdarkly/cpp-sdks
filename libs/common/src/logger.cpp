#include "../include/logger.hpp"
#include <iostream>

namespace launchdarkly {
Logger::LogRecord::LogRecord(LogLevel level, bool open)
    : level_(level), open_(open) {}

bool Logger::LogRecord::open() const {
    return open_;
}

Logger::LogRecordStream::LogRecordStream(Logger const& logger,
                                         Logger::LogRecord rec)
    : rec_(std::move(rec)), ostream_(std::ostream(&rec_)), logger_(logger) {}

Logger::LogRecordStream::~LogRecordStream() {
    if (rec_.open_) {
        ostream_.flush();
        logger_.push_record(std::move(rec_));
    }
}

Logger::Logger(std::unique_ptr<ILogBackend> backend)
    : backend_(std::move(backend)) {}

Logger::LogRecord Logger::open_record(LogLevel level) const {
    return {level, backend_->enabled(level)};
}

bool Logger::enabled(LogLevel level) const {
    return backend_->enabled(level);
}

void Logger::push_record(LogRecord record) const {
    if (record.open()) {
        backend_->write(record.level_, record.str());
    }
}
}  // namespace launchdarkly

#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <memory>
#include <vector>

namespace dbcppp {

// Platform-agnostic file reader that doesn't use iostream
// Reads files line-by-line for memory efficiency
class FileLineReader {
public:
    FileLineReader() : file_(nullptr), line_number_(0) {}

    ~FileLineReader() {
        close();
    }

    // Open file for reading
    bool open(const char* filename) {
        close();
        file_ = std::fopen(filename, "r");
        line_number_ = 0;
        return file_ != nullptr;
    }

    // Close the file
    void close() {
        if (file_) {
            std::fclose(file_);
            file_ = nullptr;
        }
    }

    // Read next line from file (returns false on EOF or error)
    // Line ending is stripped
    bool readLine(std::string& line) {
        if (!file_) return false;

        line.clear();
        char buffer[1024];

        // Read line in chunks to handle long lines
        bool line_complete = false;
        while (!line_complete) {
            if (!std::fgets(buffer, sizeof(buffer), file_)) {
                // EOF or error
                if (line.empty()) {
                    return false;  // Nothing read
                }
                line_complete = true;  // Return what we have
            } else {
                size_t len = std::strlen(buffer);
                if (len > 0 && buffer[len-1] == '\n') {
                    buffer[len-1] = '\0';  // Remove newline
                    if (len > 1 && buffer[len-2] == '\r') {
                        buffer[len-2] = '\0';  // Remove CR if CRLF
                    }
                    line_complete = true;
                }
                line.append(buffer);
            }
        }

        line_number_++;
        return true;
    }

    // Get current line number (1-based)
    size_t getLineNumber() const {
        return line_number_;
    }

    // Check if file is open
    bool isOpen() const {
        return file_ != nullptr;
    }

    // Check if we're at end of file
    bool eof() const {
        return file_ && std::feof(file_);
    }

private:
    FILE* file_;
    size_t line_number_;

    // Disable copy
    FileLineReader(const FileLineReader&) = delete;
    FileLineReader& operator=(const FileLineReader&) = delete;
};

// Memory buffer reader for testing and string input
class StringLineReader {
public:
    StringLineReader(const std::string& input)
        : input_(input), position_(0), line_number_(0) {}

    bool readLine(std::string& line) {
        if (position_ >= input_.length()) {
            return false;
        }

        line.clear();
        while (position_ < input_.length()) {
            char ch = input_[position_++];
            if (ch == '\n') {
                line_number_++;
                return true;
            } else if (ch == '\r') {
                // Handle CRLF
                if (position_ < input_.length() && input_[position_] == '\n') {
                    position_++;
                }
                line_number_++;
                return true;
            } else {
                line.push_back(ch);
            }
        }

        // Last line without newline
        if (!line.empty()) {
            line_number_++;
            return true;
        }
        return false;
    }

    size_t getLineNumber() const {
        return line_number_;
    }

private:
    std::string input_;
    size_t position_;
    size_t line_number_;
};

// Abstract interface for line readers
class ILineReader {
public:
    virtual ~ILineReader() = default;
    virtual bool readLine(std::string& line) = 0;
    virtual size_t getLineNumber() const = 0;
};

// Adapter for FileLineReader
class FileLineReaderAdapter : public ILineReader {
public:
    FileLineReaderAdapter(const char* filename) {
        reader_.open(filename);
    }

    bool readLine(std::string& line) override {
        return reader_.readLine(line);
    }

    size_t getLineNumber() const override {
        return reader_.getLineNumber();
    }

    bool isOpen() const {
        return reader_.isOpen();
    }

private:
    FileLineReader reader_;
};

// Adapter for StringLineReader
class StringLineReaderAdapter : public ILineReader {
public:
    StringLineReaderAdapter(const std::string& input)
        : reader_(input) {}

    bool readLine(std::string& line) override {
        return reader_.readLine(line);
    }

    size_t getLineNumber() const override {
        return reader_.getLineNumber();
    }

private:
    StringLineReader reader_;
};

} // namespace dbcppp
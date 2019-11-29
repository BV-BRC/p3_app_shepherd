#ifndef _buffer_h
#define _buffer_h

#include <string>
#include <iostream>

class BufferCountHolder
{
protected:

    static size_t next_buffer_id_;    
};

template <int N>
class OutputBufferT : private BufferCountHolder
{
public:
OutputBufferT(const std::string &key) : key_(key), size_(0), buffer_id_(next_buffer_id_++), truncate_(false) {
	// std::cerr << "make buffer " << key << " " << buffer_id_ << "\n";
	clear();
    }
    ~OutputBufferT() {
	// std::cerr << "destroy buffer " << key_ << " " << buffer_id_ << "\n";
    }


    void clear() { memset(buffer_, 0, N + 1); }
    size_t capacity() { return N; }
    size_t size() { return size_; }
    void size(size_t n) {
	size_ = n > N ? N : n;
    }
    char *data() { return buffer_; }
    const std::string &key() { return key_; }
    void set(const std::string &val) {
	std::cerr << "set value size " << val.size() << "\n";
	val.copy(buffer_, N);
	size_ = val.size();
    }
    std::string as_string() { return std::string(buffer_, N); }
    std::string as_string(int n) { return std::string(buffer_, n); }

    void truncate(bool v) { truncate_ = v; }
    bool truncate() { return truncate_; }

private:
    std::string key_;
    int size_;
    char buffer_[N+1];
    size_t buffer_id_;
    bool truncate_;
};

const int OutputBufferSize = 4096;
typedef OutputBufferT<OutputBufferSize> OutputBuffer;

#endif

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <array>

template <typename T, size_t S>
class CircularBuffer {
public:
	static const size_t Size = S;
	typedef size_t size_type;
	typedef T value_type;
	
private:
	typedef std::array<T, Size> Buffer;
	
public:
	CircularBuffer(): m_begin(m_buffer.begin()), m_end(m_buffer.begin()), m_count(0) {
	}

	bool empty() const { 
		return m_count == 0;
	}
	
	bool full() const { 
		return m_count == Size;
	}
	
	size_type size() const {
		return m_count;
	}
	
	const value_type& get() const {
		return *m_begin;
	}
	
	void pop_front() {
		if ( empty() ) {
			return;
		}
		inc(m_begin);
		--m_count;
	}
	
	bool push_back( const value_type& v ) {
		*m_end = v;
		if ( full() ) {
			inc(m_begin);
			return true;
		}
		inc(m_end);
		++m_count;
		return false;
	}
	
private:
	void inc( typename Buffer::iterator& it ) {
		++it;
		if ( it == m_buffer.end() ) {
			it = m_buffer.begin();
		}
	}
	
private:
	Buffer m_buffer;
	typename Buffer::iterator m_begin;
	typename Buffer::iterator m_end;
    size_type m_count;
};


#endif
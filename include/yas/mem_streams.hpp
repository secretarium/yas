
// Copyright (c) 2010-2019 niXman (i dot nixman dog gmail dot com). All
// rights reserved.
//
// This file is part of YAS(https://github.com/niXman/yas) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef __yas__mem_streams_hpp
#define __yas__mem_streams_hpp

#include <yas/detail/config/config.hpp>
#include <yas/detail/tools/cast.hpp>
#include <yas/detail/tools/noncopyable.hpp>
#include <yas/detail/type_traits/type_traits.hpp>
#include <yas/buffers.hpp>

#include <cmath>
#include <cstring>
#include <vector>

namespace yas {

/***************************************************************************/

struct mem_ostream {
    YAS_NONCOPYABLE(mem_ostream)
    YAS_MOVABLE(mem_ostream)

    mem_ostream(std::size_t reserved = 1024*20)
        :buf(reserved)
        ,beg(buf.data.get())
        ,cur(buf.data.get())
        ,end(buf.data.get()+buf.size)
    {}
    mem_ostream(void *ptr, std::size_t size)
        :buf()
        ,beg(__YAS_SCAST(char*, ptr))
        ,cur(__YAS_SCAST(char*, ptr))
        ,end(__YAS_SCAST(char*, ptr)+size)
    {}
    mem_ostream(shared_buffer b)
        :buf(std::move(b))
        ,beg(__YAS_SCAST(char*, buf.data.get()))
        ,cur(__YAS_SCAST(char*, buf.data.get()))
        ,end(__YAS_SCAST(char*, buf.data.get())+buf.size)
    {}

    template<typename T>
    std::size_t write(const T *ptr, std::size_t size) {
        if ( __YAS_UNLIKELY(cur+size > end) ) {
            realloc(size);
        }

        std::memcpy(cur, ptr, size);
        cur += size;

        return size;
    }

    shared_buffer get_shared_buffer() const { return shared_buffer(buf.data, __YAS_SCAST(std::size_t, cur-beg)); }
    intrusive_buffer get_intrusive_buffer() const { return intrusive_buffer(beg, __YAS_SCAST(std::size_t, cur-beg)); }

private:
    void realloc(std::size_t size) {
        const std::size_t olds = __YAS_SCAST(std::size_t, cur-beg);
        const std::size_t news = __YAS_SCAST(std::size_t,
            size + (olds * __YAS_SCAST(std::size_t, ((1 + std::sqrt(5)) / 1.5)))
        );

        shared_buffer::shared_array_type prev = buf.data;
        buf = shared_buffer(news);
        std::memcpy(buf.data.get(), prev.get(), olds);

        beg = buf.data.get();
        cur = beg+olds;
        end = beg+news;
    }

    shared_buffer buf;
    char *beg, *cur, *end;
}; // struct mem_ostream

/***************************************************************************/

struct mem_istream {
    YAS_NONCOPYABLE(mem_istream)
    YAS_MOVABLE(mem_istream)

    mem_istream(const void *ptr, std::size_t size)
        :beg(__YAS_SCAST(const char*, ptr))
        ,cur(__YAS_SCAST(const char*, ptr))
        ,end(__YAS_SCAST(const char*, ptr)+size)
    {}
    mem_istream(const intrusive_buffer &buf)
        :beg(buf.data)
        ,cur(buf.data)
        ,end(buf.data+buf.size)
    {}
    mem_istream(const shared_buffer &buf)
        :beg(buf.data.get())
        ,cur(buf.data.get())
        ,end(buf.data.get()+buf.size)
    {}
    mem_istream(const std::vector<char>& buf)
        :beg(buf.data())
        ,cur(buf.data())
        ,end(buf.data()+buf.size())
    {}
    mem_istream(const std::vector<int8_t>& buf)
        :beg(__YAS_RCAST(const char*,buf.data()))
        ,cur(__YAS_RCAST(const char*,buf.data()))
        ,end(__YAS_RCAST(const char*,buf.data())+buf.size())
    {}
    mem_istream(const std::vector<uint8_t>& buf)
        :beg(__YAS_RCAST(const char*,buf.data()))
        ,cur(__YAS_RCAST(const char*,buf.data()))
        ,end(__YAS_RCAST(const char*,buf.data())+buf.size())
    {}

    template<typename T>
    std::size_t read(T *ptr, const std::size_t size) {
        const std::size_t avail = __YAS_SCAST(std::size_t, end-cur);
        if ( size <= avail ) {
            std::memcpy(ptr, cur, size);
            cur += size;

            return size;
        }

        return avail;
    }

    bool empty() const { return cur == end; }
    char peekch() const { return *cur; }
    char getch() { return *cur++; }
    void ungetch(char) { --cur; }

    shared_buffer get_shared_buffer() const { return shared_buffer(cur, __YAS_SCAST(std::size_t, end-cur)); }
    intrusive_buffer get_intrusive_buffer() const { return intrusive_buffer(cur, __YAS_SCAST(std::size_t, end-cur)); }

private:
    const char *beg, *cur, *end;
}; // struct mem_istream

/***************************************************************************/

template<typename Byte>
struct vector_ostream {
    YAS_NONCOPYABLE(vector_ostream)
    YAS_MOVABLE(vector_ostream)

    vector_ostream()
        :owning_buf()
        ,buf(owning_buf)
    {}
    
    vector_ostream(std::vector<Byte>& buf_)
        : buf(buf_)
    {}

    template<typename T>
    std::size_t write(const T *ptr, std::size_t size) {
        const Byte* cptr = reinterpret_cast<const Byte*>(ptr);
        buf.insert(buf.end(), cptr, cptr + size);
        return size;
    }
    
    intrusive_buffer get_intrusive_buffer() const { return intrusive_buffer(buf); }
    
    static_assert(std::is_same<Byte,char>::value || std::is_same<Byte,int8_t>::value || std::is_same<Byte,uint8_t>::value, "template parameter should be a byte type");
    std::vector<Byte>  owning_buf;
    std::vector<Byte>& buf;
};

} // ns yas

#endif // __yas__mem_streams_hpp

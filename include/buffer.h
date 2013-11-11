#pragma once

#include <assert.h>

/** Simple data structure for fixed-size circular arrays
  Buffer<int> tab(3);
  for(int i = 0; i < 10; i++) tab.append(i);
  std::cout << tab[0] << " " << tab[1] << " " << tab[2] << "\n";
  std::cout << tab[-1] << " " << tab[-2] << " " << tab[-3] << "\n"; // reverse order
   */
namespace amu {
    template <typename T> class Buffer {
        private:
            int _size;
            int _start, _end;
            T* _elements;

            int adjust(int offset, int modulo) const {
                int result = offset % modulo;
                if(result < 0) result += modulo;
                assert(result >= 0 && result < _size);
                return result;
            }

        public:
            Buffer(const Buffer& other) : _size(other._size), _start(other._start), _end(other._end), _elements(NULL) {
                _elements = new T[_size];
                for(int i = 0; i < _size; i++) _elements[i] = other._elements[i];
            }

            Buffer(size_t size) : _size(size), _start(0), _end(0), _elements(NULL) {
                assert(size > 0);
                _elements = new T[size];
                for(int i = 0; i < size; i++) _elements[i] = T();
            }

            ~Buffer() {
                // TODO: does not work with cv::Mat
                //delete _elements;
            }

            void clear() {
                _start = 0;
                _end = 0;
            }

            void pop() {
                if(size() > 0) _end--;
            }

            void shift() {
                if(size() > 0) _start++;
            }

            T& push() {
                _end++;
                if((_end - _start) > _size) _start++;
                return _elements[adjust(_end - 1, _size)];
            }

            void push(const T& element) { // the last element is discarded
                _end++;
                if((_end - _start) > _size) _start++;
                _elements[adjust(_end - 1, _size)] = element;
            }

            T& unshift() {
                _start--;
                if((_end - _start) > _size) _end--;
                return _elements[adjust(_start, _size)];
            }
            void unshift(const T&element) {
                _start--;
                if((_end - _start) > _size) _end--;
                _elements[adjust(_start, _size)] = element;
            }

            T& operator[](const int which) { // negative addressing supported
                if(which < 0) return _elements[adjust(_end + which, _size)];
                return _elements[adjust(_start + which, _size)];
            }

            const T& operator[](const int which) const { // negative addressing supported
                if(which < 0) return _elements[adjust(_end + which, _size)];
                return _elements[adjust(_start + which, _size)];
            }

            int size() const {
                assert(_end - _start >= 0 && _end - _start <= _size);
                return _end - _start;
            }

            std::string debug() {
                std::stringstream output;
                output << "Buffer: start=" << _start << " end=" << _end << " size=" << size() << " [ ";
                for(int j = 0; j < _size; j++) output << _elements[j] << " ";
                output << "]";
                return output.str();
            }
    };
}

